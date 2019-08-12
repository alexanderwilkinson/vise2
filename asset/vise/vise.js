/*
  @file        _vise.js
  @description Handles the user interactions in web based user interface of VISE
  @author      Abhishek Dutta <adutta@robots.ox.ac.uk>
  @date        22 June 2018
*/

var _vise_server_hostname = 'http://localhost';
var _vise_server_port     = '9973';
var _vise_admin_ns       = 'vise/admin/';
var _vise_query_ns       = 'vise/query/';
var _vise_server_uri      = _vise_server_hostname + ':' + _vise_server_port + '/';
var _vise_new_search_engine = {};

// state variables
var is_new_search_engine_image_add_ongoing = false;

// misc variables
var _vise_message_clear_timer = '';

// UI html elements
var invisible_file_input = document.getElementById("invisible_file_input");

// constants
var VISE_THEME_MESSAGE_TIMEOUT_MS     = 5000;
var VISE_FILE_UPLOAD_CHUNK_SIZE       = 50;
var VISE_INDEX_STATUS_UPDATE_INTERVAL = 2000;

// invoked by onload() event handler defined in <body> of vise.html
function vise_init() {
  show_message('VISE 2.0.0 ready');
  _vise_new_search_engine = new search_engine( '', '1' );

  vise_reset_new_search_engine_panel();
/*
  document.getElementById('new_search_engine_add_image_panel').style.display = 'table-cell';
  document.getElementById('new_search_engine_image_preview_panel').style.display = 'table-cell';
  document.getElementById('new_search_engine_metadata_io_panel').style.display = 'table-cell';
  document.getElementById('new_search_engine_indexing_control_panel').style.display = 'table-cell';
  document.getElementById('new_search_engine_indexing_progress_panel').style.display = 'table-cell';
*/
}

function vise_reset_new_search_engine_panel() {
  document.getElementById('new_search_engine_add_image_panel').style.display = 'table-cell';
  document.getElementById('new_search_engine_metadata_io_panel').style.display = 'none';
  document.getElementById('new_search_engine_image_preview_panel').style.display = 'none';
  document.getElementById('new_search_engine_indexing_control_panel').style.display = 'none';
  document.getElementById('new_search_engine_indexing_progress_panel').style.display = 'none';

}

function search_engine(name, version) {
  this.name    = name;
  this.version = version || '1';
  this.desc    = '';
  this.files   = [];
  this.filename_list = [];
  this.file_upload_status = [];
  this.file_upload_count  = 0;
  this.status  = {};
}

function sel_local_images() {
  // source: https://developer.mozilla.org/en-US/docs/Using_files_from_web_applications
  if (invisible_file_input) {
    invisible_file_input.setAttribute('multiple', 'multiple')
    invisible_file_input.accept   = '.jpg,.jpeg,.png,.bmp';
    invisible_file_input.onchange = search_engine_file_add_local;
    invisible_file_input.click();
  }
}

function search_engine_file_upload_chunk(start_index, end_index) {
  var n = _vise_new_search_engine.files.length;
  if ( start_index < 0 ) {
    return;
  }
  if ( end_index >= n ) {
    end_index = n;
  }
  //console.log('search_engine_file_upload_chunk(): uploading ' + n + ':' + start_index + ':' + end_index);
  //document.getElementById('new_search_engine_message').innerHTML = 'uploading image ' + start_index + ' to ' + end_index;

  if ( (end_index - start_index) === 1 ) {
    show_message('uploading image ' + start_index + ' of ' + n);
  } else {
    show_message('uploading image ' + start_index + ' to ' + end_index + ' of ' + n);
  }

  var i;
  var promise_list = [];

  for ( i = start_index; i < end_index; ++i ) {
    var name = _vise_new_search_engine.name;
    var version = _vise_new_search_engine.version;
    var args = ['filename=' + _vise_new_search_engine.filename_list[i]];
    var payload = _vise_new_search_engine.files[i];

    promise_list.push( vise_send_server_command(name, version, 'add_file', args, payload) );
  }

  Promise.all(promise_list).then( function(values) {
    //console.log(values);
    var i;
    var n = values.length;
    for ( i = 0; i < n; ++i ) {
      if ( JSON.parse(values[i]).filename === _vise_new_search_engine.filename_list[start_index + i] ) {
        _vise_new_search_engine.file_upload_status[i] = 1;
        _vise_new_search_engine.file_upload_count += 1;
      }
    }

    if ( end_index < _vise_new_search_engine.files.length ) {
      var new_start_index = end_index;
      var new_end_index = new_start_index + VISE_FILE_UPLOAD_CHUNK_SIZE;
      search_engine_file_upload_chunk(new_start_index, new_end_index);
    } else {
      //console.log(_vise_new_search_engine.file_upload_status);
      show_message('uploaded [' + _vise_new_search_engine.file_upload_count + '] files');
      on_new_search_engine_file_upload_done();
    }
  }.bind(this), function(err) {
    console.log(err);
    show_message('failed to upload [' + err.length + '] files');
  }.bind(this));
}

