cmake_minimum_required(VERSION 3.20)
project(node-librados LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB NODE_LIBRADOS_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
add_library(node-librados SHARED ${NODE_LIBRADOS_SOURCES})
set_target_properties(node-librados PROPERTIES PREFIX "" SUFFIX ".node")

execute_process(COMMAND node -p "require('node-addon-api').include"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  OUTPUT_VARIABLE NODE_ADDON_API_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REPLACE "\"" "" NODE_ADDON_API_DIR "${NODE_ADDON_API_DIR}")
find_path(NODE_API_INCLUDE_DIR node_api.h PATH_SUFFIXES node REQUIRED)
target_include_directories(node-librados PRIVATE
  "${NODE_ADDON_API_DIR}"
  "${NODE_API_INCLUDE_DIR}"
  "${CMAKE_SOURCE_DIR}/src")
target_compile_definitions(node-librados PRIVATE NAPI_VERSION=9)
target_compile_options(node-librados PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(node-librados PRIVATE rados rbd dl)
