INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

ADD_CORE_LIBRARY(
    test_lifx_broadcast STATIC
    ${LIGHTSD_SOURCE_DIR}/core/stats.c
    ${LIGHTSD_SOURCE_DIR}/core/utils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests_shims.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../core/tests_utils.c
)

FUNCTION(ADD_BROADCAST_TEST TEST_SOURCE)
    ADD_TEST_FROM_C_SOURCES(${TEST_SOURCE} test_lifx_broadcast)
ENDFUNCTION()

FILE(GLOB TESTS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "test_*.c")
FOREACH(TEST ${TESTS})
    ADD_BROADCAST_TEST(${TEST})
ENDFOREACH()
