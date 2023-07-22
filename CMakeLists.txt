cmake_minimum_required(VERSION 3.22)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif ()

# todo: remove me
set(CMAKE_COMPILE_WARNING_AS_ERROR OFF)


project(q3d3d12
    VERSION 0.0.1
    LANGUAGES C CXX
    HOMEPAGE_URL "https://github.com/PJayB/q3d3d12"
    )

# todo: tools subdirectory

add_subdirectory(code)
