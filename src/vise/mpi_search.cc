/** @file   mpi_search.cc
 *  @brief  Search with suppport for MPI
 *
 *  @author Kaloyan Petrov (kaloyan_petrov@mail.bg)
 *  @date   11 May 2019
 */

#define RR_MPI

#include <iostream>
#include <string>
#include <map>
#include <thread>

#define BOOST_LOG_DYN_LINK 1
#include <boost/log/trivial.hpp>
#include <boost/filesystem.hpp>
//#include <boost/serialization/string.hpp>
//#include <boost/serialization/vector.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/mpi.hpp>

#include "vise/relja_retrival.h"
#include "vise/util.h"

//#include "mpi_queue.h"
using namespace std;

enum tags{ TAG_RESULT=0, TAG_QUIT};

struct result{  //search result
  unsigned int row_id;
  unsigned int file_id;
  float score;
  double H[9];
  char filename[100];
  char metadata[100];

  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive & ar, const unsigned int version)
  {
    ar & row_id;
    ar & file_id;
    ar & score;
    ar & H;
    ar & filename;
    ar & metadata;
  }
};
BOOST_IS_MPI_DATATYPE (result);

struct region_query{
  unsigned int file_id;
  unsigned int x,y,width,height;
};

static map<string, string> cfg;
static vector<struct region_query> queries;

static vise::relja_retrival * relja = NULL;

static bool se_qeury(const string& se_id, unsigned int ri, const float score_threshold, vector<struct result>& results) {

  struct region_query * r = &queries[ri];

  //result vectors
  std::vector<unsigned int> result_file_id;
  std::vector<float> result_score;
  std::vector< std::array<double, 9> > result_H;
  std::vector<std::string> result_filename;
  std::vector<std::string> result_metadata;

  //make the query
  relja->query_using_file_region(r->file_id, r->x, r->y, r->width, r->height, score_threshold,
    result_file_id, result_filename, result_metadata, result_score, result_H);

  //save result to row
  for ( std::size_t i=0; i < result_score.size(); ++i ) {

    if(result_file_id[i] == r->file_id){  //if result and row match, its the same file
      continue; //don't put it in results
    }

    struct result result;
    result.row_id   = ri;
    result.file_id  = result_file_id[i];

    strncpy(result.filename, result_filename[i].c_str(), 100);
    strncpy(result.metadata, result_metadata[i].c_str(), 100);
    result.filename[100] = 0;
    result.metadata[100] = 0;

    result.score    = result_score[i];
    memcpy(result.H, result_H[i].data(), 9*sizeof(double));

    results.push_back(result);
  }

  return true;
}

static bool load_config(map<string,string>& cfg) {

  char * config_filename = getenv("CONFIG_FILENAME");
  if(config_filename == NULL){
    cerr << "Error: CONFIG_FILENAME not set" << endl;
    return false;
  }

  ifstream ifs = ifstream(config_filename, std::ifstream::in);
  if(!ifs.is_open()){
    cerr << "Error: Can't open config %s" << config_filename << endl;
    return false;
  }

  string line;
  while (getline(ifs, line)) {  //read each line from config
    if(line[0] == '['){
      cfg[ "se_id" ] = line.substr(1, line.size()-2);
    }else{
      size_t pos = line.find('=');
      if(pos == string::npos){
        ifs.close();
        return false;
      }
      string key = line.substr(0, pos);
      string val = line.substr(pos+1);

      cfg[key] = val;
    }
  }
  ifs.close();

  if (cfg.find("data_dir") == cfg.end()){
    cerr << "Error: data_dir not defined" << endl;
    return false;
  }

  return true;
}


static bool load_queries(const char * image_coordinates_fn, vector<struct region_query>& queries){

  ifstream ifs = ifstream(image_coordinates_fn, std::ifstream::in);
  if(!ifs.is_open()){
    cerr << "Error: Can't open config %s" << image_coordinates_fn << endl;
    return false;
  }

  string line;
  while (getline(ifs, line)){  //read each line from config

    if(line.back() = '\r'){
      line.pop_back();
    }

    vector<string> columns = vise::util::split(line, ',');

    if(columns[0].compare("Filename") != 0){  //if we have a header

      struct region_query q;
      q.file_id   = relja->get_file_id(columns[0]);
      q.x         = stoul(columns[1]);
      q.y         = stoul(columns[2]);
      q.width     = stoul(columns[3]);
      q.height    = stoul(columns[4]);

      queries.push_back(q);
    }
  }
  ifs.close();

  return true;
}

