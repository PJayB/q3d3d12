add_library(server
    STATIC
    server.h
    sv_ccmds.c
    sv_net_chan.c
    sv_client.c
    sv_snapshot.c
    # sv_rankings.c
    sv_bot.c
    sv_game.c
    sv_init.c
    sv_world.c
    sv_main.c
)
