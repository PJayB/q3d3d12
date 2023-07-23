#
# Common
#
add_subdirectory(qabi)
add_subdirectory(qcommon)
add_subdirectory(qshared)
add_subdirectory(bgame)

target_link_libraries(qcommon PRIVATE cgame_public_qabi)
target_link_libraries(bgame PRIVATE qshared)

#
# VMs
#
add_subdirectory(game)
add_subdirectory(cgame)
add_subdirectory(ui)

target_link_libraries(game PRIVATE qshared game_public_qabi bgame)
target_link_libraries(cgame PRIVATE qshared cgame_public_qabi bgame)
target_link_libraries(ui PRIVATE qshared ui_public_qabi bgame)

#
# Libraries
#
add_subdirectory(account)
add_subdirectory(botlib)
add_subdirectory(client)
add_subdirectory(d3d)
add_subdirectory(d3d12)
add_subdirectory(gamethread)
add_subdirectory(q3_ui) # manifests client_ui
add_subdirectory(renderer)
add_subdirectory(server)
add_subdirectory(win)
add_subdirectory(xaudio)
add_subdirectory(xinput)

target_link_libraries(account cgame_public_qabi)
target_link_libraries(d3d12 d3d)
target_link_libraries(renderer d3d12)
target_link_libraries(server bgame game_public_qabi)
target_link_libraries(client bgame cgame_public_qabi renderer)
target_link_libraries(botlib qcommon qshared server client)
target_link_libraries(gamethread cgame_public_qabi)
target_link_libraries(client_ui PRIVATE qshared ui_public_qabi)
target_link_libraries(xinput PRIVATE cgame_public_qabi)
target_link_libraries(win PRIVATE cgame_public_qabi)

#
# Main executable
#
add_subdirectory(win32)
target_link_libraries(quake3 PRIVATE
    cgame_public_qabi
    game_public_qabi
    ui_public_qabi
    account
    botlib
    client
    client_ui
    d3d
    d3d12
    gamethread
    renderer
    server
    win
    xaudio
    xinput
)

