add_library(
    cgame
    SHARED
    cg_consolecmds.c
    cg_draw.c
    cg_drawtools.c
    cg_effects.c
    cg_ents.c
    cg_event.c
    cg_info.c
    cg_localents.c
    cg_main.c
    cg_marks.c
    # todo: audit these -- why were they stubbed out?
    #cg_newdraw.c
    #cg_particles.c
    cg_players.c
    cg_playerstate.c
    cg_predict.c
    cg_scoreboard.c
    cg_servercmds.c
    cg_snapshot.c
    cg_syscalls.c
    cg_view.c
    cg_weapons.c
)

target_include_directories(
    cgame
    PUBLIC
    # todo: make this a variable
    ${CMAKE_CURRENT_BUILD_DIR}/qabi/Q3_VS2015_Desktop/Debug
)