static void print_row(ofstream& ofs, struct result* result){
  struct region_query * query  = &queries[result->row_id];
  ofs << relja->get_filename(query->file_id) << "," << query->x << "," << query->y << "," << query->width << "," << query->height << ","
      << result->file_id << "," << result->filename << "," << result->metadata << "," << result->score << ","
      << result->H[0] << "," << result->H[1] << "," << result->H[2] << ","
      << result->H[3] << "," << result->H[4] << "," << result->H[5] << ","
      << result->H[6] << "," << result->H[7] << "," << result->H[8]
      << endl;
}

int main(int argc, char** argv) {

  MPI_INIT_ENV
  MPI_GLOBAL_ALL

  if(argc != 3){
    if(rank == 0){
      cerr << "Usage: " << argv[0] << " ImageCoordinates.csv 50.0" << endl;
    }
    boost::mpi::environment::abort(1);
  }
  int buf[2];
  const int root = (numProc - 1);

  if(numProc < 2){
    cerr << "Error: At least two nodes are required!" << endl;
    boost::mpi::environment::abort(1);
  }

  const char * image_coordinates_fn = argv[1];
  const float threshold = atof(argv[2]);

  if( !load_config(cfg)){
    if(rank == root){
      cerr << "Error: Failed to load config" << endl;
    }
    boost::mpi::environment::abort(2);
  }

  boost::filesystem::path data_path{cfg["data_dir"]};
  boost::filesystem::path asset_path{cfg["asset_dir"]};
  boost::filesystem::path temp_path{cfg["temp_dir"]};

  relja = new vise::relja_retrival(cfg["se_id"], data_path, asset_path, temp_path);
  if(relja == NULL){
    if(rank == root){
      cerr << "Error: relja_retrival new failed" << endl;
    }
    boost::mpi::environment::abort(3);
  }

  if(!relja->load()){
    if(rank == root){
      cerr << "Error: relja load failed" << endl;
    }
    boost::mpi::environment::abort(5);
  }

  if(!load_queries(image_coordinates_fn, queries)){
    return EXIT_FAILURE;
  }
  if(rank == root){
    cout << "Found " << queries.size() << " queries."<< endl;
  }

  unsigned int chunk_size = queries.size() / (numProc - 1);  //-1 because root only saves results

  //query start,end index
  unsigned int start = rank * chunk_size, end = start + chunk_size;
  if(rank == (root-1)){
    end += queries.size() % (root-1);
  }

  if(rank == root){  //only root saves results

    //append the threshold parameter to filename
    const string output_filename = "output_" + to_string(threshold) + ".csv";
    ofstream ofs = ofstream(output_filename, ofstream::out);
    if(!ofs.is_open()){
      cerr << "Error: Failed to open " << output_filename << endl;
      boost::mpi::environment::abort(4);
    }
    //show the column labels
    ofs << "#filename,x,y,width,height,file_id_result,filename2,metadata,score,H0,H1,H2,H3,H4,H5,H6,H7,H8" << endl;

    int num_searching = numProc - 1;  //count of searching nodes

    struct result result;
    while(num_searching > 0){

      boost::mpi::status st = comm.recv(boost::mpi::any_source, boost::mpi::any_tag, result);

      if(st.tag() == TAG_QUIT){  //worked sent exit marker
        num_searching --;
        cout << "Node " << st.source() << " finished" << endl;
        continue;
      }

      //cout << "Node " << st.source() << " found " << results.size() << " results" << endl;
      print_row(ofs, &result); //save result to output file
      result.clear();
    }
    ofs.close();

  }else{  //workers do the search

  cout << "Node " << rank << " queries[" << start << "," << end << "]" << endl;

#pragma omp parallel
{
  vector<struct result> results;

#pragma omp for schedule(dynamic)
  for(unsigned int i=start; i < end; ++i){
    se_qeury(cfg["se_id"], i, threshold, results); //search

    if(results.size() > 0){
      #pragma omp critical
      {
        for(unsigned int j=0; j < results.size(); j++){
          comm.send(root, TAG_RESULT, results[j]);
        }
      }
      results.clear();
    }
  }
}

  struct result result;
  comm.send(root, TAG_QUIT, result);

} //end else

  delete relja;

  return EXIT_SUCCESS;
}
