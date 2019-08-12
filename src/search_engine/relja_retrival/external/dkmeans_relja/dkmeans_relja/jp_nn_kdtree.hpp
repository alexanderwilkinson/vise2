/**
 * James Philbin <philbinj@gmail.com>
 * Engineering Department
 * University of Oxford
 * Copyright (C) 2006. All rights reserved.
 * 
 * Use and modify all you like, but do NOT redistribute. No warranty is 
 * expressed or implied. No liability or responsibility is assumed.
 */
/**
 * Implementation of randomized kd-tree's.
 *
 * Example:
 *  jp_nn_kdtree<float> kdt(data_ptr, npoints, ndims, ntrees);  -  Build the trees.
 *  pair<size_t, float> nns[num_nns+1];                         -  Must be num_nns+1 big.
 *  kdt.search(pnt_ptr, num_nns, nns, nchecks);                 -  Search the trees, saving the results in nns.
 */ 
#ifndef __JP_NN_KDTREE_HPP
#define __JP_NN_KDTREE_HPP

#include <cassert>
#include <algorithm>
#include <queue>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <vector>

#include <jp_dist2.hpp>
#include <jp_stats.hpp>

static const unsigned jp_pool_fixed_size_sz = 16384;

template<class T>
struct
jp_pool_fixed_size
{
  unsigned end;
  //unsigned char data[jp_pool_fixed_size_sz*sizeof(T)];
  T data[jp_pool_fixed_size_sz];

  jp_pool_fixed_size()
   : end(0)
  { }

  inline
  void* get_ptr()
  {
    //end++;
    return (void*)&data[end++]; 
    //return (void*)&data[(end-1)*sizeof(T)]; 
  }
};

template<class T>
struct
jp_pool
{
  std::list<jp_pool_fixed_size<T>*> lst;

public:

  inline
  void*
  allocate()
  {
    if (lst.begin() == lst.end() || lst.back()->end == jp_pool_fixed_size_sz ) {
      lst.push_back( new jp_pool_fixed_size<T>() );
    }
    return (*lst.rbegin())->get_ptr();
  }

  void
  free()
  {
    for (typename std::list<jp_pool_fixed_size<T>*>::iterator it = lst.begin();
         it != lst.end();
         ++it)
    {
      delete *it;
    }
  }

  size_t
  size()
  {
    return lst.size() * jp_pool_fixed_size_sz * sizeof(T);
  }
};

