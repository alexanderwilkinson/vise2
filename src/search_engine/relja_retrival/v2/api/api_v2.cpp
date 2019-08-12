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

#include "spatial_api.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <vector>
#include <stdexcept>
#include <string>

#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/lambda/construct.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/thread.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <fastann/fastann.hpp>

#include "clst_centres.h"
#include "dataset_v2.h"
#include "feat_getter.h"
#include "feat_standard.h"
#include "hamming.h"
#include "hamming_embedder.h"
#include "index_entry.pb.h"
#include "macros.h"
#include "mq_filter_outliers.h"
#include "par_queue.h"
#include "proto_db.h"
#include "proto_db_file.h"
#include "proto_index.h"
#include "python_cfg_to_ini.h"
#include "slow_construction.h"
#include "soft_assigner.h"
#include "spatial_verif_v2.h"
#include "tfidf_data.pb.h"
#include "tfidf_v2.h"
#include "util.h"



int main(int argc, char* argv[]){
    MPI_INIT_ENV
    
    // ------------------------------------ setup basic
    
    int APIport= 35200;
    if (argc>1) APIport= atoi(argv[1]);
    std::cout<<"APIport= "<<APIport<<"\n";
    
    std::string dsetname= "oxMini20_v2";
    if (argc>2) dsetname= argv[2];
    
    std::string configFn= "../src/ui/web/config/config.cfg";
    if (argc>3) configFn= argv[3];
    
    configFn= util::expandUser(configFn);
    std::string tempConfigFn= util::getTempFileName();
    pythonCfgToIni( configFn, tempConfigFn );
    
    boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini(tempConfigFn, pt);
    
    // ------------------------------------ read config
    
    std::string const dsetFn= util::expandUser(pt.get<std::string>( dsetname+".dsetFn" ));
    boost::optional<std::string> const clstFn= pt.get_optional<std::string>( dsetname+".clstFn" );
    std::string const iidxFn= util::expandUser(pt.get<std::string>( dsetname+".iidxFn" ));
    std::string const fidxFn= util::expandUser(pt.get<std::string>( dsetname+".fidxFn" ));
    std::string const wghtFn= util::expandUser(pt.get<std::string>( dsetname+".wghtFn" ));
    boost::optional<std::string> const trainFilesPrefix= pt.get_optional<std::string>( util::expandUser( dsetname+".trainFilesPrefix" ));
    
    boost::optional<uint32_t> const hammEmbBits= pt.get_optional<uint32_t>( dsetname+".hammEmbBits" );
    bool const useHamm= hammEmbBits.is_initialized();
    
    std::string const docMapFindPath= pt.get<std::string>( dsetname+".docMapFindPath", "" );
    boost::optional<std::string> const docMapReplacePath= pt.get_optional<std::string>( dsetname+".docMapReplacePath" );
    std::string databasePath= pt.get<std::string>( dsetname+".databasePath", "");
    ASSERT( !(docMapReplacePath.is_initialized() && databasePath.length()>0) );
    if (docMapReplacePath.is_initialized())
        databasePath= *docMapReplacePath;
    
    bool useRootSIFT= pt.get<bool>(dsetname+".RootSIFT", true);
    
    remove(tempConfigFn.c_str());
    
    datasetV2 dset( dsetFn, databasePath, docMapFindPath ); // needed for register
    
    std::cout<<dset.getFn( 0 )<<"\n";;
    
    
    sequentialConstructions *consQueue= new sequentialConstructions();
    
    
    // Set up forward index
    
    #if 0
        protoDbFile dbFidx_file(fidxFn);
        protoDbInRam dbFidx(dbFidx_file);
    #else
        protoDb *dbFidx_file= new protoDbFile(fidxFn);
        boost::function<protoDb*()> fidxInRamConstructor= boost::lambda::bind(
            boost::lambda::new_ptr<protoDbInRam>(),
            boost::cref(*dbFidx_file) );
        
        protoDbInRamStartDisk dbFidx( *dbFidx_file, fidxInRamConstructor, true, consQueue );
    #endif
    
    protoIndex fidx(dbFidx, false);
    
    
    // Set up inverted index
    
    #if 0
        protoDbFile dbIidx_file(iidxFn);
        protoDbInRam dbIidx(dbIidx_file);
    #else
        protoDb *dbIidx_file= new protoDbFile(iidxFn);
        boost::function<protoDb*()> iidxInRamConstructor= boost::lambda::bind(
            boost::lambda::new_ptr<protoDbInRam>(),
            boost::cref(*dbIidx_file) );
        
        protoDbInRamStartDisk dbIidx( *dbIidx_file, iidxInRamConstructor, true, consQueue );
        
    #endif
    
    protoIndex iidx(dbIidx, false);
    
    
    // start the construction of inRam stuff
    consQueue->start();
    
    
    // feature getter / assigner
    
    featGetter *featGetter_obj= NULL;
    clstCentres *clstCentres_obj= NULL;
    fastann::nn_obj<float> const *nn= NULL;
    softAssigner *SA= NULL;
    
    if (clstFn.is_initialized()){
        
        // feature getter
        bool SIFTscale3= pt.get<bool>( dsetname+".SIFTscale3", true);
        featGetter_obj= new featGetter_standard( (
            std::string("hesaff-") +
            std::string((useRootSIFT ? "rootsift" : "sift")) +
            std::string(SIFTscale3 ? "-scale3" : "")
            ).c_str() );
        
        // clusters
        std::cout<<"apiV2::main: Loading cluster centres\n";
        double t0= timing::tic();
        clstCentres_obj= new clstCentres( util::expandUser(*clstFn).c_str(), true );
	//std::cout<<"Yes:"<<clstFn<<'\n';
        std::cout<<"apiV2::main: Loading cluster centres - DONE ("<< timing::toc(t0) <<" ms)\n";
        
        std::cout<<"apiV2::main: Constructing NN search object\n";
        t0= timing::tic();
        
        nn=
        #if 1
            fastann::nn_obj_build_kdtree(
                clstCentres_obj->clstC_flat,
                clstCentres_obj->numClst,
                clstCentres_obj->numDims, 8, 1024);
        #else
            fastann::nn_obj_build_exact(
                clstCentres_obj->clstC_flat,
                clstCentres_obj->numClst,
                clstCentres_obj->numDims);
        #endif
        std::cout<<"apiV2::main: Constructing NN search object - DONE ("<< timing::toc(t0) << " ms)\n";
        
        // soft assigner
        if (!useHamm) {
            if (useRootSIFT)
                SA= new SA_exp( 0.02 );
            else
                SA= new SA_exp( 6250 );
        }

    ///////// just for output the file path from the docID
/*	std::ifstream fin;
	fin.open("/mnt/data2/Yujie/data/BBC/results.txt");
	std::ofstream p;
	p.open("/mnt/data2/Yujie/data/BBC/results_path.txt");
	std::string str;
        for(uint32_t i=0; i<4780; ++i){
    		uint32_t docdocID;
		fin>>docdocID;
    		p<<dset.getFn(docdocID)<<'\n';
	}

	fin.close();
	p.close();
*/
    ////////////////////////


    }
    
    
    // embedder
    embedderFactory *embFactory= NULL;
    if (useHamm){
        //uint32_t const vocSize= pt.get<uint32_t>( dsetname+".vocSize" );
        std::string const trainFilesPrefix= util::expandUser(pt.get<std::string>( dsetname+".trainFilesPrefix" ));
        //std::string const trainHammFn= trainFilesPrefix + util::uintToShortStr(vocSize) + "_hamm" + boost::lexical_cast<std::string>(*hammEmbBits) + ".v2bin";
        std::string const trainHammFn = trainFilesPrefix + "hamm.v2bin";
        
        embFactory= new hammingEmbedderFactory(trainHammFn, *hammEmbBits);
    }
    else
        embFactory= new noEmbedderFactory;
    
    // create retrievers
    retrieverFromIter *baseRetriever;
    hamming *hammingObj= NULL;
    tfidfV2 tfidfObj(
        &iidx, &fidx, wghtFn,
        featGetter_obj, nn, SA);
        // but need SA too featGetter_obj, nn);
    
    if (useHamm){
        hammingObj= new hamming(
            tfidfObj,
            &iidx,
            *dynamic_cast<hammingEmbedderFactory const *>(embFactory),
            &fidx,
            featGetter_obj, nn, clstCentres_obj);
        baseRetriever= hammingObj;
    } else
        baseRetriever= &tfidfObj;

//     fakeSpatialRetriever spatVerifObj(*baseRetriever);
    spatialVerifV2 spatVerifObj(
        *baseRetriever, &iidx, &fidx, true,
        featGetter_obj, nn, clstCentres_obj);
    
    // multiple queries
    
    multiQueryMax mqOrig( spatVerifObj );
    mqFilterOutliers *mqFilter= NULL;
    multiQuery *mq;
    
    if (hammingObj!=NULL){
        mqFilter= new mqFilterOutliers(
                        mqOrig,
                        spatVerifObj,
                        *dynamic_cast<hammingEmbedderFactory const *>(embFactory) );
        mq= mqFilter;
    } else
        mq= &mqOrig;
    
    // API object
    
    API API_obj( spatVerifObj, mq, dset );
    
    // start
    boost::asio::io_service io_service;
    API_obj.server(io_service, APIport);
    
    // make sure this is deleted before everything which uses it
    delete consQueue;
    
    if (hammingObj!=NULL){
        delete hammingObj;
        delete mqFilter;
    }
    delete embFactory;
    
    if (clstCentres_obj!=NULL){
        delete nn;
        delete clstCentres_obj;
        delete featGetter_obj;
        if (!useHamm)
            delete SA;
    }
    
    return 0;
   


}
