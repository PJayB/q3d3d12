add_library(q3_xinput
    STATIC
    xinput_main.c
    xinput_public.h
)

target_link_libraries(q3_xinput PUBLIC xinput)