function search_engine_file_add_local(event) {
  if ( ! is_new_search_engine_image_add_ongoing ) {
    // first add event
    is_new_search_engine_image_add_ongoing = true;
    var added_img_count = 0;
    var promise_list = [];
    var user_selected_images = event.target.files;
    for ( var i = 0; i < user_selected_images.length; ++i ) {
      var filetype = user_selected_images[i].type.substr(0, 5);
      if ( filetype === 'image' ) {
        _vise_new_search_engine.files.push( user_selected_images[i] );
        _vise_new_search_engine.filename_list.push(  user_selected_images[i].name );
        _vise_new_search_engine.file_upload_status.push(0);

        added_img_count += 1;
      }
    }
    show_message('added [' + added_img_count + '] files');
    on_new_search_engine_file_add();
  }
}

function vise_start_indexing() {
  var args = [];
  var payload = _vise_new_search_engine.desc;
  vise_send_server_command(_vise_new_search_engine.name, _vise_new_search_engine.version, 'init', args, payload).then( function(ok) {
    // sanity check
    console.log(ok)
    var d = JSON.parse(ok);
    if ( d.search_engine_name !== _vise_new_search_engine.name ||
         d.search_engine_version !== _vise_new_search_engine.version ) {
      show_message('Created search engine name does not match with the one sent in request!');
      return;
    }

    document.getElementById('new_search_engine_select_image_button').setAttribute('disabled', 'true');
    document.getElementById('new_search_engine_start_indexing_button').setAttribute('disabled', 'true');

    // trigger file upload
    search_engine_file_upload_chunk(0, VISE_FILE_UPLOAD_CHUNK_SIZE);
  }.bind(this), function(err) {
    console.log(err);
    show_message('Failed to initialize new search engine');
  }.bind(this));
}

function on_new_search_engine_file_upload_done() {
  var name = _vise_new_search_engine.name;
  var version = _vise_new_search_engine.version;
  vise_send_server_command(name, version, 'index_start');
  show_message('Indexing started');

  document.getElementById('new_search_engine_indexing_progress_panel').style.display = 'table-cell';

  setTimeout( vise_update_indexing_status, VISE_INDEX_STATUS_UPDATE_INTERVAL );
}

function vise_update_indexing_status() {
  var name = _vise_new_search_engine.name;
  var version = _vise_new_search_engine.version;
  vise_send_server_command(name, version, 'index_status').then( function(ok) {
    try {
      var is_error = false;
      var is_all_done  = true;
      var d = JSON.parse(ok);
      var i, n;
      n = d.length;
      var ol = document.createElement('ol');
      for ( i = 0; i < n; ++i ) {
        var li = document.createElement('li');
        li.classList.add( d[i].state );
        li.innerHTML = d[i].description;
        if ( d[i].state === 'progress' ) {
          li.innerHTML += " : " + d[i].steps_done + " of " + d[i].steps_count;
        }
        if ( d[i].state === 'error' ) {
          is_error = true;
        }
        if ( d[i].state !== 'done' ) {
          is_all_done = false;
        }
        ol.appendChild(li);
      }
      var p = document.getElementById('new_search_engine_indexing_progress_panel');
      p.innerHTML = '';
      p.appendChild(ol);

      if ( ! is_error && ! is_all_done ) {
        setTimeout( vise_update_indexing_status, VISE_INDEX_STATUS_UPDATE_INTERVAL );
      }

      if ( is_all_done ) {
        console.log('done');
        show_message('Search engine creation completed');
      }
      if ( is_error ) {
        console.log('error');
        show_message('Error occured while creating search engine');
      }
    } catch(error) {
      console.log(error);
      show_message('malformed status');
    }
  }, function(err) {
    console.log(err);
    show_message('failed to get status');
    setTimeout( vise_update_indexing_status, 2*VISE_INDEX_STATUS_UPDATE_INTERVAL );
  });
}

function vise_show_indexing_log() {

}

function is_search_engine_name_valid(name) {
  if ( name.startsWith(' ') || name.startsWith('__') ) {
    return false;
  }

  if ( name.includes('/') ) {
    return false;
  }
  if ( name.includes('*') ) {
    return false;
  }
  if ( name.includes('.') ) {
    return false;
  }
  if ( name.includes('\\') ) {
    return false;
  }
  if ( name.includes('%') ) {
    return false;
  }
  if ( name.includes('"') ) {
    return false;
  }
  if ( name.includes('\'') ) {
    return false;
  }
  if ( name.includes('$') ) {
    return false;
  }
  if ( name.includes('&') ) {
    return false;
  }
  if ( name.includes('^') ) {
    return false;
  }
  if ( name.includes('~') ) {
    return false;
  }
  if ( name.includes(':') ) {
    return false;
  }
  if ( name.includes('?') ) {
    return false;
  }
  if ( name.includes('#') ) {
    return false;
  }

  return true;
}

