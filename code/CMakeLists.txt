#
# Common
#
add_subdirectory(qabi)
add_subdirectory(qcommon)
add_subdirectory(qshared)
target_link_libraries(qcommon cgame_public_qabi)

add_subdirectory(server)
target_link_libraries(server game_public_qabi)

add_subdirectory(d3d)

add_subdirectory(d3d12)
target_link_libraries(d3d12 d3d)

add_subdirectory(renderer)
target_link_libraries(renderer d3d12)

add_subdirectory(client)
target_link_libraries(client cgame_public_qabi renderer)

add_subdirectory(botlib)
target_link_libraries(botlib qcommon qshared server client)

#add_subdirectory(game)


#
# VMs
#
#add_subdirectory(cgame)

#
# Libraries
#
#add_subdirectory(account)

#target_link_libraries(qcommon cgame)

#
# Main executable
#

