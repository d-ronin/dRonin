#!/bin/sh

ROOT_DIR=/Users/mlyle/dronin
BUILD_DIR=/Users/mlyle/dronin/build

TARGET=up_cc3d
OUTDIR=${BUILD_DIR}/${TARGET}
BOARD_ROOT_DIR=${ROOT_DIR}/flight/targets/cc3d
BLSRCDIR=${ROOT_DIR}/flight/targets/bl
BLCOMMONDIR=${BLSRCDIR}/common
BLARCHDIR=${BLSRCDIR}/f1upgrader
BLBOARDDIR=${BOARD_ROOT_DIR}/bl

export FW_DESC_BASE=0
export OSCILLATOR_FREQ=8000000
export SYSCLK_FREQ=72000000

export MAKE_INC_DIR=${ROOT_DIR}/make
export FLIGHT_BUILD_CONF=release
export MAKE_INC_DIR=${ROOT_DIR}/make
export PIOS=${ROOT_DIR}/flight/PiOS
export FLIGHTLIB=${ROOT_DIR}/flight/Libraries
export OPMODULEDIR=${ROOT_DIR}/flight/Modules
export OPUAVOBJ=${ROOT_DIR}/flight/UAVObjects
export OPUAVTALK=${ROOT_DIR}/flight/UAVTalk
export DOXYGENDIR=${ROOT_DIR}/Doxygen
export SHAREDAPIDIR=${ROOT_DIR}/shared/api
export OPUAVSYNTHDIR=${BUILD_DIR}/uavobject-synthetics/flight
ARM_SDK_PREFIX=${ROOT_DIR}"/tools/gcc-arm-none-eabi-5_2-2015q4/bin/arm-none-eabi-"

REMOVE_CMD="rm"

mkdir -p ${OUTDIR}/dep
cd ${BLARCHDIR} && \
	make -r --no-print-directory \
	BOARD_NAME=${1} \
	BOARD_SHORT_NAME=${3} \
	BUILD_TYPE=up \
	TCHAIN_PREFIX="${ARM_SDK_PREFIX}" \
	REMOVE_CMD="${RM}" \
	\
	ROOT_DIR=${ROOT_DIR} \
	BOARD_ROOT_DIR=${BOARD_ROOT_DIR} \
	BOARD_INFO_DIR=${BOARD_ROOT_DIR}/board-info \
	TARGET=${TARGET} \
	OUTDIR=${OUTDIR} \
	BLCOMMONDIR=${BLCOMMONDIR} \
	BLARCHDIR=${BLARCHDIR} \
	BLBOARDDIR=${BLBOARDDIR} \
	bin
	


