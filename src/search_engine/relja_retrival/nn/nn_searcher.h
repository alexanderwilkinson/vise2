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

#ifndef _NN_SEARCHER_H_
#define _NN_SEARCHER_H_


#include <vector>
#include <stdint.h>



class nnSearcher {
    
    public:
        
        struct vecIDdist;
        
        virtual ~nnSearcher(){}
        
        // returns in vecIDs KNNs, sorted by ascending distance
        virtual void
            findKNN( float const qVec[], uint32_t KNN, std::vector<vecIDdist> &vecIDs ) const =0;
        
        virtual vecIDdist
            findNN( float const qVec[] ) const {
                std::vector<vecIDdist> vecIDs;
                findKNN(qVec, 1, vecIDs);
                return vecIDs[0];
            }
        
        virtual uint32_t
            numDims() const =0;
        
        struct vecIDdist {
            vecIDdist(uint32_t aID= 0, float aDistSq= -1.0) : ID(aID), distSq(aDistSq) {}
            inline int operator<(vecIDdist const &rhs) const { return distSq < rhs.distSq; }
            uint32_t ID;
            float distSq;
        };
    
};

#endif
