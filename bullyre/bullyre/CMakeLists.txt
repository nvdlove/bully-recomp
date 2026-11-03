cmake_minimum_required(VERSION 3.24)
project(bullyre LANGUAGES C CXX ASM_MASM)

# While the original game runs our code (the mixed phase), bullyre.dll is loaded into
# the 32-bit game: it has to be built for Win32. The 64-bit build comes once nothing
# of the original is left.
# BULLYRE_X64_EXPERIMENT=ON (with -A x64) configures only the libraries and the object
# library of the reversed code, to see what a 64-bit compile trips over (roadmap step 7).
option(BULLYRE_X64_EXPERIMENT "compile the reversed code as x64 to survey the errors" OFF)
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4 AND NOT BULLYRE_X64_EXPERIMENT)
    message(FATAL_ERROR "bullyre.dll goes into the 32-bit game: configure with -A Win32")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# the CRT linked in: nothing extra to load while the game is still starting up
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)
FetchContent_Declare(minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG master
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(minhook)

# Lua 5.0.2 from its source, configured as the game's copy was (third_party/lua-5.0.2/BULLY.md):
# it replaces the game's (src/game/LuaLibrary.cpp registers it at the game's addresses).
# Without the parser, as the game: noparser.c stands in for lcode/llex/lparser.
file(GLOB LUA_SOURCES CONFIGURE_DEPENDS third_party/lua-5.0.2/src/*.c)
add_library(lua502 STATIC ${LUA_SOURCES})
target_include_directories(lua502 PUBLIC third_party/lua-5.0.2/include PRIVATE third_party/lua-5.0.2/src)
target_compile_definitions(lua502 PUBLIC LUA_USER_H="bully_lua.h" PRIVATE _CRT_SECURE_NO_WARNINGS)
target_compile_options(lua502 PRIVATE /W3 /wd4244 /wd4267 /wd4018 /wd4334)
# math.random and math.randomseed draw from the game's C runtime, as the game's copy does
# (one sequence with the game's own rand calls)
set_source_files_properties(third_party/lua-5.0.2/src/lmathlib.c PROPERTIES
    COMPILE_DEFINITIONS "rand=bully_lua_rand;srand=bully_lua_srand")

# The IJG JPEG library, release 6b, as the game built it for the photo album
# (third_party/jpeg-6b/BULLY.md; src/game/JpegLibrary.cpp registers it)
file(GLOB JPEG_SOURCES CONFIGURE_DEPENDS third_party/jpeg-6b/*.c)
add_library(jpeg6b STATIC ${JPEG_SOURCES})
target_include_directories(jpeg6b PUBLIC third_party/jpeg-6b)
target_compile_definitions(jpeg6b PRIVATE _CRT_SECURE_NO_WARNINGS)
target_compile_options(jpeg6b PRIVATE /W3 /wd4244 /wd4267 /wd4018)

# FAudio (third_party/FAudio, its VERSION file says which), for XACT: its FACT reads the game's
# XACT 3.1 content (src/game/XactShim.cpp puts XACT's interfaces over it). The native Win32
# backend (WASAPI), no SDL; no WMA, no XNA songs.
file(GLOB FAUDIO_SOURCES CONFIGURE_DEPENDS third_party/FAudio/src/*.c)
add_library(faudio STATIC ${FAUDIO_SOURCES})
target_include_directories(faudio PUBLIC third_party/FAudio/include PRIVATE third_party/FAudio/src)
target_compile_definitions(faudio PUBLIC FAUDIO_WIN32_PLATFORM FAUDIO_STATIC PRIVATE DISABLE_XNASONG _CRT_SECURE_NO_WARNINGS
    $<$<CONFIG:Release>:FAUDIO_DISABLE_DEBUGCONFIGURATION>)
target_compile_options(faudio PRIVATE /W3 /wd4244 /wd4267 /wd4018 /wd4305)
target_link_libraries(faudio PUBLIC dxguid uuid winmm ole32 advapi32 user32 mfplat mfreadwrite mfuuid propsys)

# every reversed subsystem in src/game, with the registry and the startup: compiled once,
# linked into both ways in (bullyre.dll and the host exe)
file(GLOB GAME_SOURCES CONFIGURE_DEPENDS src/game/*.cpp src/gamebryo/*.cpp)
add_library(bullyre_code OBJECT
    src/log.cpp
    src/reversed.cpp
    src/startup.cpp
    ${GAME_SOURCES}
)
target_include_directories(bullyre_code PUBLIC src)
target_link_libraries(bullyre_code PUBLIC minhook lua502 jpeg6b faudio)
target_compile_options(bullyre_code PUBLIC $<$<COMPILE_LANGUAGE:C,CXX>:/W4 /permissive- /Zc:__cplusplus /utf-8 /EHsc>)
target_compile_definitions(bullyre_code PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)

if(BULLYRE_X64_EXPERIMENT)
# bullyre_x64.exe: the first 64-bit program of the reversed code (src/x64/main.cpp says what it
# does). Based below 2 GB and fixed; large-address-aware (2026-11-01: the remaster's memory):
# what the game's 4-byte pointer fields point at comes from the low heap (src/x64/lowheap.cpp,
# below 2 GB: the game's malloc, all our new/delete), the system's own allocations (the driver,
# textures, sound) may go anywhere, and re::Ptr32 stops the game at a store of a pointer that
# doesn't fit (README roadmap step 7).
# Linked at the game's base with the game's range reserved at the start of .text
# (src/x64/reserve64.asm), as the 32-bit host is.
add_executable(bullyre_x64 src/x64/main.cpp src/x64/dispatch.cpp src/x64/crt64.cpp src/x64/lowheap.cpp src/x64/reserve64.asm)
target_link_libraries(bullyre_x64 PRIVATE bullyre_code)
target_link_options(bullyre_x64 PRIVATE /BASE:0x400000 /FIXED /DYNAMICBASE:NO /HIGHENTROPYVA:NO
    /LARGEADDRESSAWARE /MAP /DEBUG)
# debug information for the bring-up (the debugger's names and lines: tools/cdb, tests)
target_compile_options(bullyre_code PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:/Z7>)
target_compile_options(bullyre_x64 PRIVATE /Z7)
endif()

if(NOT BULLYRE_X64_EXPERIMENT)
# bullyre.dll: loaded into the original game by bullymenu
add_library(bullyre SHARED src/dllmain.cpp)
target_link_libraries(bullyre PRIVATE bullyre_code)
# a map file: crash addresses inside bullyre.dll resolve to functions (tools/crashmap.py)
target_link_options(bullyre PRIVATE /MAP)

# bullyre_host.exe: the game run from our own exe (src/host/host.cpp says how). Linked at the
# game's base with the game's image reserved at the start of .text (reserve.asm); HostEntry
# maps the game before our C runtime starts (no stack cookies there: the cookie isn't set
# yet). Loader settings as the original's: no DEP, no ASLR, no SafeSEH table (the game's
# exception handlers live in this image now), no UAC manifest; large-address-aware like
# bullymenu's copy.
add_executable(bullyre_host WIN32 src/host/host.cpp src/host/reserve.asm src/host/host.manifest)
target_link_libraries(bullyre_host PRIVATE bullyre_code shell32)
set_source_files_properties(src/host/host.cpp PROPERTIES COMPILE_OPTIONS "/GS-")
target_link_options(bullyre_host PRIVATE /ENTRY:HostEntry /BASE:0x400000 /FIXED /DYNAMICBASE:NO /NXCOMPAT:NO
    /SAFESEH:NO /LARGEADDRESSAWARE /MANIFESTUAC:NO /STACK:0x100000 /MAP)

# Unit tests of reversed code that can run outside the game (tests/unit)
add_executable(bullyre_unit tests/unit/tree_test.cpp src/log.cpp src/reversed.cpp src/game/TxdStore.cpp
               src/game/SeamlessDoors.cpp)
target_include_directories(bullyre_unit PRIVATE src)
target_link_libraries(bullyre_unit PRIVATE minhook)
target_compile_options(bullyre_unit PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8 /EHsc)
target_compile_definitions(bullyre_unit PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
target_link_options(bullyre_unit PRIVATE /DYNAMICBASE:NO)

# Lua from its source against every one of the game's scripts (tests/unit/lua_scripts_test.cpp)
add_executable(bullyre_luatest tests/unit/lua_scripts_test.cpp)
target_link_libraries(bullyre_luatest PRIVATE lua502)
target_compile_options(bullyre_luatest PRIVATE /W4 /permissive- /utf-8 /EHsc)
target_compile_definitions(bullyre_luatest PRIVATE _CRT_SECURE_NO_WARNINGS)

# FAudio's FACT against every one of the game's XACT banks (tests/unit/xact_banks_test.cpp)
add_executable(bullyre_xacttest tests/unit/xact_banks_test.cpp)
target_link_libraries(bullyre_xacttest PRIVATE faudio)
target_compile_options(bullyre_xacttest PRIVATE /W4 /permissive- /utf-8 /EHsc)

# The XACT shim driven through XACT's vtable slots as the game's code does (tests/unit/xact_shim_test.cpp)
add_executable(bullyre_xactshimtest tests/unit/xact_shim_test.cpp src/game/XactShim.cpp)
target_include_directories(bullyre_xactshimtest PRIVATE src)
target_link_libraries(bullyre_xactshimtest PRIVATE faudio ole32)
target_compile_options(bullyre_xactshimtest PRIVATE /W4 /permissive- /utf-8 /EHsc)
endif()   # NOT BULLYRE_X64_EXPERIMENT
