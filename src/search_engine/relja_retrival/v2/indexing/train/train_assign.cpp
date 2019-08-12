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

#include "train_assign.h"

#include <stdint.h>
#include <vector>

#include <boost/filesystem.hpp>

#include <fastann/fastann.hpp>

#include "clst_centres.h"
#include "flat_desc_file.h"
#include "mpi_queue.h"
#include "par_queue.h"
#include "timing.h"
#include "util.h"




namespace buildIndex {


typedef std::vector<uint32_t> trainAssignsResult; // clusterIDs



class trainAssignsManager : public managerWithTiming<trainAssignsResult> {
    public:
        
        trainAssignsManager(uint32_t numDocs, std::string const trainAssignsFn)
            : managerWithTiming<trainAssignsResult>(numDocs, "trainAssignsManager"),
              nextID_(0){
                f_= fopen(trainAssignsFn.c_str(), "wb");
                ASSERT(f_!=NULL);
            }
        
        ~trainAssignsManager(){ fclose(f_); }
        
        void
            compute( uint32_t jobID, trainAssignsResult &result );
    
    private:
        FILE *f_;
        uint32_t nextID_;
        std::map<uint32_t, trainAssignsResult> results_;
        
        DISALLOW_COPY_AND_ASSIGN(trainAssignsManager)
};



void
trainAssignsManager::compute( uint32_t jobID, trainAssignsResult &result ){
    // make sure results are saved sorted by job!
    results_[jobID]= result;
    if (jobID==nextID_){
        
        // save the buffered results and remove them from the map
        for (std::map<uint32_t, trainAssignsResult>::iterator it= results_.begin();
             it!=results_.end() && it->first==nextID_;
             ++nextID_){
            
            trainAssignsResult const &res= it->second;
            fwrite( &res[0],
                    sizeof(uint32_t),
                    res.size(),
                    f_ );
            
            results_.erase(it++);
        }
    }
}



class trainAssignsWorker : public queueWorker<trainAssignsResult> {
    public:
        
        trainAssignsWorker(fastann::nn_obj<float> const &nn_obj,
                           flatDescsFile const &descFile,
                           uint32_t chunkSize)
            : nn_obj_(&nn_obj),
              descFile_(&descFile),
              chunkSize_(chunkSize),
              numDescs_(descFile.numDescs())
            {}
        
        void
            operator() ( uint32_t jobID, trainAssignsResult &result ) const;
        
    private:
        
        fastann::nn_obj<float> const *nn_obj_;
        flatDescsFile const *descFile_;
        uint32_t const chunkSize_, numDescs_;
        
        DISALLOW_COPY_AND_ASSIGN(trainAssignsWorker)
};



void
trainAssignsWorker::operator() ( uint32_t jobID, trainAssignsResult &result ) const {
    
    result.clear();
    
    uint32_t start= jobID*chunkSize_;
    uint32_t end= std::min( (jobID+1)*chunkSize_, numDescs_ );
    
    float *descs;
    descFile_->getDescs(start, end, descs);
    
    result.resize(end-start);
    float *distSq= new float[end-start];
    
    nn_obj_->search_nn(descs, end-start, &result[0], distSq);
    delete []distSq;
    
    delete []descs;
}



void
computeTrainAssigns(
        std::string const clstFn,
        bool const RootSIFT,
        std::string const trainDescsFn,
        std::string const trainAssignsFn){
    
    MPI_GLOBAL_ALL;
    
    if (boost::filesystem::exists(trainAssignsFn)){
        if (rank==0)
            std::cout<<"buildIndex::computeTrainAssigns: trainAssignsFn already exist ("<<trainAssignsFn<<")\n";
        return;
    }
    ASSERT( boost::filesystem::exists(trainDescsFn) );
    
    bool useThreads= detectUseThreads();
    uint32_t numWorkerThreads= 4;
    
    // clusters
    if (rank==0)
        std::cout<<"buildIndex::computeTrainAssigns: Loading cluster centres\n";
    double t0= timing::tic();
    clstCentres clstCentres_obj( clstFn.c_str(), true );
    if (rank==0)
        std::cout<<"buildIndex::computeTrainAssigns: Loading cluster centres - DONE ("<< timing::toc(t0) <<" ms)\n";
    
    if (rank==0)
        std::cout<<"buildIndex::computeTrainAssigns: Constructing NN search object\n";
    t0= timing::tic();
    
    fastann::nn_obj<float> const *nn_obj=
    #if 1
        fastann::nn_obj_build_kdtree(
            clstCentres_obj.clstC_flat,
            clstCentres_obj.numClst,
            clstCentres_obj.numDims, 8, 1024);
    #else
        fastann::nn_obj_build_exact(
            clstCentres_obj.clstC_flat,
            clstCentres_obj.numClst,
            clstCentres_obj.numDims);
    #endif
    if (rank==0)
        std::cout<<"buildIndex::computeTrainAssigns: Constructing NN search object - DONE ("<< timing::toc(t0) << " ms)\n";
    
    flatDescsFile const descFile(trainDescsFn, RootSIFT);
    uint32_t const numTrainDescs= descFile.numDescs();
    if (rank==0)
        std::cout<<"buildIndex::computeTrainAssigns: numTrainDescs= "<<numTrainDescs<<"\n";
    
    uint32_t const chunkSize=
        std::min( static_cast<uint32_t>(10000),
                  static_cast<uint32_t>(
                      std::ceil(static_cast<double>(numTrainDescs)/std::max(numWorkerThreads, numProc))) );
    uint32_t const nJobs= static_cast<uint32_t>( std::ceil(static_cast<double>(numTrainDescs)/chunkSize) );
    
    // assign training descriptors
    
    #ifdef RR_MPI
    if (!useThreads) comm.barrier();
    #endif
    
    trainAssignsManager *manager= (rank==0) ?
        new trainAssignsManager(nJobs, trainAssignsFn) :
        NULL;
    
    trainAssignsWorker worker(*nn_obj, descFile, chunkSize);
    
    if (useThreads)
        threadQueue<trainAssignsResult>::start( nJobs, worker, *manager, numWorkerThreads );
    else
        mpiQueue<trainAssignsResult>::start( nJobs, worker, manager );
    
    if (rank==0) delete manager;
    
    delete nn_obj;
}

};
