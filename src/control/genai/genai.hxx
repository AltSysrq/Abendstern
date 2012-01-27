/**
 * @file
 * @author Jason Lingle
 * @brief Contains the GenAI Controller
 */

/*
 * genai.hxx
 *
 *  Created on: 03.11.2011
 *      Author: jason
 */

#ifndef GENAI_HXX_
#define GENAI_HXX_

#include <string>

#include "src/control/controller.hxx"
#include "c_reflex.hxx"
#include "c_run_away.hxx"
#include "c_avoid_edge.hxx"
#include "c_dodge.hxx"
#include "c_frontal.hxx"
#include "c_navigation.hxx"
#include "c_strat_weapon.hxx"
#include "c_target_analysis.hxx"
#include "c_weapon_level.hxx"
#include "c_opp_weapon.hxx"
#include "c_aiming.hxx"

/**
 * The GenAI class ties all the Cortices together as described
 * in cortexai.hxx.
 */
class GenAI:
public Controller,
public cortex_input::SelfSource,
public cortex_input::CellTSource,
public cortex_input::EmotionSource {
  ReflexCortex          reflex;
  AvoidEdgeCortex       avoidEdge;
  DodgeCortex           dodge;
  RunAwayCortex         runAway;
  FrontalCortex         frontal;
  NavigationCortex      navigation;
  TargetAnalysisCortex  targetAnalysis;
  StratWeaponCortex     stratWeapon;
  AimingCortex          aiming;
  OppWeaponCortex       oppWeapon;
  WeaponLevelCortex     weaponLevel;

  //The previous amount of damage to the target blamed on us
  float recentTargetDamage;
  //The Ship corresponding to recentTargetDamage
  Ship* recentTarget;

  bool usedDodgeLastFrame;

  GenAI(Ship*,unsigned,const libconfig::Setting&);

  public:
  /** The species of this GenAI */
  const unsigned species;
  /** The generation of this GenAI */
  const unsigned generation;

  /**
   * Tries to construct a new GenAI.
   * @param ship The Ship to control
   * @return A GenAI if successful, or NULL otherwise
   */
  static GenAI* makeGenAI(Ship* ship) throw();

  virtual void update(float) noth;
  virtual void damage(float amt, float x, float y) noth;

  /** Returns the scores of the cortices as a Tcl list; format:
   *    instance score ...
   */
  std::string getScores() const noth;
};


#endif /* GENAI_HXX_ */
