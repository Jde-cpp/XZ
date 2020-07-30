cmake_minimum_required(VERSION 3.16)
project(Jde.XZ)
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../Framework/cmake )
include(SetUpConfigurations)
#######################################################
add_library( Jde.XZ SHARED XZ.cpp )
#######################################################
target_precompile_headers(Jde.XZ
  PRIVATE
	<boost/crc.hpp>
	<boost/system/error_code.hpp>
	<spdlog/spdlog.h>
	<spdlog/sinks/basic_file_sink.h>
	<spdlog/fmt/ostr.h>
	"../../Framework/source/Exception.h"
)
