FROM buildpack-deps:sid-scm

RUN apt-get update && apt-get install -y --no-install-recommends \
	autoconf \
	automake \
	autopoint \
	bzip2 \
	cmake \
	dpkg-dev \
	file \
	g++ \
	gcc \
	gettext \
	libc6-dev \
	libconfuse-dev \
	libdb-dev \
	libevent-dev \
	libevhtp-dev \
	libsqlite3-dev \
	libssl-dev \
	libtool \
	make \
	texinfo 

ENV ROOT_HOME /root
WORKDIR ${ROOT_HOME}
RUN git clone https://github.com/emcrisostomo/fswatch.git
WORKDIR ${ROOT_HOME}/fswatch
RUN ./autogen.sh && ./configure && make -j && make install && make clean
