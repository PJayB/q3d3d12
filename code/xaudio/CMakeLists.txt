add_library(q3_xaudio
    STATIC
    xaudio_local.h
    xaudio_main.cpp
    xaudio_public.h
)

target_link_libraries(q3_xaudio PUBLIC xaudio2_8)
