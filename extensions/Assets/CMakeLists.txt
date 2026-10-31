cmake_minimum_required(VERSION 3.28)
project(vkbase)

##################
## ENGINE ASSETS #
##################

option(VKBASE_EMBED_ASSETS "Embed assets into binary using cmrc" OFF)
option(VKBASE_ASSETS_COPY "Copy assets to build/target directories when not embedding" ON)

# Backward compatibility if ASSETS is defined instead of VKBASE_ASSETS
if(NOT VKBASE_ASSETS AND ASSETS)
    set(VKBASE_ASSETS ${ASSETS})
endif()

message(STATUS "VKBASE_EMBED_ASSETS: ${VKBASE_EMBED_ASSETS}")
message(STATUS "VKBASE_ASSETS_COPY: ${VKBASE_ASSETS_COPY}")

function(vkbase_target_copy_assets target_name)
    set(TARGET_ASSETS ${ARGN})
    if(NOT TARGET_ASSETS)
        if(VKBASE_ASSETS)
            set(TARGET_ASSETS ${VKBASE_ASSETS})
        elseif(ASSETS)
            set(TARGET_ASSETS ${ASSETS})
        endif()
    endif()

    if(NOT VKBASE_EMBED_ASSETS AND VKBASE_ASSETS_COPY)
        foreach(DIR IN LISTS TARGET_ASSETS)
            if(EXISTS "${DIR}")
                get_filename_component(DIR_NAME "${DIR}" NAME)
                add_custom_command(
                    TARGET ${target_name} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
                        "${DIR}"
                        "$<TARGET_FILE_DIR:${target_name}>/Assets/${DIR_NAME}"
                    COMMENT "Copying asset directory ${DIR_NAME} to target Assets directory for ${target_name}"
                )
            endif()
        endforeach()
    endif()
endfunction()

add_library(vkbase_assets assets.cpp assets.h)
add_library(vkbase::assets ALIAS vkbase_assets)

target_include_directories(vkbase_assets
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
)

if(VKBASE_EMBED_ASSETS)
    set(VKBASE_EMBED_ASSETS_INT 1)
    set(VKBASE_ASSETS_COPY_INT 0)
    set(VKBASE_CONFIG_SEARCH_PATHS "")

    include(CMakeRC.cmake)
    cmrc_add_resource_library(vkbase_assets_rc NAMESPACE vkbase_assets)

    foreach(DIR IN LISTS VKBASE_ASSETS)
        if(EXISTS "${DIR}")
            get_filename_component(DIR_PARENT "${DIR}" DIRECTORY)
            file(GLOB_RECURSE DIR_FILES CONFIGURE_DEPENDS
                    "${DIR}/*"
            )
            if(DIR_FILES)
                cmrc_add_resources(vkbase_assets_rc WHENCE "${DIR_PARENT}" ${DIR_FILES})
            endif()
        else()
            message(WARNING "Directory not found: ${DIR}")
        endif()
    endforeach()

    target_link_libraries(vkbase_assets PUBLIC vkbase_assets_rc)
else()
    set(VKBASE_EMBED_ASSETS_INT 0)

    if(VKBASE_ASSETS_COPY)
        set(VKBASE_ASSETS_COPY_INT 1)
        set(VKBASE_CONFIG_SEARCH_PATHS "")

        set(COPY_COMMANDS "")
        foreach(DIR IN LISTS VKBASE_ASSETS)
            if(EXISTS "${DIR}")
                get_filename_component(DIR_NAME "${DIR}" NAME)
                list(APPEND COPY_COMMANDS
                    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
                        "${DIR}"
                        "${CMAKE_BINARY_DIR}/Assets/${DIR_NAME}"
                )
            endif()
        endforeach()
        if(COPY_COMMANDS)
            add_custom_target(vkbase_copy_assets ALL
                ${COPY_COMMANDS}
                COMMENT "Copying assets to build Assets directory ${CMAKE_BINARY_DIR}/Assets"
            )
            add_dependencies(vkbase_assets vkbase_copy_assets)
        endif()
    else()
        set(VKBASE_ASSETS_COPY_INT 0)
        set(SEARCH_DIRS "")
        foreach(DIR IN LISTS VKBASE_ASSETS)
            if(EXISTS "${DIR}")
                get_filename_component(DIR_PARENT "${DIR}" DIRECTORY)
                list(APPEND SEARCH_DIRS "${DIR_PARENT}")
                list(APPEND SEARCH_DIRS "${DIR}")
            endif()
        endforeach()
        list(REMOVE_DUPLICATES SEARCH_DIRS)

        set(VKBASE_CONFIG_SEARCH_PATHS "")
        foreach(SEARCH_DIR IN LISTS SEARCH_DIRS)
            file(TO_CMAKE_PATH "${SEARCH_DIR}" ESCAPED_DIR)
            string(APPEND VKBASE_CONFIG_SEARCH_PATHS "        \"${ESCAPED_DIR}\",\n")
        endforeach()
    endif()
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/vkbase_assets_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/vkbase_assets_config.h"
    @ONLY
)

target_compile_definitions(vkbase_assets PUBLIC
    VKBASE_EMBED_ASSETS=${VKBASE_EMBED_ASSETS_INT}
    VKBASE_ASSETS_COPY=${VKBASE_ASSETS_COPY_INT}
)
