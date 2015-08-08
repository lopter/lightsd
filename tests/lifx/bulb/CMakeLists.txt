INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

ADD_CORE_LIBRARY(
    test_lifx_bulb_core STATIC
    ${LIGHTSD_SOURCE_DIR}/core/log.c
    ${LIGHTSD_SOURCE_DIR}/core/router.c
    ${LIGHTSD_SOURCE_DIR}/core/stats.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests_shims.c
)

ADD_LIBRARY(
    test_lifx_bulb STATIC
    ${LIGHTSD_SOURCE_DIR}/lifx/bulb.c
    ${LIGHTSD_SOURCE_DIR}/lifx/wire_proto.c
)

FUNCTION(ADD_BULB_TEST TEST_SOURCE)
    ADD_TEST_FROM_C_SOURCES(
        ${TEST_SOURCE} test_lifx_bulb_core test_lifx_bulb
    )
ENDFUNCTION()

FILE(GLOB TESTS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "test_*.c")
FOREACH(TEST ${TESTS})
    ADD_BULB_TEST(${TEST})
ENDFOREACH()
