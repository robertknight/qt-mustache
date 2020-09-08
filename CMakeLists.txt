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

#tests related stuff
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    SET(TEST_PROPJECT_NAME "${PROJECT_NAME}_tests")
    enable_testing()
    find_package(Qt5Test REQUIRED)
    add_executable(${TEST_PROPJECT_NAME}
        tests/test_mustache.cpp
        tests/test_mustache.h
    )

    add_test(${TEST_PROPJECT_NAME} ${TEST_PROPJECT_NAME})
    target_link_libraries(${TEST_PROPJECT_NAME} Qt5::Test ${PROJECT_NAME})
    file(GLOB TEST_CONTENTS
        "tests/specs/*.json"
        "tests/partial.mustache"
    )
    file(COPY ${TEST_CONTENTS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()
