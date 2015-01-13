INCLUDE(CheckIncludeFile)

IF (NOT ENDIAN_H_PATH)
    SET(COMPAT_ENDIAN_H "${LIFXD_SOURCE_DIR}/compat/${CMAKE_SYSTEM_NAME}/endian.h")
    SET(GENERIC_ENDIAN_H "${LIFXD_SOURCE_DIR}/compat/generic/endian.h")

    SET(CMAKE_REQUIRED_QUIET TRUE)
    MESSAGE(STATUS "Looking for endian.h")
    CHECK_INCLUDE_FILE("endian.h" HAVE_ENDIAN_H)
    UNSET(CMAKE_REQUIRED_QUIET)

    IF (HAVE_ENDIAN_H)
        MESSAGE(STATUS "Looking for endan.h - found")
        SET(ENDIAN_H_PATH "using native headers" CACHE INTERNAL "endian.h path")
    ELSEIF (EXISTS "${COMPAT_ENDIAN_H}")
        MESSAGE(STATUS "Looking for endian.h - not found, using built-in compatibility file")
        FILE(COPY "${COMPAT_ENDIAN_H}" DESTINATION "${LIFXD_BINARY_DIR}/compat/")
        SET(ENDIAN_H_PATH "${COMPAT_ENDIAN_H}" CACHE INTERNAL "endian.h path")
    ELSE ()
        MESSAGE(STATUS "Looking for endian.h - not found")
    ENDIF ()
ENDIF ()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(Endian DEFAULT_MSG ENDIAN_H_PATH)
