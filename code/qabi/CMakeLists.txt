add_library(
    cgame_public_qabi
    INTERFACE
)

target_include_directories(
    cgame_public_qabi
    INTERFACE
    ${CMAKE_BINARY_DIR}/qabi
)

add_library(
    game_public_qabi
    INTERFACE
)

target_include_directories(
    game_public_qabi
    INTERFACE
    ${CMAKE_BINARY_DIR}/qabi
)

