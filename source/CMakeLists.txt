cmake_minimum_required(VERSION 3.16)
project(Jde.XZ)
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../Public/build )
include(common)
#######################################################
add_library( ${CMAKE_PROJECT_NAME} SHARED XZ.cpp JdeZip.cpp )

target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized fmt::libfmt.a )
#######################################################
target_precompile_headers( ${CMAKE_PROJECT_NAME}
  PRIVATE
	<boost/crc.hpp>
	<boost/system/error_code.hpp>
	<jde/TypeDefs.h>
	<jde/Exception.h>
)
