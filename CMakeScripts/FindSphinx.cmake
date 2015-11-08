FIND_PROGRAM(
    SPHINX_EXECUTABLE
    NAMES sphinx-build sphinx-build2
    DOC "Path to sphinx-build executable"
)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(Sphinx DEFAULT_MSG SPHINX_EXECUTABLE)
