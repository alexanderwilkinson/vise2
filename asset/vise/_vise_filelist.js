/*
  @file        _vise_filelist.js
  @description display the internal files of search engine in VISE
  @author      Abhishek Dutta <adutta@robots.ox.ac.uk>
  @date        1 August 2018
*/

/*
precondition: variable '_vise_filelist_str' contains the JSON string
representation of VISE search engine filelist in a format given below:

GET http://localhost:9973/vise/query/ox5k/1/_filelist?from=0&count=10&filename_regex=radcliff&format=json

{
  "search_engine_id": "ox5k/1",
  "image_uri_prefix": "/vise/asset/ox5k/1/",
  "home_uri": "/vise/home.html",
  "image_uri_namespace": "image/",
  "query_uri_prefix": "/vise/query/ox5k/1/",
  "FILELIST_SIZE": 282,
  "from": 0,
  "count": 10,
  "filename_regex": "radcliff",
  "file_id_list_subset": [
    4494,
    4495,
    4496,
    4497,
    4498,
    4499,
    4500,
    4501,
    4502,
    4503
  ]
}
*/

function _vise_filelist() {
  // _vise_filelist_result_str is a global variable set by the vise_server
  if ( typeof(_vise_filelist_str) === 'undefined' ) {
    console.log('Error: _vise_filelist_str is not defined');
    return;
  }

  // _vise_filelist is a global variable
  _vise_filelist = JSON.parse(_vise_filelist_str);
  //console.log(_vise_filelist); // debug

  var page = document.createElement('div');
  var nav = document.createElement('div');
  var filelist = document.createElement('div');
  filelist.setAttribute('id', 'filelist');
  page.setAttribute('id', 'filelist_page');
  page.classList.add('page');
  nav.classList.add('nav');
  filelist.classList.add('result')
  page.appendChild(nav);
  page.appendChild(filelist);

  _vise_init_top_nav(_vise_filelist, nav);
  _vise_filelist_show_all_files(_vise_filelist, filelist);

  document.body.appendChild(page);
}


function _vise_filelist_update_page_nav(d, navbar) {
  // info. about filelist
  var navinfo = document.createElement('div');
  navinfo.classList.add('navinfo');
  navbar.appendChild(navinfo);

  var navinfohtml = [];
  navinfohtml.push( 'Total ' + d.FILELIST_SIZE + ' images. Showing images from&nbsp;' );
  navinfohtml.push( d.from + d.show_from );
  navinfohtml.push( '&nbsp;to&nbsp;' );
  navinfohtml.push( d.from + d.show_to );
/*
  navinfohtml.push( '&nbsp;<label for="show_from">from</label>&nbsp;' );
  navinfohtml.push( '<input type="text" size="3" id="show_from" value="' + (d.from + d.show_from) + '">' );
  navinfohtml.push( '&nbsp;<label for="show_to">to</label>&nbsp;' );
  navinfohtml.push( '<input type="text" size="3" id="show_to" value="' + (d.from + d.show_to) + '">' );
*/

  navinfo.innerHTML = navinfohtml.join('');

  // filelist filter input
  var navtool = document.createElement('div');
  navtool.classList.add('navtool');
  navbar.appendChild(navtool);
  console.log(d.filename_regex)

  var navtoolhtml = [];
  navtoolhtml.push( '<form method="GET" action="' + d.query_uri_prefix + '_filelist" enctype="text/plain">' );
  navtoolhtml.push( '<input name="filename_regex" type="text" title="All image filenames containing the filter keyword will be shown." id="filename_regex" placeholder="keyword to filter file list e.g. &quot;Story of Chicago&quot;" size="34" value="' + d.filename_regex + '">' );
  navtoolhtml.push( '<input type="hidden" name="from" value="0">' );
  navtoolhtml.push( '<input type="hidden" name="count" value="1024">' );
  navtoolhtml.push( '<input type="hidden" name="show_from" value="0">' );
  navtoolhtml.push( '<input type="hidden" name="show_count" value="45">' );
  navtoolhtml.push( '&nbsp;<button type="submit">Filter</button>' );
  navtoolhtml.push( '</form>' );

  navtool.innerHTML = navtoolhtml.join('');

  var navbuttons = document.createElement('div');
  navbuttons.classList.add('navbuttons');
  navbar.appendChild(navbuttons);
  var links = [];

  console.log('_vise_filelist_update_page_nav(): from=' + d.from + ', count=' + d.count + ', show_from=' + d.show_from + ', show_count=' + d.show_count);
  // check if this page is the first page
  if ( (d.from + d.show_from) !== 0) {
    links.push( '<a href="' + _vise_filelist_now_get_first_uri(d) + '" title="Jump to first page">First</a>' );
    // check if Prev page contents can be served from cache
    var prev_page_from = d.show_from - d.show_count;
    if ( prev_page_from >= 0 ) {
      // show next page contents from cache
      links.push( '<span class="text_button" title="Previous Page" onclick="_vise_filelist_prev_page_from_cache()">Prev</span>' );
    } else {
      // fetch new contents to show next page
      links.push( '<a href="' + _vise_filelist_now_get_prev_uri(d) + '" title="Previous Page">Prev</a>' );
    }
  } else {
    // first page, hence deactivate Prev button
    links.push( '<span>First</span>' );
    links.push( '<span>Prev</span>' );
  }

  if ( ( d.from + d.show_to ) < (d.FILELIST_SIZE - 1 ) ) {
    // check if Next page contents can be served from cache
    var next_page_from = d.from + d.show_to;
    var next_page_count = d.show_count;
    if ( ( next_page_from + next_page_count) < (d.from + d.count) ) {
      // show next page contents from cache
      links.push( '<span class="text_button" title="Next Page" onclick="_vise_filelist_next_page_from_cache()">Next</span>' );
    } else {
      // fetch new contents to show next page
      links.push( '<a href="' + _vise_filelist_now_get_next_uri(d) + '" title="Next Page">Next</a>' );
    }
  } else {
    // end of list, hence deactivate Next button
    links.push( '<span>Next</span>' );
  }

  navbuttons.innerHTML = links.join('&nbsp;|&nbsp;');
}

