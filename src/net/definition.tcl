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
  class ShieldGenerator;
  class Cell;
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
  #include "src/exit_conditions.hxx"
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

  //Uses the SYS(name) macro to handle every known ship system type.
  #define HANDLE_SYSTEMS \
  SYS(AntimatterPower) \
  SYS(CloakingDevice) \
  SYS(DispersionShield) \
  SYS(GatlingPlasmaBurstLauncher) \
  SYS(MissileLauncher) \
  SYS(MonophasicEnergyEmitter) \
  SYS(ParticleBeamLauncher) \
  SYS(RelIonAccelerator) \
  SYS(BussardRamjet) \
  SYS(FusionPower) \
  SYS(Heatsink) \
  SYS(MiniGravwaveDriveMKII) \
  SYS(PlasmaBurstLauncher) \
  SYS(SemiguidedBombLauncher) \
  SYS(SuperParticleAccelerator) \
  SYS(Capacitor) \
  SYS(EnergyChargeLauncher) \
  SYS(FissionPower) \
  SYS(MagnetoBombLauncher) \
  SYS(MiniGravwaveDrive) \
  SYS(ParticleAccelerator) \
  SYS(PowerCell) \
  SYS(ReinforcementBulkhead) \
  SYS(SelfDestructCharge)
}

