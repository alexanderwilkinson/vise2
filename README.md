# VGG Image Search Engine (VISE)

<img src="res/logo/vise_logo.png" alt="VISE logo" title="VISE logo">

[VGG Image Search Engine](http://www.robots.ox.ac.uk/~vgg/software/vise) (VISE) 
is a standalone tool to search a large collection of images using an image region 
as query.

VISE builds on the C++ codebase (relja_retrieval, Sep. 2014) developed by 
Relja Arandjelovic during his DPhil / Postdoc at the Visual Geometry Group, 
Department of Engineering Science, University of Oxford. The VISE project
is developed and maintained by [Abhishek Dutta](adutta@robots.ox.ac.uk).

This work is supported by EPSRC programme grant Seebibyte: Visual Search for 
the Era of Big Data ([EP/M013774/1](http://www.seebibyte.org/index.html)).


## Software Design
From a common source code base, this software is built to address the needs of 
the following two groups of users:
 * Desktop user: All the user interactions (indexing, administration, query, etc) 
occur through a web browser based user interface. The main properties of VISE 
being used in this mode are:
   - no technical skill needed to install and run the software
   - single click installer for Linux, Mac and Windows platforms
   - intuitive user interface for all interactions with VISE: indexing, administration, 
query, etc.

 * Cluster user: This groups of users will be more advanced users with the ability 
to run shell scripts, install libraries and compile C++ source code. Furthermore, 
the VISE software will run in a server environment (with large number of nodes and 
memory) to serve a large number of user queries using REST API. This mode is only 
supported for the Unix platform. All operations (e.g. indexing, administration, etc) 
except search engine query will be performed using unix shell scripts. A web 
browser based user interface (or REST API) will be available for search engine 
query operations.

Abhishek Dutta  
18 June 2018