function _vise_filelist_get_show_count() {
  var p = document.getElementById('show_count');
  return parseInt(p.options[p.selectedIndex].value);
}

function _vise_filelist_next_page_from_cache() {
  _vise_filelist.show_from = _vise_filelist.show_from + _vise_filelist.show_count;
  _vise_filelist.show_count = _vise_filelist.show_count;

  var filelist = document.getElementById('filelist');
  filelist.innerHTML = '';
  _vise_filelist_show_all_files(_vise_filelist, filelist);
}

function _vise_filelist_prev_page_from_cache() {
  _vise_filelist.show_from = _vise_filelist.show_from - _vise_filelist.show_count;
  _vise_filelist.show_count = _vise_filelist.show_count;

  var filelist = document.getElementById('filelist');
  filelist.innerHTML = '';
  _vise_filelist_show_all_files(_vise_filelist, filelist);
}

function _vise_filelist_show_all_files(d, content_panel) {
  var navbar = document.createElement('div');
  navbar.classList.add('navbar');
  content_panel.appendChild(navbar);

  d.show_to = d.show_from + d.show_count;
  if ( d.show_to > d.file_id_list_subset.length ) {
    d.show_to = d.file_id_list_subset.length;
  }

  // filelist navigation bar : prev,next, ... buttons
  _vise_filelist_update_page_nav(d, navbar);

  // clear floats
  var float_clear = document.createElement('div');
  float_clear.setAttribute('style', 'clear:both;');
  navbar.appendChild(float_clear);

  var img_grid = document.createElement('div');
  img_grid.classList.add('img_grid');

  var i;
  //console.log('_vise_filelist_show_all_files(): from=' + d.form + ', count=' + d.count + ', show_from=' + d.show_from + '+' + d.show_count);
  for ( i = d.show_from; i < d.show_to; ++i ) {
    _vise_filelist_show_file_i(d, i, img_grid);
  }
  content_panel.appendChild(img_grid)
}

function _vise_filelist_show_file_i(d, i, content_panel) {
  var img_with_region = document.createElement('div');
  img_with_region.classList.add('img_with_region');
  img_with_region.classList.add('cursor_pointer');

  var link = document.createElement('a');
  link.setAttribute('href', d.query_uri_prefix + '_file?file_id=' + d.file_id_list_subset[i]);
  link.setAttribute('title', 'click to view and search using this image');

  var img = document.createElement('img');
  img.setAttribute('src', d.image_uri_prefix + d.image_uri_namespace + d.file_id_list_subset[i]);
  link.appendChild(img);
  img_with_region.appendChild(link);

  // debug
  var SVG_NS = 'http://www.w3.org/2000/svg';
  var svg = document.createElementNS(SVG_NS, 'text');
  svg.setAttributeNS(null, 'x', 50);
  svg.setAttributeNS(null, 'y', 200);
  svg.setAttributeNS(null, 'fill', 'white');
  svg.setAttributeNS(null, 'stroke', 'red');
  svg.setAttributeNS(null, 'style', 'font-size:164px;stroke-width:1rem;');
  svg.innerHTML = d.file_id_list_subset[i];
  var svg_container = document.createElementNS(SVG_NS, 'svg');
  svg_container.appendChild(svg);
  img_with_region.appendChild(svg_container);

  content_panel.appendChild(img_with_region);
}

//
// page navigation utils
//
function _vise_filelist_now_get_filelist_uri(d) {
  var uri = d.query_uri_prefix + '_filelist?';
  if ( typeof(d.filename_regex) !== 'undefined' && d.filename_regex !== '' ) {
    uri += 'filename_regex=' + d.filename_regex + '&';
  }
  return uri;
}


function _vise_filelist_now_get_first_uri(d) {
  var uri = [];
  var new_from = d.from + d.show_to;
  uri.push('from=0' );

  uri.push('count=1024' );
  uri.push('show_from=0' );
  uri.push('show_count=45' );
  return _vise_filelist_now_get_filelist_uri(d) + uri.join('&');
}

function _vise_filelist_now_get_next_uri(d) {
  var uri = [];
  var new_from = d.from + d.show_to;
  uri.push('from=' + new_from );

  uri.push('count=' + d.count );
  uri.push('show_from=0' );
  uri.push('show_count=' + d.show_count );
  return _vise_filelist_now_get_filelist_uri(d) + uri.join('&');
}

function _vise_filelist_now_get_prev_uri(d) {
  var uri = [];
  var new_from = d.from - d.count;
  if ( new_from < 0 ) {
    new_from = 0;
  }
  uri.push('from=' + new_from );

  uri.push('count=' + d.count );
  var show_from = d.from - d.show_to - new_from;
  if ( show_from < 0 ) {
    show_from = 0;
  }
  uri.push('show_from=' + show_from);
  uri.push('show_count=' + d.show_count );

  return _vise_filelist_now_get_filelist_uri(d) + uri.join('&');
}
