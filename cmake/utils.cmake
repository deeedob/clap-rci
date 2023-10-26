
# Returns a new list @out that is appended with a newline from @list
function(util_list_newline out list)
    if (ARGN)
        util_list_newline(${out} ${ARGN})
    endif()
    set(${out} "${list}\n${${out}}" PARENT_SCOPE)
endfunction()

# Returns a new list @out that is appended with @delim
function(util_list_delimiter out delim)
    list(GET ARGV 2 temp)
    math(EXPR N "${ARGC}-1")
    # check if the list is smaller than 3
    if (N LESS 3)
        set(${out} "${temp}" PARENT_SCOPE)
        return()
    endif()
    foreach(IDX RANGE 3 ${N})
        list(GET ARGV ${IDX} STR)
        set(temp "${temp}${delim}${STR}")
    endforeach()
    set(${out} "${temp}" PARENT_SCOPE)
endfunction()

# Function to add tests
function(add_test_executable name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE Catch2::Catch2WithMain)

    message(STATUS "Adding test executable: ${name}")

    set(flags)
    set(args)
    set(listArgs DEPENDENCIES)
    cmake_parse_arguments(arg "${flags}" "${args}" "${listArgs}" ${ARGN})

    # Optional libraries to link against (if provided)
    if ( arg_DEPENDENCIES )
        target_link_libraries(${name} PUBLIC ${arg_DEPENDENCIES})
        add_dependencies(${name} ${arg_DEPENDENCIES})
    endif ()
    catch_discover_tests(${name})
endfunction()

function(print_target_info target_name)
    message(STATUS "Printing information for target: ${target_name}")

    # Get target properties
    get_target_property(target_type ${target_name} TYPE)
    get_target_property(target_sources ${target_name} SOURCES)
    get_target_property(target_include_dirs ${target_name} INCLUDE_DIRECTORIES)
    get_target_property(target_compile_options ${target_name} COMPILE_OPTIONS)
    get_target_property(target_compile_definitions ${target_name} COMPILE_DEFINITIONS)
    get_target_property(target_link_libraries ${target_name} LINK_LIBRARIES)
    get_target_property(target_dependencies ${target_name} INTERFACE_LINK_LIBRARIES)

    # Print target information
    message(STATUS "Target Type: ${target_type}")
    message(STATUS "Sources: ${target_sources}")
    message(STATUS "Include Directories: ${target_include_dirs}")
    message(STATUS "Compile Options: ${target_compile_options}")
    message(STATUS "Compile Definitions: ${target_compile_definitions}")
    message(STATUS "Link Libraries: ${target_link_libraries}")
    message(STATUS "Dependencies: ${target_dependencies}")
endfunction()