verbatimc {
  #define SHGEN(ix) ((ix) < X->networkCells.size()? \
                     getShieldGenerator(X->networkCells[(ix)]) : NULL)
}
type Ship {
  extension GameObject

  void {
    inoheader {
      public:
      static ShieldGenerator* getShieldGenerator(const Cell*) throw();
    }
    enoheader {
      static ShieldGenerator* getShieldGenerator(const Cell* c) throw() {
        return INO_Ship::getShieldGenerator(c);
      }
    }
    impl {
      ShieldGenerator* INO_Ship::getShieldGenerator(const Cell* c) throw() {
        if (!c) return NULL;
        if (c->systems[0]
        &&  c->systems[0]->clazz == Classification_Shield)
          return static_cast<ShieldGenerator*>(c->systems[0]);
        if (c->systems[1]
        &&  c->systems[1]->clazz == Classification_Shield)
          return static_cast<ShieldGenerator*>(c->systems[1]);
        return NULL;
      }
    }
  }

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
  #   bool systemExistence[2]
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
  #
  # System existence is done separately from system types since it is cheaper
  # to calculate (and sending types is unnecessary for updates).
  #
  # In order to maximise data density and efficiency for ships of various sizes,
  # lay the data out as follows (note that there is a maximum of 4094 cells):
  #   nybble neighboursBits03[4*4094]
  #   nybble neighboursBits47[4*4094]
  #   nybble neighboursBits8B[4*4094]
  #   bit2   cellType[4094]
  #   byte   cellDamage[4094]
  #   bit    systemExist[2*4094]
  #   byte   systemInfo[2*4094] {bit 0,1: orientation; bit 2+: type}
  #   byte   capacitors[2*4094]
  #   byte   shieldMaxStrength[4094]
  #   float  shieldRadius[4094]
  #   byte   shieldCurrStrengthPercent[4094]
  #   byte   shieldCurrStab[4094]
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
  # 8192 so that len%stride == 0
  arr bool            8192  8 systemExist       {
    bit 1 {NAME} {
      type bool
      extract {
        {
          unsigned cellix = IX/2;
          unsigned sysix = IX&1;
          NAME = (X->networkCells.size() > cellix
              &&  X->networkCells[cellix]
              &&  X->networkCells[cellix]->systems[sysix]);
        }
      }

      update {
        {
          unsigned cellix = IX/2;
          unsigned sysix = IX&1;
          //Check for system destruction
          if (!NAME
          &&  X->networkCells.size() > cellix
          &&  X->networkCells[cellix]
          &&  X->networkCells[cellix]->systems[sysix]) {
            //Destroy it
            X->networkCells[cellix]->systems[sysix]->destroy(0xFFFFFF);
            delete X->networkCells[cellix]->systems[sysix];
            X->networkCells[cellix]->systems[sysix] = NULL;
            X->networkCells[cellix]->physicsClear(PHYS_CELL_ALL|PHYS_SHIP_ALL);
            X->cellChanged(X->networkCells[cellix]);
          }
        }
      }
    }
  }

  toggle ;# Disable updates
  arr {struct {
    unsigned char orientation, type;
  }}                  8188  1 systemInfo        {
    bit 2 {NAME.orientation} {
      extract {
        unsigned cellix = IX/2, sysix = IX&1;
        if (cellix < X->networkCells.size()
        &&  X->networkCells[cellix]
        &&  X->networkCells[cellix]->systems[sysix]) {
          NAME.orientation =
            X->networkCells[cellix]->systems[sysix]->getOrientation();
        } else {
          NAME.orientation = 0;
        }
      }
    }
    bit 6 {NAME.type} {
      extract {
        {
          #define SYS(clazz) \
          if (typeid(*sys) == typeid(clazz)) \
            NAME.type = (unsigned char)SSC##clazz; \
          else

          unsigned cellix = IX/2, sysix = IX&1;
          if (cellix < X->networkCells.size()
          &&  X->networkCells[cellix]
          &&  X->networkCells[cellix]->systems[sysix]) {
            ShipSystem*const sys = X->networkCells[cellix]->systems[sysix];
            HANDLE_SYSTEMS
            /* else */ {
              cerr << "FATAL: Unexpected ShipSystem type: "
                  << typeid(*sys).name() << endl;
              exit(EXIT_PROGRAM_BUG);
            }
          } else {
            NAME.type = 0;
          }

          #undef SYS
        }
      }
    }
  }
  arr {unsigned char} 8188  1 capacitors        {
    ui 1 {NAME} {
      extract {
        {
          unsigned cellix = IX/2, sysix = IX&1;
          if (cellix < X->networkCells.size()
          &&  X->networkCells[cellix]
          &&  X->networkCells[cellix]->systems[sysix]
          &&  typeid(*X->networkCells[cellix]->systems[sysix]) ==
              typeid(Capacitor))
            NAME = ((Capacitor*)X->networkCells[cellix]->systems[sysix])->
                   getCapacity();
        }
      }
    }
  }
  toggle ;# Enable updates
  arr {struct {
    float radius;
    byte maxStrength, currStrengthPercent, currStability, currAlpha;
  }}                  4094  1 shields           {
    toggle
    float {NAME.radius} {
      min STD_CELL_SZ*MIN_SHIELD_RAD
      max STD_CELL_SZ*MAX_SHIELD_RAD
      extract {
        {
          ShieldGenerator* gen = SHGEN(IX);
          if (gen)
            NAME.radius = gen->getRadius();
          else
            NAME.radius = 0;
        }
      }
    }
    ui 1 {NAME.maxStrength} {
      validate { NAME.maxStrength = min((byte)MAX_SHIELD_STR,
                                        max((byte)MIN_SHIELD_STR,
                                            NAME.maxStrength));
      }
      extract {
        {
          ShieldGenerator* gen = SHGEN(IX);
          if (gen)
            NAME.maxStrength = (byte)gen->getStrength();
          else
            NAME.maxStrength = 0;
        }
      }
    }
    toggle
    ui 1 {NAME.currStrengthPercent} {
      extract {
        {
          ShieldGenerator* gen = SHGEN(IX);
          if (gen)
            NAME.currStrengthPercent =
                (byte)(255*gen->getShieldStrength()/gen->getStrength());
          else
            NAME.currStrengthPercent = 0;
        }
      }
      update {
        {
          ShieldGenerator* gen = SHGEN(IX);
          if (gen)
            gen->setShieldStrength(NAME.currStrengthPercent/255.0f *
                                   gen->getStrength());
        }
      }

      compare {
        //Usually send updates for differences when near
        NEAR +=
          fabs((float)x.NAME.currStrengthPercent - y.NAME.currStrengthPercent);
      }
    }
    ui 1 {NAME.currStability} {
      extract {
        {
          ShieldGenerator* gen = SHGEN(IX);
          if (gen)
            NAME.currStability = (byte)(255.0f*gen->getShieldStability());
          else
            NAME.currStability = 0;
        }
      }

      update {
        {
          ShieldGenerator* gen = SHGEN(IX);
          if (gen)
            gen->setShieldStability(NAME.currStability/255.0f);
        }
      }

      compare {
        NEAR += fabs((float)x.NAME.currStability - y.NAME.currStability);
      }
    }
    ui 1 {NAME.currAlpha} {
      extract {
        {
          ShieldGenerator* gen = SHGEN(IX);
          if (gen)
            NAME.currAlpha = (byte)(255.0f * gen->getShieldAlpha());
          else
            NAME.currAlpha = 0;
        }
      }

      update {
        {
          ShieldGenerator* gen = SHGEN(IX);
          if (gen)
            gen->setShieldAlpha(NAME.currAlpha/255.0f);
        }
      }

      compare {
        NEAR += fabs((float)x.NAME.currAlpha - y.NAME.currAlpha);
      }
    }
  }
  toggle ;# Disable updates
  # 4096 because len%stride must be zero.
  arr {bool}          4096 8 gatPlasmaTurbo     {bit 1 {NAME} {
    type bool
    extract {
      if (IX < X->networkCells.size() && X->networkCells[IX]) {
        ShipSystem*const*const s = X->networkCells[IX]->systems;

        if (s[0] && typeid(*s[0]) == typeid(GatlingPlasmaBurstLauncher)) {
          NAME = ((GatlingPlasmaBurstLauncher*)s[0])->getTurbo();
        } else if (s[1] && typeid(*s[1])==typeid(GatlingPlasmaBurstLauncher)) {
          NAME = ((GatlingPlasmaBurstLauncher*)s[1])->getTurbo();
        } else {
          NAME = false;
        }
      }
    }
  }}

  # TODO
  # (For now, just do nothing so it compiles)
  construct {}
}
