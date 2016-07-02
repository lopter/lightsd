SET(VENV_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/py-env")
SET(VENV_PYTHON "${VENV_DIRECTORY}/bin/python")
SET(VENV_STAMP "${VENV_DIRECTORY}/stamp")
SET(PIP_REQUIREMENTS "${CMAKE_CURRENT_SOURCE_DIR}/requirements-release.txt")

CONFIGURE_FILE(release.py.in "${CMAKE_CURRENT_BINARY_DIR}/release.py" @ONLY)

ADD_CUSTOM_COMMAND(
    OUTPUT "${VENV_STAMP}"
    COMMAND "${VIRTUALENV_EXECUTABLE}" -q -p "${PYTHON_EXECUTABLE}" "${VENV_DIRECTORY}"
    COMMAND "${VENV_DIRECTORY}/bin/pip" -q install -r "${PIP_REQUIREMENTS}"
    COMMAND "${CMAKE_COMMAND}" -E touch "${VENV_STAMP}"
    DEPENDS "${PIP_REQUIREMENTS}"
    COMMENT "Setting up a Python virtualenv at ${VENV_DIRECTORY} for the release script"
    VERBATIM
)

SET(
    RELEASE_COMMANDS
    release
    pre_release
    package_release
    release_new_tag
    release_debuild
    release_makepkg
)
IF (SPHINX_FOUND)
    LIST(APPEND RELEASE_COMMANDS release_docs)
ENDIF ()
FOREACH (TARGET ${RELEASE_COMMANDS})
    ADD_CUSTOM_TARGET(
        ${TARGET}
        COMMAND "${VENV_PYTHON}" "${CMAKE_CURRENT_BINARY_DIR}/release.py" "${TARGET}"
        DEPENDS "${VENV_STAMP}"
        VERBATIM
    )
ENDFOREACH ()

IF (SPHINX_FOUND)
    ADD_DEPENDENCIES(release_docs docs)
ENDIF (SPHINX_FOUND)

SET(
    EXTRA_OUTPUT
    "${CMAKE_CURRENT_BINARY_DIR}/debuild" # release_debuild
    "${CMAKE_CURRENT_BINARY_DIR}/makepkg" # release_makepkg
    "${CMAKE_CURRENT_BINARY_DIR}/lightsd-${LIGHTSD_VERSION}.tar" # release_new_tag
    "${VENV_DIRECTORY}"
    "${VENV_STAMP}"
)
SET_DIRECTORY_PROPERTIES(
    PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${EXTRA_OUTPUT}"
)
