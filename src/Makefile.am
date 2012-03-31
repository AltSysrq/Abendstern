if ENABLE_GL14
GL14PROG=abendsternuxgl14
else
GL14PROG=
endif
if ENABLE_GL21
GL21PROG=abendsternuxgl21
else
GL21PROG=
endif
if ENABLE_GL32
GL32PROG=abendsternuxgl32
else
GL32PROG=
endif

COMMONCXXFLAGS=-Wall -I./config $(SDL_CFLAGS) $(SDLIMG_CFLAGS) $(GL_CFLAGS) $(PNG_CFLAGS) -I/usr/include/tcl8.5

./tcl_iface/bridge.cxx: ./tcl_iface/definition.tcl ./tcl_iface/generate.tcl
	./tcl_iface/generate.tcl
./tcl_iface/xxx/x0.cxx: | ./tcl_iface/bridge.cxx ./tcl_iface/definition.tcl ./tcl_iface/generate.tcl
./tcl_iface/xxx/x1.cxx: | ./tcl_iface/bridge.cxx ./tcl_iface/definition.tcl ./tcl_iface/generate.tcl
./tcl_iface/xxx/x2.cxx: | ./tcl_iface/bridge.cxx ./tcl_iface/definition.tcl ./tcl_iface/generate.tcl
./tcl_iface/xxx/x3.cxx: | ./tcl_iface/bridge.cxx ./tcl_iface/definition.tcl ./tcl_iface/generate.tcl
./tcl_iface/xxx/x4.cxx: | ./tcl_iface/bridge.cxx ./tcl_iface/definition.tcl ./tcl_iface/generate.tcl
./tcl_iface/xxx/x5.cxx: | ./tcl_iface/bridge.cxx ./tcl_iface/definition.tcl ./tcl_iface/generate.tcl
./tcl_iface/xxx/x6.cxx: | ./tcl_iface/bridge.cxx ./tcl_iface/definition.tcl ./tcl_iface/generate.tcl
./tcl_iface/xxx/x7.cxx: | ./tcl_iface/bridge.cxx ./tcl_iface/definition.tcl ./tcl_iface/generate.tcl
./tcl_iface/xxx/x8.cxx: | ./tcl_iface/bridge.cxx ./tcl_iface/definition.tcl ./tcl_iface/generate.tcl
./tcl_iface/xxx/x9.cxx: | ./tcl_iface/bridge.cxx ./tcl_iface/definition.tcl ./tcl_iface/generate.tcl
./tcl_iface/xxx/xa.cxx: | ./tcl_iface/bridge.cxx ./tcl_iface/definition.tcl ./tcl_iface/generate.tcl
./tcl_iface/xxx/xb.cxx: | ./tcl_iface/bridge.cxx ./tcl_iface/definition.tcl ./tcl_iface/generate.tcl
./tcl_iface/xxx/xc.cxx: | ./tcl_iface/bridge.cxx ./tcl_iface/definition.tcl ./tcl_iface/generate.tcl
./tcl_iface/xxx/xd.cxx: | ./tcl_iface/bridge.cxx ./tcl_iface/definition.tcl ./tcl_iface/generate.tcl
./tcl_iface/xxx/xe.cxx: | ./tcl_iface/bridge.cxx ./tcl_iface/definition.tcl ./tcl_iface/generate.tcl
./tcl_iface/xxx/xf.cxx: | ./tcl_iface/bridge.cxx ./tcl_iface/definition.tcl ./tcl_iface/generate.tcl

./net/xxx/xnetobj.cxx: ./net/definition.tcl ./net/generate.tcl
	./net/generate.tcl
./net/xxx/xnetobj.hxx: | ./net/xxx/xnetobj.cxx ./net/definition.tcl ./net/generate.tcl

