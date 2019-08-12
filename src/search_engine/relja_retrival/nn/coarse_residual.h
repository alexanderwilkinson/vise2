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

#ifndef _COARSE_RESIDUAL_H_
#define _COARSE_RESIDUAL_H_



#include <fastann/fastann.hpp>



#include "nn_searcher.h"
#include "compressor.h"
#include "index_with_data.h"
#include "index_with_data_file.h"
#include "desc_getter_from_file.h"
#include "clst_centres.h"



class coarseResidual : public nnSearcher {
    
    public:
        
        coarseResidual( std::string coarseClstFn, compressorWithDistance const &aCompressor, indexWithData const &aIdx, uint32_t aNVisitCoarse= 1, bool approx= true );
        
        ~coarseResidual();
        
        inline void
            findKNN( float const qVec[], uint32_t KNN, std::vector<vecIDdist> &vecIDdists ) const {
                findKNN( qVec, KNN, vecIDdists, NULL );
            }
        
        void
            findKNN( float const qVec[], uint32_t KNN, std::vector<vecIDdist> &vecIDdists, uint32_t const *origCoarseID ) const;
        
        void
            findKNN( uint32_t coarseID, float const qVecRes[], uint32_t KNN, std::vector<vecIDdist> &vecIDdists ) const;
        
        void
            findKNN( uint32_t coarseID, std::string const &dataRes, uint32_t KNN, std::vector<vecIDdist> &vecIDdists ) const;
        
        uint32_t
            numDims() const { return compDist->numDims(); }
    
    private:
        
        static void
            applyInd( std::vector<uint32_t> const &origIDs, std::vector<vecIDdist> &inds );
        
        clstCentres const coarseClstC_obj;
        fastann::nn_obj<float> const *nn_obj;
        compressorWithDistance const *compDist;
        indexWithData const *idx;
        uint32_t const nVisitCoarse;
    
};



class coarseResidualFidxBuilder {
    
    public:
        
        coarseResidualFidxBuilder( std::string fileName, std::string coarseClstFn, compressorWithDistance const &aCompressor, bool approx= true );
        
        coarseResidualFidxBuilder( indexWithDataBuilder &aIdxBuilder, std::string coarseClstFn, compressorWithDistance const &aCompressor, bool approx= true );
        
        ~coarseResidualFidxBuilder();
        
        void
            add(uint32_t docID, uint32_t numDescs, float const*descs);
        
        static void
            buildFromFile( std::string fileName, std::string coarseClstFn, compressorWithDistance const &aCompressor, descGetterFromFile const &descFile, bool approx= true );
        
        static void
            buildFromFile( indexWithDataBuilder &aIdxBuilder, std::string coarseClstFn, compressorWithDistance const &aCompressor, descGetterFromFile const &descFile, bool approx= true );
    
    private:
        
        void
            initNNobj(bool approx);
        
        void
            buildFromFileCore( descGetterFromFile const &descFile );
        
        indexWithDataBuilder *idxBuilder;
        bool const deleteBuilder;
        clstCentres const coarseClstC_obj;
        fastann::nn_obj<float> const *nn_obj;
        compressorWithDistance const *comp;
    
};

#endif
