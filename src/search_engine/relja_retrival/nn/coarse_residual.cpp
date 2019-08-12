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

#include "coarse_residual.h"

#include <queue>

#include "nn_compressed.h"
#include "util.h"
#include "jp_dist2.hpp"
#include "macros.h"



coarseResidual::coarseResidual( std::string coarseClstFn, compressorWithDistance const &aCompressor, indexWithData const &aIdx, uint32_t aNVisitCoarse, bool approx ) : coarseClstC_obj(coarseClstFn.c_str(), true), compDist(&aCompressor), idx(&aIdx), nVisitCoarse( std::min(aNVisitCoarse, coarseClstC_obj.numClst) ) {
    
    ASSERT( nVisitCoarse>0 );
    
    nn_obj= approx?
        fastann::nn_obj_build_kdtree(coarseClstC_obj.clstC_flat, coarseClstC_obj.numClst, coarseClstC_obj.numDims, 8, 1024)
        :
        fastann::nn_obj_build_exact(coarseClstC_obj.clstC_flat, coarseClstC_obj.numClst, coarseClstC_obj.numDims)
        ;
    
}



coarseResidual::~coarseResidual(){
    delete nn_obj;
}



void
coarseResidual::findKNN( float const qVec[], uint32_t KNN, std::vector<vecIDdist> &vecIDdists, uint32_t const *origCoarseID ) const {
    
    vecIDdists.clear();
    vecIDdists.reserve( KNN>10000? 10000 : KNN );
    
    unsigned coarseID;
    
    std::vector<vecIDdist> vecIDds;
    std::priority_queue<vecIDdist> heapVecIDds;
    
    // assign to coarse clusters
    float *distSqs= new float[nVisitCoarse];
    unsigned *coarseIDs= new unsigned[nVisitCoarse];
    nn_obj->search_knn(qVec, 1, nVisitCoarse, coarseIDs, distSqs);
    
    if (origCoarseID!=NULL){
        // we know what qVec came from (probably it is an internal query, and the vector has been decompressed)
        // check if it should be added to the list (e.g. due to a potential ANN failure)
        uint32_t iN;
        // check if not in list (good to do this due to potential numerical problems, i.e. in order not to add it twice)
        for (iN= 0; iN<nVisitCoarse && coarseIDs[iN] != *origCoarseID; ++iN);
        if (iN>=nVisitCoarse){
            // not found, so check if should be added (note that if we decide not to add it, it is possible that an internal query does not find itself)
            float origDistSq= jp_dist_l2( qVec, coarseClstC_obj.clstC_flat + (*origCoarseID) * coarseClstC_obj.numDims, coarseClstC_obj.numDims );
            for (iN= nVisitCoarse; iN>0 && distSqs[iN-1]>=origDistSq; --iN)
                if (iN<nVisitCoarse){
                    coarseIDs[iN]= coarseIDs[iN-1];
                    distSqs[iN]= distSqs[iN-1];
                }
            if (iN<nVisitCoarse){
                coarseIDs[iN]= *origCoarseID;
                distSqs[iN]= origDistSq;
            }
        }
        
    }
    
    delete []distSqs;
    
    
    for (uint32_t iCoarse= 0; iCoarse<nVisitCoarse; ++iCoarse){
        
        coarseID= coarseIDs[iCoarse];
        
        std::vector<uint32_t> vecIDs;
        unsigned char *data;
        uint32_t size;
        idx->getData(coarseID, vecIDs, data, size);
        if (size==0){
            delete []data;
            continue;
        }
        std::string dataStr(
                reinterpret_cast<const char*>(&data[0]),
                size );
        
        // compute residual
        
        float *qVecRes= new float[coarseClstC_obj.numDims];
        for (uint32_t iDim= 0; iDim < coarseClstC_obj.numDims; ++iDim)
            qVecRes[iDim]= qVec[iDim] - *(coarseClstC_obj.clstC_flat + coarseID * coarseClstC_obj.numDims + iDim);
        
        
        // find KNN in this coarse cluster
        
        if (nVisitCoarse==1){
            
            nnCompressed::findKNN( *compDist, dataStr, qVecRes, KNN, vecIDdists );
            delete []data;
            coarseResidual::applyInd( vecIDs, vecIDdists );
            
        } else {
            
            nnCompressed::findKNN( *compDist, dataStr, qVecRes, KNN, vecIDds );
            delete []data;
//             less efficient but clearer: coarseResidual::applyInd( vecIDs, vecIDds ); and then latter heapVecIDds.push( *it )
            vecIDdist toAdd;
            
            std::vector<vecIDdist>::const_iterator it= vecIDds.begin();
            for (; it!=vecIDds.end() && (heapVecIDds.size()<KNN || heapVecIDds.top().distSq > it->distSq); ++it){
                
                if (heapVecIDds.size()==KNN)
                    heapVecIDds.pop();
                
                toAdd.distSq= it->distSq;
                toAdd.ID= vecIDs[ it->ID ];
                heapVecIDds.push( toAdd );
            }
            
        }
        
        delete []qVecRes;
        
    }
    
    delete []coarseIDs;
    
    if (nVisitCoarse>1){
        
        vecIDdists.clear();
        vecIDdists.reserve(heapVecIDds.size());
        
        while (!heapVecIDds.empty()){
            vecIDdists.push_back( heapVecIDds.top() );
            heapVecIDds.pop();
        }
        std::reverse(vecIDdists.begin(), vecIDdists.end());
        
    }
    
}



