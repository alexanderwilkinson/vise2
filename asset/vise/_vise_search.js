/*
  @file        _vise_search_result.js
  @description display search results from VISE
  @author      Abhishek Dutta <adutta@robots.ox.ac.uk>
  @date        20 July 2018
*/

/*
precondition: variable '_vise_search_result_str' contains the JSON string
representation of VISE search result in a format given below:
{
  "search_engine_id": "ox5k/1",
  "query": {
    "file_id": 55,
    "filename": "all_souls_000075.jpg",
    "x": 316,
    "y": 176,
    "width": 200,
    "height": 283,
    "from": 0,
    "count": 3,
    "show_from": 0,
    "show_count": 45
  },
  "home_uri": "/vise/home.html",
  "image_uri_prefix": "/vise/asset/ox5k/1/",
  "image_uri_namespace": "image/",
  "query_uri_prefix": "/vise/query/ox5k/1/",
  "QUERY_RESULT_SIZE": 44,
  "query_result_subset": [
    {
      "file_id": 55,
      "filename": "all_souls_000075.jpg",
      "metadata": "",
      "score": 650.002,
      "H": [
        1,
        0,
        0,
        0,
        1,
        0,
        0,
        0,
        1
      ]
    }, ...
  ]
}
*/

function _vise_search() {
  // _vise_search_result_str is a global variable set by the vise_server
  if ( typeof(_vise_search_result_str) === 'undefined' ) {
    console.log('Error: _vise_search_result_data is not defined');
    return;
  }

  // _vise_search_result is a global variable
  _vise_search_result = JSON.parse(_vise_search_result_str);
  //console.log(_vise_search_result); // debug

  var page = document.createElement('div');
  var nav = document.createElement('div');
  var query_container = document.createElement('div');
  var query = document.createElement('div');
  var search_result = document.createElement('div');
  page.setAttribute('id', 'search_result_page');
  search_result.setAttribute('id', 'search_result');
  page.classList.add('page');
  nav.classList.add('nav');
  query_container.classList.add('query_container');
  query.classList.add('query')
  search_result.classList.add('result')
  page.appendChild(nav);
  query_container.appendChild(query);
  page.appendChild(query_container);
  page.appendChild(search_result);

  _vise_init_top_nav(_vise_search_result, nav);
  _vise_search_show_query(_vise_search_result, query);
  _vise_search_show_all_result(_vise_search_result, search_result);

  document.body.appendChild(page);
}

function _vise_search_update_page_nav(d, navbar) {
  // info. about filelist
  var navinfo = document.createElement('div');
  navinfo.classList.add('navinfo');
  navbar.appendChild(navinfo);

  var navinfohtml = [];
  navinfohtml.push( 'Total ' + d.QUERY_RESULT_SIZE + ' images match the query region. Showing images from&nbsp;' );
  navinfohtml.push( d.query.from + d.query.show_from );
  navinfohtml.push( '&nbsp;to&nbsp;' );
  navinfohtml.push( d.query.from + d.query.show_to );
  navinfo.innerHTML = navinfohtml.join('');

  // export search result
  var navtool = document.createElement('div');
  navtool.classList.add('navtool');
  navbar.appendChild(navtool);
  var navtoolhtml = [];
  navtoolhtml.push( '<span>Export search results as</span>&nbsp;' );
  navtoolhtml.push( '<select id="search_result_export_format">' );
  navtoolhtml.push( '<option value="plain_csv" selected>csv</option>' );
  navtoolhtml.push( '<option value="plain_json">json</option>' );
  navtoolhtml.push( '<option value="via_csv">VIA csv</option>' );
  navtoolhtml.push( '</select>' );
  navtoolhtml.push( '&nbsp;<button type="submit" onclick="_vise_search_export_search_result()">Export</button>' );
  navtool.innerHTML = navtoolhtml.join('');

  // next,prev page buttons
  var navbuttons = document.createElement('div');
  navbuttons.classList.add('navbuttons');
  navbar.appendChild(navbuttons);
  var links = [];

  //console.log('_vise_search_update_page_nav(): from=' + d.query.from + ', count=' + d.query.count + ', show_from=' + d.query.show_from + ', show_count=' + d.query.show_count + ', show_to=' + d.query.show_to);

  // check if this page is the first page
  if ( (d.query.from + d.query.show_from) !== 0) {
    links.push( '<a href="' + _vise_search_now_get_first_uri(d) + '" title="Jump to first page">First</a>' );
    // check if Prev page contents can be served from cache
    var prev_page_from = d.query.show_from - d.query.show_count;
    if ( prev_page_from >= 0 ) {
      // show next page contents from cache
      links.push( '<span class="text_button" title="Previous Page" onclick="_vise_search_prev_page_from_cache()">Prev</span>' );
    } else {
      // fetch new contents to show next page
      links.push( '<a href="' + _vise_search_now_get_prev_uri(d) + '" title="Previous Page">Prev</a>' );
    }
  } else {
    // first page, hence deactivate Prev button
    links.push( '<span>First</span>' );
    links.push( '<span>Prev</span>' );
  }

  if ( ( d.query.from + d.query.show_to ) < (d.QUERY_RESULT_SIZE - 1 ) ) {
    // check if Next page contents can be served from cache
    var next_page_from = d.query.from + d.query.show_to;
    var next_page_count = d.query.show_count;
    if ( ( next_page_from + next_page_count) < (d.query.from + d.query.count) ) {
      // show next page contents from cache
      links.push( '<span class="text_button" title="Next Page" onclick="_vise_search_next_page_from_cache()">Next</span>' );
    } else {
      // fetch new contents to show next page
      links.push( '<a href="' + _vise_search_now_get_next_uri(d) + '" title="Next Page">Next</a>' );
    }
  } else {
    // end of list, hence deactivate Next button
    links.push( '<span>Next</span>' );
  }

  navbuttons.innerHTML = links.join('&nbsp;|&nbsp;');
}

