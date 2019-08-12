/*

Register an image pair based on SIFT features

Author: Abhishek Dutta <adutta@robots.ox.ac.uk>
Date: 3 Jan. 2018

some code borrowed from: vlfeat-0.9.20/src/sift.c

*/

#include "imreg_sift/imreg_sift.h"

// normalize input points such that their centroid is the coordinate origin (0, 0)
// and their average distance from the origin is sqrt(2).
// pts is 3 x n matrix
void imreg_sift::get_norm_matrix(const MatrixXd& pts, Matrix<double,3,3>& T) {
  Vector3d mu = pts.rowwise().mean();
  MatrixXd norm_pts = (pts).colwise() - mu;

  VectorXd x2 = norm_pts.row(0).array().pow(2);
  VectorXd y2 = norm_pts.row(1).array().pow(2);
  VectorXd dist = (x2 + y2).array().sqrt();
  double scale = sqrt(2) / dist.array().mean();

  T(0,0) = scale; T(0,1) =     0; T(0,2) = -scale * mu(0);
  T(1,0) =     0; T(1,1) = scale; T(1,2) = -scale * mu(1);
  T(2,0) =     0; T(2,1) =     0; T(2,2) = 1;
}

// Implementation of Direct Linear Transform (DLT) algorithm as described in pg. 91
// Multiple View Geometry in Computer Vision, Richard Hartley and Andrew Zisserman, 2nd Edition
// assumption: X and Y are point correspondences
// X and Y : n x 3 matrix
// H : 3x3 matrix
void imreg_sift::dlt(const MatrixXd& X, const MatrixXd& Y, Matrix<double,3,3>& H) {
  size_t n = X.rows();

  MatrixXd A(2*n, 9);
  A.setZero();
  for(size_t i=0; i<n; ++i) {
    A.row(2*i).block(0,3,1,3) = -Y(i,2) * X.row(i);
    A.row(2*i).block(0,6,1,3) =  Y(i,1) * X.row(i);

    A.row(2*i + 1).block(0,0,1,3) =  Y(i,2) * X.row(i);
    A.row(2*i + 1).block(0,6,1,3) = -Y(i,0) * X.row(i);
  }

  JacobiSVD<MatrixXd> svd(A, ComputeFullV);
  // Caution: the last column of V are reshaped into 3x3 matrix as shown in Pg. 89
  // of Hartley and Zisserman (2nd Edition)
  // svd.matrixV().col(8).array().resize(3,3) is not correct!
  H << svd.matrixV()(0,8), svd.matrixV()(1,8), svd.matrixV()(2,8),
       svd.matrixV()(3,8), svd.matrixV()(4,8), svd.matrixV()(5,8),
                        0,                  0, svd.matrixV()(8,8); // affine transform
//       svd.matrixV()(6,8), svd.matrixV()(7,8), svd.matrixV()(8,8); // projective transform
}

double imreg_sift::clamp(double v, double min, double max) {
  if( v > min ) {
    if( v < max ) {
      return v;
    } else {
      return max;
    }
  } else {
    return min;
  }
}

void imreg_sift::compute_sift_features(const Magick::Image& img, vector<VlSiftKeypoint>& keypoint_list, vector< vector<vl_uint8> >& descriptor_list, bool verbose) {
  vl_bool  err    = VL_ERR_OK ;

  // algorithm parameters based on vlfeat-0.9.20/toolbox/sift/vl_sift.c
  int                O     = - 1 ;
  int                S     =   3 ;
  int                o_min =   0 ;

  double             edge_thresh = -1 ;
  double             peak_thresh = -1 ;
  double             norm_thresh = -1 ;
  double             magnif      = -1 ;
  double             window_size = -1 ;
  int                ndescriptors = 0;

  vl_sift_pix     *fdata = 0 ;
  vl_size          q ;
  int              i ;
  vl_bool          first ;

  double           *ikeys = 0 ;
  int              nikeys = 0, ikeys_size = 0 ;

  // move image data to fdata for processing by vl_sift
  // @todo: optimize and avoid this overhead

  fdata = (vl_sift_pix *) malloc( img.rows() * img.columns() * sizeof(vl_sift_pix) ) ;
  if( fdata == NULL ) {
    cout << "\nfailed to allocated memory for vl_sift_pix array" << flush;
    return;
  }

  size_t flat_index = 0;
  for( unsigned int i=0; i<img.rows(); ++i ) {
    for( unsigned int j=0; j<img.columns(); ++j ) {
      Magick::ColorGray c = img.pixelColor( j, i );
      fdata[flat_index] = c.shade();
      ++flat_index;
    }
  }

  // create filter
  VlSiftFilt *filt = 0 ;

  filt = vl_sift_new (img.columns(), img.rows(), O, S, o_min) ;

  if (peak_thresh >= 0) vl_sift_set_peak_thresh (filt, peak_thresh) ;
  if (edge_thresh >= 0) vl_sift_set_edge_thresh (filt, edge_thresh) ;
  if (norm_thresh >= 0) vl_sift_set_norm_thresh (filt, norm_thresh) ;
  if (magnif      >= 0) vl_sift_set_magnif      (filt, magnif) ;
  if (window_size >= 0) vl_sift_set_window_size (filt, window_size) ;

  if (!filt) {
    cout << "\nCould not create SIFT filter." << flush;
    goto done ;
  }

  /* ...............................................................
   *                                             Process each octave
   * ............................................................ */
  i     = 0 ;
  first = 1 ;
  descriptor_list.clear();
  keypoint_list.clear();
  while (1) {
    VlSiftKeypoint const *keys = 0 ;
    int                   nkeys ;

    /* calculate the GSS for the next octave .................... */
    if (first) {
      first = 0 ;
      err = vl_sift_process_first_octave (filt, fdata) ;
    } else {
      err = vl_sift_process_next_octave  (filt) ;
    }

    if (err) {
      err = VL_ERR_OK ;
      break ;
    }

    /* run detector ............................................. */
    vl_sift_detect(filt) ;
    keys  = vl_sift_get_keypoints(filt) ;
    nkeys = vl_sift_get_nkeypoints(filt) ;
    i     = 0 ;

    /* for each keypoint ........................................ */
    for (; i < nkeys ; ++i) {
      double                angles [4] ;
      int                   nangles ;
      VlSiftKeypoint const *k ;

      /* obtain keypoint orientations ........................... */
      k = keys + i ;

      VlSiftKeypoint key_data = *k;

      nangles = vl_sift_calc_keypoint_orientations(filt, angles, k) ;

      vl_uint8 d[128]; // added by @adutta
      /* for each orientation ................................... */
      for (q = 0 ; q < (unsigned) nangles ; ++q) {
        vl_sift_pix descr[128];

        /* compute descriptor (if necessary) */
        vl_sift_calc_keypoint_descriptor(filt, descr, k, angles[q]) ;

        vector<vl_uint8> descriptor(128);
        int j;
        for( j=0; j<128; ++j ) {
          float value = 512.0 * descr[j];
          value = ( value < 255.0F ) ? value : 255.0F;
          descriptor[j] = (vl_uint8) value;
          d[j] = (vl_uint8) value;
        }
        descriptor_list.push_back(descriptor);
        ++ndescriptors;

        keypoint_list.push_back(key_data); // add corresponding keypoint
      }
    }
  }

  done :
    /* release filter */
    if (filt) {
      vl_sift_delete (filt) ;
      filt = 0 ;
    }

    /* release image data */
    if (fdata) {
      free (fdata) ;
      fdata = 0 ;
    }
}