namespace jp_nn_kdtree_internal {

template<class Float>
class kdtree_node;

template<class Float>
class kdtree_types
{ 
public:
  typedef Float DiscFloat; // Discriminant dimension type.
  typedef Float DistFloat; // Distance type.
};

template<>
class kdtree_types<unsigned char>
{
public:
  typedef float DiscFloat;
  typedef uint32_t DistFloat;
};

template<class Float>
class
kdtree_node
{
  typedef kdtree_node<Float> this_type;

public:
  typedef typename kdtree_types<Float>::DiscFloat DiscFloat;
  typedef typename kdtree_types<Float>::DistFloat DistFloat;
  typedef std::priority_queue< std::pair<DiscFloat, kdtree_node<Float>*>,
                               std::vector< std::pair<DiscFloat, kdtree_node<Float>*> >,
                               std::greater< std::pair<DiscFloat, kdtree_node<Float>*> > > BPQ;

public:
  //static boost::pool_allocator<this_type> allocator;

//  static void
//  free()
//  {
//    (jp_pool<this_type>::get_instance())->free();
//    //boost::singleton_pool<boost::pool_allocator_tag, sizeof(this_type)>::release_memory();
//  }

  this_type* left_; // ==0 if this is a leaf.
  //this_type* right_;
  
  DiscFloat disc_;
  unsigned disc_dim_;  // I this is a leaf, disc_dim_ = ind, else discriminant dimension.

  std::pair<size_t, DiscFloat>
  choose_split(const Float* pnts, const size_t* inds, size_t N, size_t D)
  {
    // Find mean & variance.
    std::vector< jp_stats_mean_var<DiscFloat> > dim_stats(D);
    for (size_t n=0; n<std::min(N, (size_t)100); ++n) {
      for (size_t d=0; d<D; ++d) {
        dim_stats[d](pnts[inds[n]*D + d]);
      }
    }

    // Find most variant dimension and mean.
    std::vector< std::pair< DiscFloat, uint32_t> > var_dim(D); // Apparently this makes an enormous difference!!
    for (size_t d=0; d<D; ++d) {
      var_dim[d].first = dim_stats[d].variance();
      var_dim[d].second = (uint32_t)d;
    }
    // Partial sort makes a BIG difference to the build time.
    static const uint32_t nrand = D>5 ? 5 : D;
    std::partial_sort(var_dim.begin(), var_dim.begin() + nrand, var_dim.end(), std::greater<std::pair<DiscFloat, uint32_t> >());
    uint32_t randd = var_dim[(int)(drand48() * nrand)].second;
//    static const uint32_t nnrand = 10;
//    std::partial_sort(var_dim.begin(), var_dim.begin() + nnrand, var_dim.end(), std::greater<std::pair<DiscFloat, uint32_t> >());
//    size_t nrand = 1;
//    while (nrand < nnrand && nrand < D &&
//           dim_stats[var_dim[nrand].second].variance()/dim_stats[var_dim[0].second].variance() > 0.90f) nrand++;
//    uint32_t randd = var_dim[(int)(drand48() * nrand)].second;
    
    return std::make_pair(randd, dim_stats[randd].mean());
  }

  void
  split_points(const Float* pnts, size_t* inds, size_t N, size_t D, jp_pool<this_type>& pool)
  {
    std::pair<size_t, DiscFloat> spl = choose_split(pnts, inds, N, D);

    disc_dim_ = spl.first;
    disc_ = spl.second;

    size_t l = 0;
    size_t r = N;
    while (l!=r) {
      if (pnts[inds[l]*D + disc_dim_] < disc_) l++;
      else {
        r--;
        std::swap(inds[l], inds[r]);
      }
    }
    
    // If either partition is empty -> vectors identical!
    if (l==0 || l==N) { l = N/2; } // The vectors are identical, so keep nlogn performance.

    left_ = (this_type*)(pool.allocate());
    this_type* right_ = (this_type*)(pool.allocate());
    
    assert((right_ - left_)==1);
    
    new (left_) this_type(pnts, inds, l, D, pool);
    new (right_) this_type(pnts, &inds[l], N-l, D, pool);
  }

public:
  kdtree_node() : left_(0)/*, right_(0)*/ { }

  kdtree_node(const Float* pnts, size_t* inds, size_t N, unsigned D, jp_pool<this_type>& pool)
   : left_(0)/*, right_(0)*/
  {
    if (N>1) {
      split_points(pnts, inds, N, D, pool);
    }
    else if (N==1) {
      disc_dim_ = inds[0];
    }
    else {
      assert(0);
    }
  }

  void
  search(const Float* pnt,
         BPQ& pri_branch,
         const unsigned numnn,
         std::pair<size_t, DistFloat>* nns,
         std::vector<bool>& seen,
         const Float* pnts,
         unsigned D,
         DiscFloat mindsq,
         unsigned ndists,
         unsigned nchecks)
  {
    this_type* cur = this;
    this_type* follow = 0;
    this_type* other = 0;

    while (cur->left_) {
      DiscFloat diff = pnt[cur->disc_dim_] - cur->disc_;

      if (diff < 0) {
        follow = cur->left_;
        //other = cur->right_;
        other = cur->left_+1;
      }
      else {
        //follow = cur->right_;
        follow = cur->left_+1;
        other = cur->left_;
      }

      pri_branch.push(std::make_pair(mindsq + diff*diff, other)); // 36 %
      cur = follow;
    }
    
    if (seen[cur->disc_dim_]) return;
    seen[cur->disc_dim_] = true;
    
    DistFloat dsq = jp_dist_l2(pnt, &pnts[cur->disc_dim_*D], D); // 31%

    if (dsq > nns[numnn-1].second) return;

    unsigned i;
    for (i = numnn; i>0 && nns[i-1].second > dsq; --i) {
      nns[i] = nns[i-1];
    }
    nns[i] = std::make_pair(cur->disc_dim_, dsq);
  }

};

}

