cmake_minimum_required(VERSION 3.22)

project(quake3
    VERSION 1.0.0
    LANGUAGES C CXX
    HOMEPAGE_URL "https://github.com/PJayB/q3d3d12"
)

if (WIN32)
	add_definitions(-D_USE_MATH_DEFINES)
	add_definitions(-DNOMINMAX)
    add_definitions(-D_WIN32)
    add_definitions(-DWINVER=0x500)
endif()

#
# VMs
#
add_subdirectory(cgame)

#
# Libraries
#
add_subdirectory(qcommon)
add_subdirectory(account)

target_link_libraries(qcommon cgame)

#
# Main exectuable
#

