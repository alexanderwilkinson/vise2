################################################################################
############################ DOES NOT WORK NOW #################################
################################################################################
#
# Dockerfile for creating a docker image that runs the VGG Image Search Engine 
# (VISE) version 2.x.y
#
# Author: Abhishek Dutta <adutta@robots.ox.ac.uk>
# Date: 10 August 2018
#

#
# Install Debian as base image (tightly controlled and minimal, 150 MB)
# See: https://hub.docker.com/_/debian/
#
FROM debian:jessie

#
# Define image labels
#
LABEL name="VGG Image Search Engine" codename="VISE" version="2.0.0-beta1" maintainer="Abhishek Dutta <adutta@robots.ox.ac.uk>" description="VGG Image Search Engine (VISE) enables instance based search of a large collection of images."

VOLUME /data/vgg/software/vise

#
# Define environment variables
#
ENV VISE_CODE_DIR=/opt/vgg/software/vise/code DEP_PREFIX=/opt/vgg/software/vise/deps/lib DEP_SRC=/opt/vgg/software/vise/deps/tmp_libsrc

#
# Install VISE dependencies
# 
RUN apt-get update && apt-get install -y wget git joe g++ libpng12-dev libjpeg-dev libcairo2 libcairo2-dev make openmpi-bin libopenmpi-dev libopenblas-dev python-pip python-dev libprotobuf-dev protobuf-compiler

#
# Compile dependencies (fastann, pypar, etc) and then compile VISE
#
RUN /bin/bash -c 'mkdir $VISE_CODE_DIR -p && cd $VISE_CODE_DIR ; \
# Download VISE source (defer compilation until dependencies are installed)
 git clone -b vise-2.x.y --single-branch https://gitlab.com/vgg/vise.git ; \
 mkdir vise/cmake_build -p && cd vise/cmake_build ; \
# compile and install all dependencies
 mkdir $DEP_PREFIX -p && mkdir $DEP_SRC -p ; \
# cmake
cd $DEP_SRC && wget https://cmake.org/files/v3.11/cmake-3.11.4.tar.gz &&	tar -zxvf cmake-3.11.4.tar.gz && cd cmake-3.11.4 && ./configure --prefix=$DEP_PREFIX && make -j 18 && make install ; \
# imagemagick
cd $DEP_SRC && wget https://www.imagemagick.org/download/releases/ImageMagick-6.9.10-9.tar.gz && tar -zxvf ImageMagick-6.9.10-9.tar.gz && cd ImageMagick-6.9.10-9 && ./configure --prefix=$DEP_PREFIX -enable-hdri=no --with-quantum-depth=8 --disable-dependency-tracking --with-x=no --without-perl && make -j 16 && make install ; \
# boost
cd $DEP_SRC && wget https://kent.dl.sourceforge.net/project/boost/boost/1.66.0/boost_1_66_0.tar.gz && tar -zxvf boost_1_66_0.tar.gz && cd boost_1_66_0 && ./bootstrap.sh --prefix=$DEP_PREFIX --with-toolset=gcc --with-libraries=filesystem,system,thread,date_time,chrono,atomic,timer,mpi,log,serialization && echo "using mpi ;" >> project-config.jam && ./b2 --with-filesystem --with-system --with-thread --with-date_time --with-chrono --with-atomic --with-timer --with-mpi --with-log variant=release threading=multi toolset=gcc install ; \
## to fix a bug with boost
sed -i "s/\ result.count/\ (result.count)/g" $DEP_PREFIX"/include/boost/asio/detail/consuming_buffers.hpp" ; \
# eigen
cd $DEP_SRC && wget -O eigen-3.3.5.tar.gz http://bitbucket.org/eigen/eigen/get/3.3.5.tar.gz && tar -zxvf eigen-3.3.5.tar.gz && mv eigen-eigen* eigen-3.3.5 && cd eigen-3.3.5/ && mkdir cmake_build && cd cmake_build && $DEP_PREFIX"/bin/cmake" -DCMAKE_INSTALL_PREFIX=$DEP_PREFIX ../ && make -j8 && make install ; \
# vlfeat
cd $DEP_SRC && wget http://www.vlfeat.org/download/vlfeat-0.9.21-bin.tar.gz && tar -zxvf vlfeat-0.9.21-bin.tar.gz && cd vlfeat-0.9.21 && make -j 8 ; \
# fastann
## may need: yasm -f elf64 ../dist_l2_funcs_exp_64.asm -o dist_l2_funcs_exp_64.o
cd $VISE_CODE_DIR"/vise/src/search_engine/relja_retrival/external/fastann/" && mkdir cmake_build && cd cmake_build && PREFIX=$DEP_PREFIX $DEP_PREFIX"/bin/cmake" $VISE_CODE_DIR"/vise/src/search_engine/relja_retrival/external/fastann/src/" && make && make install ; \
# pypar 
cd $VISE_CODE_DIR"/vise/src/search_engine/relja_retrival/external/pypar_2.1.4_94/source" && pip install numpy && python setup.py build && python compile_pypar_locally.py && python setup.py install && ln -s /usr/lib/libmpi.so /usr/lib/libmpi.so.0 ; \
## mpirun -np 3 python -c "import pypar; pypar.finalize();" # check
# dkmeans
cd $VISE_CODE_DIR"/vise/src/search_engine/relja_retrival/external/dkmeans_relja" && python setup.py build && python setup.py install ; \
#
# Compile VISE
#
cd $VISE_CODE_DIR"/vise/cmake_build" && $DEP_PREFIX"/bin/cmake" -DCMAKE_PREFIX_PATH=$DEP_PREFIX -DVLFEAT_LIB=$DEP_SRC"/vlfeat-0.9.21/bin/glnxa64/libvl.so" -DVLFEAT_INCLUDE_DIR=$DEP_SRC"/vlfeat-0.9.21/" ../ && make -j 8 ; \
echo "Done"'