function _vise_search_show_all_result(d, content_panel) {
  var navbar = document.createElement('div');
  navbar.classList.add('navbar');
  content_panel.appendChild(navbar);

  d.query.show_to = d.query.show_from + d.query.show_count;
  if ( d.query.show_to > d.query_result_subset.length ) {
    d.query.show_to = d.query_result_subset.length;
  }

  // filelist navigation bar : prev,next, ... buttons
  _vise_search_update_page_nav(d, navbar);

  // clear floats
  var float_clear = document.createElement('div');
  float_clear.setAttribute('style', 'clear:both;');
  navbar.appendChild(float_clear);

  var img_grid = document.createElement('div');
  img_grid.classList.add('img_grid');

  var i;
  for ( i = d.query.show_from; i < d.query.show_to; ++i ) {
    _vise_search_show_result_i(d, i, img_grid);
  }
  content_panel.appendChild(img_grid)
}

function _vise_search_show_result_i(d, i, content_panel) {
  var img_with_region = document.createElement('div');
  img_with_region.classList.add('img_with_region');
  img_with_region.classList.add('cursor_pointer');
  img_with_region.setAttribute('onclick', '_vise_search_show_match_detail(' + i + ')');

  var img = document.createElement('img');
  img.setAttribute('src', d.image_uri_prefix + d.image_uri_namespace + d.query_result_subset[i].file_id);
  img.addEventListener('load', function(e) {
    // automatically set viewBox attribute of SVG element so that
    // region defined in original image space is correctly scaled
    // for resized image
    this.nextElementSibling.setAttributeNS(null, 'viewBox', '0 0 ' + this.naturalWidth + ' ' + this.naturalHeight);
    this.nextElementSibling.style.display = 'block'; // by default, the region is invisible. Now, make the region visible
  });
  img_with_region.appendChild(img);

  var query_region = [ d.query.x, d.query.y, d.query.width, d.query.height ];
  var dimg = _vise_search_tx_rect_using_homography(query_region, d.query_result_subset[i].H);
  img_with_region.appendChild( _vise_search_svg_region('polygon', dimg) );

/*
  var score = document.createElement('span');
  score.innerHTML = d.query_result_subset[i].score;
  img_with_region.appendChild(score);
*/
  content_panel.appendChild(img_with_region);
}


function _vise_search_next_page_from_cache() {
  _vise_search_result.query.show_from = _vise_search_result.query.show_from + _vise_search_result.query.show_count;
  _vise_search_result.query.show_count = _vise_search_result.query.show_count;

  var search_result = document.getElementById('search_result');
  search_result.innerHTML = '';
  _vise_search_show_all_result(_vise_search_result, search_result);
}

