add_library(q3_bgame
    STATIC
    # bg_lib.c
    bg_lib.h
    bg_local.h
    bg_misc.c
    bg_pmove.c
    bg_public.h
    bg_slidemove.c
)

target_include_directories(q3_bgame PUBLIC .)
