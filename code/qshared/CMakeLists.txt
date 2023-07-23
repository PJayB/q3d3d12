add_library(qshared
    STATIC
    q_math.c
    q_shared.h
    q_shared.c
)

target_include_directories(qshared PUBLIC .)
