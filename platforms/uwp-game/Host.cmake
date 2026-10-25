# SPDX-License-Identifier: GPL-3.0-or-later
set(MELEE_UWP_HOST_BUILD "${MELEE_ROOT}/.local/uwp-host" CACHE PATH "Verified GCC host objects")
add_library(melee-uwp-host-cxx STATIC
    "${MELEE_ROOT}/src/pc/android_compat.cpp"
    "${MELEE_ROOT}/src/pc/file_cache.cpp"
    "${MELEE_ROOT}/src/pc/launcher.cpp"
    "${MELEE_ROOT}/src/pc/launcher_data.cpp"
    "${MELEE_ROOT}/src/pc/music_stream.cpp"
    "${MELEE_ROOT}/src/pc/textures.cpp"
    "${MELEE_ROOT}/src/pc/updater.cpp"
    "${MELEE_ROOT}/src/pc/version.cpp"
    "${MELEE_ROOT}/platforms/uwp/disc_browser.cpp"
    "${MELEE_ROOT}/platforms/uwp/runtime.cpp"
    "${MELEE_ROOT}/platforms/uwp/storage.cpp"
    "${MELEE_ROOT}/platforms/uwp/mingw_runtime.cpp"
    "${MELEE_ROOT}/platforms/uwp/uwp_io.cpp")
target_compile_definitions(melee-uwp-host-cxx PRIVATE MELEE_UWP=1 TARGET_PC=1 MELEE_PC=1 MELEE_STATE_SECTIONS=1 MELEE_USE_BCRYPT _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES MELEE_APP_VERSION="v0.4.2-uwp-dev")
target_include_directories(melee-uwp-host-cxx PUBLIC "${MELEE_ROOT}/extern/aurora/include" "${MELEE_ROOT}/platforms/uwp" "${MELEE_ROOT}/src" "${MELEE_ROOT}/src/sdk_include" "${MELEE_ROOT}/extern/monocypher")
target_compile_options(melee-uwp-host-cxx PRIVATE "/FI${MELEE_ROOT}/platforms/uwp/uwp_io.h" /EHsc /W2 /clang:-ffp-contract=off /clang:-fno-fast-math)
target_link_libraries(melee-uwp-host-cxx PUBLIC aurora::core aurora::gx aurora::gd aurora::vi aurora::pad aurora::si aurora::mtx aurora::os aurora::dvd aurora::card aurora::thp sqlite3 LIBUWP WindowsApp legacy_stdio_definitions bcrypt winhttp ws2_32 iphlpapi)

# Store-CRT game objects keep original rollback exclusions and writable ranges.
set(MELEE_UWP_GAME_BUILD "${MELEE_ROOT}/.local/uwp-gamecode-store" CACHE PATH "Verified GCC game objects")
foreach(kind game host)
    if(kind STREQUAL "game")
        set(object_root "${MELEE_UWP_GAME_BUILD}")
    else()
        set(object_root "${MELEE_UWP_HOST_BUILD}")
    endif()
    file(READ "${object_root}/result.json" report)
    string(JSON count GET "${report}" units)
    string(JSON passed GET "${report}" passed)
    if(NOT count EQUAL passed)
        message(FATAL_ERROR "The ${kind} GCC object compilation is incomplete")
    endif()
    string(JSON total LENGTH "${report}" objects)
    math(EXPR last "${total}-1")
    set(coff_objects)
    foreach(index RANGE 0 ${last})
        string(JSON source GET "${report}" objects ${index} source)
        string(REPLACE "/" "_" object_name "${source}")
        set(object "${object_root}/objects/${object_name}.obj")
        if(NOT EXISTS "${object}")
            message(FATAL_ERROR "Missing GCC object: ${source}")
        endif()
        list(APPEND coff_objects "${object}")
    endforeach()
    set_source_files_properties(${coff_objects} PROPERTIES EXTERNAL_OBJECT TRUE)
    add_library(melee-uwp-${kind}-c STATIC ${coff_objects})
    set_target_properties(melee-uwp-${kind}-c PROPERTIES LINKER_LANGUAGE CXX)
endforeach()
set(MELEE_UWP_ENTRY_SOURCE "${MELEE_ROOT}/platforms/uwp-game/entry.cpp" CACHE FILEPATH "Application entry source (override only for local smoke tests)")
add_executable(melee-uwp WIN32 "${MELEE_UWP_ENTRY_SOURCE}")
target_compile_definitions(melee-uwp PRIVATE MELEE_UWP=1)
target_compile_options(melee-uwp PRIVATE /EHsc /W2)
target_link_libraries(melee-uwp PRIVATE melee-uwp-host-c melee-uwp-game-c melee-uwp-host-cxx)
target_link_options(melee-uwp PRIVATE /APPCONTAINER /STACK:16777216 /alternatename:___chkstk_ms=__chkstk /debug:symtab "/map:${CMAKE_BINARY_DIR}/melee-uwp.map")

# Stage an independent test application; never includes the user's disc image.
set(MELEE_UWP_PUBLISHER "CN=WWHDRecompDevelopment" CACHE STRING "Test signing publisher")
set(MELEE_UWP_STAGE "${CMAKE_BINARY_DIR}/package")
file(MAKE_DIRECTORY "${MELEE_UWP_STAGE}")
configure_file("${CMAKE_CURRENT_LIST_DIR}/Package.appxmanifest.in" "${MELEE_UWP_STAGE}/AppxManifest.xml" @ONLY)
add_custom_command(TARGET melee-uwp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:melee-uwp>" "${MELEE_UWP_STAGE}/melee-uwp.exe"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${MELEE_ROOT}/resources" "${MELEE_UWP_STAGE}/resources"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${MELEE_ROOT}/platforms/uwp-probe/Assets" "${MELEE_UWP_STAGE}/Assets")
foreach(dll SDL3 libuwp webgpu_dawn dxcompiler dxil nod libpng16 z)
    add_custom_command(TARGET melee-uwp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${MELEE_UWP_DEP_ROOT}/x64/bin/${dll}.dll" "${MELEE_UWP_STAGE}/${dll}.dll")
endforeach()

set_source_files_properties("${MELEE_ROOT}/platforms/uwp/uwp_io.cpp" PROPERTIES COMPILE_DEFINITIONS MELEE_UWP_IO_IMPLEMENTATION)
