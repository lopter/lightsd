# As you'll find out the code in this directory isn't really generic at all.
# I only have lifx bulbs right now and I don't want to do any premature work.

INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/../
    ${CMAKE_CURRENT_BINARY_DIR}
)

CONFIGURE_FILE(version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)

ADD_EXECUTABLE(
    lightsd
    client.c
    daemon.c
    jsmn.c
    jsonrpc.c
    listen.c
    lightsd.c
    log.c
    pipe.c
    proto.c
    router.c
    stats.c
    timer.c
)

TARGET_LINK_LIBRARIES(
    lightsd
    lifx
    ${EVENT2_CORE_LIBRARY}
    ${TIME_MONOTONIC_LIBRARY}
)

IF (HAVE_LIBBSD)
    TARGET_LINK_LIBRARIES(lightsd ${LIBBSD_LIBRARY})
ENDIF (HAVE_LIBBSD)