void imreg_sift::get_putative_matches(vector< vector<vl_uint8> >& descriptor_list1, vector< vector<vl_uint8> >& descriptor_list2, std::vector< std::pair<uint32_t, uint32_t> > &putative_matches, float threshold) {
  size_t n1 = descriptor_list1.size();
  size_t n2 = descriptor_list2.size();

  putative_matches.clear();
  for( uint32_t i=0; i<n1; i++ ) {
    unsigned int dist_best1 = numeric_limits<unsigned int>::max();
    unsigned int dist_best2 = numeric_limits<unsigned int>::max();
    uint32_t dist_best1_index = -1;

    for( uint32_t j=0; j<n2; j++ ) {
      unsigned int dist = 0;
      for( int d=0; d<128; d++ ) {
        int del = descriptor_list1[i][d] - descriptor_list2[j][d];
        dist += del*del;
        if (dist >= dist_best2) {
          break;
        }
      }

      // find the nearest and second nearest point in descriptor_list2
      if( dist < dist_best1 ) {
        dist_best2 = dist_best1;
        dist_best1 = dist;
        dist_best1_index = j;
      } else {
        if( dist < dist_best2 ) {
          dist_best2 = dist;
        }
      }
    }

    // use Lowe's 2nd nearest neighbour test
    float d1 = threshold * (float) dist_best1;

    if( (d1 < (float) dist_best2) && dist_best1_index != -1 ) {
      putative_matches.push_back( std::make_pair(i, dist_best1_index) );
    }
  }
}

