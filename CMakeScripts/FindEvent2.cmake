FIND_PATH(
    EVENT2_INCLUDE_DIR
    event2/event.h
    HINTS
    /usr/local/ # OpenBSD has libevent1 in /usr/lib, always try /usr/local first
    $ENV{EVENT2_DIR}/include # Windows...
)

FOREACH (COMPONENT ${Event2_FIND_COMPONENTS})
    STRING(TOUPPER ${COMPONENT} UPPER_COMPONENT)
    FIND_LIBRARY(
        EVENT2_${UPPER_COMPONENT}_LIBRARY
        event_${COMPONENT}
        NAMES
        libevent_${COMPONENT} # Windows
        HINTS
        /usr/local/
        $ENV{EVENT2_DIR}
    )
    IF (EVENT2_${UPPER_COMPONENT}_LIBRARY)
        SET(Event2_${COMPONENT}_FOUND TRUE)
    ENDIF ()
ENDFOREACH ()

INCLUDE(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
    Event2
    HANDLE_COMPONENTS
    REQUIRED_VARS EVENT2_CORE_LIBRARY EVENT2_INCLUDE_DIR
)
