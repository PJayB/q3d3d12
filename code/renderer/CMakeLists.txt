add_library(q3_renderer
    STATIC
    # gl_common.c
    # gl_common.h
    # gl_driver.c
    # gl_image.c
    # gl_shade.c
    # gl_shadows.c
    # proxy_main.c
    # proxy_main.h
    # qgl.h
    # qgl_linked.h
    tr_animation.c
    tr_backend.c
    tr_bsp.c
    tr_cmds.c
    tr_curve.c
    tr_flares.c
    tr_font.c
    tr_image.c
    tr_init.c
    tr_layer.c
    tr_layer.h
    tr_light.c
    tr_local.h
    tr_main.c
    tr_marks.c
    tr_mesh.c
    tr_model.c
    tr_noise.c
    tr_public.h
    tr_scene.c
    tr_shade.c
    tr_shade_calc.c
    tr_shader.c
    tr_sky.c
    tr_smp.c
    tr_state.c
    tr_state.h
    tr_surface.c
    tr_world.c
)

# TODO: enable D3D11 using an option or something

target_compile_definitions(q3_renderer PRIVATE
    # Q_HAS_D3D11
    Q_HAS_D3D12
    Q_SKIP_DLIGHT_PROJECTION
)
