verbatimc {
//MSVC++ doesn't handle inherited members accessed by friends correctly.
//This hack injects an appropriate friends list into GameObject
#ifdef WIN32
#define TclGameObject \
  TclGameObject; \
  friend class INO_EnergyCharge;\
  friend class ENO_EnergyCharge;\
  friend class INO_MagnetoBomb;\
  friend class ENO_MagnetoBomb;\
  friend class INO_SemiguidedBomb;\
  friend class ENO_SemiguidedBomb;\
  friend class INO_PlasmaBurst;\
  friend class ENO_PlasmaBurst;\
  friend class INO_Missile;\
  friend class ENO_Missile;\
  friend class INO_ParticleEmitter;\
  friend class ENO_ParticleEmitter;\
  friend class INO_Ship;\
  friend class ENO_Ship
#endif

#include "xnetobj.hxx"
}

verbatimh {
}
verbatimc {
  #include <cassert>
  #include <typeinfo>
  #include "src/sim/game_object.hxx"
  #include "src/sim/blast.hxx"
  #include "src/ship/everything.hxx"
  #include "src/ship/ship_renderer.hxx"
  #include "src/weapon/energy_charge.hxx"
  #include "src/weapon/magneto_bomb.hxx"
  #include "src/weapon/plasma_burst.hxx"
  #include "src/weapon/semiguided_bomb.hxx"
  #include "src/weapon/missile.hxx"
  #include "src/weapon/monophasic_energy_pulse.hxx"
  #include "src/weapon/particle_beam.hxx"
}

prototype GameObject {
  float vx {default 10000}
  float vy {default 10000}
  float x {
    default 32
    validate {
      if (x == x)
        x = max(0.0f, min(field->width, x + T*vx));
      else
        x = 0;
    }
  }
  float y {
    default 32
    validate {
      if (y == y)
        y = max(0.0f, min(field->height, y + T*vy));
      else
        y = 0;
    }
  }
  str 128 tag {
    extract { strncpy(tag, X->tag.c_str(), sizeof(tag-1)); }
    update { if (!X->ignoreNetworkTag) X->tag = tag; }
    post-set { if (!X->ignoreNetworkTag) X->tag = tag; }
    compare {
      if (strcmp(x.tag, y.tag)) return true; //Must send update
    }
  }
}

type EnergyCharge {
  # Never send updates
  void { compare { return false; } }
  extension GameObject
  float intensity {
    default 0
    min 0 max 1
  }
  float theta {
    extract { theta = X->theta; }
  }

  bit 1 exploded {
    type bool
    extract { exploded = X->exploded; }
    update {
      if (!X->exploded && exploded)
        X->explode(NULL);
    }
  }

  construct {
     X = new EnergyCharge(field, x, y, vx, vy, theta, intensity);
  }
}

type MagnetoBomb {
  extension GameObject

  float ax {
    default 0
    post-set { X->ax = ax; }
  }
  float ay {
    default 0
    post-set { X->ay = ay; }
  }
  float power {default 0 min 0}
  ui 2 timeAlive {
    default 0
    validate { timeAlive = max((short unsigned)0,timeAlive); }
    post-set { X->timeAlive = timeAlive; }
  }
  bit 1 exploded {
    type bool
    extract { exploded = X->exploded; }
    update {
      if (!X->exploded && exploded)
        X->explode();
    }
  }

  construct {
    X = new MagnetoBomb(field, x, y, vx, vy, power, NULL);
    X->isRemote = true;
    X->includeInCollisionDetection = false;
    X->decorative = true;
  }
}

type SemiguidedBomb {
  extension MagnetoBomb

  construct {
    X = new SemiguidedBomb(field, x, y, vx, vy, power, NULL);
    X->isRemote = true;
    X->includeInCollisionDetection = false;
    X->decorative = true;
  }
}

type PlasmaBurst {
  extension GameObject

  # Never send updates
  void { compare { return false; } }

  float mass {
    default 0
    min 0 max 100
  }
  float direction { default 0 }
  bit 1 exploded {
    extract {
      exploded = X->exploded;
    }
    update {
      if (exploded && !X->exploded) {
        X->explode(NULL);
      }
    }
  }

  construct {
    X = new PlasmaBurst(field, x, y, vx, vy, direction, mass);
  }
}

type Missile {
  extension GameObject

  float ax {
    default 1.0e8f
    min -2.0e-6f max +2.0e-6f
  }
  float ay {
    default 1.0e8f
    min -2.0e-6f max +2.0e-6f
  }
  ui 2 timeAlive {
    default 0
  }
  bit 4 level {
    extract { level = X->level; }
    update { X->level = min(10u,max(1u,level)); }
  }
  bit 1 exploded {
    extract { exploded = X->exploded; }
    update {
      if (exploded && !X->exploded) {
        X->explode(NULL);
      }
    }
  }

  construct {
    X = new Missile(field, level, x, y, vx, vy, ax, ay, timeAlive);
  }
}

