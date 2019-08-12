/*
==== Author:

Relja Arandjelovic (relja@robots.ox.ac.uk)
Visual Geometry Group,
Department of Engineering Science
University of Oxford

==== Copyright:

The library belongs to Relja Arandjelovic and the University of Oxford.
No usage or redistribution is allowed without explicit permission.
*/

#include "nn_compressed.h"

#include <iostream>
#include <algorithm>



void
nnCompressed::findKNN( compressorWithDistance const &compDist, std::string const &data, float const qVec[], uint32_t KNN, std::vector<vecIDdist> &vecIDdists ) {
    
    std::vector<float> dsSq;
    compDist.getDistsSq(qVec, data, dsSq);
    KNN= KNN<dsSq.size() ? KNN : dsSq.size();
    
    vecIDdists.clear();
    vecIDdists.reserve(dsSq.size());
    for (uint32_t i= 0; i<dsSq.size(); ++i)
        vecIDdists.push_back( vecIDdist(i, dsSq[i]) );
    
    std::partial_sort( vecIDdists.begin(), vecIDdists.begin()+KNN, vecIDdists.end() );
    
    // I expected this to be faster than partial_sort, but it doesn't seem to be
    // seems like I want findKNN to return sorted vecIDs anyway
    // std::nth_element( vecIDdists.begin(), vecIDdists.begin()+KNN, vecIDdists.end() );
    
    vecIDdists.resize( KNN );
    
}
