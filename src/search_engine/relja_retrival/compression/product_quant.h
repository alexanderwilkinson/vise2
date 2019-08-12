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

#ifndef _PRODUCT_QUANT_H_
#define _PRODUCT_QUANT_H_


#include <vector>
#include <string>
#include <stdint.h>



#include <fastann/fastann.hpp>



#include "char_streams.h"
#include "clst_centres.h"
#include "compressor.h"
#include "jp_dist2.hpp"
#include "macros.h"




class productQuant : public compressorWithDistance, public compressorIndep {
    
    public:
        
        productQuant( std::vector<std::string> const &clstFns, bool enableQuantize= false, bool approx= false );
        
        ~productQuant();
        
        // compressor
        
        void
            quantize( float const vec[], charStream &charStream_obj ) const;
        
        uint32_t
            compress( float const vecs[], uint32_t const n, std::string &data ) const;
        
        void
            decompress( std::string const &data, float *&vecs ) const;
        
        uint32_t
            numDims() const { return numDims_; }
        
        // compressorWithDistance
        
        uint32_t
            getDistsSq( float const vec[], std::string const &data, float *&distsSq  ) const;
        
        // compressorIndep
        
        charStream*
            charStreamFactoryCreate() const;
        
        uint32_t
            numBytesPerVector() const;
        
        uint32_t
            numCodePerVector() const { return nSubQuant; }
    
    
    private:
        
        void
            getDistsSq_8bit( float const vec[], unsigned char *iter, uint32_t const n, float *distsSq, float *dists[], bool *wasComputed[], float const *subVecs[] ) const;
        
        void
            getDistsSq_otherbits( float const vec[], charStream &charStream_obj, uint32_t const n, float *distsSq, float *dists[], bool *wasComputed[], float const *subVecs[] ) const;
        
        clstCentres const **clstCentres_objs;
        fastann::nn_obj<float> const **nn_objs;
        uint8_t const nSubQuant;
        uint32_t numDims_;
        uint32_t maxSubQuantK;
        
        DISALLOW_COPY_AND_ASSIGN(productQuant);
    
};


#endif