type ParticleEmitter {
  extension GameObject

  # Never send updates
  void { compare { return false; } }

  bit 2 type {
    extract { type = (unsigned)X->type; }
  }
  bit 3 rmajor { default 0 }
  bit 3 rminor { default 0 }
  ui 2 timeAlive {
    default 0
  }
  dat 8 r {
    extract { memcpy(r, X->r, sizeof(r)); }
    update {  memcpy(X->r, r, sizeof(r)); }
  }

  ui 1 blame {
    default 0
  }

  construct {
    X = new ParticleEmitter(field, (ParticleBeamType)type,
                            0xFFFFFF, //TODO: Translate to local blame
                            x, y, vx, vy,
                            r, rmajor, rminor,
                            timeAlive);
  }
}

# Cell and system type definitions
verbatimc {
  #define SQUARE_CELL 0
  #define CIRCLE_CELL 1
  #define EQUT_CELL 2
  #define RIGHTT_CELL 3

  enum ShipSystemCode {
    SSCAntimatterPower=0,
    SSCCloakingDevice,
    SSCDispersionShield,
    SSCGatlingPlasmaBurstLauncher,
    SSCMissileLauncher,
    SSCMonophasicEnergyEmitter,
    SSCParticleBeamLauncher,
    SSCRelIonAccelerator,

    SSCBussardRamjet,
    SSCFusionPower,
    SSCHeatsink,
    SSCMiniGravwaveDriveMKII,
    SSCPlasmaBurstLauncher,
    SSCSemiguidedBombLauncher,
    SSCShieldGenerator,
    SSCSuperParticleAccelerator,

    SSCCapacitor,
    SSCEnergyChargeLauncher,
    SSCFissionPower,
    SSCMagnetoBombLauncher,
    SSCMiniGravwaveDrive,
    SSCParticleAccelerator,
    SSCPowerCell,
    SSCReinforcementBulkhead,
    SSCSelfDestructCharge
  };
}

