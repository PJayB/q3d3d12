add_library(bgame
    STATIC
    # bg_lib.c
    bg_lib.h
    bg_local.h
    bg_misc.c
    bg_pmove.c
    bg_public.h
    bg_slidemove.c
)

target_include_directories(bgame PUBLIC .)