function on_new_search_engine_name_update(p) {
  var name = p.value.trim();
  var version = '1';

  if ( ! is_search_engine_name_valid(name) ) {
    p.value = '';
    show_message('Entered name is invalid. Please chose another name.');
    return;
  }

  vise_send_server_command(name, version, 'engine_exists').then( function(ok) {
    var d = JSON.parse(ok);
    if ( d.no ) {
      _vise_new_search_engine.name = name;
      _vise_new_search_engine.version = version;

      document.getElementById('new_search_engine_add_image_panel').style.display = 'table-cell';
      document.getElementById('new_search_engine_image_preview_panel').style.display = 'table-cell';
      document.getElementById('new_search_engine_metadata_io_panel').style.display = 'table-cell';
      document.getElementById('new_search_engine_indexing_control_panel').style.display = 'table-cell';
      document.getElementById('new_search_engine_indexing_progress_panel').style.display = 'none';
    } else {
      p.value = '';
      show_message('Entered name is invalid. Please chose another name.');
    }
  }.bind(this), function(err) {
    show_message('failed to validate engine name');
  }.bind(this));
}

function on_new_search_engine_desc_update(p) {
  _vise_new_search_engine.desc = p.value.trim();
}

function vise_send_server_command(name, version, cmd, uri_param, payload) {
  return new Promise( function(ok_callback, err_callback) {
    var xhr = new XMLHttpRequest();
    xhr.addEventListener('load', function(e) {
      ok_callback(xhr.responseText);
    });
    xhr.addEventListener('error', function(e) {
      err_callback(xhr.responseText);
    });
    xhr.addEventListener('abort', function(e) {
      err_callback(xhr.responseText);
    });
    xhr.addEventListener('timeout', function(e) {
      err_callback(xhr.responseText);
    });

    if ( typeof(uri_param) === 'undefined' ) {
      xhr.open('POST', _vise_server_uri + '/' + name + '/' + version + '/' + cmd);
    } else {
      xhr.open('POST', _vise_server_uri + '/' + name + '/' + version + '/' + cmd + '?' + uri_param.join('&'));
    }

    if ( typeof(payload) === 'undefined' ) {
      xhr.send();
    } else {
      xhr.send(payload);
    }
  });
}

//
// ui state maintainers
//
function on_new_search_engine_file_add() {
  console.log('updating dropdown');
  var e = document.getElementById('new_search_engine_select_preview_image');
  e.innerHTMl = '';
  var i, n;
  n = _vise_new_search_engine.filename_list.length;
  for ( i = 0; i < n; ++i ) {
    if ( _vise_new_search_engine.file_upload_status ) {
      var o = document.createElement('option');
      o.setAttribute('value', i);
      o.innerHTML = '[' + (i+1) + '] ' + _vise_new_search_engine.filename_list[i];
      e.appendChild(o);
    }
  }
  e.selectedIndex = 0;
  update_new_search_engine_preview_image(0);

  document.getElementById('new_search_engine_add_image_panel').style.display = 'table-cell';
  document.getElementById('new_search_engine_image_preview_panel').style.display = 'table-cell';
  document.getElementById('new_search_engine_metadata_io_panel').style.display = 'table-cell';
  document.getElementById('new_search_engine_indexing_control_panel').style.display = 'none';
  document.getElementById('new_search_engine_indexing_progress_panel').style.display = 'none';
}

function on_new_search_engine_filename_dropdown_update(e) {
  var index = e.options[e.selectedIndex].value;
  update_new_search_engine_preview_image(index);
}

function update_new_search_engine_preview_image(index) {
  var filereader = new FileReader();
  filereader.addEventListener( 'load', function() {
    document.getElementById('new_search_engine_preview_image').src = filereader.result;
  });
  filereader.readAsDataURL( _vise_new_search_engine.files[index] );
}

//
// misc
//

function show_message(msg, t) {
  if ( _vise_message_clear_timer ) {
    clearTimeout(_vise_message_clear_timer); // stop any previous timeouts
  }
  var timeout = t;
  if ( typeof t === 'undefined' ) {
    timeout = VISE_THEME_MESSAGE_TIMEOUT_MS;
  }
  document.getElementById('message_panel_content').innerHTML = msg;
  document.getElementById('message_panel').style.display = 'block';

  _vise_message_clear_timer = setTimeout( function() {
    document.getElementById('message_panel').style.display = 'none';
  }, timeout);
}


function generate_search_engine_rand_name() {
  //source: http://stackoverflow.com/questions/105034/how-to-create-a-guid-uuid-in-javascript
  return '__' + Math.random().toString(36).substring(2, 15) + Math.random().toString(36).substring(2, 15);
  //return new Date();
}
