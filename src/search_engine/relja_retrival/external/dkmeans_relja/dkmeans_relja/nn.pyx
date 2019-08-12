import warnings
import numpy as np

cdef extern from "stdlib.h":
  ctypedef unsigned long size_t

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

cdef extern from "jp_nn_kdtree.hpp":
  void* jp_nn_kdtree_f4_new(float* y, unsigned y_sz, unsigned ndims, unsigned ntrees, unsigned seed)
  void jp_nn_kdtree_f4_search(void* tree, float* x, unsigned x_sz,
                              unsigned ndims, unsigned nchecks, unsigned* inds, float* dsqs)
  void jp_nn_kdtree_f4_search_knn(void* tree, float* x, unsigned x_sz,
                              unsigned ndims, unsigned nchecks, unsigned* inds, float* dsqs, unsigned knn)
  void jp_nn_kdtree_f4_del(void* tree)

  void* jp_nn_kdtree_f8_new(double* y, unsigned y_sz, unsigned ndims, unsigned ntrees, unsigned seed)
  void jp_nn_kdtree_f8_search(void* tree, double* x, unsigned x_sz,
                              unsigned ndims, unsigned nchecks, unsigned* inds, double* dsqs)
  void jp_nn_kdtree_f8_search_knn(void* tree, double* x, unsigned x_sz,
                              unsigned ndims, unsigned nchecks, unsigned* inds, double* dsqs, unsigned knn)
  void jp_nn_kdtree_f8_del(void* tree)

cdef class nn_approx:
  cdef void* tree
  cdef unsigned ndims
  cdef unsigned ntrees
  cdef unsigned nchecks
  cdef object dtype

  def __cinit__(self, ndarray Y, unsigned ntrees, unsigned nchecks, unsigned seed):
    if not (Y.flags & NPY_CONTIGUOUS):
      raise RuntimeError, 'array must be contiguous'

    self.dtype = Y.dtype
    self.ndims = Y.shape[1]
    self.ntrees = ntrees
    self.nchecks = nchecks

    if self.dtype=='float32':
      self.tree = jp_nn_kdtree_f4_new(<float*>Y.data, Y.shape[0], self.ndims, self.ntrees, seed)
    elif self.dtype=='float64':
      self.tree = jp_nn_kdtree_f8_new(<double*>Y.data, Y.shape[0], self.ndims, self.ntrees, seed)
    else:
      raise RuntimeError, 'only float32 and float64 types are supported'

  def __dealloc__(self):
    if self.dtype=='float32':
      jp_nn_kdtree_f4_del(self.tree)
    else:
      jp_nn_kdtree_f8_del(self.tree)


  def __call__(self, ndarray X, knn = 1):
    cdef ndarray inds
    cdef ndarray dsqs

    if not (X.flags & NPY_CONTIGUOUS):
      raise RuntimeError, 'array must be contiguous'

    if X.dtype!=self.dtype:
      raise RuntimeError, 'wrong types %s!=%s' % (X.dtype, self.dtype)

    if self.ndims!=X.shape[1]:
      raise RuntimeError, 'different number of dimensions'

    inds = np.empty((X.shape[0], knn), dtype='uint32')
    dsqs = np.empty((X.shape[0], knn), dtype=X.dtype)
    if knn==1:
      if self.dtype=='float32':
        jp_nn_kdtree_f4_search(self.tree, <float*>X.data, X.shape[0], X.shape[1], self.nchecks,
                               <unsigned*>inds.data, <float*>dsqs.data)
      else:
        jp_nn_kdtree_f8_search(self.tree, <double*>X.data, X.shape[0], X.shape[1], self.nchecks,
                               <unsigned*>inds.data, <double*>dsqs.data)
    else:
      if self.dtype=='float32':
        jp_nn_kdtree_f4_search_knn(self.tree, <float*>X.data, X.shape[0], X.shape[1], self.nchecks,
                                   <unsigned*>inds.data, <float*>dsqs.data, knn)
      else:
        jp_nn_kdtree_f8_search_knn(self.tree, <double*>X.data, X.shape[0], X.shape[1], self.nchecks,
                                   <unsigned*>inds.data, <double*>dsqs.data, knn)
    
    return (inds, dsqs)

cdef extern from "jp_nn.hpp":
  void jp_nn_search_f4_f4(float* x, unsigned x_sz, float* y, unsigned y_sz,
                          unsigned ndims,
                          unsigned* inds, float* dsts)
  void jp_nn_search_f8_f8(double* x, unsigned x_sz, double* y, unsigned y_sz,
                          unsigned ndims,
                          unsigned* inds, double* dsts)

cdef class nn:
  cdef ndarray Y
  cdef unsigned ndims

  def __cinit__(self, ndarray Y):
    if not (Y.flags & NPY_CONTIGUOUS):
      raise RuntimeError, 'array must be contiguous'

    if Y.dtype not in ['float32','float64']:
      raise RuntimeError, 'array must be float32 or float64'
    
    self.Y = Y
    self.ndims = Y.shape[1]

  def __call__(self, ndarray X):
    cdef ndarray inds
    cdef ndarray dsqs
    
    if not (X.flags & NPY_CONTIGUOUS):
      raise RuntimeError, 'array must be contiguous'

    if X.dtype!=self.Y.dtype:
      raise RuntimeError, 'wrong type'

    if X.shape[1]!=self.Y.shape[1]:
      raise RuntimeError, "number of dimensions don't match"
    
    inds = np.empty(X.shape[0],dtype='uint32')
    dsqs = np.empty(X.shape[0],dtype=X.dtype)

    if X.dtype=='float32':
      jp_nn_search_f4_f4(<float*>X.data, X.shape[0], <float*>self.Y.data, self.Y.shape[0],
                         self.Y.shape[1],
                         <unsigned*>inds.data, <float*>dsqs.data)
    else:
      jp_nn_search_f8_f8(<double*>X.data, X.shape[0], <double*>self.Y.data, self.Y.shape[0],
                         self.Y.shape[1],
                         <unsigned*>inds.data, <double*>dsqs.data)

    return (inds, dsqs)



