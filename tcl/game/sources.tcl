# Loads all tcl/game/ files in the correct order.
foreach base {
  schema
  communicator
  loopback_communicator
  basic_game
  spectator
  spawn_manager
  mod_autobot
  mod_free_spawn
  mod_perfect_radar
  mod_stats_ffa
  mod_team
  mod_stats_team
  mod_score_frags
  mod_score_teamfrag
  mod_match
  mod_round
  mod_round_spawn
  g_dm
  g_xtdm
  g_lms
  g_lxts
  g_hvc
} {
  source "tcl/game/$base.tcl"
}
