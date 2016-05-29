# it probably works with older versions but this the oldest tested one:
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.9)

PROJECT(LIGHTSD C)

SET(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${LIGHTSD_SOURCE_DIR}/CMakeScripts")
INCLUDE(LightsdVersion)

MESSAGE(STATUS "lightsd version: ${LIGHTSD_VERSION}")
MESSAGE(STATUS "CMake version: ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}")
MESSAGE(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
MESSAGE(STATUS "Target system: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION} ${CMAKE_SYSTEM_PROCESSOR}")
MESSAGE(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
MESSAGE(STATUS "Source directory: ${LIGHTSD_SOURCE_DIR}")

ENABLE_TESTING()

### Platform checks ############################################################

# TODO: we need at least 2.0.19-stable because of the logging defines
FIND_PACKAGE(Event2 REQUIRED COMPONENTS core)
FIND_PACKAGE(Endian REQUIRED)
FIND_PACKAGE(Sphinx)

# Release tools:
FIND_PACKAGE(Gzip)
FIND_PACKAGE(Hg)
FIND_PACKAGE(PythonInterp 3.3) # >= 3.3 for os.replace
FIND_PACKAGE(BsdTar)
FIND_PACKAGE(Virtualenv)
FIND_PACKAGE(Xz)

INCLUDE(CheckFunctionExists)
INCLUDE(CheckVariableExists)
INCLUDE(TestBigEndian)

INCLUDE(CompatReallocArray)
INCLUDE(CompatSetProctitle)
INCLUDE(CompatTimeMonotonic)

TEST_BIG_ENDIAN(BIG_ENDIAN_SYSTEM)

### Global definitions #########################################################

INCLUDE(AddAllSubdirectories)
INCLUDE(AddTestFromSources)

SET(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

SET(CMAKE_C_FLAGS "-pipe ${CMAKE_C_FLAGS}")
STRING(STRIP "${CMAKE_C_FLAGS}" CMAKE_C_FLAGS)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE}}")
STRING(STRIP "${CMAKE_C_FLAGS}" CMAKE_C_FLAGS)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra -Wall -Wstrict-prototypes -std=c99")
SET(CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE} "")

ADD_DEFINITIONS(
    # Only relevant for the GNU libc:
    "-D_POSIX_C_SOURCE=200809L"
    "-D_BSD_SOURCE=1"
    "-D_DEFAULT_SOURCE=1"

    "-D_DARWIN_C_SOURCE=1"

    "-DLGTD_BIG_ENDIAN_SYSTEM=${BIG_ENDIAN_SYSTEM}"
    "-DLGTD_SIZEOF_VOID_P=${CMAKE_SIZEOF_VOID_P}"

    "-DLGTD_HAVE_SETPROCTITLE=${HAVE_SETPROCTITLE}"
    "-DLGTD_HAVE_REALLOCARRAY=${HAVE_REALLOCARRAY}"

    "-DJSMN_STRICT=1"
    "-DJSMN_PARENT_LINKS=1"
)

IF (CMAKE_BUILD_TYPE MATCHES "DEBUG")
    ADD_DEFINITIONS("-DQUEUE_MACRO_DEBUG=1")
    IF (CMAKE_COMPILER_IS_GNUCC)
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -ggdb3")
    ENDIF ()
ENDIF ()

IF (NOT LGTD_RUNTIME_DIRECTORY)
    SET(LGTD_RUNTIME_DIRECTORY "${LIGHTSD_BINARY_DIR}")
ENDIF ()
MESSAGE(STATUS "lightsd runtime directory: ${LGTD_RUNTIME_DIRECTORY}")

INCLUDE_DIRECTORIES(
    ${EVENT2_INCLUDE_DIR}
    ${LIGHTSD_BINARY_DIR}/compat
    ${LIGHTSD_BINARY_DIR}/compat/generic
)

ADD_SUBDIRECTORY(compat)
ADD_SUBDIRECTORY(core)
ADD_SUBDIRECTORY(lifx)

# 2.8.11 is the first version with TARGET_INCLUDE_DIRECTORIES:
IF (CMAKE_VERSION VERSION_GREATER 2.8.10)
    CONFIGURE_FILE(CTestCustom.cmake.in "${LIGHTSD_BINARY_DIR}/CTestCustom.cmake" @ONLY)
    ADD_SUBDIRECTORY(tests)
ELSE ()
    MESSAGE(
        STATUS
        "The tests suite requires CMake >= 2.8.11 "
        "but you have ${CMAKE_VERSION}, disabling it"
    )
ENDIF ()

IF (SPHINX_FOUND)
    MESSAGE(STATUS "Sphinx found, docs generation enabled")
    ADD_SUBDIRECTORY(docs)
ELSE ()
    MESSAGE(STATUS "Shpinx wasn't found, docs generation disabled")
ENDIF ()

IF (
    PYTHONINTERP_FOUND
    AND PYTHON_VERSION_MAJOR EQUAL 3
    AND PYTHON_VERSION_MINOR GREATER 2
    AND VIRTUALENV_FOUND
    AND HG_FOUND
    AND BSDTAR_FOUND
    AND GZIP_FOUND
    AND XZ_FOUND
)
    MESSAGE(
        STATUS
        "Python >= 3.3, virtualenv, bsdtar, gzip, xz, and mercurial (hg) "
        "found, release commands enabled"
    )
    ADD_SUBDIRECTORY(dist)
ELSE ()
    MESSAGE(
        STATUS
        "Python >= 3.3 and/or virtualenv, bsdtar, gzip, xz, mercurial (hg) "
        "weren't found, release commands disabled"
    )
ENDIF ()

INSTALL(
    FILES COPYING README.rst docs/protocol.rst
    DESTINATION share/doc/lightsd
)
INSTALL(
    DIRECTORY examples
    DESTINATION share/lightsd
    USE_SOURCE_PERMISSIONS
    REGEX ".*\\.sw.$" EXCLUDE
)
INSTALL(FILES share/lightsc.sh DESTINATION share/lightsd)
INSTALL(FILES dist/lightsd.service DESTINATION lib/systemd/system)
