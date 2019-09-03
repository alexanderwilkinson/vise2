/** @file   mpi_search.cc
 *  @brief  Search with suppport for MPI
 *
 *  @author Kaloyan Petrov (kaloyan_petrov@mail.bg)
 *  @date   11 May 2019
 */

#define RR_MPI
#include <omp.h>

#include <iostream>
#include <string>
#include <map>
#include <thread>

#define BOOST_LOG_DYN_LINK 1
#include <boost/log/trivial.hpp>
#include <boost/filesystem.hpp>

#include "vise/relja_retrival.h"
#include "vise/util.h"

//#include "mpi_queue.h"
using namespace std;

struct result{  //search result
  unsigned int file_id;
  float score;
  double H[9];
  string filename;
  string metadata;
};

struct region_query{
  int file_id;
  int x,y,width,height;
};

static map<string, string> cfg;
static vector<struct region_query> queries;

static vise::relja_retrival * relja = NULL;

static bool se_qeury(const string& se_id, struct region_query * q, const float score_threshold, vector<struct result>& results) {

  //result vectors
  std::vector<unsigned int> result_file_id;
  std::vector<float> result_score;
  std::vector< std::array<double, 9> > result_H;
  std::vector<std::string> result_filename;
  std::vector<std::string> result_metadata;

  //make the query
  relja->query_using_file_region(q->file_id, q->x, q->y, q->width, q->height, score_threshold,
    result_file_id, result_filename, result_metadata, result_score, result_H);

  //save result to row
  for ( std::size_t i=0; i < result_score.size(); ++i ) {

    if(result_file_id[i] == q->file_id){  //if result and row match, its the same file
      continue; //don't put it in results
    }

    struct result result;
    result.file_id  = result_file_id[i];
    result.filename = result_filename[i];
    result.metadata = result_metadata[i];
    result.score    = result_score[i];
    memcpy(&result.H, result_H[i].data(), 9*sizeof(double));

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


static bool load_rows(const char * image_coordinates_fn, vector<struct region_query>& queries){

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
      q.x         = stoi(columns[1]);
      q.y         = stoi(columns[2]);
      q.width     = stoi(columns[3]);
      q.height    = stoi(columns[4]);

      queries.push_back(q);
    }
  }
  ifs.close();

  return true;
}

static void print_row(ofstream& ofs, struct region_query *query, vector<struct result>& results){

  for(unsigned int j=0; j < results.size(); j++){
    struct result * result = &results[j];
    ofs << relja->get_filename(query->file_id) << "," << query->x << "," << query->y << "," << query->width << "," << query->height << ","
        << result->file_id << "," << result->filename << "," << result->metadata << "," << result->score << ","
        << result->H[0] << "," << result->H[1] << "," << result->H[2] << ","
        << result->H[3] << "," << result->H[4] << "," << result->H[5] << ","
        << result->H[6] << "," << result->H[7] << "," << result->H[8]
        << endl;
  }
}

int main(int argc, char** argv) {

  MPI_INIT_ENV

  if(argc != 3){

    cerr << "Usage: " << argv[0] << " ImageCoordinates.csv 50.0" << endl;
    return 1;
  }

  const char * image_coordinates_fn = argv[1];
  const float threshold = atof(argv[2]);

  if( !load_config(cfg)){
    cerr << "Error: Failed to load config" << endl;
    return 2;
  }

  boost::filesystem::path data_path{cfg["data_dir"]};
  boost::filesystem::path asset_path{cfg["asset_dir"]};
  boost::filesystem::path temp_path{cfg["temp_dir"]};

  relja = new vise::relja_retrival(cfg["se_id"], data_path, asset_path, temp_path);
  if(relja == NULL){
    cerr << "Error: relja_retrival new failed" << endl;
    return 3;
  }
  relja->load();

  if(!load_rows(image_coordinates_fn, queries)){
    return EXIT_FAILURE;
  }
  cout << "Found " << queries.size() << " queries."<< endl;

  //append the threshold parameter to filename
  const string output_filename = "output_" + to_string(threshold) + ".csv";
  ofstream ofs = ofstream(output_filename, ofstream::out);
  if(!ofs.is_open()){
    cerr << "Error: Failed to open " << output_filename << endl;
    return 4;
  }
  //show the column labels
  ofs << "#filename,x,y,width,height,file_id_result,filename2,metadata,score,H0,H1,H2,H3,H4,H5,H6,H7,H8" << endl;

#pragma omp parallel
{
  vector<struct result> results;  //result array for each thread

#pragma omp for schedule(dynamic)
  for(unsigned int i=0; i < queries.size(); i++){

    struct region_query * r = &queries[i];
    se_qeury(cfg["se_id"], r, threshold, results); //search

    if(results.size() > 0){
      #pragma omp critical
      {
        cout << "Thread " << omp_get_thread_num() << " found " << results.size() << " results for row_id=" << i << endl;
        print_row(ofs, r, results); //save result to output file
      }
      results.clear();
    }
  }
}

  ofs.close();
  delete relja;

  return EXIT_SUCCESS;
}