void imreg_sift::ransac_dlt(const char im1_fn[], const char im2_fn[],
                            double x, double y, double width, double height,
                            MatrixXd& H_optimal, size_t& fp_match_count,
                            Magick::Image& im2_crop_tx, bool& success) {
  success = false;

  Magick::Image im1; im1.read( im1_fn );
  Magick::Image im2; im2.read( im2_fn );

  // to ensure that image pixel values are 8bit RGB
  im1.type(Magick::TrueColorType);
  im2.type(Magick::TrueColorType);

  Magick::Image im1_g = im1;
  im1_g.crop( Magick::Geometry(width, height, x, y) );

  Magick::Image im2_g = im2;

  vector<VlSiftKeypoint> keypoint_list1, keypoint_list2;
  vector< vector<vl_uint8> > descriptor_list1, descriptor_list2;

  compute_sift_features(im1_g, keypoint_list1, descriptor_list1, false);
  compute_sift_features(im2_g, keypoint_list2, descriptor_list2, false);

  // use Lowe's 2nd nn test to find putative matches
  float threshold = 1.5f;
  std::vector< std::pair<uint32_t, uint32_t> > putative_matches;
  get_putative_matches(descriptor_list1, descriptor_list2, putative_matches, threshold);

  size_t n_match = putative_matches.size();
  fp_match_count = n_match;

  if( n_match < 9 ) {
    //cout << "\nInsufficiet number (" << n_match << ") of putative matches! Exiting." << flush;
    return;
  }

  // Normalize points so that centroid lies at origin and mean distance to
  // original points in sqrt(2)
  MatrixXd im1_match_kp(3, n_match);
  MatrixXd im2_match_kp(3, n_match);

  for( size_t i=0; i<n_match; ++i) {
    VlSiftKeypoint kp1 = keypoint_list1.at( putative_matches[i].first );
    VlSiftKeypoint kp2 = keypoint_list2.at( putative_matches[i].second );
    im1_match_kp(0, i) = kp1.x;
    im1_match_kp(1, i) = kp1.y;
    im1_match_kp(2, i) = 1.0;
    im2_match_kp(0, i) = kp2.x;
    im2_match_kp(1, i) = kp2.y;
    im2_match_kp(2, i) = 1.0;
  }

  //cout << "\nNormalizing keypoints" << flush;
  Matrix<double,3,3> im1_match_kp_tform, im2_match_kp_tform;
  get_norm_matrix(im1_match_kp, im1_match_kp_tform);
  get_norm_matrix(im2_match_kp, im2_match_kp_tform);
  MatrixXd im2_match_kp_tform_inv = im2_match_kp_tform.inverse();

  MatrixXd im1_match_norm = im1_match_kp_tform * im1_match_kp;
  MatrixXd im2_match_norm = im2_match_kp_tform * im2_match_kp;

  // memory cleanup as these are not required anymore
  im1_match_kp.resize(0,0);
  im2_match_kp.resize(0,0);

  // initialize random number generator to randomly sample putative_matches
  random_device rand_device;
  mt19937 generator(rand_device());
  uniform_int_distribution<> dist(0, n_match-1);

  // estimate homography using RANSAC
  size_t max_score = 0;
  Matrix<double,3,3> Hi;
  vector<unsigned int> best_inliers_index;

  // see Hartley and Zisserman p.119
  // in the original image 2 domain, error = sqrt(5.99) * 1 ~ 3 (for SD of 1 pixel error)
  // in the normalized image 2 domain, we have to transform this 3 pixel to normalized coordinates
  double geom_err_threshold_norm = im2_match_kp_tform(0,0) * 3;
  size_t RANSAC_ITER_COUNT = (size_t) ((double) n_match * 0.6);
  for( unsigned int iter=0; iter<RANSAC_ITER_COUNT; iter++ ) {
    //cout << "\n==========================[ iter=" << iter << " ]==============================" << flush;

    // randomly select 4 matches from putative_matches
    int kp_id1 = dist(generator);
    int kp_id2 = dist(generator);
    int kp_id3 = dist(generator);
    int kp_id4 = dist(generator);
    //cout << "\n  Random entries from putative_matches: " << kp_id1 << "," << kp_id2 << "," << kp_id3 << "," << kp_id4 << flush;

    MatrixXd X(4,3);
    X.row(0) = im1_match_norm.col(kp_id1).transpose();
    X.row(1) = im1_match_norm.col(kp_id2).transpose();
    X.row(2) = im1_match_norm.col(kp_id3).transpose();
    X.row(3) = im1_match_norm.col(kp_id4).transpose();

    MatrixXd Y(4,3);
    Y.row(0) = im2_match_norm.col(kp_id1).transpose();
    Y.row(1) = im2_match_norm.col(kp_id2).transpose();
    Y.row(2) = im2_match_norm.col(kp_id3).transpose();
    Y.row(3) = im2_match_norm.col(kp_id4).transpose();

    dlt(X, Y, Hi);

    size_t score = 0;
    vector<unsigned int> inliers_index;
    inliers_index.reserve(n_match);
    MatrixXd im1tx_norm = Hi * im1_match_norm; // 3 x n

    im1tx_norm.row(0) = im1tx_norm.row(0).array() / im1tx_norm.row(2).array();
    im1tx_norm.row(1) = im1tx_norm.row(1).array() / im1tx_norm.row(2).array();

    MatrixXd del(2,n_match);
    del.row(0) = im1tx_norm.row(0) - im2_match_norm.row(0);
    del.row(1) = im1tx_norm.row(1) - im2_match_norm.row(1);
    del = del.array().pow(2);
    VectorXd error = del.row(0) + del.row(1);
    error = error.array().sqrt();

    for( size_t k=0; k<n_match; k++ ) {
      if(error(k) < geom_err_threshold_norm) {
        score++;
        inliers_index.push_back(k);
      }
    }
    if( score > max_score ) {
      max_score = score;
      best_inliers_index.swap(inliers_index);
    }
  }

  // Recompute homography using all the inliers
  // This does not improve the registration
  size_t n_inliers = best_inliers_index.size();
  //std::cout << "\nn_inliers = " << n_inliers << std::flush;
  if ( n_inliers < 5 ) {
    return;
  }
  MatrixXd X(n_inliers,3);
  MatrixXd Y(n_inliers,3);
  for( size_t i=0; i<n_inliers; ++i ) {
    X.row(i) = im1_match_norm.col( best_inliers_index.at(i) ).transpose();
    Y.row(i) = im2_match_norm.col( best_inliers_index.at(i) ).transpose();
  }

  Matrix<double, 3, 3> Hopt_norm, H;
  dlt(X, Y, Hopt_norm);
  H = im2_match_kp_tform_inv * Hopt_norm * im1_match_kp_tform; // see Hartley and Zisserman p.109
  H = H / H(2,2);
  H_optimal = H; // Hopt is the reference variable passed to this method

  //cout << "\nTransforming image ..." << flush;
  double x0,x1,y0,y1;
  double xt, yt, homogeneous_norm;
  double dx0, dx1, dy0, dy1;
  double fxy0, fxy1;
  double fxy_red, fxy_green, fxy_blue;
  double xi, yi;

  for(unsigned int j=0; j<im2_crop_tx.rows(); j++) {
    for(unsigned int i=0; i<im2_crop_tx.columns(); i++) {
      xi = ((double) i) + 0.5; // center of pixel
      yi = ((double) j) + 0.5; // center of pixel
      xt = H(0,0) * xi + H(0,1) * yi + H(0,2);
      yt = H(1,0) * xi + H(1,1) * yi + H(1,2);
      homogeneous_norm = H(2,0) * xi + H(2,1) * yi + H(2,2);
      xt = xt / homogeneous_norm;
      yt = yt / homogeneous_norm;

      // neighbourhood of xh
      x0 = ((int) xt);
      x1 = x0 + 1;
      dx0 = xt - x0;
      dx1 = x1 - xt;

      y0 = ((int) yt);
      y1 = y0 + 1;
      dy0 = yt - y0;
      dy1 = y1 - yt;

      Magick::ColorRGB fx0y0 = im2.pixelColor(x0, y0);
      Magick::ColorRGB fx1y0 = im2.pixelColor(x1, y0);
      Magick::ColorRGB fx0y1 = im2.pixelColor(x0, y1);
      Magick::ColorRGB fx1y1 = im2.pixelColor(x1, y1);

      // Bilinear interpolation: https://en.wikipedia.org/wiki/Bilinear_interpolation
      fxy0 = dx1 * fx0y0.red() + dx0 * fx1y0.red(); // note: x1 - x0 = 1
      fxy1 = dx1 * fx0y1.red() + dx0 * fx1y1.red(); // note: x1 - x0 = 1
      fxy_red = dy1 * fxy0 + dy0 * fxy1;

      fxy0 = dx1 * fx0y0.green() + dx0 * fx1y0.green(); // note: x1 - x0 = 1
      fxy1 = dx1 * fx0y1.green() + dx0 * fx1y1.green(); // note: x1 - x0 = 1
      fxy_green = dy1 * fxy0 + dy0 * fxy1;

      fxy0 = dx1 * fx0y0.blue() + dx0 * fx1y0.blue(); // note: x1 - x0 = 1
      fxy1 = dx1 * fx0y1.blue() + dx0 * fx1y1.blue(); // note: x1 - x0 = 1
      fxy_blue = dy1 * fxy0 + dy0 * fxy1;

      Magick::ColorRGB fxy(fxy_red, fxy_green, fxy_blue);
      im2_crop_tx.pixelColor(i, j, fxy);
    }
  }
  success = true;
}

