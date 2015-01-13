INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

CONFIGURE_FILE(version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)

ADD_EXECUTABLE(
    lifxd
    broadcast.c
    bulb.c
    client.c
    gateway.c
    lifxd.c
    log.c
    ${TIME_MONOTONIC_IMPL}
    timer.c
    wire_proto.c
)

TARGET_LINK_LIBRARIES(lifxd ${EVENT2_CORE_LIBRARY})
