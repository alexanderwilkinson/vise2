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

#include "char_streams.h"
#include "same_random.h"
#include "macros.h"



bool
testStreams(uint32_t bits){
    
    std::cout<<"testing "<<bits<<": \t"; std::cout.flush();
    uint32_t maxNum= (bits>=32) ? // do special for 64bit
        ~static_cast<uint32_t>(0) :
        (static_cast<uint32_t>(1) << bits);
    
    std::vector<uint64_t> vals;
    vals.reserve(5000);
    
    for (uint32_t iTest=0; iTest<100; ++iTest){
        
        sameRandomUint32 rand(6000 * ( (bits<=32) ? 1 : 2 ), 43+iTest);
        sameRandomStreamUint32 randStream(rand);
        
        uint32_t N= randStream.getNextNtoM(3000, 5000);
        
        charStream *c1= charStream::charStreamCreate(bits);
        c1->reserve(N);
        
        vals.clear();
        for (uint32_t i=0; i<N; ++i){
            vals.push_back( (bits<=32) ?
                randStream.getNext0ToN(maxNum) :
                (static_cast<uint64_t>(randStream.getNext0ToN(maxNum))<<32) | randStream.getNext0ToN(maxNum) );
            c1->add( vals.back() );
        }
        
        charStream *c2= charStream::charStreamCreate(bits);
        c2->setDataCopy( c1->getDataCopy() );
        
        delete c1;
        
        ASSERT(N == c2->getNum());
        
        uint64_t v;
        for (uint32_t i=0; i<N; ++i){
            v= c2->getNextUnsafe();
            if (vals[i] != v){
                std::cout<<i<<" "<<vals[i]<<" "<<v<<"\n";
                ASSERT( vals[i] == v );
            }
        }
        for (uint32_t i=N; i>0; --i){
            c2->setIter(i-1);
            v= c2->getNextUnsafe();
            if (vals[i-1] != v){
                std::cout<<i-1<<" "<<vals[i-1]<<" "<<v<<"\n";
                ASSERT( vals[i-1] == v );
            }
        }
        
        delete c2;
    }
    
    std::cout<<"OK\n";
    
    return true;
}



int main(){
    
    bool allok= true;
    allok= testStreams( 4); if (!allok) return 1;
    allok= testStreams( 6); if (!allok) return 1;
    allok= testStreams( 8); if (!allok) return 1;
    allok= testStreams(10); if (!allok) return 1;
    allok= testStreams(12); if (!allok) return 1;
    allok= testStreams(16); if (!allok) return 1;
    allok= testStreams(32); if (!allok) return 1;
    allok= testStreams(64); if (!allok) return 1;
    
    return 0;
}