template<class Float>
class
jp_nn_kdtree
{
  typedef jp_nn_kdtree_internal::kdtree_node<Float> node_type;
  typedef typename node_type::DiscFloat DiscFloat;
  typedef typename node_type::DistFloat DistFloat;
  typedef typename node_type::BPQ BPQ;

  std::vector< node_type* > trees_;
  size_t N_;
  unsigned D_;
  const Float* pnts_;

  jp_pool<node_type> pool;

public:
  jp_nn_kdtree(const Float* pnts, size_t N, unsigned D, unsigned ntrees = 8, unsigned seed=42)
   : N_(N), D_(D), pnts_(pnts)
  {
    srand(seed);
    srand48(seed);
    // Create inds.
    std::vector<size_t> inds(N);
    for (size_t n=0; n<N; ++n) inds[n] = n;
    // Need to randomize the inds for the sampled mean and variance.
    std::random_shuffle(inds.begin(), inds.end());

    // Create trees.
    for (unsigned t=0; t<ntrees; ++t) {
      //node_type* n = (node_type*)(pool.allocate());
      //new (n) node_type(pnts, &inds[0], N, D, pool);
      trees_.push_back(new node_type(pnts, &inds[0], N, D, pool));
    }
  }

  void
  cache_fix()
  {
    // Crawl the leaves in order and record the order of the indices.
    // Then, shuffle the point data such that the points are contiguous.
    // Then, crawl back over the leaves and fix the indices.
    // We only do this for the first tree, but need to fix the rest.
    Float* pnts = const_cast<Float*>(pnts_); // Should only do this if we own pnts_.
    // 1. Find the order.
    std::vector<unsigned> inds_in_order;
    std::queue<node_type*> nodes_to_visit; nodes_to_visit.push(trees_[0]);
    while (nodes_to_visit.size()) {
      node_type* cur = nodes_to_visit.front(); nodes_to_visit.pop();

      if (cur->left_==0) {
        // It's a leaf.
        inds_in_order.push_back(cur->disc_dim_);
      }
      else {
        // BFS.
        nodes_to_visit.push(cur->left_);
        nodes_to_visit.push(cur->left_+1);
      }
    }
    // 2. Do a shuffle.
    std::map<unsigned, unsigned> ind_map;
    Float* tmp_pnts = new Float[N_*D_]; // This is inefficient, BUT, it's bug-free.
    for (size_t i=0; i<inds_in_order.size(); ++i) {
      ind_map[inds_in_order[i]] = i;
      for (unsigned d=0; d<D_; ++d) {
        tmp_pnts[i*D_ + d] = pnts_[inds_in_order[i]*D_ + d];
      }
    }
    std::copy(tmp_pnts, tmp_pnts+N_*D_, pnts);
    delete[] tmp_pnts;
    // 3. Rejig the trees.
    for (size_t t=0; t<trees_.size(); ++t) {
      std::queue<node_type*> nodes_to_visit; nodes_to_visit.push(trees_[t]);
      while (nodes_to_visit.size()) {
        node_type* cur = nodes_to_visit.front(); nodes_to_visit.pop();

        if (cur->left_==0) {
          cur->disc_dim_ = ind_map[cur->disc_dim_];
        }
        else {
          nodes_to_visit.push(cur->left_);
          nodes_to_visit.push(cur->left_+1);
        }
      }
    }
  }

  size_t
  size()
  {
    return pool.size();
  }

  ~jp_nn_kdtree()
  {
    pool.free();
    //node_type::free();
    
    while (trees_.size()>0) {
        delete trees_.back();
        trees_.pop_back();
    }
  }

  void
  search(const Float* pnt, unsigned numnn, std::pair<size_t, DistFloat>* nns, unsigned nchecks, const Float* pnts = 0) const
  {
    if (pnts == 0) pnts = pnts_;

    BPQ pri_branch;
    
    for (unsigned i=0; i<numnn; ++i) { nns[i] = std::make_pair(-1, std::numeric_limits<DistFloat>::max()); }

    std::vector<bool> seen(N_, false);

    // Search each tree at least once.
    for (size_t t=0; t<trees_.size(); ++t) {
      trees_[t]->search(pnt, pri_branch, numnn, nns, seen, pnts, D_, DiscFloat(), 0, nchecks);
    }
    unsigned num_dists = 0;

    // Carry on searching until we've computed nchecks_ distances.
    while(pri_branch.size() && num_dists < nchecks) {
      std::pair<DiscFloat, node_type* > pr = pri_branch.top();
      pri_branch.pop();

      pr.second->search(pnt, pri_branch, numnn, nns, seen, pnts, D_, pr.first, num_dists, nchecks);
      num_dists++;
    }
  }
};

