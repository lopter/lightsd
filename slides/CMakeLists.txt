FUNCTION(LIGHTSD_COPY_COMMON_LATEX)
    SET(
        SOURCES
        common/lgtdfigs.sty
        common/lgtdslides.cls
    )
    FOREACH(F ${SOURCES})
        FILE(
            COPY ${PROJECT_SOURCE_DIR}/slides/${F}
            DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
        )
    ENDFOREACH()
ENDFUNCTION()

ADD_ALL_SUBDIRECTORIES()
