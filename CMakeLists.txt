#
# @author Tobias Weber
# @date Apr-2018
# @license see 'LICENSE.GPL' file
#

project(sgbrowser)
cmake_minimum_required(VERSION 3.0)

find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_STANDARD 17)
add_definitions(-std=c++17 -fconcepts)
include_directories("${PROJECT_SOURCE_DIR}")

add_executable(sgbrowser
	tools/browser/main.cpp
	tools/browser/browser.cpp tools/browser/browser.h
)

target_link_libraries(sgbrowser
	Qt5::Core Qt5::Gui Qt5::Widgets
)
