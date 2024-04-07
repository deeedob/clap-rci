if(UNIX)
    if(APPLE)
        set(CLAP_SYSTEM_PATH "/Library/Audio/Plug-Ins/CLAP")
        set(CLAP_USER_PATH "$ENV{HOME}/Library/Audio/Plug-Ins/CLAP")
    else()
        set(CLAP_SYSTEM_PATH "/usr/lib/clap")
        set(CLAP_USER_PATH "$ENV{HOME}/.clap")
    endif()
elseif(WIN32)
    set(CLAP_SYSTEM_PATH "$ENV{COMMONPROGRAMFILES}\\CLAP")
    set(CLAP_USER_PATH "$ENV{LOCALAPPDATA}\\Programs\\Common\\CLAP")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

function(clap_symlink TARGET_NAME)

    set_target_properties(${TARGET_NAME} PROPERTIES SUFFIX ".clap" PREFIX "")
    # Create a custom command to create the symlink
    add_custom_command(
        TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink "$<TARGET_FILE:${TARGET_NAME}>"
                "${CLAP_USER_PATH}/$<TARGET_FILE_NAME:${TARGET_NAME}>"
    )

    add_custom_target(
        create_symlink_${TARGET_NAME} ALL DEPENDS ${TARGET_NAME}
        COMMENT "Creating symlink for ${TARGET_NAME}"
    )

    add_dependencies(create_symlink_${TARGET_NAME} ${TARGET_NAME})

    add_custom_command(
        TARGET create_symlink_${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo
                "Symlink created: ${CLAP_USER_PATH}/$<TARGET_FILE_NAME:${TARGET_NAME}>"
    )
endfunction()
