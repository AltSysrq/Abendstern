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
  friend class INO_MonophasicEnergyPulse;\
  friend class ENO_MonophasicEnergyPulse;\
  friend class INO_Ship;\
  friend class ENO_Ship;\
  friend class INO_Spectator;\
  friend class ENO_Spectator
//MSVC++ can't handle fabs(int)
#define fabs(x) std::fabs((float)(x))
#endif
#include "xnetobj.hxx"
}

verbatimh {
  class ShieldGenerator;
  class Cell;
  #include "src/weapon/explode_listener.hxx"
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
  #include "src/weapon/explode_listener.hxx"
  #include "src/camera/spectator.hxx"
  #include "src/exit_conditions.hxx"
  #include "../ship_damage_geraet.hxx"
}

# Inserts common ExplodeListener code to the various weapon types.
proc explodable {clazz} {
  void "
    enoheader {
      class ExplListener: public ExplodeListener<$clazz> {
        ENO_@@@*const that;
      public:
        ExplListener($clazz* it, ENO_@@@* that_);

        virtual void exploded($clazz*) throw() {
          that->forceUpdate();
        }
      } explListener;
    }
    impl {
      ENO_@@@::ExplListener::ExplListener($clazz* it, ENO_@@@* that_)
      : ExplodeListener<$clazz>(it), that(that_) {}
    }
    enoconstructor {
      ,explListener(($clazz*)local.ref, this)
    }
  "
}

prototype GameObject {
  # Maximum velocity is 32 screen/sec
  fixed 2 32.0e-3f vx {default 10000}
  fixed 2 32.0e-3f vy {default 10000}
  # Maximum coordinate is 128,128
  fixed u2 128.0f x {
    default 128
    validate {
      x = max(0.0f, min(field->width-0.0001f, x + T*vx));
    }
  }
  fixed u2 128.0f y {
    default 128
    validate {
      y = max(0.0f, min(field->height-0.0001f, y + T*vy));
    }
  }
}

type EnergyCharge {
  explodable EnergyCharge

  # Never send updates
  void { compare { return false; } }
  extension GameObject
  ui 1 theta {
    extract { theta = (byte)(X->theta/2/pi*255.0f); }
  }
  bit 7 intensity {
    extract { intensity = (byte)(127.0f*X->intensity); }
  }

  bit 1 exploded {
    type bool
    extract { exploded = X->exploded; }
    update {
      if (!X->exploded && exploded) {
        X->explode(NULL);
        DESTROY(false);
      }
    }
  }

  construct {
     X = new EnergyCharge(field, x, y, vx, vy,
                          theta*pi*2/255.0f, intensity/127.0f);
  }
}

type MagnetoBomb {
  extension GameObject
  explodable MagnetoBomb

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
      if (!X->exploded && exploded) {
        X->explode();
        DESTROY(false);
      }
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
  explodable PlasmaBurst

  # Never send updates
  void { compare { return false; } }

  ui 1 direction {
    extract {
      direction = (byte)(255.0f*X->direction/2/pi);
    }
  }
  bit 7 mass {
    default 0
    validate { if (mass > 100) mass = 100; }
  }
  bit 1 exploded {
    extract {
      exploded = X->exploded;
    }
    update {
      if (exploded && !X->exploded) {
        X->explode(NULL);
        DESTROY(false);
      }
    }
  }

  construct {
    X = new PlasmaBurst(field, x, y, vx, vy, direction*pi*2/255.0f, mass);
  }
}

