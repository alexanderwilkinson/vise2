import numpy as np

cdef extern from "numpy/arrayobject.h":
  ctypedef int npy_intp

  cdef int NPY_CONTIGUOUS

  # Classes
  ctypedef extern class numpy.dtype [object PyArray_Descr]:
    cdef int type_num, elsize, alignment
    cdef char type, kind, byteorder, hasobject
    cdef object fields, typeobj

  ctypedef extern class numpy.ndarray [object PyArrayObject]:
    cdef char *data
    cdef int nd
    cdef npy_intp *dimensions
    cdef npy_intp *strides
    cdef object base
    cdef dtype descr
    cdef int flags

  void import_array()

# Must have this
import_array()

cdef extern from "dkmeans_utils.hpp":
  void accumulate_clusters_f4(unsigned* rnge_inds, float* rnge_pnts,
                              unsigned npnts, unsigned ndims,
                              float* clst_sums, float* clst_sums_n)
  void accumulate_clusters_f8(unsigned* rnge_inds, double* rnge_pnts,
                              unsigned npnts, unsigned ndims,
                              double* clst_sums, double* clst_sums_n)

def accumulate_clusters(ndarray rnge_inds, ndarray rnge_pnts,
                        ndarray clst_sums, ndarray clst_sums_n):
  if not (rnge_pnts.flags&NPY_CONTIGUOUS) or not (clst_sums.flags&NPY_CONTIGUOUS):
    raise RuntimeError, 'arrays must be contiguous'
  if rnge_pnts.dtype!=clst_sums.dtype:
    raise RuntimeError, 'types must be the same'
  if rnge_pnts.dtype=='float32':
    accumulate_clusters_f4(<unsigned*>rnge_inds.data, <float*>rnge_pnts.data,
                           rnge_pnts.shape[0], rnge_pnts.shape[1],
                           <float*>clst_sums.data, <float*>clst_sums_n.data)
  elif rnge_pnts.dtype=='float64':
    accumulate_clusters_f8(<unsigned*>rnge_inds.data, <double*>rnge_pnts.data,
                           rnge_pnts.shape[0], rnge_pnts.shape[1],
                           <double*>clst_sums.data, <double*>clst_sums_n.data)
  else:
    raise RuntimeError, 'only float32 and float64 supported'