type Ship {
  extension GameObject

  # The name of the target, or empty string for NULL.
  # (This matches the tag of another Ship.)
  str 128 target {
    extract {
      if (X->target.ref)
        strncpy(target, X->target.ref->tag.c_str(), 128);
      else
        target[0]=0;
    }
    update {
      if (target[0]) {
        //Stop immediately if no change
        if (X->target.ref
        &&  0 == strcmp(target, X->target.ref->tag.c_str()))
          goto targetFound;

        //Seacrh the field for such a ship
        {
          GameField::iterator it = field->begin(), end = field->end();
          for (; it != end; ++it) {
            GameObject* go = *it;
            if (go->getClassification() == GameObject::ClassShip) {
              Ship* s = (Ship*)go;
              if (s->hasPower()
              &&  Allies != getAlliance(X->insignia, s->insignia)
              &&  0 == strcmp(target, s->tag.c_str())) {
                //Found
                X->target.assign(s);
                goto targetFound;
              }
            }
          }
        }

        //Not found
        X->target.assign(NULL);

        targetFound:;
      } else {
        X->target.assign(NULL);
      }
    }

    compare {
      //Only prioritise updating the target if near
      if (strcmp(x.target, y.target))
        NEAR += 100;
    }
  }

  toggle ;# Ignore (and don't send) colour changes.
  # Core, scalar information
  float colourR { default 10 min 0 max 1 }
  float colourG { default 10 min 0 max 1 }
  float colourB { default 10 min 0 max 1 }
  toggle ;# Reenable updates
  void {
    post-set {
      X->setColour(colourR, colourG, colourB);
    }
  }
  float theta { default 10 post-set { X->theta = theta; } }
  float vtheta { default 0.01 post-set { X->vtheta = vtheta; } }
  # Maintain the cached cos() and sin() of theta
  void {
    update {
      X->cosTheta = cos(X->theta);
      X->sinTheta = sin(X->theta);
    }
    post-set {
      X->cosTheta = cos(X->theta);
      X->sinTheta = sin(X->theta);
    }
  }
  float thrustPercent { default 10 min 0 max 1 update {} }
  float reinforcement {
    default 0
    min 0 max 32
    update {}
    post-set {
      X->setReinforcement(reinforcement);
    }
  }
  ui 1 currentCapacitancePercent {
    extract {
      currentCapacitancePercent = 255*X->getCapacitancePercent();
    }
    update {
      X->physicsRequire(PHYS_SHIP_CAPAC_BIT);
      X->currentCapacitance=X->totalCapacitance*
                            currentCapacitancePercent/255.0f;
    }
  }

  ui 8 insignia {
    default 100
    post-set {
      X->insignia = insignia;
    }
  }

  bit 1 isFragment {
    type bool
    extract {
      isFragment = X->isFragment;
    }
    update {
      if (isFragment && !X->isFragment) {
        X->spontaneouslyDie();
        cxn->unsetReference(X);
      }
    }
    post-set {
      if (isFragment) {
        X->spontaneouslyDie();
      } else {
        cxn->setReference(X);
      }
    }
    compare {
      if (x.isFragment != y.isFragment)
        return true; //MUST update
    }
  }
  bit 1 thrustOn {
    type bool
    default 1
    update {}
  }
  bit 1 brakeOn {
    type bool
    default 1
    update {}
  }
  bit 1 shieldsDeactivated {
    type bool
    default 1
    update {
      shield_deactivate(X);
      X->shieldsDeactivated=true;
      //Clear all cell power bits that have shields
      X->physicsRequire(PHYS_SHIP_SHIELD_INVENTORY_BIT);
      for (unsigned i=0; i<X->shields.size(); ++i)
        X->shields[i]->getParent()->physicsClear(PHYS_CELL_POWER_BITS
                                                |PHYS_CELL_POWER_PROD_BITS);
      X->shields.clear();
    }
  }
  bit 1 stealthMode {
    type bool
    default 10000
    update {
      X->setStealthMode(stealthMode);
    }
    post-set {
      X->setStealthMode(stealthMode);
    }
  }
  bit 1 rootIsBridge {
    type bool
    extract {
      rootIsBridge = (X->cells[0]->usage == CellBridge);
    }
  }

  # Configure engines all at once
  void {
    update {
      X->configureEngines(thrustPercent, thrustOn, brakeOn);
    }
    post-set {
      X->configureEngines(thrustPercent, thrustOn, brakeOn);
    }
  }

  si 2 rootTheta {
    extract {
      rootTheta = X->cells[0]->getT();
    }
  }

  # The information to associate with each cell is:
  #   uint2 cellType
  #   byte damage
  #   uint12 neighbours[4]
  #   uint2 systemOrientations[2]
  #   uint6 systemTypes[2]
  #   byte capacitors[2]
  #   byte shieldMaxStrength
  #   float shieldRadius
  #   byte shieldCurrStrengthPercent
  #   byte shiepdCurrAlpha
  #   bool gatPlasmaTurbo
  #
  # damage is inverse, where 0 = non-existent and 255 is undamaged.
  # A neighbour of zero is non-existent, and 1 is destroyed; anything else
  # is two plus the index of the neighbour.
  # A system type of zero is no system; anything else is a specific system type.
  #
  # In order to maximise data density and efficiency for ships of various sizes,
  # lay the data out as follows (note that there is a maximum of 4094 cells):
  #   nybble neighboursBits03[4*4094]
  #   nybble neighboursBits47[4*4094]
  #   nybble neighboursBits8B[4*4094]
  #   bit2   cellType[4094]
  #   byte   cellDamage[4094]
  #   byte   systemInfo[2*4094] {bit 0,1: orientation; bit 2+: type}
  #   byte   capacitors[2*4094]
  #   byte   shieldMaxStrength[4094]
  #   float  shieldRadius[4094]
  #   byte   shieldCurrStrengthPercent[4094]
  #   byte   shieldCurrAlpha[4094]
  #   bit    gatPlasmaTurbo[4094]

  toggle ;# Do not modify these data or expect them to be modifyed
  arr {unsigned char} 16376 2 neighboursBits03  {nybble {NAME}}
  arr {unsigned char} 16376 2 neighboursBits47  {nybble {NAME}}
  arr {unsigned char} 16376 2 neighboursBits8B  {nybble {NAME}}
  arr {unsigned int}  16376 1 neighbours {
    void {
      decode {
        NAME = (neighboursBits03[IX] << 0)
             | (neighboursBits47[IX] << 4)
             | (neighboursBits8B[IX] << 8);
      }
      encode {
        neighboursBits03[IX] = NAME & 15;
        neighboursBits47[IX] = (NAME >> 4) & 15;
        neighboursBits8B[IX] = NAME >> 8;
      }
      extract {
        {
          const unsigned neigh = IX&3;
          const unsigned cellix = IX>>2;
          if (X->networkCells.size() > cellix
          &&  X->networkCells[cellix]
          &&  X->networkCells[cellix]->neighbours[neigh]) {
            //Exists, but EmptyCells are encoded specially
            if (X->networkCells[cellix]->neighbours[neigh]->isEmpty) {
              //Special value: 1
              NAME = 1;
            } else {
              //Generic
              NAME = 2 + X->networkCells[cellix]->neighbours[neigh]->netIndex;
            }
          } else {
            //Nonexistent
            NAME = 0;
          }
        }
      }
    }
  }

  # 4096 because len%stride must be zero.
  arr {unsigned char} 4096  4 cellType          {bit 2 {NAME} {
    extract {
      //Only bother initialising if the cell actually exists
      if (X->networkCells.size() > IX && X->networkCells[IX]) {
        Cell* c = X->networkCells[IX];
        if (typeid(*c) == typeid(SquareCell))
          NAME = SQUARE_CELL;
        else if (typeid(*c) == typeid(CircleCell))
          NAME = CIRCLE_CELL;
        else if (typeid(*c) == typeid(EquTCell))
          NAME = EQUT_CELL;
        else {
          assert(typeid(*c) == typeid(RightTCell));
          NAME = RIGHTT_CELL;
        }
      }
    }
  }}
  toggle ;# End no updates
  arr {unsigned char} 4094  1 cellDamage        {ui 1 {NAME} {
    extract {
      if (IX < X->networkCells.size() && X->networkCells[IX]) {
        Cell*const c = X->networkCells[IX];
        NAME = max((byte)1,
                   (byte)(255 - 255*c->getCurrDamage()/c->getMaxDamage()));
      } else {
        NAME = 0; //Nonexistent
      }
    }
    update {
      if (IX < X->networkCells.size() && X->networkCells[IX]) {
        Cell*const c = X->networkCells[IX];
        if (!NAME && !IX) {
          //Illegal attempt to destroy root; ignore
          #ifdef DEBUG
          cerr << "Warning: Ignoring illegal attempt to destroy Ship root!"
               << endl;
          #endif
          continue;
        }
        if (!NAME) {
          //Delink the cell from its neighbours, spawning PlasmaFires
          //if appropriate.
          for (unsigned n = 0; n < 4; ++n) {
            if (c->neighbours[n]) {
              unsigned ret = c->neighbours[n]->getNeighbour(c);
              EmptyCell* ec = new EmptyCell(X, c->neighbours[n]);
              c->neighbours[n]->neighbours[ret] = ec;
              if (highQuality && EXPCLOSE(x,y))
                field->add(new PlasmaFire(ec));
            }
          }

          //Spawn fragments if appropriate
          pair<float,float> coord = X->cellCoord(X, c);
          Blast blast(field, 0, coord.first, coord.second,
                      STD_CELL_SZ/2, c->getMaxDamage()-c->getCurrDamage(),
                      true, STD_CELL_SZ/16, false, true, false);
          CellFragment::spawn(c, &blast);

          //Destroy systems within cell
          if (c->systems[0]) c->systems[0]->destroy(0xFFFFFF);
          if (c->systems[1]) c->systems[1]->destroy(0xFFFFFF);
          //Remove cell from ship
          X->preremove(c);
          if (X->renderer)
            X->renderer->cellRemoved(c);
          X->removeCell(c);
          X->networkCells[IX] = NULL;

          //Free
          delete c;
        } else {
          //Damage the cell by the appropriate amount
          float newdmg = 1.0f - NAME/255.0f;
          newdmg *= c->getMaxDamage();
          float olddmg = c->getCurrDamage();
          #ifndef NDEBUG
          bool destroyed =
          #endif
          c->applyDamage(newdmg-olddmg, 0xFFFFFF);
          assert(!destroyed);
          X->cellDamaged(c);
        }
      }
    }
    compare {
      //If one is zero and the other not, we must send an update
      if ((x.NAME == 0) != (y.NAME == 0))
        return true;

      //Consider 10% difference worth it at close range; don't care at far
      NEAR += fabs((x.NAME-y.NAME)/25.6f);
    }
  }}
  arr {struct {
    unsigned char orientation, type;
  }}                  8188  1 systemInfo        {bit 2 {NAME.orientation}
                                                 bit 6 {NAME.type}}
  toggle ;# Disable updates
  arr {unsigned char} 8188  1 capacitors        {ui 1 {NAME}}
  toggle ;# Enable updates
  arr {struct {
    float radius;
    byte maxStrength, currStrengthPercent, currAlpha;
  }}                  4094  1 shields           {
    toggle
    float {NAME.radius} {min STD_CELL_SZ*MIN_SHIELD_RAD
                         max STD_CELL_SZ*MAX_SHIELD_RAD}
    ui 1 {NAME.maxStrength} {
      validate { NAME.maxStrength = min((byte)MAX_SHIELD_STR,
                                        max((byte)MIN_SHIELD_STR,
                                            NAME.maxStrength)); }
    }
    toggle
    ui 1 {NAME.currStrengthPercent}
    ui 1 {NAME.currAlpha}
  }
  toggle ;# Disable updates
  # 4096 because len%stride must be zero.
  arr {bool}          4096 8 gatPlasmaTurbo     {bit 1 {NAME} {type bool}}

  # TODO
  # (For now, just do nothing so it compiles)
  construct {}
}
