cmake_minimum_required(VERSION 3.25)
set(CMAKE_SYSTEM_NAME WindowsStore)
set(CMAKE_SYSTEM_VERSION 10.0)
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
set(CMAKE_C_STANDARD_LIBRARIES "WindowsApp.lib" CACHE STRING "UWP platform import library")
set(CMAKE_CXX_STANDARD_LIBRARIES "WindowsApp.lib" CACHE STRING "UWP platform import library")
project(melee_uwp_aurora LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static host dependencies" FORCE)
get_filename_component(MELEE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
include(Dependencies.cmake)
add_compile_definitions(WINAPI_FAMILY=WINAPI_FAMILY_APP _CRT_APP NOMINMAX UNICODE _UNICODE AURORA_WINDOWS_STORE)
add_compile_options(/utf-8)
set(AURORA_DAWN_PROVIDER system CACHE STRING "" FORCE)
set(AURORA_SDL3_PROVIDER system CACHE STRING "" FORCE)
set(AURORA_NOD_PROVIDER system CACHE STRING "" FORCE)
set(AURORA_ENABLE_RMLUI ON CACHE BOOL "Preserve the original launcher" FORCE)
set(AURORA_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
set(AURORA_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
# Providers write default backend cache entries without FORCE. Keep the UWP
# renderer restricted to the backend supported by this dependency depot.
set(DAWN_ENABLE_D3D12 ON CACHE INTERNAL "" FORCE)
foreach(backend D3D11 VULKAN METAL DESKTOP_GL OPENGLES NULL)
    set(DAWN_ENABLE_${backend} OFF CACHE INTERNAL "" FORCE)
endforeach()
add_subdirectory("${MELEE_ROOT}/extern/aurora" aurora)
target_link_libraries(aurora_core PRIVATE LIBUWP WindowsApp)

# FreeType's desktop Windows file backend uses CreateFileA, unavailable in
# AppContainer. Use its upstream portable CRT backend instead.
if(TARGET freetype)
    get_target_property(ft_sources freetype SOURCES)
    get_target_property(ft_root freetype SOURCE_DIR)
    list(FILTER ft_sources EXCLUDE REGEX "builds/windows/ftsystem[.]c$")
    list(APPEND ft_sources "${ft_root}/src/base/ftsystem.c")
    set_property(TARGET freetype PROPERTY SOURCES "${ft_sources}")
endif()

if(TARGET sqlite3)
    target_compile_definitions(sqlite3 PRIVATE SQLITE_OS_WINRT=1)
endif()

# Match upstream simulation-adjacent FP rules and trig routing.
foreach(component aurora_mtx aurora_gx aurora_gd)
    target_compile_options(${component} PRIVATE /clang:-ffp-contract=off /clang:-fno-fast-math)
endforeach()
target_compile_options(aurora_mtx PRIVATE
    /clang:-fno-builtin-sinf /clang:-fno-builtin-cosf
    /clang:-fno-builtin-tanf /clang:-fno-builtin-atanf
    "/FI${MELEE_ROOT}/src/pc/libm/pc_trig.h")

option(MELEE_UWP_BUILD_HOST "Build the original Melee native host" ON)
if(MELEE_UWP_BUILD_HOST)
    include("${MELEE_ROOT}/platforms/uwp-game/Host.cmake")
endif()

foreach(source lib/io.cpp lib/dolphin/dvd/dvd.cpp lib/dolphin/card.cpp lib/card/FileIO.cpp lib/card/CardGciFolder.cpp lib/card/CardRawFile.cpp)
    set_property(SOURCE "${MELEE_ROOT}/extern/aurora/${source}" DIRECTORY "${MELEE_ROOT}/extern/aurora" APPEND PROPERTY COMPILE_OPTIONS "/FI${MELEE_ROOT}/platforms/uwp/uwp_io.h")
endforeach()
