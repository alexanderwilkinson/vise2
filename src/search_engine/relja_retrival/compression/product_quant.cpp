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

#include "product_quant.h"

#include <math.h>
#include <cstring> // for memset and memcpy

#include <iostream>



productQuant::productQuant( std::vector<std::string> const &clstFns, bool enableQuantize, bool approx ) : compressorWithDistance(), nSubQuant(clstFns.size()) {
    
    numDims_= 0;
    
    // load centres and potentially initialize NN search for quantization
    
    nn_objs= enableQuantize ? new fastann::nn_obj<float> const *[nSubQuant] : NULL;
    clstCentres_objs= new clstCentres const *[nSubQuant];
    
    maxSubQuantK= 0;
    
    for (uint8_t iSub= 0; iSub<nSubQuant; ++iSub){
        
        clstCentres_objs[iSub]= new clstCentres( clstFns[iSub].c_str(), true );
        numDims_+= clstCentres_objs[iSub]->numDims;
        
        if (iSub>0 && clstCentres_objs[iSub-1]->numDims!=clstCentres_objs[iSub]->numDims)
            std::cout<<"warning: space, in this implementation, is optimally used only when all subquantizers opperate on same dimensions!\n";
        
        maxSubQuantK= (maxSubQuantK >= clstCentres_objs[iSub]->numClst )? maxSubQuantK : clstCentres_objs[iSub]->numClst ;
        
        if (enableQuantize){
            
            nn_objs[iSub]= approx?
                fastann::nn_obj_build_kdtree(clstCentres_objs[iSub]->clstC_flat, clstCentres_objs[iSub]->numClst, clstCentres_objs[iSub]->numDims, 8, 1024)
                :
                fastann::nn_obj_build_exact(clstCentres_objs[iSub]->clstC_flat, clstCentres_objs[iSub]->numClst, clstCentres_objs[iSub]->numDims)
                ;
        
        }
        
    }
    
}



productQuant::~productQuant(){
    
    for (uint8_t iSub= 0; iSub<nSubQuant; ++iSub){
        delete clstCentres_objs[iSub];
        if (nn_objs!=NULL)
            delete nn_objs[iSub];
    }
    delete []clstCentres_objs;
    if (nn_objs!=NULL)
        delete []nn_objs;
    
}



void
productQuant::quantize( float const vec[], charStream &charStream_obj ) const {
    
    unsigned clusterID;
    float distSq;
    
    float const *subVec= vec;
    for (uint8_t iSub= 0; iSub<nSubQuant; ++iSub){
        nn_objs[iSub]->search_nn(subVec, 1, &clusterID, &distSq);
        subVec+= clstCentres_objs[iSub]->numDims;
        charStream_obj.add( clusterID );
    }
    
}



uint32_t
productQuant::compress( float const vecs[], uint32_t const n, std::string &data ) const {
    
    charStream *charStream_obj= charStreamFactoryCreate();
    charStream_obj->reserve(n*nSubQuant);
    
    float const *thisVec= vecs;
    
    for (uint32_t i= 0; i<n; ++i){
        quantize(thisVec, *charStream_obj);
        thisVec+= numDims_;
    }
    
    data= charStream_obj->getDataCopy();
    uint32_t size= charStream_obj->getByteSize();
    delete charStream_obj;
    return size;
    
}



void
productQuant::decompress( std::string const &data, float *&vecs ) const {
    
    charStream *charStream_obj= charStreamFactoryCreate();
    charStream_obj->setDataCopy(data);
    uint32_t n= charStream_obj->getNum() / nSubQuant;
    
    vecs= new float[numDims_ * n];
    float *thisSubVec= vecs;
    
    for (uint32_t i= 0; i<n; ++i){
        
        for (uint8_t iSub= 0; iSub<nSubQuant; ++iSub){
            
            unsigned subClusterID= charStream_obj->getNextUnsafe();
            
            std::memcpy( thisSubVec, clstCentres_objs[iSub]->clstC_flat + subClusterID * (clstCentres_objs[iSub]->numDims), clstCentres_objs[iSub]->numDims * sizeof(float) );
            
            thisSubVec+= clstCentres_objs[iSub]->numDims;
            
        }
        
    }
    
    delete charStream_obj;
    
}



