/*
  @file        _vise_file.js
  @description query the details of a file
  @author      Abhishek Dutta <adutta@robots.ox.ac.uk>
  @date        6 August 2018
*/

/*
precondition: variable '_vise_file_str' contains the JSON string
representation of VISE search engine file in a format given below:

GET http://localhost:9973/vise/query/ox5k/1/_file?file_id=10&format=json
{
  "search_engine_id": "ox5k/1",
  "home_uri": "/vise/home.html",
  "image_uri_prefix": "/vise/asset/ox5k/1/",
  "image_uri_namespace": "image/",
  "query_uri_prefix": "/vise/query/ox5k/1/",
  "filename": "all_souls_000012.jpg",
  "file_id": 10
}
*/

function _vise_file() {
  // _vise_file_result_str is a global variable set by the vise_server
  if ( typeof(_vise_file_str) === 'undefined' ) {
    console.log('Error: _vise_file_str is not defined');
    return;
  }

  // _vise_file is a global variable
  _vise_file = JSON.parse(_vise_file_str);
  //console.log(_vise_file); // debug

  var page = document.createElement('div');
  var file = document.createElement('div');
  var nav = document.createElement('div');
  page.classList.add('page');
  file.classList.add('file')
  nav.classList.add('nav');
  page.appendChild(nav);
  page.appendChild(file);

  _vise_init_top_nav(_vise_file, nav);

  // initialize search toolbar
  var search_toolbar = document.createElement('div');
  search_toolbar.classList.add('search_toolbar');
  _vise_file_init_search_toolbar(_vise_file, search_toolbar);
  file.appendChild(search_toolbar);

  // initialize VIA
  var via_container = document.createElement('div');
  via_container.classList.add('region_selector_panel');
  var via_panel = document.createElement('div');
  via_container.appendChild(via_panel);
  _vise_file_init_via_panel(_vise_file, via_panel);
  file.appendChild(via_container);

  // initialize metadata
  var metadata_panel = document.createElement('div');
  metadata_panel.classList.add('metadata_panel');
  _vise_file_init_metadata_panel(_vise_file, metadata_panel);
  file.appendChild(metadata_panel);

  document.body.appendChild(page);
}

function _vise_file_init_via_panel(d, panel) {
  _vise_file_via = new _via();
  _vise_file_via.init(panel);

  var img_url = d.image_uri_prefix + d.image_uri_namespace + d.file_id;
  var promise = _vise_file_via.m.add_file_from_url(img_url, 'image');
  promise.then( function(ok) {
    _vise_file_via.c.load_file_from_index(0);
    _vise_file_init_via_hooks();
  }, function(err) {
    console.log('Error loading image into VIA')
  });
}

function _vise_file_init_via_hooks() {
  _vise_file_via.c.add_hook(_vise_file_via.c.hook.id.REGION_ADDED, function(param) {
    console.log('hook: region added : fileid=' + param.fileid + ', rid=' + param.rid);
    // delete old region from image1
    if ( _vise_file_via.v.now.all_rid_list.length > 1 ) {
      var old_region = [ _vise_file_via.v.now.all_rid_list[0] ];
      _vise_file_via.c.region_delete(old_region);
    }
    var rid = _vise_file_via.v.now.all_rid_list[0];
    var region = _vise_file_via.m.regions[param.fileid][rid].dimg.slice(0);
    
    if ( _vise_file_via.v.now.all_rid_list.length ) {
      document.getElementById('search_button').removeAttribute('disabled');
      _vise_file_set_image_region(region);
    } else {
      document.getElementById('search_button').setAttribute('disabled', '');
      _vise_file_set_image_region([]);
    }
  });

  _vise_file_via.c.add_hook(_vise_file_via.c.hook.id.REGION_RESIZED, function(param) {
    var region = _vise_file_via.m.regions[param.fileid][param.rid].dimg.slice(0);
    _vise_file_set_image_region(region);
  });
  _vise_file_via.c.add_hook(_vise_file_via.c.hook.id.REGION_MOVED, function(param) {
    var region = _vise_file_via.m.regions[param.fileid][param.rid].dimg.slice(0);
    _vise_file_set_image_region(region);
  });
}

function _vise_file_set_image_region(r) {
  var p = document.getElementById('search_region');
  var region = [];
  region.push(r[0]);
  region.push(r[1]);
  region.push(r[2] - r[0]);
  region.push(r[3] - r[1]);

  document.getElementById('region_x').setAttribute('value', region[0]);
  document.getElementById('region_y').setAttribute('value', region[1]);
  document.getElementById('region_width').setAttribute('value', region[2]);
  document.getElementById('region_height').setAttribute('value', region[3]);

  var search_region_info = document.getElementById('search_region_info');
  search_region_info.innerHTML = 'selected region = ' + JSON.stringify(region);
}

function _vise_file_init_metadata_panel(d, panel) {
  var html = [];
  var img_url = d.image_uri_prefix + d.image_uri_namespace + d.file_id;
  html.push('<div>Filename: <a href="' + img_url + '">' + d.filename + '</a></div>');
  html.push('<div class="info" id="search_region_info">To define a search region in the image shown above, drag mouse cursor by keeping the right mouse button pressed on the image.</div>');
  panel.innerHTML = html.join('');
}

function _vise_file_init_search_toolbar(d, panel) {
  var html = [];
  var query_uri = d.query_uri_prefix + '_search';
  html.push( '<form method="GET" action="' + query_uri + '">' );
  html.push( '<input type="hidden" name="file_id" value="' + d.file_id + '">' );
  html.push( '<input type="hidden" id="region_x" name="x" value="">' );
  html.push( '<input type="hidden" id="region_y" name="y" value="">' );
  html.push( '<input type="hidden" id="region_width" name="width" value="">' );
  html.push( '<input type="hidden" id="region_height" name="height" value="">' );
  html.push( '<input type="hidden" name="from" value="0">' );
  html.push( '<input type="hidden" name="count" value="1024">' );
  html.push( '<input type="hidden" name="show_from" value="0">' );
  html.push( '<input type="hidden" name="show_count" value="45">' );
  html.push( '<input type="hidden" name="score_threshold" value="8">' );
  html.push( '<button id="search_button" disabled title="First define a search region and press this button to perform visual search of this region." type="submit">Search</button' );
  html.push( '</form>' );
  panel.innerHTML = html.join('');
}