function _vise_search_prev_page_from_cache() {
  _vise_search_result.query.show_from = _vise_search_result.query.show_from - _vise_search_result.query.show_count;
  _vise_search_result.query.show_count = _vise_search_result.query.show_count;

  var search_result = document.getElementById('search_result');
  search_result.innerHTML = '';
  _vise_search_show_all_result(_vise_search_result, search_result);
}


function _vise_search_tx_rect_using_homography(drect, H) {
  var points = [];
  var p 
  p = _vise_search_tx_point_using_homography(drect[0], drect[1], H);
  points.push(p[0]);
  points.push(p[1]);
  p = _vise_search_tx_point_using_homography(drect[0], drect[1] + drect[3], H);
  points.push(p[0]);
  points.push(p[1]);
  p = _vise_search_tx_point_using_homography(drect[0] + drect[2], drect[1] + drect[3], H);
  points.push(p[0]);
  points.push(p[1]);
  p = _vise_search_tx_point_using_homography(drect[0] + drect[2], drect[1], H);
  points.push(p[0]);
  points.push(p[1]);
  return points;
}

function _vise_search_tx_point_using_homography(x, y, H) {
  var p = [];
  p[0] = H[0]*x + H[1]*y + H[2];
  p[1] = H[3]*x + H[4]*y + H[5];
  return p;
}

function _vise_search_show_query(d, content_panel) {
  var img_with_region = document.createElement('div');
  img_with_region.classList.add('img_with_region');

  var img = document.createElement('img');
  img.setAttribute('src', d.image_uri_prefix + d.image_uri_namespace + d.query.file_id);
  img.addEventListener('load', function(e) {
    // automatically set viewBox attribute of SVG element so that
    // the correct region is visible
    this.nextElementSibling.setAttributeNS(null, 'viewBox', '0 0 ' + this.naturalWidth + ' ' + this.naturalHeight);
    this.nextElementSibling.style.display = 'block'; // by default, the region is invisible. Now, make the region visible
  });

  var dimg = [ d.query.x, d.query.y, d.query.width, d.query.height ];

  img_with_region.appendChild(img);
  img_with_region.appendChild( _vise_search_svg_region('rect', dimg) );

  var query_image = document.createElement('div');
  query_image.classList.add('query_image');
  query_image.appendChild(img_with_region);

  content_panel.appendChild( _vise_search_show_query_metadata(d) );
  content_panel.appendChild( query_image );
}

function _vise_search_show_query_metadata(d) {
  var metadata = document.createElement('div');
  metadata.classList.add('metadata');
  var html = [];
  html.push('<p>Search Query</p>');
  var query_img_url = d.query_uri_prefix + '_file?file_id=' + d.query.file_id;
  html.push('<div class="row"><span class="col">Filename</span><span class="col"><a href="' + query_img_url + '">' + d.query.filename + '</a></span></div>');
  var dimg = [ d.query.x, d.query.y, d.query.width, d.query.height ];
  html.push('<div class="row"><span class="col">Region</span><span class="col" title="x, y, width, height">[' + dimg.join(', ') + ']</span></div>');
  html.push('<div class="row"><span class="col">Score Threshold</span><span class="col">');
  var query_uri = d.query_uri_prefix + '_search';
  html.push( '<form method="GET" action="' + query_uri + '">' );
  html.push( '<input type="hidden" name="file_id" value="' + d.query.file_id + '">' );
  html.push( '<input type="hidden" name="x" value="' + d.query.x + '">' );
  html.push( '<input type="hidden" name="y" value="' + d.query.y + '">' );
  html.push( '<input type="hidden" name="width" value="' + d.query.width + '">' );
  html.push( '<input type="hidden" name="height" value="' + d.query.height + '">' );
  html.push( '<input type="hidden" name="from" value="0">' );
  html.push( '<input type="hidden" name="count" value="1024">' );
  html.push( '<input type="hidden" name="show_from" value="0">' );
  html.push( '<input type="hidden" name="show_count" value="45">' );
  html.push( '<input type="text" size="4" name="score_threshold" value="' + d.query.score_threshold + '" placeholder="score threshold">' );
  html.push( '&nbsp;<button id="search_button" title="Search again using new score threshold" type="submit">Search Again</button' );
  html.push( '</form>' );

  html.push('</span></div>');
  metadata.innerHTML = html.join('');
  return metadata;
}

