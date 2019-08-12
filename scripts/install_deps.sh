#!/bin/sh

if [ "$#" -ne 1 ] || ! [ -d "$1" ]; then
  echo "Usage: $0 DEPENDENCY_FILES_LOCATION/" >&2
  exit 1
fi

# $1
# |- lib  
# |- tmp_libsrc
DEP_BASEDIR=$1
DEP_LIBDIR="${DEP_BASEDIR}lib/"
DEP_SRCDIR="${DEP_BASEDIR}tmp_libsrc"

#
# Install cmake
#
CMAKE_LIBDIR=$DEP_LIBDIR"cmake/"
if [ -d "$CMAKE_LIBDIR" ]; then
  echo "Skipping cmake library as it already exists at "$CMAKE_LIBDIR
else
  echo "Installing cmake at "$CMAKE_LIBDIR
	wget https://cmake.org/files/v3.11/cmake-3.11.4.tar.gz
	tar -zxvf cmake-3.11.4.tar.gz && cd cmake-3.11.4
	./configure --prefix=/home/tlm/deps/vise/lib
fi


#
# Install ImageMagick
#
IMAGEMAGICK_LIBDIR=$DEP_LIBDIR"imagemagick_q8/"
if [ -d "$IMAGEMAGICK_LIBDIR" ]; then
    echo "Skipping ImageMagick library as it already exists at "$IMAGEMAGICK_LIBDIR
else
    echo "Installing imagemagick at "$IMAGEMAGICK_LIBDIR
    cd $DEP_SRCDIR
    #wget https://www.imagemagick.org/download/releases/ImageMagick-6.9.8-4.tar.gz
    #wget -O ImageMagick-6.9.8-4.tar.gz http://git.imagemagick.org/repos/ImageMagick/repository/archive.tar.gz?ref=6.9.8-4
    #wget -O ImageMagick-7.0.5-6.tar.gz http://git.imagemagick.org/repos/ImageMagick/repository/archive.tar.gz?ref=7.0.5-6
    #wget -O ImageMagick-6.9.8-8.tar.gz http://git.imagemagick.org/repos/ImageMagick/repository/archive.tar.gz?ref=6.9.8-8
    #tar -zxvf ImageMagick-6.9.8-8.tar.gz
    #wget https://www.imagemagick.org/download/releases/ImageMagick-6.9.10-1.zip
		wget https://www.imagemagick.org/download/releases/ImageMagick-6.9.10-3.zip
    cd ImageMagick-6.9.8-8
    ./configure --prefix=$IMAGEMAGICK_LIBDIR -enable-hdri=no --with-quantum-depth=8 --disable-dependency-tracking --with-x=no --without-perl
    make -j8
    make install
fi

#
# Install vlfeat
#
#VLFEAT_LIBDIR=$DEP_LIBDIR"vlfeat/"
#if [ -d "$VLFEAT_LIBDIR" ]; then
#    echo "Skipping VLFEAT library as it already exists at "$VLFEAT_LIBDIR
#else
#    echo "Installing VLFEAT at "$VLFEAT_LIBDIR
#    cd $DEP_SRCDIR
#    wget http://www.vlfeat.org/download/vlfeat-0.9.21-bin.tar.gz
#    tar -zxvf vlfeat-0.9.21-bin.tar.gz 
#    cd vlfeat-0.9.21/
#    make -j8
#    make install
#fi

#
# Install eigen
#
EIGEN_LIBDIR=$DEP_LIBDIR"eigen/"
if [ -d "$EIGEN_LIBDIR" ]; then
    echo "Skipping EIGEN library as it already exists at "$EIGEN_LIBDIR
else
    echo "Installing EIGEN at "$EIGEN_LIBDIR
    cd $DEP_SRCDIR
    wget -O eigen-3.3.4.tar.gz http://bitbucket.org/eigen/eigen/get/3.3.4.tar.gz
    tar -zxvf eigen-3.3.4.tar.gz
    cd eigen-3.3.4/
    mkdir build
    cd build
    /ssd/adutta/build_deps/lib/cmake/bin/cmake -DCMAKE_INSTALL_PREFIX=/ssd/adutta/build_deps/lib ../
    make install
fi

#
# Install Boost
#
BOOST_LIBDIR=$LIBDIR"boost/"
if [ -d "$BOOST_LIBDIR" ]; then
    echo "Skipping BOOST library as it already exists at "$BOOST_LIBDIR
else
    cd $TMP_BASEDIR
    wget https://netcologne.dl.sourceforge.net/project/boost/boost/1.64.0/boost_1_64_0.tar.gz
    tar -zxvf boost_1_64_0.tar.gz
    cd boost_1_64_0
    ./bootstrap.sh --prefix=$BOOST_LIBDIR --with-toolset=gcc --with-libraries=filesystem,system,thread,date_time,chrono,atomic,timer,mpi,log
    #sed -i.old 's/using gcc ;/using gcc : 6.3.0 : g++-6 ;/g' project-config.jam
		echo "using mpi ;" >> project-config.jam
    ./b2 --with-filesystem --with-system --with-thread --with-date_time --with-chrono --with-atomic --with-timer --with-log variant=release threading=multi toolset=gcc install

    #@todo
    # compiling boost
    #./bootstrap.sh --prefix=$BOOST_LIBDIR --with-toolset=gcc --with-libraries=filesystem,system,thread,date_time,chrono,atomic,timer,mpi
    #add to project-config.jam: using mpi ;
    #./b2 --with-filesystem --with-system --with-thread --with-date_time --with-chrono --with-atomic --with-timer --with-mpi variant=release threading=multi toolset=gcc install
fi


