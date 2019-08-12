#ifndef _VL_REGISTER_IMAGES_H_
#define _VL_REGISTER_IMAGES_H_

extern "C" {
#include <vl/generic.h>
#include <vl/stringop.h>
#include <vl/pgm.h>
#include <vl/sift.h>
#include <vl/getopt_long.h>

}

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <limits>
#include <random>
#include <algorithm>
#include <fstream>
#include <chrono>  // for high_resolution_clock

#include <cmath>

#include <Eigen/Dense>
#include <Eigen/SVD>

#include <boost/filesystem.hpp>

#include <Magick++.h>

using namespace Eigen;
using namespace std;

class imreg_sift {
 public:
  // Applies RANSAC and Direct Linear Transform (DLT)
  // see Chapter 4 of Hartley and Zisserman (2nd Edition)
  static void ransac_dlt(const char im1_fn[], const char im2_fn[],
                          double xl, double xu, double yl, double yu,
                          MatrixXd& Hopt, size_t& fp_match_count,
                          const char im1_crop_fn[], const char im2_crop_fn[], const char im2_tx_fn[],
                          const char diff_image_fn[],
                          const char overlap_image_fn[],
                          bool& success);

  static void ransac_dlt(const char im1_fn[], const char im2_fn[],
                         double x, double y, double width, double height,
                         MatrixXd& Hopt, size_t& fp_match_count,
                         Magick::Image& im2_crop_tx, bool& success);

  // Applies robust filtering of point correspondences and uses Thin Plate Spline for image registration
  //
  // Robust filtering of point correspondences are based on:
  // Tran, Q.H., Chin, T.J., Carneiro, G., Brown, M.S. and Suter, D., 2012, October. In defence of RANSAC for outlier rejection in deformable registration. ECCV.
  //
  // Thin plate spline registration based on:
  // Bookstein, F.L., 1989. Principal warps: Thin-plate splines and the decomposition of deformations. IEEE TPAMI.
  static void robust_ransac_tps(const char im1_fn[], const char im2_fn[],
                                double xl, double xu, double yl, double yu,
                                MatrixXd& Hopt, size_t& fp_match_count,
                                const char im1_crop_fn[], const char im2_crop_fn[], const char im2_tx_fn[],
                                const char diff_image_fn[],
                                const char overlap_image_fn[],
                                bool& success);

 private:
  // implementation of Direct Linear Transform (DLT)
  // see Chapter 4 of Hartley and Zisserman (2nd Edition)
  static void dlt(const MatrixXd& X, const MatrixXd& Y, Matrix<double,3,3>& H);

  // use vlfeat to compute SIFT keypoint and descriptors
  //static void compute_sift_features(const string filename, vector<VlSiftKeypoint>& keypoint_list, vector< vector<vl_uint8> >& descriptor_list, bool verbose=false);
  static void compute_sift_features(const Magick::Image& img, vector<VlSiftKeypoint>& keypoint_list, vector< vector<vl_uint8> >& descriptor_list, bool verbose=false);

  // get putative matches based on Lowe's algorithm
  static void get_putative_matches(vector< vector<vl_uint8> >& descriptor_list1, vector< vector<vl_uint8> >& descriptor_list2, std::vector< std::pair<uint32_t, uint32_t> > &putative_matches, float threshold);

  // utility functions
  static void get_norm_matrix(const MatrixXd& pts, Matrix<double,3,3>& T);
  static vector<double> get_pixel_percentile(Magick::Image& img, const vector<unsigned int> percentile);
  static inline double clamp(double v, double min, double max);
  static void get_diff_image(Magick::Image& im1, Magick::Image& im2, Magick::Image& cdiff);
};


#endif
