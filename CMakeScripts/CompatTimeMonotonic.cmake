IF (NOT TIME_MONOTONIC_LIBRARY)
    SET(COMPAT_TIME_MONOTONIC_IMPL "${LIGHTSD_SOURCE_DIR}/compat/${CMAKE_SYSTEM_NAME}/time_monotonic.c")
    SET(GENERIC_TIME_MONOTONIC_IMPL "${LIGHTSD_SOURCE_DIR}/compat/generic/time_monotonic.c")
    SET(TIME_MONOTONIC_LIBRARY time_monotonic CACHE INTERNAL "lgtd_time_monotonic implementation")

    IF (APPLE)
        # Hopefully my intuition is right and this fixes false positives on mac
        # os < 10.12 with X Code >= 8 (where clock_gettime is defined in the
        # SDKs but actually doesn't exists so dyld blows up at run time).
        #
        # -u symbol_name
        #         Specified that symbol symbol_name must be defined for the
        #         link to succeed.  This is useful to force selected functions
        #         to be loaded from a static library.
        SET(CMAKE_REQUIRED_FLAGS "-Wl,-u=clock_gettime")
    ENDIF ()

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

    UNSET(CMAKE_REQUIRED_FLAGS)

    IF (HAVE_CLOCK_GETTIME)
        MESSAGE(STATUS "Looking for clock_gettime - found")
        SET(
            TIME_MONOTONIC_IMPL ${GENERIC_TIME_MONOTONIC_IMPL}
            CACHE INTERNAL "lgtd_time_monotonic (POSIX generic source)"
        )
    ELSEIF (EXISTS "${COMPAT_TIME_MONOTONIC_IMPL}")
        MESSAGE(STATUS "Looking for clock_gettime - not found, using built-in compatibilty file")
        SET(
            TIME_MONOTONIC_IMPL "${COMPAT_TIME_MONOTONIC_IMPL}"
            CACHE INTERNAL "lgtd_time_monotonic (${CMAKE_SYSTEM_NAME} specific source)"
        )
    ELSE ()
        MESSAGE(SEND_ERROR "Looking for clock_gettime - not found")
    ENDIF ()

    SET(COMPAT_SLEEP_MONOTONIC_IMPL "${LIGHTSD_SOURCE_DIR}/compat/${CMAKE_SYSTEM_NAME}/sleep_monotonic.c")
    SET(GENERIC_SLEEP_MONOTONIC_IMPL "${LIGHTSD_SOURCE_DIR}/compat/generic/sleep_monotonic.c")

    MESSAGE(STATUS "Looking for nanosleep")
    CHECK_FUNCTION_EXISTS("nanosleep" HAVE_NANOSLEEP)
    IF (HAVE_NANOSLEEP)
        MESSAGE(STATUS "Looking for nanosleep - found")
        SET(
            SLEEP_MONOTONIC_IMPL "${GENERIC_SLEEP_MONOTONIC_IMPL}"
            CACHE INTERNAL "lgtd_sleep_monotonic (POSIX generic source)"
        )
    ELSEIF (EXISTS "${COMPAT_SLEEP_MONOTONIC_IMPL}")
        MESSAGE(STATUS "Looking for nanosleep - not found, using built-in compatibilty file")
        SET(
            SLEEP_MONOTONIC_IMPL "${COMPAT_SLEEP_MONOTONIC_IMPL}"
            CACHE INTERNAL "lgtd_sleep_monotonic (${CMAKE_SYSTEM_NAME} specific source)"
        )
        IF (WIN32)
            SET(TIME_MONOTONIC_LIBRARY_DEP Winmm CACHE INTERNAL "dependency for lgtd_sleep_monotonic")
        ENDIF ()
    ENDIF ()

    UNSET(CMAKE_REQUIRED_QUIET)
ENDIF ()

ADD_LIBRARY(
    ${TIME_MONOTONIC_LIBRARY}
    STATIC
    "${TIME_MONOTONIC_IMPL}"
    "${SLEEP_MONOTONIC_IMPL}"
)

IF (TIME_MONOTONIC_LIBRARY_DEP)
    TARGET_LINK_LIBRARIES(${TIME_MONOTONIC_LIBRARY} ${TIME_MONOTONIC_LIBRARY_DEP})
ENDIF ()
