CONFIGURE_FILE(version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)

ADD_EXECUTABLE(
    lifxd
    bulb.c
    client.c
    discovery.c
    gateway.c
    lifxd.c
    log.c
    wire_proto.c
)

TARGET_LINK_LIBRARIES(lifxd ${EVENT2_CORE_LIBRARY})
