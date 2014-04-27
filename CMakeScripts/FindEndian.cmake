INCLUDE(CheckIncludeFile)

SET(COMPAT_ENDIAN_H "${LIFXD_SOURCE_DIR}/compat/${CMAKE_SYSTEM_NAME}/endian.h")

IF (NOT ENDIAN_H_PATH)
    CHECK_INCLUDE_FILE("endian.h" HAVE_ENDIAN_H)

    IF (HAVE_ENDIAN_H)
        SET(ENDIAN_H_PATH "using native header" CACHE INTERNAL "endian.h path")
    ELSEIF (EXISTS "${COMPAT_ENDIAN_H}")
        FILE(
            COPY "${COMPAT_ENDIAN_H}"
            DESTINATION "${LIFXD_BINARY_DIR}/compat/"
        )
        MESSAGE(STATUS "Using compatibility endian.h for ${CMAKE_SYSTEM_NAME}")
        SET(ENDIAN_H_PATH "${COMPAT_ENDIAN_H}" CACHE INTERNAL "endian.h path")
    ENDIF ()
ENDIF ()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(Endian DEFAULT_MSG ENDIAN_H_PATH)
