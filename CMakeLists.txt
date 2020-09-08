cmake_minimum_required(VERSION 3.5)

project(qt-mustache LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

set(QT_MUSTACHE_SOURCES
    src/mustache.h
    src/mustache.cpp
    )
add_library(${PROJECT_NAME}
    ${QT_MUSTACHE_SOURCES}
    )
target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)
find_package(Qt5 COMPONENTS Core REQUIRED)

target_link_libraries(${PROJECT_NAME}
    PRIVATE Qt5::Core)
