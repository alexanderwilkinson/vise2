#ifndef __DKMEANS_UTILS_HPP
#define __DKMEANS_UTILS_HPP

#include <stdint.h>

template<class Float>
void
accumulate_clusters(uint32_t* rnge_inds, Float* rnge_pnts,
                    unsigned npnts, unsigned ndims,
                    Float* clst_sums, Float* clst_sums_n)
{
  for (unsigned p=0; p<npnts; ++p) {
    for (unsigned d=0; d<ndims; ++d) {
      clst_sums[rnge_inds[p]*ndims + d] += rnge_pnts[p*ndims + d];
    }
    clst_sums_n[rnge_inds[p]]++;
  }
}

extern "C" void accumulate_clusters_f4(unsigned* rnge_inds, float* rnge_pnts,
                                       unsigned npnts, unsigned ndims,
                                       float* clst_sums, float* clst_sums_n)
{
  accumulate_clusters(rnge_inds, rnge_pnts, npnts, ndims, clst_sums, clst_sums_n);
}

extern "C" void accumulate_clusters_f8(unsigned* rnge_inds, double* rnge_pnts,
                                       unsigned npnts, unsigned ndims,
                                       double* clst_sums, double* clst_sums_n)
{
  accumulate_clusters(rnge_inds, rnge_pnts, npnts, ndims, clst_sums, clst_sums_n);
}

#endif
