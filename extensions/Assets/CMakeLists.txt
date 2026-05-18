cmake_minimum_required(VERSION 3.28 )
project(vkbase)

##################
## ENGINE ASSETS #
##################

include(CMakeRC.cmake)

cmrc_add_resource_library(vkbase_assets_rc NAMESPACE vkbase_assets)

foreach(DIR IN LISTS ASSETS)
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




add_library(vkbase_assets assets.cpp assets.h)
target_link_libraries(vkbase_assets PUBLIC vkbase_assets_rc)

add_library(vkbase::assets ALIAS vkbase_assets)