function _vise_search_svg_region(shape, dimg) {
  var SVG_NS = 'http://www.w3.org/2000/svg';
  var svg = document.createElementNS(SVG_NS, shape);

  switch(shape) {
    case 'rect':
      svg.setAttributeNS(null, 'x', Math.floor(dimg[0]));
      svg.setAttributeNS(null, 'y', Math.floor(dimg[1]));
      svg.setAttributeNS(null, 'width', Math.floor(dimg[2]));
      svg.setAttributeNS(null, 'height', Math.floor(dimg[3]));
      break;
    case 'polygon':
      var pts = [];
      var i, n;
      n = dimg.length;
      for ( i = 0 ; i < n; i = i+2) {
        pts.push( Math.floor(dimg[i]) + ',' + Math.floor(dimg[i+1]) );
      }
      svg.setAttributeNS(null, 'points', pts.join(' ') );
      break;
  }
  var svg_container = document.createElementNS(SVG_NS, 'svg');
  svg_container.appendChild(svg);
  return svg_container;
}

function _vise_search_hide_detail() {
  var page_overlay = document.getElementById('page_overlay');
  if ( page_overlay ) {
    page_overlay.style.display = 'none';
    page_overlay.parentNode.removeChild(page_overlay);
    document.body.classList.remove('noscroll');
  }
}

function _vise_search_show_match_detail(match_index) {
  var page_overlay = document.createElement('div');
  page_overlay.setAttribute('id', 'page_overlay');
  page_overlay.style.display = 'block';
  page_overlay.addEventListener('click', function(e) {
    // click on the outside overlay region hides the overlay
    if ( e.target.id === 'page_overlay' ) {
      _vise_search_hide_detail();
    }
  });

  var content = document.createElement('div');
  page_overlay.appendChild(content);
  content.classList.add('content');

  var toolbar = document.createElement('div');
  toolbar.classList.add('toolbar');
  content.appendChild(toolbar);
  toolbar.innerHTML = '<span class="text_button" onclick="_vise_search_hide_detail()">&times;</span>';
  
  _vise_search_show_match_pair_comparison(match_index, content);

  document.body.classList.add('noscroll');
  document.body.appendChild(page_overlay);
}

function _vise_search_show_match_pair_comparison(match_index, content) {
  if ( typeof(_vise_search_result) === 'undefined' ) {
    console.log('_vise_search_result is not defined');
    return;
  }

  var compare_panel = document.createElement('div');
  content.appendChild(compare_panel);
  compare_panel.classList.add('compare_panel');

  // caption row
  var r1 = document.createElement('div');
  compare_panel.appendChild(r1);
  r1.classList.add('row');
  var r1c1 = document.createElement('span');
  r1.appendChild(r1c1);
  r1c1.classList.add('col');
  r1c1.classList.add('col_query');
  r1c1.innerHTML = 'Query';
  var r1c2 = document.createElement('span');
  r1.appendChild(r1c2);
  r1c2.classList.add('col');
  r1c2.classList.add('col_compare');
  r1c2.innerHTML = 'Comparison';
  var r1c3 = document.createElement('span');
  r1.appendChild(r1c3);
  r1c3.classList.add('col');
  r1c3.classList.add('col_match');
  r1c3.innerHTML = 'Match ( score = ' + _vise_search_result.query_result_subset[match_index].score + ' )';

  // image row
  var r2 = document.createElement('div');
  compare_panel.appendChild(r2);
  r2.classList.add('row');

  // left: query
  var r2c1 = document.createElement('span');
  r2.appendChild(r2c1);
  r2c1.classList.add('query');
  var qsrc = _vise_search_result.image_uri_prefix + _vise_search_result.image_uri_namespace + _vise_search_result.query.file_id;
  _vise_search_set_cropped_img(r2c1, qsrc, _vise_search_result.query.x, _vise_search_result.query.y, _vise_search_result.query.width, _vise_search_result.query.height)

  // center: compare element
  var r2c2 = document.createElement('span');
  r2.appendChild(r2c2);
  r2c2.classList.add('compare');
  _vise_search_set_compare_element(r2c2);

  // right: match
  var r2c3 = document.createElement('span');
  r2.appendChild(r2c3);
  r2c3.classList.add('match');
  _vise_search_set_cropped_match_img(r2c3, match_index);

  // filename row
  var r3 = document.createElement('div');
  compare_panel.appendChild(r3);
  r3.classList.add('row');
  var r3c1 = document.createElement('span');
  r3.appendChild(r3c1);
  r3c1.classList.add('col');
  var query_img_url = _vise_search_result.query_uri_prefix + '_file?file_id=' + _vise_search_result.query.file_id;
  r3c1.innerHTML = '<a href="' + query_img_url + '">' + _vise_search_result.query.filename + '</a>';
  var r3c2 = document.createElement('span');
  r3.appendChild(r3c2);
  r3c2.classList.add('col');
  r3c2.innerHTML = 'Click on the image to flip and compare';
  var r3c3 = document.createElement('span');
  r3.appendChild(r3c3);
  r3c3.classList.add('col');
  var match_img_url = _vise_search_result.query_uri_prefix + '_file?file_id=' + _vise_search_result.query_result_subset[match_index].file_id;
  r3c3.innerHTML  = '<a href="' + match_img_url + '">' + _vise_search_result.query_result_subset[match_index].filename + '</a>';
}

