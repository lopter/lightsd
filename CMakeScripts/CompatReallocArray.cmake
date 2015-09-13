IF (DEFINED HAVE_REALLOCARRAY)
    RETURN()
ENDIF ()

MESSAGE(STATUS "Looking for reallocarray")

IF (HAVE_LIBBSD)
    MESSAGE(STATUS "Looking for reallocarray - found")
    SET(HAVE_REALLOCARRAY 1 CACHE INTERNAL "reallocarray found in libbsd")
    RETURN()
ENDIF ()

SET(CMAKE_REQUIRED_QUIET TRUE)
CHECK_FUNCTION_EXISTS("reallocarray" HAVE_REALLOCARRAY)
UNSET(CMAKE_REQUIRED_QUIET)
IF (HAVE_REALLOCARRAY)
    MESSAGE(STATUS "Looking for reallocarray - found")
    SET(
        HAVE_REALLOCARRAY 1
        CACHE INTERNAL
        "reallocarray found on the system"
    )
ELSE ()
    MESSAGE(
        STATUS
        "Looking for reallocarray - not found, using built-in compatibilty file"
    )
    SET(
        HAVE_REALLOCARRAY 0
        CACHE INTERNAL
        "reallocarray not found, using internal implementation"
    )
ENDIF ()
