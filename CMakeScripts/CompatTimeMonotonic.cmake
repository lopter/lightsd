INCLUDE(CheckFunctionExists)

IF (NOT TIME_MONOTONIC_LIBRARY)
    SET(COMPAT_TIME_MONOTONIC_IMPL "${LIGHTSD_SOURCE_DIR}/compat/${CMAKE_SYSTEM_NAME}/time_monotonic.c")
    SET(COMPAT_TIME_MONOTONIC_H "${LIGHTSD_SOURCE_DIR}/compat/${CMAKE_SYSTEM_NAME}/time_monotonic.h")
    SET(GENERIC_TIME_MONOTONIC_IMPL "${LIGHTSD_SOURCE_DIR}/compat/generic/time_monotonic.c")
    SET(GENERIC_TIME_MONOTONIC_H "${LIGHTSD_SOURCE_DIR}/compat/generic/time_monotonic.h")
    SET(TIME_MONOTONIC_LIBRARY time_monotonic CACHE INTERNAL "lgtd_time_monotonic implementation")

    SET(CMAKE_REQUIRED_QUIET TRUE)
    MESSAGE(STATUS "Looking for clock_gettime")
    CHECK_FUNCTION_EXISTS("clock_gettime" HAVE_CLOCK_GETTIME)
    IF (NOT HAVE_CLOCK_GETTIME)
        # glibc < 2.17:
        MESSAGE(STATUS "Looking for clock_gettime again in librt")
        UNSET(HAVE_CLOCK_GETTIME CACHE)
        SET(TIME_MONOTONIC_LIBRARY_DEP rt CACHE INTERNAL "dependency for lgtd_time_monotonic")
        SET(CMAKE_REQUIRED_LIBRARIES ${TIME_MONOTONIC_LIBRARY_DEP})
        CHECK_FUNCTION_EXISTS("clock_gettime" HAVE_CLOCK_GETTIME)
        UNSET(CMAKE_REQUIRED_LIBRARIES)
        IF (NOT HAVE_CLOCK_GETTIME)
            UNSET(TIME_MONOTONIC_LIBRARY_DEP CACHE)
        ENDIF ()
    ENDIF ()
    UNSET(CMAKE_REQUIRED_QUIET)

    IF (HAVE_CLOCK_GETTIME)
        MESSAGE(STATUS "Looking for clock_gettime - found")
        FILE(COPY "${GENERIC_TIME_MONOTONIC_H}" DESTINATION "${LIGHTSD_BINARY_DIR}/core/")
        SET(
            TIME_MONOTONIC_IMPL ${GENERIC_TIME_MONOTONIC_IMPL}
            CACHE INTERNAL "lgtd_time_monotonic (POSIX generic source)"
        )
    ELSEIF (EXISTS "${COMPAT_TIME_MONOTONIC_IMPL}")
        MESSAGE(STATUS "Looking for clock_gettime - not found, using built-in compatibilty file")
        FILE(COPY "${COMPAT_TIME_MONOTONIC_H}" DESTINATION "${LIGHTSD_BINARY_DIR}/core/")
        SET(
            TIME_MONOTONIC_IMPL "${COMPAT_TIME_MONOTONIC_IMPL}"
            CACHE INTERNAL "lgtd_time_monotonic (${CMAKE_SYSTEM_NAME} specific source)"
        )
    ELSE ()
        MESSAGE(SEND_ERROR "Looking for clock_gettime - not found")
    ENDIF ()
ENDIF ()

ADD_LIBRARY(${TIME_MONOTONIC_LIBRARY} STATIC "${TIME_MONOTONIC_IMPL}")

IF (TIME_MONOTONIC_LIBRARY_DEP)
    TARGET_LINK_LIBRARIES(${TIME_MONOTONIC_LIBRARY} ${TIME_MONOTONIC_LIBRARY_DEP})
ENDIF ()