uint32_t
productQuant::getDistsSq( float const vec[], std::string const &data, float *&distsSq ) const {
    
    charStream *charStream_obj= charStreamFactoryCreate();
    charStream_obj->setDataCopy(data);
    uint32_t n= charStream_obj->getNum() / nSubQuant;
    distsSq= new float[n];
    
    // remember distances to subclusters
    
    float *dists[nSubQuant];
    bool *wasComputed[nSubQuant];
    
    float const *subVecs[nSubQuant]; // don't delete as there is no allocation
    subVecs[0]= vec;
    
    for (uint8_t iSub= 0; iSub<nSubQuant; ++iSub){
        
        if (iSub+1<nSubQuant)
            subVecs[iSub+1]= subVecs[iSub] + clstCentres_objs[iSub]->numDims;
        
        dists[iSub]= new float[ clstCentres_objs[iSub]->numClst ];
        wasComputed[iSub]= new bool[ clstCentres_objs[iSub]->numClst ];
        
        // compute none - do it on demand
        std::memset( wasComputed[iSub], 0, clstCentres_objs[iSub]->numClst * sizeof(bool) );
        
    }
    
    // do the work
    
//     if (charStream_obj->goNative())
//         // a lot faster
//         getDistsSq_8bit( vec, charStream_obj->getData(), n, distsSq, dists, wasComputed, subVecs );
//     else
        getDistsSq_otherbits( vec, *charStream_obj, n, distsSq, dists, wasComputed, subVecs );
    
    
    // free
    delete charStream_obj;
    for (uint8_t iSub= 0; iSub<nSubQuant; ++iSub){
        delete []dists[iSub];
        delete []wasComputed[iSub];
    }
    
    return n;
    
}



void
productQuant::getDistsSq_8bit( float const vec[], unsigned char *iter, uint32_t const n, float *distsSq, float *dists[], bool *wasComputed[], float const *subVecs[] ) const {
    
    // compute distances to all points
    
    for (uint32_t i= 0; i<n; ++i){
        
        distsSq[i]= 0;
        
        for (uint8_t iSub= 0; iSub<nSubQuant; ++iSub){
            
            unsigned subClusterID= static_cast<unsigned>( *iter );
            ++iter;
            
            if (!wasComputed[iSub][subClusterID]){
                dists[iSub][subClusterID]= jp_dist_l2(
                    subVecs[iSub],
                    clstCentres_objs[iSub]->clstC_flat + subClusterID * (clstCentres_objs[iSub]->numDims),
                    clstCentres_objs[iSub]->numDims );
                wasComputed[iSub][subClusterID]= true;
            }
            
            distsSq[i]+= dists[iSub][subClusterID];
            
        }
        
    }
    
}



void
productQuant::getDistsSq_otherbits( float const vec[], charStream &charStream_obj, uint32_t const n, float *distsSq, float *dists[], bool *wasComputed[], float const *subVecs[] ) const {
    
    // compute distances to all points
    
    for (uint32_t i= 0; i<n; ++i){
        
        distsSq[i]= 0;
        
        for (uint8_t iSub= 0; iSub<nSubQuant; ++iSub){
            
            unsigned subClusterID= charStream_obj.getNextUnsafe();
            
            if (!wasComputed[iSub][subClusterID]){
                dists[iSub][subClusterID]= jp_dist_l2(
                    subVecs[iSub],
                    clstCentres_objs[iSub]->clstC_flat + subClusterID * (clstCentres_objs[iSub]->numDims),
                    clstCentres_objs[iSub]->numDims );
                wasComputed[iSub][subClusterID]= true;
            }
            
            distsSq[i]+= dists[iSub][subClusterID];
            
        }
        
    }
    
}



charStream*
productQuant::charStreamFactoryCreate() const {
    charStream *charStream_obj;
    ASSERT( maxSubQuantK<=4096 );
    if (maxSubQuantK<=16)
        charStream_obj= new charStream4;
    else if (maxSubQuantK<=64)
        charStream_obj= new charStream6;
    else if (maxSubQuantK<=256)
        charStream_obj= new charStreamNative<uint8_t>;
    else if (maxSubQuantK<=1024)
        charStream_obj= new charStream10;
    else
        charStream_obj= new charStream12;
    return charStream_obj;
}



uint32_t
productQuant::numBytesPerVector() const {
    ASSERT( maxSubQuantK<=4096 );
    if (maxSubQuantK<=16)
        return charStream4::numBytesForN(nSubQuant);
    else if (maxSubQuantK<=64)
        return charStream6::numBytesForN(nSubQuant);
    else if (maxSubQuantK<=256)
        return charStreamNative<uint8_t>::numBytesForN(nSubQuant);
    else if (maxSubQuantK<=1024)
        return charStream10::numBytesForN(nSubQuant);
    // else (effectively)
        return charStream12::numBytesForN(nSubQuant);
}
