# Copyright (c) 2017-2019 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.10)
set(PROJECT_NAME prop_stream)
project(${PROJECT_NAME})

# set CMAKE_INSTALL_PREFIX
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_LIST_DIR}/build CACHE PATH "..." FORCE)
endif()
set(EXECUTABLE_OUTPUT_PATH bin)
set(LIBRARY_OUTPUT_PATH lib)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
# Disable ZERO_CHECK project
set(CMAKE_SUPPRESS_REGENERATION true)

# OpenCV
find_package(OpenCV REQUIRED)

# Boost
set(BOOST_ROOT "")
foreach(env IN ITEMS $ENV{PATH})
  if("${env}" MATCHES ".*boost.*" AND EXISTS "${env}/boost")
    list(APPEND BOOST_ROOT "${env}")
  endif()
endforeach()
set(Boost_USE_STATIC_LIBS ON)
# set(Boost_USE_STATIC_RUNTIME ON)
find_package(Boost REQUIRED COMPONENTS system thread regex date_time) 

# MFX
set(MFX_INCLUDE_DIRS
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/include/api/include
)
if(WIN32)
  set(MFX_LIBRARIES
    ${PROJECT_SOURCE_DIR}/lib/libmfx_vs2015.lib
    ${PROJECT_SOURCE_DIR}/lib/intel_qsv_decoder.lib
  )
else()
  set(MFX_LIBRARIES
    ${PROJECT_SOURCE_DIR}/lib/libintel_qsv_decoder.so
    ${PROJECT_SOURCE_DIR}/lib/libmfx.so.1
  )
endif()

if(WIN32)
  # make executable file
  add_executable(winsock_test ${PROJECT_SOURCE_DIR}/src/winsock_main.cpp)
  # include path
  target_link_libraries(winsock_test ${OpenCV_LIBS} ${FFMPEG_LIBRARIES} ${Boost_LIBRARIES})
  # link library
  target_include_directories(winsock_test PRIVATE ${OpenCV_INCLUDE_DIRS} ${FFMPEG_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
endif()

# make executable file
add_executable(boost_test ${PROJECT_SOURCE_DIR}/src/boost_main.cpp)
# link library
target_link_libraries(boost_test
  ${OpenCV_LIBS}
  ${Boost_LIBRARIES}
  ${MFX_LIBRARIES}
)
# include path
target_include_directories(boost_test PRIVATE
  ${OpenCV_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${MFX_INCLUDE_DIRS}
)

