#ifndef __JP_NN_HPP
#define __JP_NN_HPP

#include <jp_dist2.hpp>

template<class AccumFloat, class Float1, class Float2>
inline
void
jp_nn(const Float1* pnts, unsigned npnts,
      const Float2* clts, unsigned nclts,
      unsigned ndims,
      unsigned* argmins,
      AccumFloat* mins)
{
  for (unsigned i=0; i<npnts; ++i) {
    argmins[i] = (unsigned)-1;
    mins[i] = std::numeric_limits<AccumFloat>::max();
    for (unsigned j=0; j<nclts; ++j) {
      AccumFloat dsq = jp_dist_l2(&pnts[i*ndims], &clts[j*ndims], ndims);
      if (dsq < mins[i]) {
        mins[i] = dsq;
        argmins[i] = j;
      }
    }
  }
}

extern "C" void jp_nn_search_f4_f4(float* x, unsigned x_sz,
                                   float* y, unsigned y_sz,
                                   unsigned ndims,
                                   unsigned* inds, float* dsts)
{
  jp_nn(x, x_sz, y, y_sz, ndims, inds, dsts);
}

extern "C" void jp_nn_search_f8_f8(double* x, unsigned x_sz,
                                   double* y, unsigned y_sz,
                                   unsigned ndims,
                                   unsigned* inds, double* dsts)
{
  jp_nn(x, x_sz, y, y_sz, ndims, inds, dsts);
}

#endif
