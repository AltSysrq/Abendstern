# Loads all tcl/game/ files in the correct order.
foreach base {
  schema
  communicator
  loopback_communicator
  network_communicator
  game_manager
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
  mod_saw_clock
  mod_saw_best_player
  mod_saw_local_player
  mod_saw_match_time_left
  mod_saw_players_left
  mod_saw_rounds_of_match
  g_dm
  g_xtdm
  g_lms
  g_lxts
  g_hvc
  g_null
} {
  source "tcl/game/$base.tcl"
}
