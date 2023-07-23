add_library(q3_shared
    STATIC
    q_math.c
    q_shared.h
    q_shared.c
)

target_include_directories(q3_shared PUBLIC .)