function _vise_search_get_img_element(src, id, classname, alt, title) {
  var p = document.createElement('img');
  p.setAttribute('src', src);

  if ( id ) {
    p.setAttribute('id', id);
  }

  if ( classname ) {
    p.classList.add(classname);
  }

  if ( alt ) {
    p.setAttribute('alt', alt);
  }

  if ( title ) {
    p.setAttribute('title', title);
  }
  return p;
}

function _vise_search_set_compare_element(container) {
  var left = document.getElementById('compare_detail_left_canvas');
  if ( !left ) {
    // retry after some delay
    setTimeout( function() {
      _vise_search_set_compare_element(container);
    }.bind(this), 100);
    return;
  }

  var c = document.createElement('canvas');
  c.setAttribute('id', 'compare_detail_center_canvas');
  c.setAttribute('style', 'cursor:pointer;');
  var cx = c.getContext('2d');
  c.height = left.height;
  c.width  = left.width;
  cx.drawImage(left, 0, 0);
  c.addEventListener('mousedown', function(e) {
    var right = document.getElementById('compare_detail_right_img');
    if ( right ) {
      cx.drawImage(right, 0, 0);
    }
  });
  c.addEventListener('mouseup', function(e) {
    cx.drawImage(left, 0, 0);
  });
  container.appendChild(c);
}

function _vise_search_set_cropped_img(element, img_src, x, y, width, height) {
  var img = new Image();
  img.addEventListener('load', function(e) {
    var c = document.createElement('canvas');
    c.setAttribute('id', 'compare_detail_left_canvas');
    var cx = c.getContext('2d');
    c.height = height;
    c.width  = width;
    cx.drawImage(img, x, y, width, height, 0, 0, width, height);
    element.appendChild(c);
  });
  img.src = img_src;
}

function _vise_search_set_cropped_match_img(element, match_index) {
  var match_img = document.createElement('img');
  match_img.setAttribute('id', 'compare_detail_right_img');
  match_img.addEventListener('error', function() {
    element.innerHTML = '<p style="color:red;">Failed to register match image with the query region</p>';
  });
  match_img.addEventListener('load', function() {
    element.innerHTML = '';
    element.appendChild(match_img);
  });
  var register_uri = [];
  register_uri.push('file1_id=' + _vise_search_result.query.file_id);
  register_uri.push('file2_id=' + _vise_search_result.query_result_subset[match_index].file_id);
  register_uri.push('x1=' + _vise_search_result.query.x);
  register_uri.push('y1=' + _vise_search_result.query.y);
  register_uri.push('width1=' + _vise_search_result.query.width);
  register_uri.push('height1=' + _vise_search_result.query.height);
  match_img.src = _vise_search_result.query_uri_prefix + '_register?' + register_uri.join('&');
  element.innerHTML = '<p style="color:blue;">Registering match image with the query region ...</p>';
  element.innerHTML += '<p>(this usually takes around 2 sec but may take longer for high resolution images)</p>';
}