void
coarseResidual::findKNN( uint32_t coarseID, float const qVecRes[], uint32_t KNN, std::vector<vecIDdist> &vecIDdists ) const {
    
    float *qVec= new float[coarseClstC_obj.numDims];
    
    // go back to original vector
    for (uint32_t iDim= 0; iDim < coarseClstC_obj.numDims; ++iDim)
        qVec[iDim]= qVecRes[iDim] + *(coarseClstC_obj.clstC_flat + coarseID * coarseClstC_obj.numDims + iDim);
    
    findKNN( qVec, KNN, vecIDdists, &coarseID );
    
    delete []qVec;
    
}



void
coarseResidual::findKNN( uint32_t coarseID, std::string const &dataRes, uint32_t KNN, std::vector<vecIDdist> &vecIDdists ) const {
    
    float *qVecRes;
    compDist->decompress( dataRes, qVecRes );
    
    findKNN( coarseID, qVecRes, KNN, vecIDdists );
    
    delete []qVecRes;
    
}



void
coarseResidual::applyInd( std::vector<uint32_t> const &origIDs, std::vector<vecIDdist> &inds ){
    for (uint32_t i= 0; i<inds.size(); ++i)
        inds[i].ID= origIDs[ inds[i].ID ];
}



void
coarseResidualFidxBuilder::initNNobj(bool approx){
    
    nn_obj= approx?
        fastann::nn_obj_build_kdtree(coarseClstC_obj.clstC_flat, coarseClstC_obj.numClst, coarseClstC_obj.numDims, 8, 1024)
        :
        fastann::nn_obj_build_exact(coarseClstC_obj.clstC_flat, coarseClstC_obj.numClst, coarseClstC_obj.numDims)
        ;
    
}



coarseResidualFidxBuilder::coarseResidualFidxBuilder( std::string fileName, std::string coarseClstFn, compressorWithDistance const &aCompressor, bool approx ) : deleteBuilder(true),  coarseClstC_obj(coarseClstFn.c_str(), true), comp(&aCompressor) {
    
    idxBuilder= new indexWithDataFileBuilder(fileName);
    
    initNNobj(approx);
    
}



coarseResidualFidxBuilder::coarseResidualFidxBuilder( indexWithDataBuilder &aIdxBuilder, std::string coarseClstFn, compressorWithDistance const &aCompressor, bool approx ) : idxBuilder(&aIdxBuilder), deleteBuilder(false), coarseClstC_obj(coarseClstFn.c_str(), true), comp(&aCompressor) {
    
    initNNobj(approx);
    
}



coarseResidualFidxBuilder::~coarseResidualFidxBuilder(){
    idxBuilder->close();
    if (deleteBuilder)
        delete idxBuilder;
    delete nn_obj;
}



void
coarseResidualFidxBuilder::add(uint32_t docID, uint32_t numDescs, float const* descs){
    
    float *vecResiduals= new float[numDescs * coarseClstC_obj.numDims];
    float *thisRes= vecResiduals;
    
    std::vector<uint32_t> vecIDs(numDescs);
    
    for (uint32_t iDesc= 0; iDesc<numDescs; ++iDesc){
        
        unsigned clusterID;
        float distSq;
        nn_obj->search_nn(descs + iDesc*coarseClstC_obj.numDims, 1, &clusterID, &distSq);
        vecIDs[iDesc]= clusterID;
        
        for (uint32_t iDim= 0; iDim < coarseClstC_obj.numDims; ++iDim, ++thisRes)
            *thisRes= descs[iDesc*coarseClstC_obj.numDims+iDim] - *(coarseClstC_obj.clstC_flat + clusterID * coarseClstC_obj.numDims + iDim);
        
    }
    
    std::string dataStr;
    uint32_t size= comp->compress( vecResiduals, numDescs, dataStr);
    idxBuilder->addData(docID, vecIDs,
                        reinterpret_cast<uint8_t const*>(dataStr.c_str()),
                        size);
    
    delete []vecResiduals;
    
}



void
coarseResidualFidxBuilder::buildFromFileCore( descGetterFromFile const &descFile ){
    
    uint32_t numDocs= descFile.numDocs();
    float *descs;
    
    for (uint32_t docID= 0; docID<numDocs; ++docID){
        
        if (docID%100000==0)
            std::cout<<"coarseResidualFidxBuilder::buildFromFile: adding docID= "<<docID<<" / "<<numDocs<<"\n";
        
        uint32_t numDescs;
        descFile.getDescs(docID, numDescs, descs);
        
        add(docID, numDescs, descs);
        
        delete []descs;
    }
    
    std::cout<<"coarseResidualFidxBuilder::buildFromFile: computing the forward index - DONE\n";
    
}


    
void
coarseResidualFidxBuilder::buildFromFile( std::string fileName, std::string coarseClstFn, compressorWithDistance const &aCompressor, descGetterFromFile const &descFile, bool approx ){
    
    std::cout<<"coarseResidualFidxBuilder::buildFromFile: computing the forward index\n";
    
    coarseResidualFidxBuilder crFidxBuilder( fileName, coarseClstFn, aCompressor, approx );
    
    crFidxBuilder.buildFromFileCore( descFile );
    
}



void
coarseResidualFidxBuilder::buildFromFile( indexWithDataBuilder &aIdxBuilder, std::string coarseClstFn, compressorWithDistance const &aCompressor, descGetterFromFile const &descFile, bool approx ){
    
    std::cout<<"coarseResidualFidxBuilder::buildFromFile: computing the forward index\n";
    
    coarseResidualFidxBuilder crFidxBuilder( aIdxBuilder, coarseClstFn, aCompressor, approx );
    
    crFidxBuilder.buildFromFileCore( descFile );
    
}
