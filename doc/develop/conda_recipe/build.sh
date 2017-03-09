#!/bin/bash

if [ ! -d build ]; then
    mkdir build
fi
cd build

PY_LIB="libpython${PY_VER}m${SHLIB_EXT}"
MACOSX_DEPLOYMENT_TARGET=10.8

cmake \
    -G "${CMAKE_GENERATOR}" \
    -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
    -DOVITO_BUILD_GUI=OFF \
    -DOVITO_BUILD_DOCUMENTATION=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DPYTHON_EXECUTABLE=${PYTHON} \
    -DPYTHON_INCLUDE_DIR=${PREFIX}/include/python${PY_VER}m \
    -DPYTHON_LIBRARY=${PREFIX}/lib/${PY_LIB} \
    ${SRC_DIR}

make -j${CPU_COUNT}
make install

if [ `uname` == Darwin ]; then
    echo "${PREFIX}/Ovito.app/Contents/Resources/python" > ${SP_DIR}/${PKG_NAME}.pth
else
    echo "${PREFIX}/lib/ovito/plugins/python" > ${SP_DIR}/${PKG_NAME}.pth
fi
echo "SP_DIR=${SP_DIR}"
