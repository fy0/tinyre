
cmake_minimum_required(VERSION 2.8)

project (tinyre)

cmake_policy(SET CMP0015 OLD)

#set(CMAKE_BUILD_TYPE Debug)
if (MSVC)
    set(CMAKE_GENERATOR_TOOLSET "v120_xp" CACHE STRING "Platform Toolset" FORCE) 
    set(CMAKE_VS_PLATFORM_TOOLSET "v120_xp" CACHE STRING "Platform Toolset" FORCE)
endif(MSVC)

macro(source_group_by_dir source_files)
    if(MSVC)
        set(sgbd_cur_dir ${CMAKE_CURRENT_SOURCE_DIR})
        foreach(sgbd_file ${${source_files}})
            string(REGEX REPLACE ${sgbd_cur_dir}/\(.*\) \\1 sgbd_fpath ${sgbd_file})
            string(REGEX REPLACE "\(.*\)/.*" \\1 sgbd_group_name ${sgbd_fpath})
            string(COMPARE EQUAL ${sgbd_fpath} ${sgbd_group_name} sgbd_nogroup)
            string(REPLACE "/" "\\" sgbd_group_name ${sgbd_group_name})
            if(sgbd_nogroup)
                set(sgbd_group_name "\\")
            endif(sgbd_nogroup)
            source_group(${sgbd_group_name} FILES ${sgbd_file})
        endforeach(sgbd_file)
    endif(MSVC)
endmacro(source_group_by_dir)

INCLUDE_DIRECTORIES()

if (MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "/MTd")
    set(CMAKE_CXX_FLAGS_RELEASE "/MT")
else(MSVC)
    set(CMAKE_C_FLAGS "-std=c99")
endif(MSVC)

add_definitions(-DUNICODE -D_UNICODE -DDEMO)

IF(DEBUG_mode)
    add_definitions(-DTRE_DEBUG)
ENDIF()

file(GLOB_RECURSE project_headers src/*.h)
file(GLOB_RECURSE project_cpps src/*.c)
set (SRC_LIST ${project_headers} ${project_cpps})

source_group_by_dir(SRC_LIST)

add_executable(tinyre ${SRC_LIST})
#add_library(tinyre ${SRC_LIST})

if (NOT MSVC)
    target_link_libraries(tinyre m)
endif(MSVC)

