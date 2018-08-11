
macro(configure_files srcDir destDir)
    message(STATUS "Configuring directory ${destDir}")
    make_directory(${destDir})

    file(GLOB templateFiles RELATIVE ${srcDir} ${srcDir}/*)
    foreach(templateFile ${templateFiles})
        set(srcTemplatePath ${srcDir}/${templateFile})
        if(NOT IS_DIRECTORY ${srcTemplatePath})
            message(STATUS "Configuring file ${templateFile}")
            configure_file(
                    ${srcTemplatePath}
                    ${destDir}/${templateFile}
                    @ONLY)
        endif(NOT IS_DIRECTORY ${srcTemplatePath})
    endforeach(templateFile)
endmacro(configure_files)

macro(create_git_version)
    # Get the current working branch
    execute_process(
            COMMAND git rev-parse --abbrev-ref HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_BRANCH
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # http://xit0.org/2013/04/cmake-use-git-branch-and-commit-details-in-project/
    # Get the latest abbreviated commit hash of the working branch
    execute_process(
            COMMAND git log -1 --format=%h
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Get the date and time of last commit
    execute_process(
            COMMAND git log -1 --format=%cd --date=short
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT_DATETIME
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Get current branch name
    execute_process(
            COMMAND git rev-parse --abbrev-ref HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_BRANCH_NAME
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    configure_file(
            ${CMAKE_SOURCE_DIR}/src/version.h.in
            ${CMAKE_BINARY_DIR}/gen/version.h
    )

    include_directories(${CMAKE_BINARY_DIR}/gen)

endmacro(create_git_version)



macro(resource_dir srcDir)
    # Scan through resource folder for updated files and copy if none existing or changed
    file (GLOB_RECURSE resources "${srcDir}/*.*")
    foreach(resource ${resources})
        get_filename_component(filename ${resource} NAME)
        get_filename_component(dir ${resource} DIRECTORY)
        get_filename_component(dirname ${dir} NAME)

        set(topdir ${dirname})

        set (output "")

        while(NOT ${dirname} STREQUAL ${srcDir})
            get_filename_component(path_component ${dir} NAME)
            set (output "${path_component}/${output}")
            get_filename_component(dir ${dir} DIRECTORY)
            get_filename_component(dirname ${dir} NAME)
        endwhile()

        set(output "${CMAKE_CURRENT_BINARY_DIR}/${topdir}/${filename}")

        add_custom_command(
                COMMENT "Moving updated resource-file '${filename}' to ${output}"
                OUTPUT ${output}
                DEPENDS ${resource}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${resource}
                ${output}
        )
        add_custom_target(${filename} ALL DEPENDS ${resource} ${output})

    endforeach()
endmacro(resource_dir)