extern "C" void* jp_nn_kdtree_f4_new(float* y, unsigned y_sz,
                                     unsigned ndims,
                                     unsigned ntrees,
                                     unsigned seed)
{
  void* ret = (void*)(new jp_nn_kdtree<float>(y, y_sz, ndims, ntrees, seed));
  return ret;
}

extern "C" void jp_nn_kdtree_f4_del(void* tree)
{
  delete (jp_nn_kdtree<float>*)tree;
}

extern "C" void jp_nn_kdtree_f4_search(void* tree,
                                       float* x, unsigned x_sz,
                                       unsigned ndims, unsigned nchecks,
                                       unsigned* inds, float* dsqs)
{
  jp_nn_kdtree<float>* kdt = (jp_nn_kdtree<float>*)tree;
  std::pair<size_t, float> nns[2];
  for (unsigned i=0; i<x_sz; ++i) {
    kdt->search(&x[i*ndims], 1, nns, nchecks);
    inds[i] = nns[0].first;
    dsqs[i] = nns[0].second;
  }
}

extern "C" void jp_nn_kdtree_f4_search_knn(void* tree,
                                           float* x, unsigned x_sz,
                                           unsigned ndims, unsigned nchecks,
                                           unsigned* inds, float* dsqs,
                                           unsigned knn)
{
  jp_nn_kdtree<float>* kdt = (jp_nn_kdtree<float>*)tree;
  std::vector< std::pair<size_t,float> > nns(knn+1);
  for (unsigned i=0; i<x_sz; ++i) {
    kdt->search(&x[i*ndims], knn, &nns[0], nchecks);
    for (unsigned j=0; j<knn; ++j) {
      inds[knn*i + j] = nns[j].first;
      dsqs[knn*i + j] = nns[j].second;
    }
  }
}

extern "C" void* jp_nn_kdtree_f8_new(double* y, unsigned y_sz,
                                     unsigned ndims,
                                     unsigned ntrees,
                                     unsigned seed)
{
  void* ret = (void*)(new jp_nn_kdtree<double>(y, y_sz, ndims, ntrees, seed));
  return ret;
}

extern "C" void jp_nn_kdtree_f8_del(void* tree)
{
  delete (jp_nn_kdtree<double>*)tree;
}

extern "C" void jp_nn_kdtree_f8_search(void* tree,
                                       double* x, unsigned x_sz,
                                       unsigned ndims, unsigned nchecks,
                                       unsigned* inds, double* dsqs)
{
  jp_nn_kdtree<double>* kdt = (jp_nn_kdtree<double>*)tree;
  std::pair<size_t, double> nns[2];
  for (unsigned i=0; i<x_sz; ++i) {
    kdt->search(&x[i*ndims], 1, nns, nchecks);
    inds[i] = nns[0].first;
    dsqs[i] = nns[0].second;
  }
}

extern "C" void jp_nn_kdtree_f8_search_knn(void* tree,
                                           double* x, unsigned x_sz,
                                           unsigned ndims, unsigned nchecks,
                                           unsigned* inds, double* dsqs,
                                           unsigned knn)
{
  jp_nn_kdtree<double>* kdt = (jp_nn_kdtree<double>*)tree;
  std::vector< std::pair<size_t,double> > nns(knn);
  for (unsigned i=0; i<x_sz; ++i) {
    kdt->search(&x[i*ndims], knn, &nns[0], nchecks);
    for (unsigned j=0; j<knn; ++j) {
      inds[knn*i + j] = nns[j].first;
      dsqs[knn*i + j] = nns[j].second;
    }
  }
}
#endif
