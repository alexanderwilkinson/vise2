#include <sstream>

#include "imreg_sift/imreg_sift.h"

void test_diff_image() {
  string compare_id = "test_vl_register_images";
  boost::filesystem::path upload_dir_("/home/tlm/exp/imcomp/images/traherne_book/set2/");
  boost::filesystem::path result_dir_("/home/tlm/exp/imcomp/images/traherne_book/set2/result/");

  string fid1 = "im1";
  string fid2 = "im2";

  boost::filesystem::path im1_fn = upload_dir_ / (fid1 + ".jpg");
  boost::filesystem::path im2_fn = upload_dir_ / (fid2 + ".jpg");
  boost::filesystem::path im1_out_fn = result_dir_ / (fid1 + "_" + compare_id + "_crop.jpg");
  boost::filesystem::path im2_out_fn = result_dir_ / (fid2 + "_" + compare_id + + "_crop.jpg");
  boost::filesystem::path im2_tx_fn  = result_dir_ / (fid2 + "_" + compare_id + + "_crop_tx.jpg");
  boost::filesystem::path diff_fn    = result_dir_ / (fid1 + "_" + fid2 + "_" + compare_id + "_diff.jpg");
  boost::filesystem::path overlap_fn    = result_dir_ / (fid1 + "_" + fid2 + "_" + compare_id + "_overlap.jpg");

  unsigned int file1_region[4] = {0, 0, 550, 482}; // x0, y0, x1, y1

  size_t fp_match_count = -1;
  MatrixXd H(3,3);
  bool success;

  imreg_sift::ransac_dlt( im1_fn.string().c_str(),
                           im2_fn.string().c_str(),
                           file1_region[0], file1_region[2], file1_region[1], file1_region[3],
                           H, fp_match_count,
                           im1_out_fn.string().c_str(),
                           im2_out_fn.string().c_str(),
                           im2_tx_fn.string().c_str(),
                           diff_fn.string().c_str(),
                           overlap_fn.string().c_str(),
                           success
                           );
}
void test_projective_reg() {
  string compare_id = "test";

  boost::filesystem::path upload_dir_("/home/tlm/exp/imcomp/images/traherne_book/set3/");
  boost::filesystem::path result_dir_("/home/tlm/exp/imcomp/images/traherne_book/set3/result_proj/");

  //unsigned int file1_region[4] = {0, 0, 741, 1023}; // x0, y0, x1, y1 : set 1
  //unsigned int file1_region[4] = {0, 0, 551, 482}; // x0, y0, x1, y1 : set 2
  unsigned int file1_region[4] = {0, 0, 347, 239}; // x0, y0, x1, y1 : set 3
  //unsigned int file1_region[4] = {0, 0, 577, 1023}; // x0, y0, x1, y1 : set 3


/*
  // synthetic test case
  boost::filesystem::path upload_dir_("/home/tlm/exp/imcomp/images/15cbt/set1/");
  boost::filesystem::path result_dir_("/home/tlm/exp/imcomp/images/15cbt/set1/result/");
  unsigned int file1_region[4] = {0, 0, 646, 478}; // x0, y0, x1, y1 : set 1
*/

  string fid1 = "im1";
  string fid2 = "im2";

  boost::filesystem::path im1_fn = upload_dir_ / (fid1 + ".jpg");
  boost::filesystem::path im2_fn = upload_dir_ / (fid2 + ".jpg");

  boost::filesystem::path im1_out_fn = result_dir_ / (fid1 + "_crop.jpg");
  boost::filesystem::path im2_out_fn = result_dir_ / (fid2 + "_crop.jpg");
  boost::filesystem::path im2_tx_fn  = result_dir_ / (fid2 + "_tx.jpg");
  boost::filesystem::path diff_fn    = result_dir_ / ("diff.jpg");
  boost::filesystem::path overlap_fn    = result_dir_ / ("overlap.jpg");

  size_t fp_match_count = -1;
  MatrixXd H(3,3);
  bool success;

  imreg_sift::ransac_dlt( im1_fn.string().c_str(),
                           im2_fn.string().c_str(),
                           file1_region[0], file1_region[2], file1_region[1], file1_region[3],
                           H, fp_match_count,
                           im1_out_fn.string().c_str(),
                           im2_out_fn.string().c_str(),
                           im2_tx_fn.string().c_str(),
                           diff_fn.string().c_str(),
                           overlap_fn.string().c_str(),
                           success
                           );
}

void test_tps_reg() {
  string compare_id = "tps";
  boost::filesystem::path upload_dir_("/home/tlm/exp/imcomp/images/traherne_demo/set1/");
  boost::filesystem::path result_dir_("/home/tlm/exp/imcomp/images/traherne_demo/set1/result_tps/");

  //unsigned int file1_region[4] = {0, 0, 741, 1023}; // x0, y0, x1, y1 : set 1
  unsigned int file1_region[4] = {0, 0, 1121, 1559}; // x0, y0, x1, y1 : set 2
  //unsigned int file1_region[4] = {0, 0, 347, 239}; // x0, y0, x1, y1 : set 3

  string fid1 = "im1";
  string fid2 = "im2";

  boost::filesystem::path im1_fn = upload_dir_ / (fid1 + ".jpg");
  boost::filesystem::path im2_fn = upload_dir_ / (fid2 + ".jpg");

  boost::filesystem::path im1_out_fn = result_dir_ / (fid1 + "_" + compare_id + "_crop.jpg");
  boost::filesystem::path im2_out_fn = result_dir_ / (fid2 + "_" + compare_id + + "_crop.jpg");
  boost::filesystem::path im2_tx_fn  = result_dir_ / (fid2 + "_" + compare_id + + "_crop_tx.jpg");
  boost::filesystem::path diff_fn    = result_dir_ / (fid1 + "_" + fid2 + "_" + compare_id + "_diff.jpg");
  boost::filesystem::path overlap_fn    = result_dir_ / (fid1 + "_" + fid2 + "_" + compare_id + "_overlap.jpg");
  //boost::filesystem::path overlap_fn    = result_dir_;

  size_t fp_match_count = -1;
  MatrixXd H(3,3);
  bool success;

  imreg_sift::robust_ransac_tps( im1_fn.string().c_str(),
                           im2_fn.string().c_str(),
                           file1_region[0], file1_region[2], file1_region[1], file1_region[3],
                           H, fp_match_count,
                           im1_out_fn.string().c_str(),
                           im2_out_fn.string().c_str(),
                           im2_tx_fn.string().c_str(),
                           diff_fn.string().c_str(),
                           overlap_fn.string().c_str(),
                           success
                           );
}

int main(int argc, char** argv) {
  Magick::InitializeMagick(*argv);

  //test_diff_image();
  test_tps_reg();
  //test_projective_reg();

  return 0;
}