void imreg_sift::ransac_dlt(const char im1_fn[], const char im2_fn[],
                                double xl, double xu, double yl, double yu,
                                MatrixXd& Hopt, size_t& fp_match_count,
                                const char im1_crop_fn[], const char im2_crop_fn[], const char im2_tx_fn[],
                                const char diff_image_fn[],
                                const char overlap_image_fn[],
                                bool& success) {
  Magick::Image im1; im1.read( im1_fn );
  Magick::Image im2; im2.read( im2_fn );

  // to ensure that image pixel values are 8bit RGB
  im1.type(Magick::TrueColorType);
  im2.type(Magick::TrueColorType);

  Magick::Image im1_g = im1;
  //im1_g.magick("pgm");
  im1_g.crop( Magick::Geometry(xu-xl, yu-yl, xl, yl) );

  Magick::Image im2_g = im2;
  //im2_g.magick("pgm");

  vector<VlSiftKeypoint> keypoint_list1, keypoint_list2;
  vector< vector<vl_uint8> > descriptor_list1, descriptor_list2;

  //compute_sift_features(filename1.string().c_str(), keypoint_list1, descriptor_list1, true);
  compute_sift_features(im1_g, keypoint_list1, descriptor_list1, false);
  //cout << "\nFilename = " << im1_fn << flush;
  //cout << "\n  keypoint_list = " << keypoint_list1.size() << flush;
  //cout << "\n  descriptor_list = " << descriptor_list1.size() << flush;

  //compute_sift_features(filename2.string().c_str(), keypoint_list2, descriptor_list2, true);
  compute_sift_features(im2_g, keypoint_list2, descriptor_list2, false);
  //cout << "\nFilename = " << im2_fn << flush;
  //cout << "\n  keypoint_list = " << keypoint_list2.size() << flush;
  //cout << "\n  descriptor_list = " << descriptor_list2.size() << flush;

  // use Lowe's 2nd nn test to find putative matches
  float threshold = 1.5f;
  std::vector< std::pair<uint32_t, uint32_t> > putative_matches;
  get_putative_matches(descriptor_list1, descriptor_list2, putative_matches, threshold);

/*
  cout << "\nShowing putative matches :" << flush;
  for( size_t i=0; i<putative_matches.size(); i++ ) {
    cout << "[" << putative_matches[i].first << ":" << putative_matches[i].second << "], " << flush;
  }
*/

  size_t n_match = putative_matches.size();
  fp_match_count = n_match;
  //cout << "\nPutative matches (using Lowe's 2nd NN test) = " << n_match << flush;

  if( n_match < 9 ) {
    //cout << "\nInsufficiet number of putative matches! Exiting." << flush;
    return;
  }

  // Normalize points so that centroid lies at origin and mean distance to
  // original points in sqrt(2)
  MatrixXd im1_match_kp(3, n_match);
  MatrixXd im2_match_kp(3, n_match);

  for( size_t i=0; i<n_match; ++i) {
    VlSiftKeypoint kp1 = keypoint_list1.at( putative_matches[i].first );
    VlSiftKeypoint kp2 = keypoint_list2.at( putative_matches[i].second );
    im1_match_kp(0, i) = kp1.x;
    im1_match_kp(1, i) = kp1.y;
    im1_match_kp(2, i) = 1.0;
    im2_match_kp(0, i) = kp2.x;
    im2_match_kp(1, i) = kp2.y;
    im2_match_kp(2, i) = 1.0;
  }

  //cout << "\nNormalizing keypoints" << flush;
  Matrix<double,3,3> im1_match_kp_tform, im2_match_kp_tform;
  get_norm_matrix(im1_match_kp, im1_match_kp_tform);
  get_norm_matrix(im2_match_kp, im2_match_kp_tform);
  MatrixXd im2_match_kp_tform_inv = im2_match_kp_tform.inverse();

  MatrixXd im1_match_norm = im1_match_kp_tform * im1_match_kp;
  MatrixXd im2_match_norm = im2_match_kp_tform * im2_match_kp;
  //cout << "\nim1_match_kp_tform=" << im1_match_kp_tform << flush;
  //cout << "\nim2_match_kp_tform=" << im2_match_kp_tform << flush;

  // memory cleanup as these are not required anymore
  im1_match_kp.resize(0,0);
  im2_match_kp.resize(0,0);

  // initialize random number generator to randomly sample putative_matches
  random_device rand_device;
  mt19937 generator(rand_device());
  uniform_int_distribution<> dist(0, n_match-1);

  // estimate homography using RANSAC
  size_t max_score = 0;
  Matrix<double,3,3> Hi;
  vector<unsigned int> best_inliers_index;

  // see Hartley and Zisserman p.119
  // in the original image 2 domain, error = sqrt(5.99) * 1 ~ 3 (for SD of 1 pixel error)
  // in the normalized image 2 domain, we have to transform this 3 pixel to normalized coordinates
  double geom_err_threshold_norm = im2_match_kp_tform(0,0) * 3;
  size_t RANSAC_ITER_COUNT = (size_t) ((double) n_match * 0.6);
  for( unsigned int iter=0; iter<RANSAC_ITER_COUNT; iter++ ) {
    //cout << "\n==========================[ iter=" << iter << " ]==============================" << flush;

    // randomly select 4 matches from putative_matches
    int kp_id1 = dist(generator);
    int kp_id2 = dist(generator);
    int kp_id3 = dist(generator);
    int kp_id4 = dist(generator);
    //cout << "\n  Random entries from putative_matches: " << kp_id1 << "," << kp_id2 << "," << kp_id3 << "," << kp_id4 << flush;

    MatrixXd X(4,3);
    X.row(0) = im1_match_norm.col(kp_id1).transpose();
    X.row(1) = im1_match_norm.col(kp_id2).transpose();
    X.row(2) = im1_match_norm.col(kp_id3).transpose();
    X.row(3) = im1_match_norm.col(kp_id4).transpose();

    MatrixXd Y(4,3);
    Y.row(0) = im2_match_norm.col(kp_id1).transpose();
    Y.row(1) = im2_match_norm.col(kp_id2).transpose();
    Y.row(2) = im2_match_norm.col(kp_id3).transpose();
    Y.row(3) = im2_match_norm.col(kp_id4).transpose();

    dlt(X, Y, Hi);

    size_t score = 0;
    vector<unsigned int> inliers_index;
    inliers_index.reserve(n_match);
    MatrixXd im1tx_norm = Hi * im1_match_norm; // 3 x n

    im1tx_norm.row(0) = im1tx_norm.row(0).array() / im1tx_norm.row(2).array();
    im1tx_norm.row(1) = im1tx_norm.row(1).array() / im1tx_norm.row(2).array();

    MatrixXd del(2,n_match);
    del.row(0) = im1tx_norm.row(0) - im2_match_norm.row(0);
    del.row(1) = im1tx_norm.row(1) - im2_match_norm.row(1);
    del = del.array().pow(2);
    VectorXd error = del.row(0) + del.row(1);
    error = error.array().sqrt();

    for( size_t k=0; k<n_match; k++ ) {
      if(error(k) < geom_err_threshold_norm) {
        score++;
        inliers_index.push_back(k);
      }
    }

    //cout << "\n  iter " << iter << " of " << RANSAC_ITER_COUNT << " : score=" << score << ", max_score=" << max_score << ", total_matches=" << putative_matches.size() << flush;
    if( score > max_score ) {
      max_score = score;
      best_inliers_index.swap(inliers_index);
    }
  }

  // Recompute homography using all the inliers
  // This does not improve the registration
  size_t n_inliers = best_inliers_index.size();
  MatrixXd X(n_inliers,3);
  MatrixXd Y(n_inliers,3);
  for( size_t i=0; i<n_inliers; ++i ) {
    X.row(i) = im1_match_norm.col( best_inliers_index.at(i) ).transpose();
    Y.row(i) = im2_match_norm.col( best_inliers_index.at(i) ).transpose();
  }

  Matrix<double, 3, 3> Hopt_norm, H;
  dlt(X, Y, Hopt_norm);
  H = im2_match_kp_tform_inv * Hopt_norm * im1_match_kp_tform; // see Hartley and Zisserman p.109
  H = H / H(2,2);
  Hopt = H; // Hopt is the reference variable passed to this method
  //cout << "\nH (recomputed using all inliers) = " << endl << H;

  // im1 crop
  Magick::Image im1_crop(im1);
  Magick::Geometry cropRect1(xu-xl, yu-yl, xl, yl);
  im1_crop.crop( cropRect1 );
  im1_crop.write( im1_crop_fn );

  Magick::Image im2_crop(im2);
  im2_crop.crop( cropRect1 );
  //im2_crop.write( im2_crop_fn );

  // im2 crop and transform
  Magick::Image im2t_crop( im1_crop.size(), "white");
  //cout << "\nTransforming image ..." << flush;
  double x0,x1,y0,y1;
  double x, y, homogeneous_norm;
  double dx0, dx1, dy0, dy1;
  double fxy0, fxy1;
  double fxy_red, fxy_green, fxy_blue;
  double xi, yi;
  Magick::Image overlap(im1_crop.size(), "white");
  //cout << "\nTransforming im2 ... " << flush;

  for(unsigned int j=0; j<im2t_crop.rows(); j++) {
    for(unsigned int i=0; i<im2t_crop.columns(); i++) {
      xi = ((double) i) + 0.5; // center of pixel
      yi = ((double) j) + 0.5; // center of pixel
      x = H(0,0) * xi + H(0,1) * yi + H(0,2);
      y = H(1,0) * xi + H(1,1) * yi + H(1,2);
      homogeneous_norm = H(2,0) * xi + H(2,1) * yi + H(2,2);
      x = x / homogeneous_norm;
      y = y / homogeneous_norm;

      // neighbourhood of xh
      x0 = ((int) x);
      x1 = x0 + 1;
      dx0 = x - x0;
      dx1 = x1 - x;

      y0 = ((int) y);
      y1 = y0 + 1;
      dy0 = y - y0;
      dy1 = y1 - y;

      Magick::ColorRGB fx0y0 = im2.pixelColor(x0, y0);
      Magick::ColorRGB fx1y0 = im2.pixelColor(x1, y0);
      Magick::ColorRGB fx0y1 = im2.pixelColor(x0, y1);
      Magick::ColorRGB fx1y1 = im2.pixelColor(x1, y1);

      // Bilinear interpolation: https://en.wikipedia.org/wiki/Bilinear_interpolation
      fxy0 = dx1 * fx0y0.red() + dx0 * fx1y0.red(); // note: x1 - x0 = 1
      fxy1 = dx1 * fx0y1.red() + dx0 * fx1y1.red(); // note: x1 - x0 = 1
      fxy_red = dy1 * fxy0 + dy0 * fxy1;

      fxy0 = dx1 * fx0y0.green() + dx0 * fx1y0.green(); // note: x1 - x0 = 1
      fxy1 = dx1 * fx0y1.green() + dx0 * fx1y1.green(); // note: x1 - x0 = 1
      fxy_green = dy1 * fxy0 + dy0 * fxy1;

      fxy0 = dx1 * fx0y0.blue() + dx0 * fx1y0.blue(); // note: x1 - x0 = 1
      fxy1 = dx1 * fx0y1.blue() + dx0 * fx1y1.blue(); // note: x1 - x0 = 1
      fxy_blue = dy1 * fxy0 + dy0 * fxy1;

      Magick::ColorRGB fxy(fxy_red, fxy_green, fxy_blue);
      im2t_crop.pixelColor(i, j, fxy);

      // overlap
      Magick::ColorRGB c1 = im1_crop.pixelColor(i,j);
      double red_avg = (c1.red() + fxy_red) / (2.0f);
      double green_avg = (c1.green() + fxy_green) / (2.0f);
      double blue_avg = (c1.blue() + fxy_blue) / (2.0f);
      overlap.pixelColor(i, j, Magick::ColorRGB(red_avg, green_avg, blue_avg));
    }
  }
  im2t_crop.write( im2_tx_fn );

  // difference image
  Magick::Image cdiff(im1_crop.size(), "black");
  get_diff_image(im1_crop, im2t_crop, cdiff);
  cdiff.write(diff_image_fn);

  // write overlap image
  overlap.write(overlap_image_fn);
  success = true;
}

