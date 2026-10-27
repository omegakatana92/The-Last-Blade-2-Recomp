cmake_minimum_required(VERSION 3.21)
project(lastblade2 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# SDL2 gives the player its window, sound and gamepads. On Windows the
# official SDL2 VC development package is expected under deps/sdl2/<ver>/;
# pass -DLASTBLADE2_BUNDLED_SDL2=OFF to use another one (for example
# -DSDL2_DIR=<...>/cmake). Without SDL2 the player still builds,
# headless-only.
option(LASTBLADE2_BUNDLED_SDL2 "Prefer the SDL2 development package in deps/sdl2" ON)
if(WIN32 AND LASTBLADE2_BUNDLED_SDL2)
    file(GLOB lastblade2_sdl_cmake "${CMAKE_CURRENT_SOURCE_DIR}/deps/sdl2/*/cmake/sdl2-config.cmake")
    if(lastblade2_sdl_cmake)
        list(GET lastblade2_sdl_cmake 0 lastblade2_sdl_config)
        get_filename_component(lastblade2_sdl_dir "${lastblade2_sdl_config}" DIRECTORY)
        set(SDL2_DIR "${lastblade2_sdl_dir}" CACHE PATH "SDL2 CMake package directory" FORCE)
        list(APPEND CMAKE_PREFIX_PATH "${lastblade2_sdl_dir}")
        message(STATUS "lastblade2: using bundled SDL2 at ${lastblade2_sdl_dir}")
    endif()
endif()

# The neogeorecomp toolkit: the vendored copy (a submodule in a clean
# checkout), or a checkout next to this project.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/neogeorecomp/CMakeLists.txt)
    set(ng_default ${CMAKE_CURRENT_SOURCE_DIR}/neogeorecomp)
else()
    set(ng_default ${CMAKE_CURRENT_SOURCE_DIR}/../neogeorecomp)
endif()
set(NEOGEORECOMP_DIR ${ng_default} CACHE PATH "neogeorecomp checkout")
add_subdirectory(${NEOGEORECOMP_DIR} neogeorecomp_build)

# Your ROM files: the MAME "lastbld2" set and the required "neogeo" system
# ROMs (sp-s2.sp1, sfix.sfix, sm1.sm1, 000-lo.lo), unzipped into one
# directory. Without this the player builds interpreter-only.
set(LASTBLADE2_ROM_DIR "" CACHE PATH "Directory with your lastbld2 and Neo Geo BIOS ROM files")
# Optional: entry points a previous run found at runtime (--dump-misses).
set(LASTBLADE2_ENTRIES "${CMAKE_BINARY_DIR}/entries.txt" CACHE FILEPATH "Extra entry points for the recompiler")

# Stage 1: the generator.
add_executable(lastblade2_recomp src/recomp_main.c src/game.c)
target_link_libraries(lastblade2_recomp PRIVATE m68krecomp_core)
target_include_directories(lastblade2_recomp PRIVATE src)

# Stage 2: the player, with the generated C when ROMs are available.
set(GAME_SOURCES src/main.c)
set(GEN_DIR ${CMAKE_BINARY_DIR}/generated)
set(GEN_FILES 16)
if(LASTBLADE2_ROM_DIR)
    set(GEN_SOURCES ${GEN_DIR}/recomp_table.c)
    math(EXPR last "${GEN_FILES} - 1")
    foreach(i RANGE ${last})
        if(i LESS 10)
            list(APPEND GEN_SOURCES ${GEN_DIR}/recomp_0${i}.c)
        else()
            list(APPEND GEN_SOURCES ${GEN_DIR}/recomp_${i}.c)
        endif()
    endforeach()
    # The entry list starts empty; the `profile` target fills it from a
    # scripted play session and the next build regenerates with it.
    if(NOT EXISTS ${LASTBLADE2_ENTRIES})
        file(WRITE ${LASTBLADE2_ENTRIES} "")
    endif()
    file(MAKE_DIRECTORY ${GEN_DIR})
    add_custom_command(
        OUTPUT ${GEN_SOURCES} ${GEN_DIR}/recomp_common.h
        COMMAND lastblade2_recomp --rom-path ${LASTBLADE2_ROM_DIR} --out ${GEN_DIR} --files ${GEN_FILES}
                --entries ${LASTBLADE2_ENTRIES}
        DEPENDS lastblade2_recomp ${LASTBLADE2_ENTRIES}
        COMMENT "Recompiling lastbld2 from ${LASTBLADE2_ROM_DIR}")
    list(APPEND GAME_SOURCES ${GEN_SOURCES})
endif()

add_executable(lastblade2 ${GAME_SOURCES} src/game.c)
if(WIN32)
    set_target_properties(lastblade2 PROPERTIES OUTPUT_NAME "Last blade 2")
else()
    set_target_properties(lastblade2 PROPERTIES OUTPUT_NAME "lastblade2")
endif()
target_link_libraries(lastblade2 PRIVATE neogeorecomp)
target_include_directories(lastblade2 PRIVATE src ${GEN_DIR})
if(LASTBLADE2_ROM_DIR)
    # Play the scripted sessions and record the entry points that only show
    # up at runtime (computed jumps into unrolled code), then rebuild.
    add_custom_target(profile
        COMMAND $<TARGET_FILE:lastblade2> --headless --rom-path ${LASTBLADE2_ROM_DIR} --frames 6000 --dump-misses ${LASTBLADE2_ENTRIES}
        COMMAND $<TARGET_FILE:lastblade2> --headless --rom-path ${LASTBLADE2_ROM_DIR} --frames 24000
                --input ${CMAKE_CURRENT_SOURCE_DIR}/tests/play_long.txt --dump-misses ${LASTBLADE2_ENTRIES}
        DEPENDS lastblade2
        COMMENT "Profiling a scripted session for runtime-only entry points")
    target_compile_definitions(lastblade2 PRIVATE LASTBLADE2_HAVE_RECOMP)
    if(MSVC)
        # Generated routines are large; /bigobj lifts the section limit.
        # /O1 /Ob1 keeps the giant dispatch functions tractable: MSVC's
        # optimizer grows superlinearly with them at /O2.
        set_source_files_properties(${GEN_SOURCES} PROPERTIES COMPILE_OPTIONS "/bigobj;/O1;/Ob1")
    elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
        set_source_files_properties(${GEN_SOURCES} PROPERTIES COMPILE_OPTIONS "-O2")
    else()
        # GCC: -O2 keeps the giant dispatch functions tractable; -O3 is
        # superlinear on them and gains little for this kind of code.
        set_source_files_properties(${GEN_SOURCES} PROPERTIES COMPILE_OPTIONS "-O2")
    endif()
endif()
if(MSVC)
    target_compile_definitions(lastblade2 PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(lastblade2_recomp PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Ship the SDL2 DLL (when the window is built) next to the executable.
if(WIN32 AND TARGET SDL2::SDL2)
    add_custom_command(TARGET lastblade2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:lastblade2> $<TARGET_FILE_DIR:lastblade2>
        COMMAND_EXPAND_LISTS)
endif()
