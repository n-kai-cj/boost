set(PROJECT_NAME prop_stream)
project(${PROJECT_NAME})
cmake_minimum_required(VERSION 3.14)

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
find_package(Boost 1.70.0 REQUIRED COMPONENTS system thread regex date_time) 

# MFX
set(MFX_INCLUDE_DIRS
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/include/api/include
)
set(MFX_LIBRARIES
  ${PROJECT_SOURCE_DIR}/lib/libmfx_vs2015.lib
  ${PROJECT_SOURCE_DIR}/lib/intel_qsv_decoder.lib
)

# # make executable file
# add_executable(winsock_test ${PROJECT_SOURCE_DIR}/src/winsock_main.cpp)
# # include path
# target_link_libraries(winsock_test ${OpenCV_LIBS} ${FFMPEG_LIBRARIES} ${Boost_LIBRARIES})
# # link library
# target_include_directories(winsock_test PRIVATE ${OpenCV_INCLUDE_DIRS} ${FFMPEG_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})

# make executable file
add_executable(boost_test ${PROJECT_SOURCE_DIR}/src/boost_main.cpp)
# link library
target_link_libraries(boost_test
  ${OpenCV_LIBS}
  # ${FFMPEG_LIBRARIES}
  ${Boost_LIBRARIES}
  ${MFX_LIBRARIES}
)
# include path
target_include_directories(boost_test PRIVATE
  ${OpenCV_INCLUDE_DIRS}
  # ${FFMPEG_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${MFX_INCLUDE_DIRS}
)

