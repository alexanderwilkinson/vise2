import warnings
try:
  import pypar as mpi
except ImportError, e:
  warnings.warn("Couldn't find pypar -- not using MPI")
  import pypar_dummy as mpi
import os,sys,time
import numpy as np
import numpy.random as npr
import nn
import dkmeans_utils
import mpi_queue
from feature_map import featureNoWrapper, toHellinger;
from io_util import *
import datetime

_proc_status = '/proc/%d/status' % os.getpid()
_scale = {'kB': 1024.0, 'mB': 1024.0*1024.0,
          'KB': 1024.0, 'MB': 1024.0*1024.0}
def _VmB(VmKey):
     # get pseudo file  /proc/<pid>/status
    try:
        t = open(_proc_status)
        v = t.read()
        t.close()
    except:
        return 0.0  # non-Linux?
     # get VmKey line e.g. 'VmRSS:  9999  kB\n ...'
    i = v.index(VmKey)
    v = v[i:].split(None, 3)  # whitespace
    if len(v) < 3:
        return 0.0  # invalid format?
     # convert Vm value to bytes
    return float(v[1]) * _scale[v[2]]

def memory(since=0.0):
    return _VmB('VmSize:') - since

def resident(since=0.0):
    return _VmB('VmRSS:') - since

def stacksize(since=0.0):
    return _VmB('VmStk:') - since

def walkNodesDF_array(node):
  if node._c_classId=='GROUP':
    for node2 in node._f_iterNodes():
      for node3 in walkNodesDF_array(node2):
        yield node3
  elif node._c_classId=='CARRAY':
    yield node

rank,nprocs = mpi.rank(), mpi.size()

class dkmeans3_worker_func(object):
  def __init__(self, pnts, nn_functor, clst_sums, clst_sums_n, distortion, pref_dtype, featureWrapper= featureNoWrapper):
    self.pnts = pnts
    self.nn_functor = nn_functor
    self.clst_sums = clst_sums
    self.clst_sums_n = clst_sums_n
    self.distortion = distortion
    self.pref_dtype = pref_dtype
    self.featureWrapper= featureWrapper;

  def __call__(self, pnts_range):
    l, r = pnts_range
    rnge_pnts = self.featureWrapper( self.pnts[l:r].astype(self.pref_dtype) );

    rnge_inds, rnge_dsqs = self.nn_functor(rnge_pnts)

    dkmeans_utils.accumulate_clusters(rnge_inds, rnge_pnts, self.clst_sums, self.clst_sums_n)

    self.distortion += np.sum(rnge_dsqs)

    del rnge_pnts, rnge_inds, rnge_dsqs # We should clear any extraneous data

    return 0

def dkmeans3_result_func(res):
  pass

