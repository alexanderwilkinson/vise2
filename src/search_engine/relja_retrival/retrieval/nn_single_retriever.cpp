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

#include "nn_single_retriever.h"

#include <stdexcept>
#include <limits>
#include <iostream>

#include <boost/filesystem.hpp>

#include "util.h"



void
nnSingleRetriever::queryExecute( query const &query_obj, std::vector<indScorePair> &queryRes, uint32_t toReturn ) const {
    
    std::vector<nnSearcher::vecIDdist> vecIDdists;
    
    if (query_obj.isInternal){
        
        // load from fidx
        
        if (fidx==NULL)
            throw std::runtime_error("Haven't provided a forward index object!");
        
        unsigned char *data;
        std::vector<uint32_t> vecIDs;
        uint32_t size;
        uint32_t N= fidx->getData(query_obj.docID, vecIDs, data, size);
        ASSERT( N==1 );
        std::string dataStr(
            reinterpret_cast<const char*>(&data[0]),
            size );
        
        // query
        CR->findKNN( vecIDs[0], dataStr, toReturn==0 ? std::numeric_limits<uint32_t>::max() : toReturn, vecIDdists );
        delete []data;
        
    } else {
        
        // dataFn should contain the single query feature (as 4 byte floats)
        
        if ( !boost::filesystem::exists( query_obj.compDataFn ) ){
            // doesn't exist
            std::cout<< "The file containing the query feature doesn't exist (" << query_obj.compDataFn <<" )\n";
            // TODO: ideally compute one but this is not currently implemented so return empty
            queryRes.clear();
            return;
        }
        
        uint32_t numDims_= CR->numDims();
        
        // check the file size is correct
        uint64_t fileSize= util::fileSize(query_obj.compDataFn);
        if ( (fileSize % (numDims_*sizeof(float)) != 0) || (fileSize != numDims_*sizeof(float)) ){
            
            // corrupt file
            std::cout<< "The file containing the query feature is not of appropriate size ( dimension * sizeof(float) )\n";
            // TODO: ideally compute one but this is not currently implemented so return empty
            queryRes.clear();
            return;
            
        }
        
        // load qVec from disk
        float *qVec= new float[ numDims_ ];
        FILE *fFeat= fopen( query_obj.compDataFn.c_str(), "rb" );
        uint32_t temp_= fread( qVec, numDims_, sizeof(float), fFeat );
        if (false && temp_) {} // to avoid the warning about not checking temp_
        fclose(fFeat);
        
        // query
        CR->findKNN( qVec, toReturn==0 ? std::numeric_limits<uint32_t>::max() : toReturn, vecIDdists );
        delete []qVec;
        
    }
    
    if (toReturn!=0 && vecIDdists.size()>toReturn)
        vecIDdists.resize(toReturn);
    
    convertIDdistsToIndScores( vecIDdists, queryRes );
    
}



void
nnSingleRetriever::convertIDdistsToIndScores( std::vector<nnSearcher::vecIDdist> const &vecIDdists, std::vector<indScorePair> &queryRes ) {
    
    queryRes.clear();
    queryRes.reserve( vecIDdists.size() );
    for (std::vector<nnSearcher::vecIDdist>::const_iterator it= vecIDdists.begin(); it!=vecIDdists.end(); ++it)
        queryRes.push_back( std::make_pair( it->ID, 1 - it->distSq/2 ) );
    
}
