#
# @author Tobias Weber
# @date Oct-2018
# @license: see 'LICENSE.GPL' file
#

project(pol)
cmake_minimum_required(VERSION 3.0)

find_package(Boost REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets OpenGL)

set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTOMOC TRUE)

set(CMAKE_CXX_STANDARD 17)
add_definitions(-std=c++17 -fconcepts)
add_definitions(${Boost_CXX_FLAGS})
include_directories("${PROJECT_SOURCE_DIR}" "${Boost_INCLUDE_DIRS}/.." "../..")

add_executable(pol pol.cpp
	../glplot/glplot_common.cpp ../glplot/glplot_common.h
	../glplot/glplot_nothread.cpp ../glplot/glplot_nothread.h)
target_link_libraries(pol ${Boost_LIBRARIES})
qt5_use_modules(pol Core Gui Widgets OpenGL)
