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
add_subdirectory(q3_ui)

target_link_libraries(game PRIVATE qshared game_public_qabi bgame)
target_link_libraries(cgame PRIVATE qshared cgame_public_qabi bgame)
target_link_libraries(ui PRIVATE qshared ui_public_qabi bgame)

#
# Libraries
#
add_subdirectory(account)
add_subdirectory(d3d)
add_subdirectory(d3d12)
add_subdirectory(renderer)
add_subdirectory(client)
add_subdirectory(botlib)
add_subdirectory(server)
add_subdirectory(gamethread)

target_link_libraries(account cgame_public_qabi)
target_link_libraries(d3d12 d3d)
target_link_libraries(renderer d3d12)
target_link_libraries(server bgame game_public_qabi)
target_link_libraries(client bgame cgame_public_qabi renderer)
target_link_libraries(botlib qcommon qshared server client)
target_link_libraries(gamethread cgame_public_qabi)

#
# Main executable
#

