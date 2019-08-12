#!/usr/bin/env python

import sys
import warnings
try:
  import pypar as mpi
except ImportError, e:
  warnings.warn("Couldn't find pypar -- not using MPI")
  import pypar_dummy as mpi

rank, nprocs = mpi.rank(), mpi.size()

def mpi_queue(items, worker_func, result_func, queue_rank=0, dynamic_thresh=4):
  """Implements a generic shared work queue over mpi.
  
  == Inputs ==
    items       -- List of items to be distributed to workers.
    worker_func -- Function which takes one work item and produces
                   a result.
    result_func -- Function which processes the result (can be 0).
                   Only process queue_rank calls this function.                 

  == Optional inputs ==
    queue_rank        -- Rank of the process which owns the queue.
    dynamic_thresh    -- Threshold for dynamic scheduling (see notes).
    static_sync_size  -- Block size for static scheduling.

  == Notes ==
    If the nprocs < dynamic_thresh, a static scheduling algorithm is
  used (all processes do work) with synchronization occurring after
  each work item. Otherwise a dynamic algorithm is used (process 
  queue_rank does not do work).
  """
  if nprocs < dynamic_thresh: # STATIC LOAD QUEUE
    if rank == queue_rank:
      while len(items):
        # Send items to work on.
        item = items.pop()

        expected_procs = []
        for proc_num in range(nprocs):
          if proc_num!=queue_rank and len(items):
            mpi.send('go', proc_num)
            mpi.send(items.pop(), proc_num)
            expected_procs.append(proc_num)

        result = worker_func(item)
        result_func(result)
        
        for proc_num in expected_procs:
          result = mpi.receive(proc_num)
          result_func(result)

      for proc_num in range(nprocs):
        if proc_num!=queue_rank:
          mpi.send('stop', proc_num)
    else:
      while 1:
        msg = mpi.receive(queue_rank)
        if msg=='go':
          item = mpi.receive(queue_rank)
          result = worker_func(item)
          mpi.send(result, queue_rank)
        elif msg=='stop':
          break
        else:
          print >>sys.stderr, 'message %s not recognized' % msg
    
  else: # DYNAMIC QUEUE
    if rank == queue_rank:
      expected_procs = set()
      nprocs_to_stop = nprocs - 1
      while len(items) or len(expected_procs):
        cmd, status = mpi.receive(mpi.any_source, return_status=True)
        proc = status.source

        if cmd=='pop': # proc wants another item
          if len(items):
            mpi.send('go', proc)
            mpi.send(items.pop(), proc)
            expected_procs.add(proc)
          else:
            mpi.send('stop', proc)
            nprocs_to_stop-=1
        elif cmd=='result': # proc wants to send a result
          result = mpi.receive(proc)
          result_func(result)
          expected_procs.remove(proc)
      
      while nprocs_to_stop:
        cmd, status = mpi.receive(mpi.any_source, return_status=True)
        proc = status.source
        mpi.send('stop', proc)

        nprocs_to_stop-=1
        
    else:
      while 1:
        mpi.send('pop', queue_rank) # We want some work
        msg = mpi.receive(queue_rank)
        if msg=='go':
          item = mpi.receive(queue_rank)
          result = worker_func(item)

          mpi.send('result', queue_rank)
          mpi.send(result, queue_rank)
        elif msg=='stop':
          break

if __name__=="__main__":
  # Run a primes test.
  def is_prime(num):
    for x in range(2, int(num**0.5)+1):
      if num%x==0:
        return (num, False)
    return (num, True)

  def record_prime(list, result):
    num, primeness = result
    if primeness: list.append(num)

  primes = []
  mpi_queue(range(2,1000), is_prime, lambda x: record_prime(primes, x))

  if rank==0:
    primes.sort()
    print primes