void imreg_sift::get_diff_image(Magick::Image& im1, Magick::Image& im2, Magick::Image& cdiff) {
  Magick::Image im1g = im1;
  Magick::Image im2g = im2;
  im1g.type(Magick::GrayscaleType);
  im2g.type(Magick::GrayscaleType);

  const vector<unsigned int> percentile = {5,25};
  vector<double> im1_percentile_values = get_pixel_percentile(im1g, percentile);
  vector<double> im2_percentile_values = get_pixel_percentile(im2g, percentile);

  double px1, px2, sum;
  double del1 = im1_percentile_values.at(0) - im1_percentile_values.at(1);
  double del2 = im2_percentile_values.at(0) - im2_percentile_values.at(1);

  //cout << "\nWriting diff image ... " << flush;
  for(unsigned int j=0; j<im1g.rows(); j++) {
    for(unsigned int i=0; i<im1g.columns(); i++) {
      Magick::ColorGray c1 = im1g.pixelColor(i,j);
      Magick::ColorGray c2 = im2g.pixelColor(i,j);
      px1 = ( c1.shade() - im1_percentile_values.at(1) ) / del1;
      px2 = ( c2.shade() - im2_percentile_values.at(1) ) / del2;

      sum = clamp(px1 + px2, 0.0, 1.0);
      px1 = clamp(px1, 0.0, 1.0);
      px2 = clamp(px2, 0.0, 1.0);

      cdiff.pixelColor(i, j, Magick::ColorRGB(1.0 - px1, 1.0 - sum, 1.0 - px2));
    }
  }
}

