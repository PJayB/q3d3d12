set(MINGW32 1)
set(WIN32 1)
set(CMAKE_SYSTEM_NAME Windows)

add_definitions(-D_USE_MATH_DEFINES)
add_definitions(-DNOMINMAX)
add_definitions(-D_WIN32)
add_definitions(-DWINVER=_WIN32_WINNT_WIN10)
add_definitions(-D_WIN32_WINNT=_WIN32_WINNT_WIN10)

#
# todo: detect whether we're building in Msys or not
#

if ("${MINGW_USE_NATIVE_GCC}" STREQUAL "1")
    set(CMAKE_C_COMPILER gcc)
    set(CMAKE_CXX_COMPILER g++)
    set(CMAKE_RC_COMPILER windres)
    set(CMAKE_RANLIB ranlib)
else()
    set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc-posix)
    set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++-posix)
    set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
    set(CMAKE_RANLIB x86_64-w64-mingw32-ranlib)
endif()
