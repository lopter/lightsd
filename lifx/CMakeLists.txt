INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/../
    ${CMAKE_CURRENT_BINARY_DIR}
)

ADD_LIBRARY(
    lifx
    broadcast.c
    bulb.c
    gateway.c
    tagging.c
    timer.c
    wire_proto.c
)
