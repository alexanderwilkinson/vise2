/*
  @file        _via_common.js
  @description methods shared by all VISE pages
  @author      Abhishek Dutta <adutta@robots.ox.ac.uk>
  @date        22 June 2017
*/

function _vise_init_top_nav(d, nav) {
  var left = [];
//  nav.innerHTML  = '<a href="' + d.home_uri + '" title="VISE home page which shows a list of available search engines">VISE Home</a>';
  var search_engine_id = '<span>' + d.search_engine_id + ' : </span>';
  var search_engine_home_url = d.query_uri_prefix + '_filelist?from=0&count=1024&show_from=0&show_count=45';
  left.push( '<a href="' + search_engine_home_url + '" title="Search using image the search engine dataset">Image List</a>' );

  var right = [];
  if ( _VISE_VERSION ) {
    right.push( '<a target="_blank" href="http://www.robots.ox.ac.uk/~vgg/software/vise/">VISE ' + _VISE_VERSION + '</a>');
  } else {
    right.push( '<a target="_blank" href="http://www.robots.ox.ac.uk/~vgg/software/vise/">VISE</a>');
  }
  nav.innerHTML = '<div>' + search_engine_id + left.join('&nbsp;|&nbsp;') + '</div><div>' + right.join('') + '</div>';
}

function _vise_save_data_as_local_file(data, filename) {
  var a      = document.createElement('a');
  a.href     = URL.createObjectURL(data);
  a.target   = '_blank';
  a.download = filename;

  // simulate a mouse click event
  var event = new MouseEvent('click', {
    view: window,
    bubbles: true,
    cancelable: true
  });

  a.dispatchEvent(event);
}

//
// cookie manager
// source: https://www.quirksmode.org/js/cookies.html
//
function _vise_create_cookie(name,value,days) {
	if (days) {
		var date = new Date();
		date.setTime(date.getTime()+(days*24*60*60*1000));
		var expires = "; expires="+date.toGMTString();
	}
	else var expires = "";
	document.cookie = name+"="+value+expires+"; path=/";
}

function _vise_read_cookie(name) {
	var nameEQ = name + "=";
	var ca = document.cookie.split(';');
	for(var i=0;i < ca.length;i++) {
		var c = ca[i];
		while (c.charAt(0)==' ') c = c.substring(1,c.length);
		if (c.indexOf(nameEQ) == 0) return c.substring(nameEQ.length,c.length);
	}
	return null;
}

function _vise_erase_cookie(name) {
	createCookie(name,"",-1);
}

