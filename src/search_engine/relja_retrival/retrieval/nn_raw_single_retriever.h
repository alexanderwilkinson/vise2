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

#ifndef _NN_RAW_SINGLE_RETRIEVER_
#define _NN_RAW_SINGLE_RETRIEVER_

#include <stdexcept>

#include "retriever.h"
#include "desc_from_flat_file.h"



class rawSingleRetriever : public retriever {
    public:
        
        rawSingleRetriever( descGetterFromFile const &aDescFile ) : descFile( &aDescFile ), numDocs_(aDescFile.numDocs()), numDims_(aDescFile.numDims()) {}
        
        void
            internalQuery( query const &query_obj, std::vector<indScorePair> &queryRes, uint32_t toReturn= 0 ) const;
        
        void
            externalQuery_computeData( std::string imageFn, query const &query_obj ) const
                { throw std::runtime_error("Not implemented"); }
        
        void
            queryExecute( query const &query_obj, std::vector<indScorePair> &queryRes, uint32_t toReturn= 0 ) const
                { throw std::runtime_error("Not implemented"); }
        
        uint32_t
            numDocs() const { return numDocs_; }
    
    private:
        descGetterFromFile const *descFile;
        uint32_t const numDocs_, numDims_;
        
};

#endif
