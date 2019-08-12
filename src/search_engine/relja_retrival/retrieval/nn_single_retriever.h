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

#ifndef _NN_SINGLE_RETRIEVER_
#define _NN_SINGLE_RETRIEVER_

#include <stdexcept>

#include "retriever.h"
#include "coarse_residual.h"
#include "index_with_data.h"



class nnSingleRetriever : public retriever {
    
    public:
        
        nnSingleRetriever( indexWithData const *aFidx, coarseResidual const &aCR ) : fidx(aFidx), CR( &aCR ) {}
        
        virtual
            ~nnSingleRetriever() {}
        
        virtual void
            externalQuery_computeData( std::string imageFn, query const &query_obj ) const
                { throw std::runtime_error("Not implemented"); }
        
        virtual void
            queryExecute( query const &query_obj, std::vector<indScorePair> &queryRes, uint32_t toReturn= 0 ) const;
        
        uint32_t
            numDocs() const { return fidx->numIDs(); }
    
    private:
        
        static void
            convertIDdistsToIndScores( std::vector<nnSearcher::vecIDdist> const &vecIDdists, std::vector<indScorePair> &queryRes );
        
        indexWithData const *fidx;
        coarseResidual const *CR;
        
};

#endif