//
// affine homography utils
//
function _vise_homography_inverse(H) {
  var invdet = H[0]*H[4]*H[8] - H[0]*H[5]*H[7] - H[3]*H[1]*H[8] + H[3]*H[2]*H[7] + H[6]*H[1]*H[5] - H[6]*H[2]*H[4];
  invdet = 1.0 / invdet;
  var Hinv = [];
  Hinv[0]=  (H[4]*H[8] - H[5]*H[7])*invdet;
  Hinv[1]= -(H[1]*H[8] - H[2]*H[7])*invdet;
  Hinv[2]=  (H[1]*H[5] - H[2]*H[4])*invdet;
  Hinv[3]= -(H[3]*H[8] - H[5]*H[6])*invdet;
  Hinv[4]=  (H[0]*H[8] - H[2]*H[6])*invdet;
  Hinv[5]= -(H[0]*H[5] - H[2]*H[3])*invdet;
  Hinv[6]=  (H[3]*H[7] - H[4]*H[6])*invdet;
  Hinv[7]= -(H[0]*H[7] - H[1]*H[6])*invdet;
  Hinv[8]=  (H[0]*H[4] - H[1]*H[3])*invdet;
  return _vise_homography_normalize(Hinv); // normalize such that H[8] = 1.0
}

function _vise_homography_normalize(H) {
  var h = H.slice(0);
  h[0]/= h[8]; h[1]/= h[8]; h[2]/= h[8];
  h[3]/= h[8]; h[4]/= h[8]; h[5]/= h[8];
  h[6]/= h[8]; h[7]/= h[8]; h[8]= 1.0;
  return h;
}


//
// page navigation utils
//
function _vise_search_now_get_search_uri(d) {
  var uri = d.query_uri_prefix + '_search?file_id=' + d.query.file_id + '&' + 'x=' + d.query.x + '&y=' + d.query.y + '&width=' + d.query.width + '&height=' + d.query.height + '&';
  return uri;
}


function _vise_search_now_get_first_uri(d) {
  var uri = [];
  uri.push('from=0' );

  uri.push('count=1024' );
  uri.push('show_from=0' );
  uri.push('show_count=45' );
  return _vise_search_now_get_search_uri(d) + uri.join('&');
}

function _vise_search_now_get_next_uri(d) {
  var uri = [];
  var new_from = d.query.from + d.query.show_to;
  uri.push('from=' + new_from );

  uri.push('count=' + d.query.count );
  uri.push('show_from=0' );
  uri.push('show_count=' + d.query.show_count );
  return _vise_search_now_get_search_uri(d) + uri.join('&');
}

function _vise_search_now_get_prev_uri(d) {
  var uri = [];
  var new_from = d.query.from - d.query.count;
  if ( new_from < 0 ) {
    new_from = 0;
  }
  uri.push('from=' + new_from );

  uri.push('count=' + d.query.count );
  var show_from = d.query.from - d.query.show_to - new_from;
  if ( show_from < 0 ) {
    show_from = 0;
  }
  uri.push('show_from=' + show_from);
  uri.push('show_count=' + d.query.show_count );

  return _vise_search_now_get_search_uri(d) + uri.join('&');
}

//
// search result export
//
function _vise_search_export_search_result() {
  var p = document.getElementById('search_result_export_format');
  var format = p.options[ p.selectedIndex ].value;
  var ext = format.split('_')[1];    

  var search_result_data = _vise_search_result;
  if ( _vise_search_result.QUERY_RESULT_SIZE > _vise_search_result.query.count ) {
    // get full search results from server
    // search_result_data = ...
    var uri = [];
    uri.push('from=0' );

    uri.push('count=0' );
    uri.push('show_from=0' );
    uri.push('show_count=0' );
    uri.push('format=json' );

    var xhr = new XMLHttpRequest();
    xhr.addEventListener('load', function() {
      var response_str = xhr.responseText;
      var search_result_data = JSON.parse(response_str);

      var d = _vise_search_prepare_search_result_export(search_result_data, format);
      // Javascript strings (DOMString) is automatically converted to utf-8
      // see: https://developer.mozilla.org/en-US/docs/Web/API/Blob/Blob
      var blob_mime = {type: 'text/' + ext + ';charset=utf-8'};
      var export_blob = new Blob( [ d ], blob_mime);
      _vise_save_data_as_local_file(export_blob, 'vise_search_result.' + ext);
    });
    xhr.open('GET', _vise_search_now_get_search_uri(_vise_search_result) + uri.join('&'));
    xhr.send();
  } else {
    var d = _vise_search_prepare_search_result_export(search_result_data, format);
    // Javascript strings (DOMString) is automatically converted to utf-8
    // see: https://developer.mozilla.org/en-US/docs/Web/API/Blob/Blob
    var blob_mime = {type: 'text/' + ext + ';charset=utf-8'};
    var export_blob = new Blob( [ d ], blob_mime);
    _vise_save_data_as_local_file(export_blob, 'vise_search_result.' + ext);
  }
}


