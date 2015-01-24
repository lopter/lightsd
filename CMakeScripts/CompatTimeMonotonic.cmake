INCLUDE(CheckFunctionExists)

IF (NOT TIME_MONOTONIC_IMPL)
    SET(COMPAT_TIME_MONOTONIC_IMPL "${LIGHTSD_SOURCE_DIR}/compat/${CMAKE_SYSTEM_NAME}/time_monotonic.c")
    SET(COMPAT_TIME_MONOTONIC_H "${LIGHTSD_SOURCE_DIR}/compat/${CMAKE_SYSTEM_NAME}/time_monotonic.h")
    SET(GENERIC_TIME_MONOTONIC_IMPL "${LIGHTSD_SOURCE_DIR}/compat/generic/time_monotonic.c")
    SET(GENERIC_TIME_MONOTONIC_H "${LIGHTSD_SOURCE_DIR}/compat/generic/time_monotonic.h")

    SET(CMAKE_REQUIRED_QUIET TRUE)
    MESSAGE(STATUS "Looking for clock_gettime")
    CHECK_FUNCTION_EXISTS("clock_gettime" HAVE_CLOCK_GETTIME)
    UNSET(CMAKE_REQUIRED_QUIET)

    IF (HAVE_CLOCK_GETTIME)
        MESSAGE(STATUS "Looking for clock_gettime - found")
        FILE(COPY "${GENERIC_TIME_MONOTONIC_H}" DESTINATION "${LIGHTSD_BINARY_DIR}/core/")
        SET(
            TIME_MONOTONIC_IMPL "${GENERIC_TIME_MONOTONIC_IMPL}"
            CACHE INTERNAL "lgtd_time_monotonic (POSIX generic implementation)"
        )
    ELSEIF (EXISTS "${COMPAT_TIME_MONOTONIC_IMPL}")
        MESSAGE(STATUS "Looking for clock_gettime - not found, using built-in compatibilty file")
        FILE(COPY "${COMPAT_TIME_MONOTONIC_H}" DESTINATION "${LIGHTSD_BINARY_DIR}/core/")
        SET(
            TIME_MONOTONIC_IMPL "${COMPAT_TIME_MONOTONIC_IMPL}"
            CACHE INTERNAL "lgtd_time_monotonic (${CMAKE_SYSTEM_NAME} specific implementation)"
        )
    ELSE ()
        MESSAGE(SEND_ERROR "Looking for clock_gettime - not found")
    ENDIF ()
ENDIF ()
