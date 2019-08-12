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

#ifndef _NN_COMPRESSED_H_
#define _NN_COMPRESSED_H_

#include <vector>
#include <stdint.h>

#include "compressor.h"
#include "macros.h"
#include "nn_searcher.h"



class nnCompressed : public nnSearcher {
    
    public:
        
        nnCompressed( compressorWithDistance const &compressor, std::string const &data, uint32_t aSize ) : compDist_(&compressor), data_(data) {}
        
        inline void
            findKNN( float const qVec[], uint32_t KNN, std::vector<vecIDdist> &vecIDdists ) const {
                findKNN( *compDist_, data_, qVec, KNN, vecIDdists );
            }
        
        static void
            findKNN( compressorWithDistance const &compDist, std::string const &data, float const qVec[], uint32_t KNN, std::vector<vecIDdist> &vecIDdists );
        
        inline uint32_t
            numDims() const { return compDist_->numDims(); }
        
    private:
        
        compressorWithDistance const *compDist_;
        std::string const data_;
        
        DISALLOW_COPY_AND_ASSIGN(nnCompressed);
    
};

#endif
