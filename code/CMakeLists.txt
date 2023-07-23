#
# Common
#
add_subdirectory(qabi)
add_subdirectory(qcommon)
add_subdirectory(qshared)
add_subdirectory(bgame)

target_link_libraries(q3_common PRIVATE cgame_public_qabi)
target_link_libraries(q3_bgame PRIVATE q3_shared)

#
# VMs
#
add_subdirectory(game)
add_subdirectory(cgame)
add_subdirectory(q3_ui)
add_subdirectory(ui)

target_link_libraries(vm_game PRIVATE q3_shared game_public_qabi q3_bgame)
target_link_libraries(vm_cgame PRIVATE q3_shared cgame_public_qabi q3_bgame)
target_link_libraries(vm_ui PRIVATE q3_shared ui_public_qabi q3_bgame)
target_link_libraries(vm_ui_2 PRIVATE q3_shared ui_public_qabi q3_bgame)

#
# Libraries
#
add_subdirectory(account)
add_subdirectory(botlib)
add_subdirectory(client)
add_subdirectory(d3d)
add_subdirectory(d3d12)
add_subdirectory(gamethread)
add_subdirectory(renderer)
add_subdirectory(server)
add_subdirectory(win)
add_subdirectory(xaudio)
add_subdirectory(xinput)

target_link_libraries(q3_account PRIVATE cgame_public_qabi)
target_link_libraries(q3_d3d12 PRIVATE q3_d3d)
target_link_libraries(q3_renderer PRIVATE q3_d3d12)
target_link_libraries(q3_server PRIVATE q3_bgame game_public_qabi)
target_link_libraries(q3_client PRIVATE q3_bgame cgame_public_qabi q3_renderer)
target_link_libraries(q3_botlib PRIVATE q3_common q3_shared q3_server q3_client)
target_link_libraries(q3_gamethread PRIVATE cgame_public_qabi)
target_link_libraries(q3_xinput PRIVATE cgame_public_qabi)
target_link_libraries(q3_win PRIVATE cgame_public_qabi)

#
# Main executable
#
add_subdirectory(win32)
target_link_libraries(quake3 PRIVATE
    # Internal
    cgame_public_qabi
    game_public_qabi
    ui_public_qabi
    q3_account
    q3_botlib
    q3_client
    q3_d3d
    q3_d3d12
    q3_gamethread
    q3_renderer
    q3_server
    q3_win
    q3_xaudio
    q3_xinput

    # System
    d3d12
    dxgi
    ws2_32
)

target_compile_definitions(quake3 PRIVATE
    # Q_HAS_D3D11
    Q_HAS_D3D12
)
