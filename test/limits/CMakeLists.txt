#
# Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
# Copyright (c) 2025 Mohammad Nejati
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#
# Official repository: https://github.com/cppalliance/http
#

set(TEST_MAIN ../../../url/extra/test_suite/test_main.cpp ../../../url/extra/test_suite/test_suite.cpp)

file(GLOB_RECURSE LIMITS_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../../src/*.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES limits.cpp Jamfile)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/../../../url/extra/test_suite PREFIX "_extra" FILES ${TEST_MAIN})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/../../src PREFIX "_extra" FILES ${LIMITS_SOURCES})
add_executable(boost_http_limits limits.cpp Jamfile ${TEST_MAIN} ${LIMITS_SOURCES})
target_include_directories(boost_http_limits PRIVATE ../../)
target_include_directories(boost_http_limits PRIVATE ../../include ../../../url/extra/test_suite ../../..)
target_compile_definitions(boost_http_limits PRIVATE
    BOOST_HTTP_TEST_LIMITS
    BOOST_HTTP_NO_LIB
    BOOST_HTTP_STATIC_LINK
)
target_link_libraries(boost_http_limits PRIVATE
    Boost::align
    Boost::assert
    Boost::config
    Boost::container_hash
    Boost::capy
    Boost::system
    Boost::throw_exception
    Boost::url
    Boost::utility)

target_link_libraries(boost_http_limits INTERFACE Boost::http)

add_test(NAME boost_http_limits COMMAND boost_http_limits)
add_dependencies(tests boost_http_limits)