def dkmeans3(pnts_fn, nk, niters, clst_fn, nn_class=nn.nn, seed=42, pnts_step=50000, iters_to_output=[], root_rank=0, checkpoint = True, featureWrapper= featureNoWrapper):
  """
  Distributed k-means.
  """

  if featureWrapper==None:
      featureWrapper= featureNoWrapper;
  elif featureWrapper=='hell':
      featureWrapper= toHellinger;

  npr.seed(seed)

  pnts= pointsObj(pnts_fn)
  npnts = pnts.shape[0]
  ndims = pnts.shape[1]

  if rank==root_rank:
    print 'Using a (%d x %d) %s array for the datapoints' % (npnts,ndims,pnts.dtype)

  if rank==root_rank and ndims>npnts:
    raise RuntimeError, 'dodgy matrix format -- number of dimensions is greater than the number of points!'

  # Find preferred dtype
  if pnts.dtype=='float64': pref_dtype = 'float64'
  else: pref_dtype = 'float32'

  start_iter = np.zeros((1,),dtype='int')
  distortion = np.zeros((1,))
  clst_data = np.empty((nk,ndims), dtype=pref_dtype)
  if rank==root_rank:
    print 'Using a (%d x %d) %s array for the clusters' % (clst_data.shape[0], clst_data.shape[1], clst_data.dtype)
    checkpoint_fn = clst_fn + '.checkpoint'
    if os.path.exists(checkpoint_fn):
      start_iter[0], clst_data, distortion[0] = dkmeans3_read_clusters(checkpoint_fn)
      print 'Restarting from checkpoint. Start iteration = %d' % start_iter
    else:
      clst_inds = np.arange(npnts)
      npr.shuffle(clst_inds)
      clst_inds = clst_inds[:nk]
      clst_inds.sort()
      for i,ind in enumerate(clst_inds):
        clst_data[i] = featureWrapper( pnts[ind] )

      if 0 in iters_to_output:
        dkmeans3_save_clusters(clst_fn + '.000', clst_data, 0, niters, pnts.shape, seed, 0.0)

  mpi.broadcast(start_iter, root_rank)

  # Start iterations
  for iter_num in range(start_iter[0], niters):
    t1 = time.time()

    mpi.broadcast(clst_data, root_rank) # Broadcast the cluster centers to all nodes.

    nn_functor = nn_class(clst_data) # Build the NN functor

    clst_sums = np.zeros((nk, ndims), dtype=pref_dtype)
    # NOTE: The accumulator here is floating point to avoid a cast when used with numpy.
    clst_sums_n = np.zeros(nk, dtype=pref_dtype) # Be careful here -- float32 has 24bits of integer precision.
    distortion = np.zeros((1,))

    # Let's do nearest neighbours
    stack = []
    if rank==root_rank:
      for l in range(0, npnts, pnts_step):
        r = min(l+pnts_step, npnts)
        stack.append((l,r))
      stack.reverse()

    mpi_queue.mpi_queue(
              stack,
              dkmeans3_worker_func(pnts, nn_functor, clst_sums, clst_sums_n, distortion, pref_dtype, featureWrapper= featureWrapper),
              dkmeans3_result_func,
              queue_rank=root_rank)

    mpi.inplace_reduce(clst_sums, mpi.SUM, root_rank)
    mpi.inplace_reduce(clst_sums_n, mpi.SUM, root_rank)
    mpi.inplace_reduce(distortion, mpi.SUM, root_rank)

    if rank==root_rank:
      # Check for clusters with no assignments.
      noassign_inds = np.where(clst_sums_n == 0)[0]
      if len(noassign_inds):
        warnings.warn('iter %d: %d clusters have zero points assigned to them - using random points' % (iter_num, len(noassign_inds)))
        clst_sums_n[noassign_inds] = 1
        for ind in noassign_inds:
          clst_sums[ind] = featureWrapper( pnts[npr.randint(0, pnts.shape[0])] );

      clst_sums /= clst_sums_n.reshape(-1,1)
      clst_data = clst_sums

      t2 = time.time()

      #print 'Iteration %d, sse = %g, mem = %.2fMB, took %.2fs' % (iter_num+1, distortion[0], resident()/2**20, t2-t1)
      print 'relja_retrival,dkmeans::cluster,%s,%d,%d,%g' % (str(datetime.datetime.now()), iter_num+1, niters, distortion[0] )

      # Potentially save the clusters.
      if checkpoint:
        dkmeans3_save_clusters(checkpoint_fn, clst_data, iter_num+1, niters, pnts.shape, seed, distortion[0])
      if (iter_num+1) in iters_to_output:
        dkmeans3_save_clusters(clst_fn + '.%03d' % (iter_num+1), clst_data, iter_num+1, niters, pnts.shape, seed, distortion[0])

    del clst_sums
    del clst_sums_n
    
  if rank==root_rank:
    dkmeans3_save_clusters(clst_fn, clst_data, niters, niters, pnts.shape, seed, distortion[0])
    if checkpoint:
      try:
        os.remove(checkpoint_fn) # Remove the checkpoint file once we've got here.
      except OSError:
        pass

  del clst_data
  #del clst_sums
  #del clst_sums_n

  mpi.barrier() # Is this needed?




def compute_subsample(pnt_fn, feat_fn, N):
  import file_utils
  if rank==0:
    print 'Computing subsample:'

    file_utils.remove_if_newer(pnt_fn, [feat_fn])
    file_utils.remove_if_corrupt_points(pnt_fn)

    if not os.path.exists(pnt_fn):
      subsample_db5(feat_fn, pnt_fn, N)
    else:
      print '... already exists ...'

  mpi.barrier()


def compute_clusters(clst_fn, pnt_fn, nclusters, niters=30, ntrees=8, 
                     nchecks=512, seed=42, iters_to_output=[], pnts_step=50000,
                     approx=True,
                     featureWrapper= featureNoWrapper):
  import file_utils
  if rank==0:
    print 'Computing clusters:'

    file_utils.remove_if_newer(clst_fn, [pnt_fn])
    file_utils.remove_if_corrupt_clusters(clst_fn)
  
  mpi.barrier()
  
  if not os.path.exists(clst_fn):
    if approx:
      nn_class = lambda y: nn.nn_approx(y, ntrees, nchecks, seed)
    else:
      nn_class = nn.nn
    dkmeans3(pnt_fn, nclusters, niters, clst_fn, nn_class=nn_class, seed=seed, iters_to_output=iters_to_output, pnts_step=pnts_step, featureWrapper= featureWrapper)
  else:
    if rank==0:
      print '... already exists ...'