bin_PROGRAMS=$(GL14PROG) $(GL21PROG) $(GL32PROG)
COMMONLDADD=$(SDL_LIBS) $(SDLIMG_LIBS) $(GL_LIBS) $(PNG_LIBS)
COMMONSOURCES= \
./tcl_iface/bridge.cxx \
./tcl_iface/xxx/x0.cxx \
./tcl_iface/xxx/x1.cxx \
./tcl_iface/xxx/x2.cxx \
./tcl_iface/xxx/x3.cxx \
./tcl_iface/xxx/x4.cxx \
./tcl_iface/xxx/x5.cxx \
./tcl_iface/xxx/x6.cxx \
./tcl_iface/xxx/x7.cxx \
./tcl_iface/xxx/x8.cxx \
./tcl_iface/xxx/x9.cxx \
./tcl_iface/xxx/xa.cxx \
./tcl_iface/xxx/xb.cxx \
./tcl_iface/xxx/xc.cxx \
./tcl_iface/xxx/xd.cxx \
./tcl_iface/xxx/xe.cxx \
./tcl_iface/xxx/xf.cxx \
./audio/wavload.cxx \
./audio/ship_mixer.cxx \
./audio/synth.cxx \
./audio/audio.cxx \
./audio/ship_effects.cxx \
./weapon/energy_charge.cxx \
./weapon/semiguided_bomb.cxx \
./weapon/monophasic_energy_pulse.cxx \
./weapon/particle_beam.cxx \
./weapon/plasma_burst.cxx \
./weapon/magneto_bomb.cxx \
./weapon/missile.cxx \
./core/init_state.cxx \
./core/game_state.cxx \
./core/lxn.cxx \
./core/aobject.cxx \
./test_state.cxx \
./control/genetic_ai.cxx \
./control/ai/mod_misc/aim_self_destruct.cxx \
./control/ai/mod_core/aim_procedure.cxx \
./control/ai/mod_core/aim_idle.cxx \
./control/ai/mod_core/aim_varset.cxx \
./control/ai/mod_introspection/aim_intro_throttle.cxx \
./control/ai/mod_introspection/aim_intro_weapon.cxx \
./control/ai/mod_introspection/aim_intro_engines.cxx \
./control/ai/mod_attack/aim_basic_aim.cxx \
./control/ai/mod_attack/aim_fire_weapon.cxx \
./control/ai/mod_attack/aim_select_weapon.cxx \
./control/ai/mod_attack/aim_weapon_level.cxx \
./control/ai/mod_attack/aim_veto_fire.cxx \
./control/ai/mod_state/aim_state_by_target_dist.cxx \
./control/ai/mod_state/aim_edge_detect.cxx \
./control/ai/mod_state/aim_goto.cxx \
./control/ai/mod_state/aim_detect_overlap.cxx \
./control/ai/mod_state/aim_indgoto.cxx \
./control/ai/mod_state/aim_check_has_weapons.cxx \
./control/ai/mod_target/aim_target_nearest.cxx \
./control/ai/aimod.cxx \
./control/ai/mod_navigation/aim_park.cxx \
./control/ai/mod_navigation/aim_nav_to_target.cxx \
./control/ai/mod_navigation/aim_run_away_simple.cxx \
./control/ai/mod_navigation/aim_avoid_edge.cxx \
./control/ai/aictrl.cxx \
./control/hc_conf.cxx \
./control/genai/ci_self.cxx \
./control/genai/c_aiming.cxx \
./control/genai/c_reflex.cxx \
./control/genai/genai.cxx \
./control/genai/c_weapon.cxx \
./control/genai/ci_cellt.cxx \
./control/genai/c_frontal.cxx \
./control/genai/ci_weapon.cxx \
./control/genai/c_dodge.cxx \
./control/genai/ci_emotion.cxx \
./control/genai/c_strat_weapon.cxx \
./control/genai/c_target_analysis.cxx \
./control/genai/ci_objective.cxx \
./control/genai/c_run_away.cxx \
./control/genai/c_weapon_level.cxx \
./control/genai/c_navigation.cxx \
./control/genai/c_avoid_edge.cxx \
./control/genai/c_opp_weapon.cxx \
./control/genai/cortex.cxx \
./control/genai/ci_cellex.cxx \
./control/genai/ci_field.cxx \
./control/human_controller.cxx \
./globals.cxx \
./ship/shipio.cxx \
./ship/sys/system_textures.cxx \
./ship/sys/a/dispersion_shield.cxx \
./ship/sys/a/missile_launcher.cxx \
./ship/sys/a/rel_ion_accelerator.cxx \
./ship/sys/a/antimatter_power.cxx \
./ship/sys/a/particle_beam_launcher.cxx \
./ship/sys/a/cloaking_device.cxx \
./ship/sys/a/gatling_plasma_burst_launcher.cxx \
./ship/sys/a/monophasic_energy_emitter.cxx \
./ship/sys/ship_system.cxx \
./ship/sys/power_plant.cxx \
./ship/sys/c/self_destruct_charge.cxx \
./ship/sys/c/energy_charge_launcher.cxx \
./ship/sys/c/particle_accelerator.cxx \
./ship/sys/c/mini_gravwave_drive.cxx \
./ship/sys/c/power_cell.cxx \
./ship/sys/c/capacitor.cxx \
./ship/sys/c/reinforcement_bulkhead.cxx \
./ship/sys/c/magneto_bomb_launcher.cxx \
./ship/sys/c/fission_power.cxx \
./ship/sys/engine.cxx \
./ship/sys/b/fusion_power.cxx \
./ship/sys/b/heatsink.cxx \
./ship/sys/b/plasma_burst_launcher.cxx \
./ship/sys/b/mini_gravewave_drive_mkii.cxx \
./ship/sys/b/shield_generator.cxx \
./ship/sys/b/semiguided_bomb_launcher.cxx \
./ship/sys/b/bussard_ramjet.cxx \
./ship/sys/b/super_particle_accelerator.cxx \
./ship/sys/launcher.cxx \
./ship/auxobj/cell_fragment.cxx \
./ship/auxobj/shield.cxx \
./ship/auxobj/plasma_fire.cxx \
./ship/ship.cxx \
./ship/render_quad.cxx \
./ship/ship_renderer.cxx \
./ship/editor/system_creation_rectangle.cxx \
./ship/editor/cell_creation_brush.cxx \
./ship/editor/system_rotator.cxx \
./ship/editor/cell_rotator.cxx \
./ship/editor/manipulator.cxx \
./ship/editor/cell_deletion_brush.cxx \
./ship/editor/abstract_system_painter.cxx \
./ship/editor/system_info_copier.cxx \
./ship/editor/system_creation_brush.cxx \
./ship/editor/bridge_relocator.cxx \
./ship/cell/rightt_cell.cxx \
./ship/cell/equt_cell.cxx \
./ship/cell/cell.cxx \
./ship/cell/circle_cell.cxx \
./ship/cell/empty_cell.cxx \
./ship/cell/square_cell.cxx \
./ship/insignia.cxx \
./abendstern.cxx \
./camera/camera.cxx \
./camera/hud.cxx \
./camera/fixed_camera.cxx \
./camera/forwarding_effects_handler.cxx \
./camera/spectator.cxx \
./camera/effects_handler.cxx \
./camera/dynamic_camera.cxx \
./config/libconfig.cxx \
./background/explosion_pool.cxx \
./background/background_object.cxx \
./background/nebula.cxx \
./background/old_style_explosion.cxx \
./background/star.cxx \
./background/explosion.cxx \
./background/planet.cxx \
./background/star_field.cxx \
./sim/blast.cxx \
./sim/game_object.cxx \
./sim/collision.cxx \
./sim/game_field.cxx \
./sim/object_intref.cxx \
./sim/game_env.cxx \
./sim/lazy_transform_collision_tree.cxx \
./net/crypto.cxx \
./net/globalid.cxx \
./net/antenna.cxx \
./net/tuner.cxx \
./net/connection_listener.cxx \
./net/network_assembly.cxx \
./net/network_connection.cxx \
./net/synchronous_control_geraet.cxx \
./net/async_ack_geraet.cxx \
./net/game_advertiser.cxx \
./net/game_discoverer.cxx \
./net/seq_text_geraet.cxx \
./net/text_message_geraet.cxx \
./net/lat_disc_geraet.cxx \
./net/block_geraet.cxx \
./net/object_geraet.cxx \
./net/anticipatory_channels.cxx \
./net/ship_damage_geraet.cxx \
./net/network_game.cxx \
./net/network_test.cxx \
./net/xxx/xnetobj.cxx \
./secondary/namegen.cxx \
./secondary/light_trail.cxx \
./secondary/frame_recorder.cxx \
./secondary/confreg.cxx \
./secondary/planet_generator.cxx \
./secondary/global_chat.cxx \
./fasttrig.cxx \
./graphics/matops.cxx \
./graphics/mat.cxx \
./graphics/asgi.cxx \
./graphics/index_texture.cxx \
./graphics/vaoemu.cxx \
./graphics/shader.cxx \
./graphics/glhelp.cxx \
./graphics/square.cxx \
./graphics/gl32emu.cxx \
./graphics/cmn_shaders.cxx \
./graphics/acsgi.cxx \
./graphics/vec.cxx \
./graphics/imgload.cxx \
./graphics/font.cxx \
./tcl_iface/slave_thread.cxx \
./tcl_iface/common_keyboard_client.cxx \
./tcl_iface/square_icon.cxx \
./tcl_iface/implementation.cxx

abendsternuxgl14_CXXFLAGS=$(COMMONCXXFLAGS) -DAB_OPENGL_14
abendsternuxgl14_LDADD=$(COMMONLDADD)
abendsternuxgl14_SOURCES=$(COMMONSOURCES)
abendsternuxgl21_CXXFLAGS=$(COMMONCXXFLAGS) -DAB_OPENGL_21 $(GLEW_CFLAGS)
abendsternuxgl21_LDADD=$(COMMONLDADD) $(GLEW_LIBS)
abendsternuxgl21_SOURCES=$(COMMONSOURCES)
abendsternuxgl32_CXXFLAGS=$(COMMONCXXFLAGS) $(GLEW_CFLAGS)
abendsternuxgl32_LDADD=$(COMMONLDADD) $(GLEW_LIBS)
abendsternuxgl32_SOURCES=$(COMMONSOURCES)
