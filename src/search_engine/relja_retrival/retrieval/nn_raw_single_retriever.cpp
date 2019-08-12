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

#include "nn_raw_single_retriever.h"

#include "jp_dist2.hpp"




void
rawSingleRetriever::internalQuery( query const &query_obj, std::vector<indScorePair> &queryRes, uint32_t toReturn ) const {
    uint32_t numDescs;
    float *desc, *qDesc;
    descFile->getDescs( query_obj.docID, numDescs, qDesc );
    queryRes.reserve( numDocs_ );
    for (uint32_t docID= 0; docID<numDocs_; ++docID){
        descFile->getDescs( docID, numDescs, desc );
        queryRes.push_back( std::make_pair(docID, 1 - jp_dist_l2(qDesc, desc, numDims_)/2 ) );
        delete []desc;
    }
    delete []qDesc;
    retriever::sortResults(queryRes, 0, toReturn);
}
