#!/bin/bash

#
# Build the Subsurface Windows installer and the Smtk2ssrf Windows installer in a Docker container
#

croak() {
	echo "$0: $*" >&2
	exit 1
}

CONTAINER_NAME=subsurface-windows-builder

pushd .
cd "$(dirname "$0")/../.."
SUBSURFACE_ROOT="${PWD}"
popd

OUTPUT_DIR=output/windows
CONTAINER_ROOT_DIR=/win
CONTAINER_SUBSURFACE_DIR=${CONTAINER_ROOT_DIR}/subsurface

LOGIN_USER=$(id -u)
LOGIN_GROUP=$(id -g)
FULL_USER=${LOGIN_USER}:${LOGIN_GROUP}

# Check if our container exists
CONTAINER_ID=$(docker container ls -a -q -f name=${CONTAINER_NAME})

# Create the container if it does not exist
if [[ -z "${CONTAINER_ID}" ]]; then
	# We'll need these later
	if [ -z ${GIT_AUTHOR_NAME+X} -o -z ${GIT_AUTHOR_EMAIL+X} ]; then
		croak "Please make sure GIT_AUTHOR_NAME and GIT_AUTHOR_EMAIL are set for the first run of this script."
	fi

	docker create -v ${SUBSURFACE_ROOT}:${CONTAINER_SUBSURFACE_DIR} --name=${CONTAINER_NAME} subsurface/mxe-build:3.4.0 sleep infinity
fi

# Start the container
docker start ${CONTAINER_NAME}

if [[ -z "${CONTAINER_ID}" ]]; then
	# Prepare the image for first use
	docker exec -t ${CONTAINER_NAME} groupadd $(id -g -n) -o -g ${LOGIN_GROUP}
	docker exec -t ${CONTAINER_NAME} useradd $(id -u -n) -o -u ${LOGIN_USER} -g ${LOGIN_GROUP} -d ${CONTAINER_ROOT_DIR}
	docker exec -t ${CONTAINER_NAME} find ${CONTAINER_ROOT_DIR} -type d -exec chown ${FULL_USER} {} \;

	docker exec -u ${FULL_USER} -t ${CONTAINER_NAME} git config --global user.name "${GIT_AUTHOR_NAME}"
	docker exec -u ${FULL_USER} -t ${CONTAINER_NAME} git config --global user.email "${GIT_AUTHOR_EMAIL}"

	docker exec -u ${FULL_USER} -t ${CONTAINER_NAME} bash -x subsurface/packaging/windows/container-prep.sh
fi

# Build
mkdir -p "${SUBSURFACE_ROOT}/${OUTPUT_DIR}"
docker exec -u ${FULL_USER} -e OUTPUT_DIR=${CONTAINER_SUBSURFACE_DIR}/${OUTPUT_DIR} -t ${CONTAINER_NAME} bash -x subsurface/packaging/windows/in-container-build.sh

# Stop the container
docker stop ${CONTAINER_NAME}
