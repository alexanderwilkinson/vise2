#
# Dockerfile for creating a docker image that runs the VGG Image Search Engine (VISE).
#
# Author: Kaloyan Petrov <kaloyan_petrov@mail.bg>
# Date: 30 April 2019
#
FROM ubuntu:bionic

#
# Define image labels
#
LABEL name="VGG Image Search Engine" codename="VISE" version="2.x.y" maintainer="Kaloyan Petrov <kaloyan_petrov@mail.bg>" description="VGG Image Search Engine (VISE) enables image instance based search of a large collection of images."

#
# Define ports used by VISE
#   VISE trainer/loader : 9971
#   VISE loaded engine frontend : 9973
#
EXPOSE 9971 9973

VOLUME /opt/vgg/vise/application_data/ /opt/vgg/mydata/images/

#
# Define environment variables
#
ENV VGG_ROOT_DIR="/opt/vgg/"
ENV VISE_ROOT_DIR="/opt/vgg/vise/"
ENV VISE_CODE_DIR="/opt/vgg/vise/vise-2.x.y/"
ENV VISE_DEP_DIR="/opt/vgg/vise/build_deps/"
ENV VISE_DEP_LIBDIR="/opt/vgg/vise/build_deps/lib/"
ENV VISE_DEP_SRCDIR="/opt/vgg/vise/build_deps/tmp_libsrc/"
ENV VISE_DATA_DIR='/opt/vgg/software/vise/data'

#
# Prepare environment variables
#
RUN mkdir -p $VGG_ROOT_DIR $VISE_ROOT_DIR $VISE_DEP_DIR $VISE_DEP_SRCDIR $VISE_DEP_LIBDIR

#
# Install VISE dependencies
#
RUN apt-get update && apt-get install -y software-properties-common && add-apt-repository universe
RUN apt-get install -y curl libpng-dev libjpeg-dev ssh openmpi-bin libopenmpi-dev python python-dev python-pip unzip libmagick++-dev libprotobuf-dev protobuf-compiler libboost-atomic-dev libboost-chrono-dev libboost-filesystem-dev libboost-system-dev libboost-thread-dev libboost-timer-dev libcairo2 libcairo2-dev git libeigen3-dev libboost-mpi-dev libboost-log-dev
RUN pip install cherrypy numpy pillow cython


#install latest cmake
ADD https://cmake.org/files/v3.14/cmake-3.14.0-Linux-x86_64.sh /tmp/cmake-3.14.0-Linux-x86_64.sh
RUN /bin/bash -c 'mkdir /opt/cmake ; \
sh /tmp/cmake-3.14.0-Linux-x86_64.sh --prefix=/opt/cmake --skip-license ; \
ln -s /opt/cmake/bin/cmake /usr/local/bin/cmake ; \
cmake --version'

# Download VISE source
RUN /bin/bash -c 'cd ${VISE_ROOT_DIR} ; \
wget -P/tmp https://github.com/alexanderwilkinson/vise2/archive/master.zip ; \
unzip /tmp/master.zip ; \
rm -f /tmp/master.zip ; \
mv vise2-master/ vise-2.x.y'

#We need! vlfeat 0.9.21
ADD http://www.vlfeat.org/download/vlfeat-0.9.21-bin.tar.gz /tmp/vlfeat-0.9.21-bin.tar.gz
RUN /bin/bash -c 'cd ${VISE_DEP_DIR} ; \
tar -zxvf /tmp/vlfeat-0.9.21-bin.tar.gz ; \
rm -f /tmp/vlfeat-0.9.21-bin.tar.gz ; \
pushd vlfeat-0.9.21 ; \
  make ; \
popd'

# Compile dependencies (fastann, pypar, etc) and then compile VISE
RUN /bin/bash -c 'export FASTANN_LIBDIR=$VISE_DEP_LIBDIR"fastann/" ; \
cd "${VISE_CODE_DIR}/src/search_engine/relja_retrival/external/fastann/"  ; \
#
mkdir -p cmake_build ; \
pushd cmake_build ; \
 cmake ${VISE_CODE_DIR}"/src/search_engine/relja_retrival/external/fastann/src/" ; \
 make ; \
 make install ; \
popd'

# Compile pypar and dkmeans (needed for clustering)
RUN /bin/bash -c 'cd ${VISE_CODE_DIR}"/src/search_engine/relja_retrival/external/pypar_2.1.4_94/source" ; \
sed -i.save "s/libmpi.so.0/libmpi.so/" pypar.py ; \
python setup.py build ; \
python compile_pypar_locally.py ; \
python setup.py install ; \
#
cd ${VISE_CODE_DIR}"/src/search_engine/relja_retrival/external/dkmeans_relja" ; \
python setup.py build ; \
python setup.py install'

# Compile VISE
RUN /bin/bash -c 'mkdir -p "${VISE_CODE_DIR}/build/" ; \
pushd "${VISE_CODE_DIR}/build/" ; \
cmake -DVLFEAT_LIB=${VISE_DEP_DIR}"/vlfeat-0.9.21/bin/glnxa64/libvl.so" -DVLFEAT_INCLUDE_DIR=${VISE_DEP_DIR}"/vlfeat-0.9.21/" .. ; \
make ; \
popd ; \
#Some cleanup
rm -rf "${VISE_CODE_DIR}/.git/" ; \
rm -f /tmp/cmake-3.14.0-Linux-x86_64.sh'

ADD start_vise_server.sh /tmp/start_vise_server.sh
RUN /bin/bash -c 'useradd -m ndhum003c ; \
chown -R ndhum003c:ndhum003c ${VISE_CODE_DIR} ; \
cp /tmp/start_vise_server.sh /home/ndhum003c/ ; \
rm -f /tmp/start_vise_server.sh'

#TODO: restore original entry point ?
#ENTRYPOINT $VISE_CODE_DIR"build/vice" $VISE_CODE_DIR /opt/vgg/vise/application_data/ /opt/vgg/mydata/images/
ENTRYPOINT $VISE_CODE_DIR"build/" $VISE_CODE_DIR /opt/vgg/vise/application_data/ /opt/vgg/mydata/images/