function _vise_search_prepare_search_result_export(data, format) {
  var export_data = '';
  switch(format) {
    case 'plain_csv':
      var d = [];
      d.push('file_id,type,filename,score,x,y,width,height,homography00,homography01,homography02,homography10,homography11,homography12');
      var row = [];
      row.push(data.query.file_id);
      row.push('query');
      row.push(data.query.filename);
      row.push('-1');
      row.push(data.query.x);
      row.push(data.query.y);
      row.push(data.query.width);
      row.push(data.query.height);
      row.push('1,0,0,0,1,0,0,0,1');
      d.push( row.join(',') );
      var i;
      for ( i = 0; i < data.query_result_subset.length; ++i ) {
        row = [];
        row.push(data.query_result_subset[i].file_id);
        row.push('match');
        row.push(data.query_result_subset[i].filename);
        row.push(data.query_result_subset[i].score);
        row.push(data.query.x);
        row.push(data.query.y);
        row.push(data.query.width);
        row.push(data.query.height);
        row.push(data.query_result_subset[i].H[0]);
        row.push(data.query_result_subset[i].H[1]);
        row.push(data.query_result_subset[i].H[2]);
        row.push(data.query_result_subset[i].H[3]);
        row.push(data.query_result_subset[i].H[4]);
        row.push(data.query_result_subset[i].H[5]);      
        d.push( row.join(',') );
      }
      export_data = d.join('\n');
      break;
    case 'plain_json':
      export_data = JSON.stringify(data);
      break;
    case 'via_csv':
      var d = [];
      d.push('filename,file_size,file_attributes,region_count,region_id,region_shape_attributes,region_attributes');
      var row = [];
      row.push(data.query.filename);
      row.push('-1');
      row.push('"{""file_id"":' + data.query.file_id + '}"');
      row.push('1,0');
      row.push('"{""name"":""rect"",""x"":' + data.query.x + ',""y"":' + data.query.y + ',""width"":' + data.query.width + ',""height"":' + data.query.height + '}"');
      row.push('"{""type"":""query""}"');
      d.push( row.join(',') );
      var i;
      var query_region = [ data.query.x, data.query.y, data.query.width, data.query.height ];

      for ( i = 0; i < data.query_result_subset.length; ++i ) {
        row = [];
        row.push(data.query_result_subset[i].filename);
        row.push('-1');
        row.push('"{""file_id"":' + data.query_result_subset[i].file_id + '}"');
        row.push('1,0');
        var dimg = _vise_search_tx_rect_using_homography(query_region, data.query_result_subset[i].H);
        var all_points_x = [];
        all_points_x.push(dimg[0]);
        all_points_x.push(dimg[2]);
        all_points_x.push(dimg[4]);
        all_points_x.push(dimg[6]);
        var all_points_y = [];
        all_points_y.push(dimg[1]);
        all_points_y.push(dimg[3]);
        all_points_y.push(dimg[5]);
        all_points_y.push(dimg[7]);
        row.push('"{""name"":""polygon"",""all_points_x"":[' + all_points_x + '],""all_points_y"":[' + all_points_y + ']}"');
        row.push('"{""type"":""match"",""score"":' + data.query_result_subset[i].score + '}"');
        d.push( row.join(',') );
      }
      export_data = d.join('\n');
      break;
    case 'via_json':
      break;
    default:
      console.log('unknown export format: ' + format);
      break;
  }
  return export_data;
}
