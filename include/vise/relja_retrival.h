/** @file   search_engine.h
 *  @brief  Interface to a search engine indexed using VISE
 *
 *
 *  @author Abhishek Dutta (adutta@robots.ox.ac.uk)
 *  @date   01 July 2018
 */

#ifndef _VISE_RELJA_RETRIVAL_H_
#define _VISE_RELJA_RETRIVAL_H_

#include "vise/search_engine.h"

#include <cstdio> // for sprintf()
#include <string>
#include <array>

// for thread management
#include <thread>
#include <mutex>

// for filesystem i/o
#include <boost/filesystem.hpp>

// for logging
#define BOOST_LOG_DYN_LINK 1
#include <boost/log/trivial.hpp>

// for config file i/o
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

// used by relja_retrival
#include <boost/lambda/construct.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

// relja_retrival
#include <fastann.hpp>
#include "dataset_v2.h"        // for datasetAbs
#include "slow_construction.h" // for sequentialConstructions
#include "proto_db.h"
#include "proto_db_file.h"
#include "proto_index.h"
#include "feat_getter.h"
#include "feat_standard.h"
#include "clst_centres.h"
#include "soft_assigner.h"
#include "hamming.h"
#include "hamming_embedder.h"
#include "tfidf_data.pb.h"
#include "tfidf_v2.h"
#include "spatial_verif_v2.h"
#include "mq_filter_outliers.h"

//#include "query.h"
//#include "multi_query.h"
//#include "util.h"
//#include "par_queue.h"
//#include "macros.h"
//#include "mpi_queue.h"

using namespace std;

namespace vise {
  class relja_retrival: public search_engine {
    std::string search_engine_id_;

    // config
    boost::property_tree::ptree config_;
    void load_config();
    void save_config();
    void set_default_config();
    void set_config(const std::string name, const std::string value);

    // resources
    boost::filesystem::path data_dir_;
    boost::filesystem::path temp_dir_;
    boost::filesystem::path asset_dir_;

    // state variable
    bool is_search_engine_loaded_;

    // resources
    boost::filesystem::path dset_fn_;
    boost::filesystem::path clst_fn_;
    boost::filesystem::path iidx_fn_;
    boost::filesystem::path fidx_fn_;
    boost::filesystem::path wght_fn_;
    boost::filesystem::path hamm_fn_;
    boost::filesystem::path assign_fn_;
    boost::filesystem::path image_dir_;
    boost::filesystem::path thumbnail_dir_;
    boost::filesystem::path original_dir_;
    boost::filesystem::path config_fn_;
    boost::filesystem::path imlist_fn_;

    // search engine data structures
    datasetV2 *dataset_;
    sequentialConstructions *cons_queue_;
    protoDbFile *dbFidx_file_;
    protoDbFile *dbIidx_file_;
    protoDbInRamStartDisk *dbFidx_;
    protoDbInRamStartDisk *dbIidx_;
    protoIndex *fidx_;
    protoIndex *iidx_;
    featGetter *feat_getter_;
    clstCentres *clst_centres_;
    fastann::nn_obj<float> *nn_;
    bool use_hamm_;
    softAssigner *soft_assigner_;
    hamming *hamming_emb_;
    embedderFactory *emb_factory_;
    tfidfV2 *tfidf_;
    retrieverFromIter *base_retriever_;
    spatialVerifV2 *spatial_verif_v2_;
    spatialRetriever *spatial_retriever_;
    multiQuery *multi_query_;
    multiQueryMax *multi_query_max_;

    // threads management
    std::mutex load_mutex_;

  public:
    relja_retrival( const std::string search_engine_id,
                    const boost::filesystem::path data_dir,
                    const boost::filesystem::path asset_dir,
                    const boost::filesystem::path temp_dir);
    std::string id();
    bool init();
    bool load();
    bool unload();

    bool is_loaded();
    bool is_load_possible();

    bool query_using_upload_region();
    bool query_using_file_region(unsigned int file_id,
                                 unsigned int x, unsigned int y, unsigned int w, unsigned int h,
                                 float score_threshold,
                                 std::vector<unsigned int> &result_file_id,
                                 std::vector<std::string> &result_filename,
                                 std::vector<std::string> &result_metadata,
                                 std::vector<float> &result_score,
                                 std::vector< std::array<double, 9> > &result_H);

    bool index();

    void get_filelist(std::vector<unsigned int> &file_id_list);
    void get_filelist(const std::string filename_regex,
                      std::vector<unsigned int> &file_id_list);
    uint32_t get_filelist_size();

    std::string get_filename(unsigned int file_id);
    bool file_exists(std::string filename);
    bool file_exists(unsigned int file_id);
    std::string get_filename_absolute_path(std::string filename);
    std::string get_filename_absolute_path(unsigned int file_id);
    unsigned int get_file_id(std::string filename);
  };
}
#endif