vector<double> imreg_sift::get_pixel_percentile(Magick::Image& img, const vector<unsigned int> percentile) {
  size_t npixel = img.rows() * img.columns();
  vector<double> pixel_values;
  pixel_values.reserve(npixel);
  for(unsigned int j=0; j<img.rows(); j++) {
    for(unsigned int i=0; i<img.columns(); i++) {
      Magick::ColorGray c = img.pixelColor(i,j);
      pixel_values.push_back(c.shade());
    }
  }

  //sort(pixel_values.begin(), pixel_values.end());
  vector<double> percentile_values;
  percentile_values.reserve(percentile.size());
  for( size_t i = 0; i < percentile.size(); i++ ) {
    double k = percentile.at(i);
    // source: https://stackoverflow.com/a/28548904
    auto nth = pixel_values.begin() + (k * pixel_values.size())/100;
    std::nth_element(pixel_values.begin(), nth, pixel_values.end());
    percentile_values.push_back( *nth );
  }

  return percentile_values;
}

///
/// Robust matching and Thin Plate Spline based image registration
///
/// Robust matching based on:
/// Tran, Q.H., Chin, T.J., Carneiro, G., Brown, M.S. and Suter, D., 2012, October. In defence of RANSAC for outlier rejection in deformable registration. ECCV.
///
/// Thin plate spline registration based on:
/// Bookstein, F.L., 1989. Principal warps: Thin-plate splines and the decomposition of deformations. IEEE TPAMI.
///

