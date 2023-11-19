#!/bin/bash

set -Eeuo pipefail

# Config vars
LUA_VERSION=5.4.6
LUA_TARGZ_FILENAME=lua-${LUA_VERSION}.tar.gz
LUA_TARGZ_PREFIX_URL=https://www.lua.org/ftp/
LUA_OUTPUT_FOLDER=./external/lua-${LUA_VERSION}
DOWNEXTRACT_PREFIX=./external/

echo "Downloading lua from ${LUA_TARGZ_PREFIX_URL}${LUA_TARGZ_FILENAME}"
curl "${LUA_TARGZ_PREFIX_URL}${LUA_TARGZ_FILENAME}" -o "${DOWNEXTRACT_PREFIX}${LUA_TARGZ_FILENAME}"
echo "Success"

echo "Extracting ${DOWNEXTRACT_PREFIX}${LUA_TARGZ_FILENAME} into folder ${LUA_OUTPUT_FOLDER}"
mkdir -p ${LUA_OUTPUT_FOLDER}
tar -xf ${DOWNEXTRACT_PREFIX}${LUA_TARGZ_FILENAME} -C ${DOWNEXTRACT_PREFIX}
echo "Success"

echo "Change directory to ${LUA_OUTPUT_FOLDER}"
cd ${LUA_OUTPUT_FOLDER}
echo "Success"

echo "Begin build lua version ${LUA_VERSION}"
${MAKE} $1
echo "Success"