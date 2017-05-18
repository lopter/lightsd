INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

ADD_LIBRARY(
    lifx
    broadcast.c
    bulb.c
    discovery.c
    gateway.c
    tagging.c
    wire_proto.c
)
