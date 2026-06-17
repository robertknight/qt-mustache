cmake_minimum_required(VERSION 3.19)

project(qt-mustache LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core)

include(GNUInstallDirs)

set(QT_MUSTACHE_SOURCES
    src/mustache.h
    src/mustache.cpp
)

add_library(${PROJECT_NAME} ${QT_MUSTACHE_SOURCES})
add_library(mustache::mustache ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core)

set(STRICT_COMPILE_DEFINITIONS
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
    QT_NO_CAST_FROM_BYTEARRAY
    QT_NO_URL_CAST_FROM_STRING
    QT_NO_CONTEXTLESS_CONNECT
    QT_NO_NARROWING_CONVERSIONS_IN_CONNECT
    QT_NO_QSNPRINTF
    QT_STRICT_ITERATORS
)

target_compile_definitions(${PROJECT_NAME} PRIVATE ${STRICT_COMPILE_DEFINITIONS})
set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_WARNING_AS_ERROR ON)

# Install rules
install(TARGETS ${PROJECT_NAME}
    EXPORT mustache-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES src/mustache.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT mustache-targets
    NAMESPACE mustache::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mustache
)

# Tests — off by default when consumed as a dependency
string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}" _mustache_is_top_level)
option(MUSTACHE_BUILD_TESTS "Build qt-mustache tests and benchmarks" ${_mustache_is_top_level})

if(MUSTACHE_BUILD_TESTS)
    set(TEST_PROJECT_NAME "${PROJECT_NAME}_tests")
    enable_testing()

    find_package(Qt6 REQUIRED COMPONENTS Test)

    add_executable(${TEST_PROJECT_NAME}
        tests/test_mustache.cpp
        tests/test_mustache.h
    )
    add_test(${TEST_PROJECT_NAME} ${TEST_PROJECT_NAME})
    target_link_libraries(${TEST_PROJECT_NAME} PRIVATE Qt6::Test mustache::mustache)
    target_compile_definitions(${TEST_PROJECT_NAME} PRIVATE ${STRICT_COMPILE_DEFINITIONS})
    set_target_properties(${TEST_PROJECT_NAME} PROPERTIES COMPILE_WARNING_AS_ERROR ON)

    file(GLOB TEST_CONTENTS
        "tests/specs/*.json"
        "tests/partial.mustache"
    )
    file(COPY ${TEST_CONTENTS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

    # Benchmarks
    set(BENCH_PROJECT_NAME "${PROJECT_NAME}_bench")
    add_executable(${BENCH_PROJECT_NAME}
        tests/bench_mustache.cpp
    )
    target_link_libraries(${BENCH_PROJECT_NAME} PRIVATE Qt6::Test mustache::mustache)
    target_compile_definitions(${BENCH_PROJECT_NAME} PRIVATE ${STRICT_COMPILE_DEFINITIONS})
    set_target_properties(${BENCH_PROJECT_NAME} PROPERTIES COMPILE_WARNING_AS_ERROR ON)
endif()