type Missile {
  extension GameObject
  explodable Missile

  fixed 2 2.0e-6f ax {
    default 1.0e6f
    post-set { X->ax = ax; }
  }
  fixed 2 2.0e-6f ay {
    default 1.0e6f
    post-set { X->ay = ay; }
  }
  fixed 2 2.0e-6f xdir { default 0 post-set { X->xdir = xdir; } }
  fixed 2 2.0e-6f ydir { default 0 post-set { X->ydir = ydir; } }
  bit 4 level {
    extract { level = X->level; }
    update { X->level = min(10u,max(1u,level)); }
  }
  bit 1 exploded {
    extract { exploded = X->exploded; }
    update {
      if (exploded && !X->exploded) {
        X->explode(NULL);
        DESTROY(false);
      }
    }
  }
  toggle ;# Disable updates
  ui 1 timeAlive {
    extract {
      timeAlive = X->timeAlive/12;
    }
  }
  toggle

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

type MonophasicEnergyPulse {
  extension GameObject
  explodable MonophasicEnergyPulse

  bit 1 exploded {
    extract { exploded = X->exploded; }
    update {
      if (exploded && !X->exploded) {
        X->explode(NULL);
        DESTROY(false);
      }
    }
  }
  toggle ;# Disable updates
  bit 7 power { default 0 }
  ui 2 timeAlive { default 0 }
  toggle ;# Reenable

  construct {
    X = new MonophasicEnergyPulse(field, x, y, vx, vy, (float)power, timeAlive);
  }
}

# Spectator must be exported since it serves as a reference
type Spectator {
  extension GameObject

  void { set-reference { cxn->setReference(X); } }

  construct {
    X = new Spectator(field, x, y, vx, vy);
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
  SYS(ShieldGenerator) \
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

  #define SHGEN(ix) ((ix) < X->networkCells.size()? \
                     getShieldGenerator(X->networkCells[(ix)]) : NULL)

  //ShipSystem constructor helper
  template<typename T>
  struct ShipSystemConstructor {
    inline
    static T* construct(Ship* ship, unsigned cellix, unsigned sysix,
                        unsigned capacitance, bool gatPlasmaTurbo,
                        float shieldRad, unsigned shieldStrength)
    throw() {
      return new T(ship);
    }
  };

  template<>
  struct ShipSystemConstructor<Capacitor> {
    inline
    static Capacitor* construct(Ship* ship, unsigned cellix, unsigned sysix,
                                unsigned capacitance, bool gatPlasmaTurbo,
                                float shieldRad, unsigned shieldStrength)
    throw() {
      return new Capacitor(ship, capacitance);
    }
  };

  template<>
  struct ShipSystemConstructor<GatlingPlasmaBurstLauncher> {
    inline static GatlingPlasmaBurstLauncher*
    construct(Ship* ship, unsigned cellix, unsigned sysix,
              unsigned capacitance, bool gatPlasmaTurbo,
              float shieldRad, unsigned shieldStrength)
    throw() {
      return new GatlingPlasmaBurstLauncher(ship, gatPlasmaTurbo);
    }
  };

  template<>
  struct ShipSystemConstructor<ShieldGenerator> {
    inline static ShieldGenerator*
    construct(Ship* ship, unsigned cellix, unsigned sysix,
              unsigned capacitance, bool gatPlasmaTurbo,
              float shieldRad, unsigned shieldStrength)
    throw() {
      return new ShieldGenerator(ship, shieldStrength, shieldRad);
    }
  };
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
    init {
      cxn->sdg->addLocalShip(channel, X);
    }
    enodestructor {
      cxn->sdg->delLocalShip(channel);
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

  # Max vtheta of 32 rad/sec
  fixed 2 32.0e-3f vtheta {
    default 1000
    post-set { X->vtheta = vtheta; }
    compare {
      NEAR += 1000*fabs(x.vtheta-y.vtheta);
    }
  }
  ui 1 theta {
    extract  { theta = (byte)(X->theta*255.0f/pi/2); }
    update   { X->theta = theta/255.0f*pi*2 + vtheta*T; }
    post-set { X->theta = theta/255.0f*pi*2 + vtheta*T; }
    compare  {
      NEAR += fabs((((float)x.theta) - ((float)y.theta))/6.0f);
      FAR +=  fabs((((float)x.theta) - ((float)y.theta))/48.0f);
    }
  }
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

  str 128 tag {
    extract { strncpy(tag, X->tag.c_str(), sizeof(tag)); }
    update { if (!X->ignoreNetworkTag) X->tag = tag; }
    post-set { if (!X->ignoreNetworkTag) X->tag = tag; }
    compare {
      if (strcmp(x.tag, y.tag)) return true; //Must send update
    }
  }

  # The name of the target, or empty string for NULL.
  # (This matches the tag of another Ship.)
  str 128 target {
    extract {
      if (X->target.ref)
        strncpy(target, X->target.ref->tag.c_str(), 128);
      else
        memset(target, 0, sizeof(target));
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
        X->isFragment = true;
        cxn->unsetReference(X);
      }
    }
    post-set {
      if (isFragment) {
        X->isFragment = true;
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
      if (shieldsDeactivated && X->shieldsDeactivated) {
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
      X->configureEngines(thrustOn, brakeOn, thrustPercent);
    }
    post-set {
      X->configureEngines(thrustOn, brakeOn, thrustPercent);
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
      } else {
        NAME = 0;
      }
    }
  }}
  toggle ;# End no updates
  void {
    declaration { bool destruction; }
    update { destruction = false; }
  }
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
          DESTROY(true);
        }
        if (!NAME) {
          destruction = true;
          X->physicsRequire(PHYS_SHIP_DS_INVENTORY_BIT
                           |PHYS_CELL_DS_NEAREST_BIT);
          //Delink the cell from its neighbours, spawning PlasmaFires
          //if appropriate.
          //Empty neighbours will cease to exist.
          for (unsigned n = 0; n < 4; ++n) {
            if (c->neighbours[n]) {
              if (c->neighbours[n]->isEmpty) {
                X->removeCell(c->neighbours[n]);
                delete c->neighbours[n];
              } else {
                unsigned ret = c->neighbours[n]->getNeighbour(c);
                EmptyCell* ec = new EmptyCell(X, c->neighbours[n]);
                c->neighbours[n]->neighbours[ret] = ec;
                X->cells.push_back(ec);
                if (highQuality && !isFragment)
                  field->add(new PlasmaFire(ec));
                X->physicsClear(PHYS_CELL_LOCATION_PROPERTIES_BITS
                               |PHYS_SHIP_COORDS_BITS);
              }
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
          #ifndef AB_OPENGL_14
          if (X->renderer)
            X->renderer->cellRemoved(c);
          #endif
          X->removeCell(c);
          X->networkCells[IX] = NULL;

          //Free
          delete c;

          if (X->cells.empty()) {
            #ifdef DEBUG
            cerr << "Warning: Discarding ship with no cells." << endl;
            #endif
            DESTROY(true);
          }
        } else {
          //Damage the cell by the appropriate amount
          float newdmg = 1.0f - NAME/255.0f;
          newdmg *= c->getMaxDamage();
          float olddmg = c->getCurrDamage();
          if (newdmg > olddmg) {
            #ifndef NDEBUG
            bool intact =
            #endif
            c->applyDamage(newdmg-olddmg, 0xFFFFFF);
            assert(intact);
            X->cellDamaged(c);
          }
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
  void { update {
    if (destruction) {
      X->refreshUpdates();

      //Call detectPhysics() on all systems
      for (unsigned i=0; i<X->cells.size(); ++i)
        for (unsigned s=0; s<2; ++s)
          if (X->cells[i]->systems[s])
            X->cells[i]->systems[s]->detectPhysics();
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
            X->refreshUpdates();
          }
        }
      }

      compare {
        //Only matters nearby (where it is important)
        if (y.NAME != x.NAME)
          NEAR += 1;
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
          else
            NAME = 0;
        }
      }

      validate {
        NAME = min((unsigned char)CAPACITOR_MAX, max((unsigned char)1, NAME));
      }
    }
  }
  toggle ;# Enable updates
  arr {struct {
    float radius;
    byte maxStrength, currStrengthPercent, currStability, currAlpha;
  }}                  4094  1 shields           {
    float {NAME.radius} {
      min MIN_SHIELD_RAD
      max MAX_SHIELD_RAD
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
    ui 1 {NAME.currStrengthPercent} {
      extract {
        {
          ShieldGenerator* gen = SHGEN(IX);
          if (gen)
            NAME.currStrengthPercent =
              (byte)(255*min(1.0f,gen->getShieldStrength()));
          else
            NAME.currStrengthPercent = 0;
        }
      }
      update {
        {
          ShieldGenerator* gen = SHGEN(IX);
          if (gen)
            gen->setShieldStrength(NAME.currStrengthPercent/255.0f);
        }
      }
      post-set {
        {
          ShieldGenerator* gen = SHGEN(IX);
          if (gen)
            gen->setShieldStrength(NAME.currStrengthPercent/255.0f);
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
            NAME.currStability =
                (byte)(255.0f*min(1.0f,gen->getShieldStability()));
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
      post-set {
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
            NAME.currAlpha = (byte)(255.0f * max(0.0f,gen->getShieldAlpha()));
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
      post-set {
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

  toggle ;# Reenable updates

  void { set-reference { if (!X->isFragment) cxn->setReference(X); } }
  # Ensure that the ship's cells have been numbered
  # (This is at the end since extraction runs in reverse order)
  void {
    extract {
      if (X->networkCells.empty()) {
        for (unsigned i = 0; i < X->cells.size(); ++i) {
          if (!X->cells[i]->isEmpty) {
            X->cells[i]->netIndex = X->networkCells.size();
            const_cast<Ship*>(X)->networkCells.push_back(X->cells[i]);
          }
        }
      }
    }
  }

  construct {
    X = new Ship(field);
    //Set fields from GameObject
    X->x = x;
    X->y = y;
    X->vx = vx;
    X->vy = vy;
    X->isRemote = true;

    //Count the number of cell slots used
    //The last cell with non-zero health is the last index we must store,
    //so the length is one plus that index.
    unsigned cellCount = lenof(cellDamage);
    while (cellCount > 0 && !cellDamage[cellCount-1]) --cellCount;

    X->networkCells.resize(cellCount, NULL);

    if (!cellDamage[0]) {
      #ifdef DEBUG
      cerr << "Warning: Ignoring ship with nonexistent root." << endl;
      #endif
      DESTROY(true);
    }

    if (rootIsBridge && rootTheta != 0) {
      #ifdef DEBUG
      cerr << "Warning: Ignoring ship with rotated bridge." << endl;
      #endif
      DESTROY(true);
    }

    //Initialise living cells
    for (unsigned i = 0; i < cellCount; ++i) {
      if (cellDamage[i]) {
        Cell* c;
        switch (cellType[i]) {
          case SQUARE_CELL:
            c = new SquareCell(X);
            break;

          case CIRCLE_CELL:
            c = new CircleCell(X);
            break;

          case EQUT_CELL:
            c = new EquTCell(X);
            break;

          default: //RIGHTT_CELL
            assert(cellType[i] == RIGHTT_CELL);
            if (i == 0 && rootIsBridge) {
              #ifdef DEBUG
              cerr << "Warning: Attempt to make right triangle bridge." << endl;
              #endif
              DESTROY(true);
            }
            c = new RightTCell(X);
            break;
        }
        //Wait with applying damage until ready to do physics

        //Add cell to ship
        X->cells.push_back(c);
        X->networkCells[i] = c;
        c->netIndex = i;
        if (i == 0 && rootIsBridge)
          c->usage = CellBridge;
      }
    }

    //Link cells to each other
    for (unsigned i=0; i < cellCount; ++i) {
      if (cellDamage[i]) {
        unsigned numNeighbours = (cellType[i] <= CIRCLE_CELL? 4 : 3);
        for (unsigned n = 0; n < numNeighbours; ++n) {
          if (unsigned nix = neighbours[i*4+n]) {
            //There is a linkage to this neighbour
            Cell* neighbour;
            if (nix == 1) {
              //Special case: EmptyCell
              neighbour = new EmptyCell(X, X->networkCells[i]);
              X->cells.push_back(neighbour);
            } else if (nix-2 < cellCount) {
              //General case
              neighbour = X->networkCells[nix-2];
            } else {
              #ifdef DEBUG
              cerr << "Warning: Neighbour index out of bounds: " << (nix-2)
                   << endl;
              #endif
              neighbour = NULL;
            }

            if (!neighbour) {
              #ifdef DEBUG
              cerr << "Warning: Nonexistent neighbour." << endl;
              #endif
              DESTROY(true);
            }

            X->networkCells[i]->neighbours[n] = neighbour;
          }
        }
      }
    }

    //Verify that all cells have bidirectional linkage
    for (unsigned i=0; i < X->cells.size(); ++i) {
      Cell* c = X->cells[i];
      for (unsigned n=0; n < 4; ++n) {
        if (c->neighbours[n]) {
          Cell* d = c->neighbours[n];
          for (unsigned m = 0; m < 4; ++m) {
            if (d->neighbours[m] == c)
              goto nextN;
          }

          //Shouldn't get here if all linkage is valid
          #ifdef DEBUG
          cerr << "Warning: Ignoring ship with monodirectional linkage." <<endl;
          #endif
          DESTROY(true);
        }
        nextN:;
      }
    }

    //Orient the cells
    X->cells[0]->orient(rootTheta);

    //Add systems
    for (unsigned i=0; i<cellCount; ++i) if (cellDamage[i]) {
      if (i==0 && rootIsBridge) continue; //Bridge has no systems

      unsigned syscount = (cellType[i] <= CIRCLE_CELL? 2:1);
      for (unsigned s=0; s<syscount; ++s) if (systemExist[i*2+s]) {
        ShipSystem* ss;
        #define SYS(systype) \
        case (unsigned)SSC##systype: \
        ss = ShipSystemConstructor<systype>::construct( \
                X, i, s, capacitors[i*2+s], gatPlasmaTurbo[i], \
                shields[i].radius, shields[i].maxStrength); \
        break;
        switch (systemInfo[i*2+s].type) {
          HANDLE_SYSTEMS
          default:
            #ifdef DEBUG
            cerr << "Warning: Ignoring unknown ship system type: "
                 << systemInfo[i*2+s].type << endl;
            #endif
            continue;
        }
        #undef SYS

        assert(ss);

        //Add system to ship
        X->networkCells[i]->systems[s] = ss;
        ss->container = X->networkCells[i];

        //Configure system and ensure it is happy there
        const char* error;
        if ((error = ss->setOrientation(systemInfo[i*2+s].orientation))
        &&  !isFragment) {
          #ifdef DEBUG
          cerr << "Warning: Rejecting ship system with bad orientation: "
               << error << endl;
          #endif
          DESTROY(true);
        }
      }
    }

    //Ensure ship is valid
    if (const char* error = verify(X)) {
      #ifdef DEBUG
      cerr << "Warning: Discarding invalid ship: " << error << endl;
      #endif
      DESTROY(true);
    }

    //Call detectPhysics() on all systems
    for (unsigned i=0; i<X->cells.size(); ++i)
      for (unsigned s=0; s<2; ++s)
        if (X->cells[i]->systems[s])
          X->cells[i]->systems[s]->detectPhysics();
    X->refreshUpdates();

    //Register with SDG
    #ifndef LOCAL_CLONE
    X->shipDamageGeraet = cxn->sdg;
    cxn->sdg->addRemoteShip(X, inputChannel);
    #endif /* LOCAL_CLONE */
  }
}
