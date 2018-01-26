#!/bin/bash

usage_short() {
	echo "
usage: compile_dep.sh [-h] [-n <NAPLUGIN>] [-c <CLUSTER>] [-j <COMPILE_CORES>]
                      source_path install_path
	"
}

help_msg() {

	usage_short
    echo "
This script compiles all ADA-FS dependencies (excluding the fs itself)

positional arguments:
	source_path		path to the cloned dependencies path from clone_dep.sh
	install_path		path to the install path of the compiled dependencies


optional arguments:
	-h, --help		shows this help message and exits
	-n <NAPLUGIN>, --na <NAPLUGIN>
				network layer that is used for communication. Valid: {bmi,cci,ofi,all}
				defaults to 'all'
	-c <CLUSTER>, --cluster <CLUSTER>
				additional configurations for specific compute clusters
				supported clusters: {mogon1,fh2}
	-j <COMPILE_CORES>, --compilecores <COMPILE_CORES>
				number of cores that are used to compile the depdencies
				defaults to number of available cores
	"
}

prepare_build_dir() {
    if [ ! -d "$1/build" ]; then
        mkdir $1/build
    fi
    rm -rf $1/build/*
}
PATCH_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PATCH_DIR="${PATCH_DIR}/patches"
CLUSTER=""
NA_LAYER=""
CORES=""
SOURCE=""
INSTALL=""

POSITIONAL=()
while [[ $# -gt 0 ]]
do
key="$1"

case ${key} in
    -n|--na)
    NA_LAYER="$2"
    shift # past argument
    shift # past value
    ;;
	-c|--cluster)
    CLUSTER="$2"
    shift # past argument
    shift # past value
    ;;
	-j|--compilecores)
    CORES="$2"
    shift # past argument
    shift # past value
    ;;
    -h|--help)
    help_msg
	exit
    #shift # past argument
    ;;
    *)    # unknown option
    POSITIONAL+=("$1") # save it in an array for later
    shift # past argument
    ;;
esac
done
set -- "${POSITIONAL[@]}" # restore positional parameters

# deal with positional arguments
if [[ ( -z ${1+x} ) || ( -z ${2+x} ) ]]; then
    echo "Positional arguments missing."
    usage_short
    exit
fi
SOURCE=$1
INSTALL=$2

# deal with optional arguments
if [ "${NA_LAYER}" == "" ]; then
	echo "Defaulting NAPLUGIN to 'all'"
	NA_LAYER="all"
fi
if [ "${CORES}" == "" ]; then
	CORES=$(grep -c ^processor /proc/cpuinfo)
	echo "CORES = ${CORES} (default)"
else
	if [ ! "${CORES}" -gt "0" ]; then
		echo "CORES set to ${CORES} which is invalid.
Input must be numeric and greater than 0."
		usage_short
		exit
	else
		echo CORES    = "${CORES}"
	fi	 
fi
if [ "${NA_LAYER}" == "cci" ] || [ "${NA_LAYER}" == "bmi" ] || [ "${NA_LAYER}" == "ofi" ] || [ "${NA_LAYER}" == "all" ]; then
	echo NAPLUGIN = "${NA_LAYER}"
else
    echo "No valid plugin selected"
    usage_short
    exit
fi
if [[ "${CLUSTER}" != "" ]]; then
	if [[ ( "${CLUSTER}" == "mogon1" ) || ( "${CLUSTER}" == "fh2" ) ]]; then
		echo CLUSTER  = "${CLUSTER}"
    else
        echo "${CLUSTER} cluster configuration is invalid. Exiting ..."
        usage_short
        exit
    fi
else
    echo "No cluster configuration set."
fi

#LOG=/tmp/adafs_install.log
#echo "" &> $LOG
USE_BMI="-DNA_USE_BMI:BOOL=OFF"
USE_CCI="-DNA_USE_CCI:BOOL=OFF"
USE_OFI="-DNA_USE_OFI:BOOL=OFF"

echo "Source path = '$1'";
echo "Install path = '$2'";

mkdir -p ${SOURCE}

# Set cluster dependencies first
if [[ ( "${CLUSTER}" == "mogon1" ) || ( "${CLUSTER}" == "fh2" ) ]]; then
    # get libtool
    echo "############################################################ Installing:  libtool"
    CURR=${SOURCE}/libtool
    prepare_build_dir ${CURR}
    cd ${CURR}/build
    ../configure --prefix=${INSTALL} || exit 1
    make -j${CORES} || exit 1
    make install || exit 1
    # compile libev
    echo "############################################################ Installing:  libev"
    CURR=${SOURCE}/libev
    prepare_build_dir ${CURR}
    cd ${CURR}/build
    ../configure --prefix=${INSTALL} || exit 1
    make -j${CORES} || exit 1
    make install || exit 1
    # compile gflags
    echo "############################################################ Installing:  gflags"
    CURR=${SOURCE}/gflags
    prepare_build_dir ${CURR}
    cd ${CURR}/build
    cmake -DCMAKE_INSTALL_PREFIX=${INSTALL} -DCMAKE_BUILD_TYPE:STRING=Release .. || exit 1
    make -j${CORES} || exit 1
    make install || exit 1
    # compile zstd
    echo "############################################################ Installing:  zstd"
    CURR=${SOURCE}/zstd/build/cmake
    prepare_build_dir ${CURR}
    cd ${CURR}/build
    cmake -DCMAKE_INSTALL_PREFIX=${INSTALL} -DCMAKE_BUILD_TYPE:STRING=Release .. || exit 1
    make -j${CORES} || exit 1
    make install || exit 1
    echo "############################################################ Installing:  lz4"
    CURR=${SOURCE}/lz4
	cd ${CURR}
    make -j${CORES} || exit 1
    make DESTDIR=${INSTALL} PREFIX="" install || exit 1
    echo "############################################################ Installing:  snappy"
    CURR=${SOURCE}/snappy
    prepare_build_dir ${CURR}
    cd ${CURR}/build
    cmake -DCMAKE_INSTALL_PREFIX=${INSTALL} -DCMAKE_BUILD_TYPE:STRING=Release .. || exit 1
    make -j${CORES} || exit 1
    make install || exit 1
fi

if [ "$NA_LAYER" == "bmi" ] || [ "$NA_LAYER" == "all" ]; then
    USE_BMI="-DNA_USE_BMI:BOOL=ON"
    echo "############################################################ Installing:  BMI"
    # BMI
    CURR=${SOURCE}/bmi
    prepare_build_dir ${CURR}
    cd ${CURR}
    ./prepare || exit 1
    cd ${CURR}/build
    ../configure --prefix=${INSTALL} --enable-shared --enable-bmi-only  || exit 1
    make -j${CORES} || exit 1
    make install || exit 1
fi

if [ "$NA_LAYER" == "cci" ] || [ "$NA_LAYER" == "all" ]; then
    USE_CCI="-DNA_USE_CCI:BOOL=ON"
    echo "############################################################ Installing:  CCI"
    # CCI
    CURR=${SOURCE}/cci
    prepare_build_dir ${CURR}
    cd ${CURR}
    # patch hanging issue
    echo "########## Applying cci hanging patch"
    git apply ${PATCH_DIR}/cci_hang_final.patch || exit 1
    ./autogen.pl || exit 1
    cd ${CURR}/build
if [[ ("${CLUSTER}" == "mogon1") || ("${CLUSTER}" == "fh2") ]]; then
    ../configure --with-verbs --prefix=${INSTALL} LIBS="-lpthread"  || exit 1
else
    ../configure --prefix=${INSTALL} LIBS="-lpthread"  || exit 1
fi
    make -j${CORES} || exit 1
    make install || exit 1
    make check || exit 1
fi

if [ "$NA_LAYER" == "ofi" ] || [ "$NA_LAYER" == "all" ]; then
    USE_OFI="-DNA_USE_OFI:BOOL=ON"
    echo "############################################################ Installing:  LibFabric"
    #libfabric
    CURR=${SOURCE}/libfabric
    prepare_build_dir ${CURR}
    cd ${CURR}
    ./autogen.sh || exit 1
    cd ${CURR}/build
    ../configure --prefix=${INSTALL}  || exit 1
    make -j${CORES} || exit 1
    make install || exit 1
    make check || exit 1
fi

echo "############################################################ Installing:  Mercury"

# Mercury
CURR=${SOURCE}/mercury
prepare_build_dir ${CURR}
cd ${CURR}
# patch cci verbs addr lookup error handling
echo "########## Applying cci addr lookup error handling patch"
git apply ${PATCH_DIR}/mercury_cci_verbs_lookup.patch || exit 1
cd ${CURR}/build
# XXX Note: USE_EAGER_BULK is temporarily disabled due to bugs in Mercury with smaller amounts of data
# Apparantly this is fixed in the new Mercury version. TODO check if it works now
cmake -DMERCURY_USE_SELF_FORWARD:BOOL=ON -DMERCURY_USE_CHECKSUMS:BOOL=OFF -DBUILD_TESTING:BOOL=ON \
-DMERCURY_USE_BOOST_PP:BOOL=ON -DBUILD_SHARED_LIBS:BOOL=ON -DCMAKE_INSTALL_PREFIX=${INSTALL} \
-DCMAKE_BUILD_TYPE:STRING=Release -DMERCURY_USE_EAGER_BULK:BOOL=ON ${USE_BMI} ${USE_CCI} ${USE_OFI} ../  || exit 1
make -j${CORES}  || exit 1
make install  || exit 1

echo "############################################################ Installing:  Argobots"

# Argobots
CURR=${SOURCE}/argobots
prepare_build_dir ${CURR}
cd ${CURR}
./autogen.sh || exit 1
cd ${CURR}/build
../configure --prefix=${INSTALL} || exit 1
make -j${CORES} || exit 1
make install || exit 1
make check || exit 1

echo "############################################################ Installing:  Abt-snoozer"
# Abt snoozer
CURR=${SOURCE}/abt-snoozer
prepare_build_dir ${CURR}
cd ${CURR}
./prepare.sh || exit 1
cd ${CURR}/build
../configure --prefix=${INSTALL} PKG_CONFIG_PATH=${INSTALL}/lib/pkgconfig || exit 1
make -j${CORES} || exit 1
make install || exit 1
make check || exit 1

echo "############################################################ Installing:  Margo"
# Margo
CURR=${SOURCE}/margo
prepare_build_dir ${CURR}
cd ${CURR}
./prepare.sh || exit 1
cd ${CURR}/build
../configure --prefix=${INSTALL} PKG_CONFIG_PATH=${INSTALL}/lib/pkgconfig CFLAGS="-g -Wall" || exit 1
make -j${CORES} || exit 1
make install || exit 1
make check || exit 1

echo "############################################################ Installing:  Rocksdb"
# Rocksdb
CURR=${SOURCE}/rocksdb
cd ${CURR}
make clean || exit 1
sed -i.bak "s#INSTALL_PATH ?= /usr/local#INSTALL_PATH ?= ${INSTALL}#g" Makefile
make -j${CORES} static_lib || exit 1
make install || exit 1

echo "Done"