void imreg_sift::robust_ransac_tps(const char im1_fn[], const char im2_fn[],
                                double xl, double xu, double yl, double yu,
                                MatrixXd& Hopt, size_t& fp_match_count,
                                const char im1_crop_fn[], const char im2_crop_fn[], const char im2_tx_fn[],
                                const char diff_image_fn[],
                                const char overlap_image_fn[],
                                bool& success) {
  auto start = std::chrono::high_resolution_clock::now();

  Magick::Image im1; im1.read( im1_fn );
  Magick::Image im2; im2.read( im2_fn );

  // to ensure that image pixel values are 8bit RGB
  im1.type(Magick::TrueColorType);
  im2.type(Magick::TrueColorType);

  Magick::Image im1_g = im1;
  im1_g.crop( Magick::Geometry(xu-xl, yu-yl, xl, yl) );

  Magick::Image im2_g = im2;

  vector<VlSiftKeypoint> keypoint_list1, keypoint_list2;
  vector< vector<vl_uint8> > descriptor_list1, descriptor_list2;

  //cout << "\nComputing SIFT features for im1 ..." << flush;
  compute_sift_features(im1_g, keypoint_list1, descriptor_list1);
  //cout << "\nFilename = " << filename1 << flush;
  //cout << "\n  keypoint_list = " << keypoint_list1.size() << flush;
  //cout << "\n  descriptor_list = " << descriptor_list1.size() << flush;

  //cout << "\nComputing SIFT features for im2 ..." << flush;
  compute_sift_features(im2_g, keypoint_list2, descriptor_list2);
  //cout << "\nFilename = " << filename2 << flush;
  //cout << "\n  keypoint_list = " << keypoint_list2.size() << flush;
  //cout << "\n  descriptor_list = " << descriptor_list2.size() << flush;

  // use Lowe's 2nd nn test to find putative matches
  float threshold = 1.5f;
  std::vector< std::pair<uint32_t, uint32_t> > putative_matches;
  get_putative_matches(descriptor_list1, descriptor_list2, putative_matches, threshold);

  size_t n_lowe_match = putative_matches.size();
  //cout << "\nPutative matches (using Lowe's 2nd NN test) = " << putative_matches.size() << flush;

  if( n_lowe_match < 9 ) {
    fp_match_count = n_lowe_match;
    //cout << "\nInsufficinet number of putative matches! Exiting." << flush;
    return;
  }

  // Normalize points so that centroid lies at origin and mean distance to
  // original points in sqrt(2)
  //cout << "\nCreating matrix of size 3x" << n_lowe_match << " ..." << flush;
  MatrixXd pts1(3, n_lowe_match);
  MatrixXd pts2(3, n_lowe_match);
  //cout << "\nCreating point set matched using lowe ..." << flush;
  for( size_t i=0; i<n_lowe_match; ++i) {
    VlSiftKeypoint kp1 = keypoint_list1.at( putative_matches[i].first );
    VlSiftKeypoint kp2 = keypoint_list2.at( putative_matches[i].second );
    pts1(0, i) = kp1.x;
    pts1(1, i) = kp1.y;
    pts1(2, i) = 1.0;
    pts2(0, i) = kp2.x;
    pts2(1, i) = kp2.y;
    pts2(2, i) = 1.0;
  }
  //cout << "\nNormalizing points ..." << flush;

  Matrix3d pts1_tform;
  get_norm_matrix(pts1, pts1_tform);
  //cout << "\nNormalizing matrix (T) = " << pts1_tform << flush;

  MatrixXd pts1_norm = pts1_tform * pts1;
  MatrixXd pts2_norm = pts1_tform * pts2;

  // form a matrix containing match pair (x,y) <-> (x',y') as follows
  // [x y x' y'; ...]
  MatrixXd S_all(4, n_lowe_match);
  S_all.row(0) = pts1_norm.row(0);
  S_all.row(1) = pts1_norm.row(1);
  S_all.row(2) = pts2_norm.row(0);
  S_all.row(3) = pts2_norm.row(1);

  // initialize random number generator to randomly sample putative_matches
  mt19937 generator(9973);
  uniform_int_distribution<> dist(0, n_lowe_match-1);

  MatrixXd S(4,3), hatS(4,3);
  double residual;

  vector<size_t> best_robust_match_idx;
  // @todo: tune this threshold for better generalization
  double robust_ransac_threshold = 0.09;
  size_t RANSAC_ITER_COUNT = (size_t) ((double) n_lowe_match * 0.6);
  //cout << "\nRANSAC_ITER_COUNT = " << RANSAC_ITER_COUNT << flush;
  for( int i=0; i<RANSAC_ITER_COUNT; ++i ) {
    S.col(0) = S_all.col( dist(generator) ); // randomly select a match from S_all
    S.col(1) = S_all.col( dist(generator) );
    S.col(2) = S_all.col( dist(generator) );

    Vector4d muS = S.rowwise().mean();
    hatS = S.colwise() - muS;

    JacobiSVD<MatrixXd> svd(hatS, ComputeFullU);

    MatrixXd As = svd.matrixU().block(0, 0, svd.matrixU().rows(), 2);

    MatrixXd dx = S_all.colwise() - muS;
    MatrixXd del = dx - (As * As.transpose() * dx);

    vector<size_t> robust_match_idx;
    robust_match_idx.clear();
    for( int j=0; j<del.cols(); ++j ) {
      residual = sqrt( del(0,j)*del(0,j) + del(1,j)*del(1,j) + del(2,j)*del(2,j) + del(3,j)*del(3,j) );
      if ( residual < robust_ransac_threshold ) {
        robust_match_idx.push_back(j);
      }
    }

    //cout << "\n" << i << ": robust_match_idx=" << robust_match_idx.size() << flush;
    if ( robust_match_idx.size() > best_robust_match_idx.size() ) {
      best_robust_match_idx.clear();
      best_robust_match_idx.swap(robust_match_idx);
      //cout << "\t[MIN]" << flush;
    }
  }
  fp_match_count = best_robust_match_idx.size();
  //cout << "\nrobust match pairs = " << fp_match_count << flush;

  // bin each correspondence pair into cells dividing the original image into KxK cells
  unsigned int POINTS_PER_CELL = 1; // a single point in each cell ensures that no two control points are very close
  unsigned int n_cell_w, n_cell_h;

  if( im1.rows() > 500 ) {
    n_cell_h = 9;
  } else {
    n_cell_h = 5;
  }
  unsigned int ch = (unsigned int) (im1.rows() / n_cell_h);

  if( im1.columns() > 500 ) {
    n_cell_w = 9;
  } else {
    n_cell_w = 5;
  }
  unsigned int cw = (unsigned int) (im1.columns() / n_cell_w);

  //printf("\nn_cell_w=%d, n_cell_h=%d, cw=%d, ch=%d", n_cell_w, n_cell_h, cw, ch);
  //printf("\nimage size = %ld x %ld", im1.columns(), im1.rows());
  vector<size_t> sel_best_robust_match_idx;
  for( unsigned int i=0; i<n_cell_w; ++i ) {
    for( unsigned int j=0; j<n_cell_h; ++j ) {
      unsigned int xl = i * cw;
      unsigned int xh = (i+1)*cw - 1;
      if( xh > im1.columns() ) {
        xh = im1.columns() - 1;
      }

      unsigned int yl = j * ch;
      unsigned int yh = (j+1)*ch - 1;
      if( yh > im1.rows() ) {
        yh = im1.rows() - 1;
      }

      //printf("\ncell(%d,%d) = (%d,%d) to (%d,%d)", i, j, xl, yl, xh, yh);
      //cout << flush;

      vector< size_t > cell_pts;
      for( unsigned int k=0; k<best_robust_match_idx.size(); ++k ) {
        size_t match_idx = best_robust_match_idx.at(k);
        double x = pts1(0, match_idx);
        double y = pts1(1, match_idx);

        if( x >= xl && x < xh && y >= yl && y < yh ) {
          cell_pts.push_back( match_idx );
        }
      }
      if( cell_pts.size() >= POINTS_PER_CELL ) {
        uniform_int_distribution<> dist2(0, cell_pts.size()-1);
        for( unsigned int k=0; k<POINTS_PER_CELL; ++k ) {
          unsigned long cell_pts_idx = dist2(generator);
          sel_best_robust_match_idx.push_back( cell_pts.at(cell_pts_idx) );
        }
      }
      //printf(" has points=%ld", cell_pts.size());
    }
  }

  size_t n_cp = sel_best_robust_match_idx.size();
  MatrixXd cp1(2,n_cp);
  MatrixXd cp2(2,n_cp);

  // write to file for debug
  //std::ofstream cpf("/home/tlm/cp.csv");

  for( size_t i=0; i<n_cp; ++i ) {
    unsigned long match_idx = sel_best_robust_match_idx.at(i);

    cp1(0,i) = pts1(0, match_idx ); cp1(1,i) = pts1(1, match_idx );
    cp2(0,i) = pts2(0, match_idx ); cp2(1,i) = pts2(1, match_idx );
    //cpf << pts1(0, match_idx) << "," << pts1(1, match_idx) << "," << pts2(0, match_idx) << "," << pts2(1, match_idx) << endl;
  }
  //cpf.close();
  //cout << "\nUsing " << n_cp << " control points for TPS" << flush;

  // im1 crop
  Magick::Image im1_crop(im1);
  Magick::Geometry cropRect1(xu-xl, yu-yl, xl, yl);
  im1_crop.crop( cropRect1 );
  im1_crop.magick("JPEG");
  //cout << "\nWriting to " << im1_crop_fn << flush;
  im1_crop.write( im1_crop_fn );

  Magick::Image im2_crop(im2);
  im2_crop.crop( cropRect1 );
  //im2_crop.write( im2_crop_fn );

  double lambda = 0.001;
  double lambda_norm = lambda * (im1_crop.rows() * im1_crop.columns());

  // create matrix K
  MatrixXd K(n_cp, n_cp);
  K.setZero();
  double rx, ry, r, r2;
  for(unsigned int i=0; i<n_cp; ++i ) {
    for(unsigned int j=i; j<n_cp; ++j ) {
      // image grid coordinate = (i,j)
      // control point coordinate = cp(:,k)
      rx = cp1(0,i) - cp1(0,j);
      ry = cp1(1,i) - cp1(1,j);
      r  = sqrt(rx*rx + ry*ry);
      r2 = r * r;
      K(i, j) = r2*log(r2);
      K(j, i) = K(i,j);
    }
  }
  //cout << "\nK=" << K.rows() << "," << K.cols() << flush;
  //cout << "\nlambda = " << lambda << flush;
  // create matrix P
  MatrixXd P(n_cp, 3);
  for(unsigned int i=0; i<n_cp; ++i ) {
    //K(i,i) = 0; // ensure that the diagonal elements of K are 0 (as log(0) = nan)
    // approximating thin-plate splines based on:
    // Rohr, K., Stiehl, H.S., Sprengel, R., Buzug, T.M., Weese, J. and Kuhn, M.H., 2001. Landmark-based elastic registration using approximating thin-plate splines.
    K(i,i) = ((double) n_cp) * lambda * 1;
    P(i,0) = 1;
    P(i,1) = cp1(0,i);
    P(i,2) = cp1(1,i);
  }
  //cout << "\nP=" << P.rows() << "," << P.cols() << flush;
  //cout << "\nK=" << endl << K.block(0,0,6,6) << flush;

  // create matrix L
  MatrixXd L(n_cp+3, n_cp+3);
  L.block(0, 0, n_cp, n_cp) = K;
  L.block(0, n_cp, n_cp, 3) = P;
  L.block(n_cp, 0, 3, n_cp) = P.transpose();
  L.block(n_cp, n_cp, 3, 3).setZero();
  //cout << "\nL=" << L.rows() << "," << L.cols() << flush;

  // create matrix V
  MatrixXd V(n_cp+3, 2);
  V.setZero();
  for(unsigned int i=0; i<n_cp; ++i ) {
    V(i,0) = cp2(0,i);
    V(i,1) = cp2(1,i);
  }
  //cout << "\nV=" << V.rows() << "," << V.cols() << flush;

  MatrixXd Linv = L.inverse();
  //cout << "\nLinv=" << endl << Linv << flush;

  MatrixXd W = Linv * V;
  //cout << "\nW=" << W.rows() << "," << W.cols() << flush;

  // each column of W denotes the coefficients of (1,x,y) and U() to get the image of point (X,y) in the warped domain
  // For a point (x,y) in the original image, the warped point (x',y') is given by
  // x' = W(0,0) + W(0,1)*x + W(0,2)*y + \sum{i=0}^{N} W(0,3+i) * U( ||Pi - (x,y)|| )
  // y' = W(1,0) + W(1,1)*x + W(1,2)*y + \sum{i=0}^{N} W(1,3+i) * U( ||Pi - (x,y)|| )

  Magick::Image im2t_crop( im1_crop.size(), "white");
  //cout << "\nTransforming image ..." << flush;
  double x0,x1,y0,y1;
  double x, y;
  double dx0, dx1, dy0, dy1;
  double fxy0, fxy1;
  double fxy_red, fxy_green, fxy_blue;
  double xi, yi;
  double x_non_linear_terms, y_non_linear_terms;
  double x_affine_terms, y_affine_terms;
  Magick::Image overlap(im1_crop.size(), "white");

  // for debug
  //im2.read( "/home/tlm/dev/imcomp/test/data/checkerboard_1024x1024.jpg" );
  for(unsigned int j=0; j<im1_crop.rows(); j++) {
    for(unsigned int i=0; i<im1_crop.columns(); i++) {
      //cout << "\n(" << i << "," << j << ") :" << flush;
      xi = ((double) i) + 0.5; // center of pixel
      yi = ((double) j) + 0.5; // center of pixel
      x_non_linear_terms = 0.0;
      y_non_linear_terms = 0.0;
      for(unsigned int k=0; k<n_cp; ++k) {
        //cout << "\n  k=" << k << flush;
        rx = cp1(0,k) - xi;
        ry = cp1(1,k) - yi;
        r = sqrt(rx*rx + ry*ry);
        r2 = r*r;
        x_non_linear_terms += W(k, 0) * r2 * log(r2);
        y_non_linear_terms += W(k, 1) * r2 * log(r2);
        //cout << "(" << x_non_linear_terms << "," << y_non_linear_terms << ")" << flush;
      }
      x_affine_terms = W(n_cp,0) + W(n_cp+1,0)*xi + W(n_cp+2,0)*yi;
      y_affine_terms = W(n_cp,1) + W(n_cp+1,1)*xi + W(n_cp+2,1)*yi;
      x = x_affine_terms + x_non_linear_terms;
      y = y_affine_terms + y_non_linear_terms;

      //printf("\n(%d,%d) : (xi,yi)=(%.2f,%.2f) (x,y)=(%.2f,%.2f) : (x,y)-affine = (%.2f,%.2f) (x,y)-nonlin = (%.2f,%.2f)", i, j, xi, yi, x, y, x_affine_terms, y_affine_terms, x_non_linear_terms, y_non_linear_terms);

      // neighbourhood of xh
      x0 = ((int) x);
      x1 = x0 + 1;
      dx0 = x - x0;
      dx1 = x1 - x;

      y0 = ((int) y);
      y1 = y0 + 1;
      dy0 = y - y0;
      dy1 = y1 - y;

      Magick::ColorRGB fx0y0 = im2.pixelColor(x0, y0);
      Magick::ColorRGB fx1y0 = im2.pixelColor(x1, y0);
      Magick::ColorRGB fx0y1 = im2.pixelColor(x0, y1);
      Magick::ColorRGB fx1y1 = im2.pixelColor(x1, y1);

      // Bilinear interpolation: https://en.wikipedia.org/wiki/Bilinear_interpolation
      fxy0 = dx1 * fx0y0.red() + dx0 * fx1y0.red(); // note: x1 - x0 = 1
      fxy1 = dx1 * fx0y1.red() + dx0 * fx1y1.red(); // note: x1 - x0 = 1
      fxy_red = dy1 * fxy0 + dy0 * fxy1;

      fxy0 = dx1 * fx0y0.green() + dx0 * fx1y0.green(); // note: x1 - x0 = 1
      fxy1 = dx1 * fx0y1.green() + dx0 * fx1y1.green(); // note: x1 - x0 = 1
      fxy_green = dy1 * fxy0 + dy0 * fxy1;

      fxy0 = dx1 * fx0y0.blue() + dx0 * fx1y0.blue(); // note: x1 - x0 = 1
      fxy1 = dx1 * fx0y1.blue() + dx0 * fx1y1.blue(); // note: x1 - x0 = 1
      fxy_blue = dy1 * fxy0 + dy0 * fxy1;

      Magick::ColorRGB fxy(fxy_red, fxy_green, fxy_blue);
      im2t_crop.pixelColor(i, j, fxy);

      // overlap
      Magick::ColorRGB c1 = im1_crop.pixelColor(i,j);
      double red_avg = (c1.red() + fxy_red) / (2.0f);
      double green_avg = (c1.green() + fxy_green) / (2.0f);
      double blue_avg = (c1.blue() + fxy_blue) / (2.0f);
      overlap.pixelColor(i, j, Magick::ColorRGB(red_avg, green_avg, blue_avg));
    }
  }
  //cout << "\nWriting to " << im2_tx_fn << flush;
  im2t_crop.write( im2_tx_fn );
  //cout << "\nWriting to " << overlap_image_fn << flush;
  overlap.write(overlap_image_fn);

  //auto finish = std::chrono::high_resolution_clock::now();
  //std::chrono::duration<double> elapsed = finish - start;
  //std::cout << "\ntps registration completed in " << elapsed.count() << " s" << flush;

  // difference image
  Magick::Image cdiff(im1_crop.size(), "black");
  get_diff_image(im1_crop, im2t_crop, cdiff);
  //cout << "\nWriting to " << diff_image_fn << flush;
  cdiff.magick("JPEG");
  cdiff.write(diff_image_fn);

  success = true;
}
