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
  #include "src/sim/game_object.hxx"
  #include "src/ship/everything.hxx"
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

  # Core, scalar information
  float colourR { default 10 min 0 max 1 }
  float colourG { default 10 min 0 max 1 }
  float colourB { default 10 min 0 max 1 }
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
      if (isFragment && !X->isFragment)
        X->spontaneouslyDie();
    }
    post-set {
      if (isFragment)
        X->spontaneouslyDie();
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
      //TODO
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
  #   byte   systemInfo[4094]
  #   byte   capacitors[2*4094]
  #   byte   shieldMaxStrength[4094]
  #   float  shieldRadius[4094]
  #   byte   shieldCurrStrengthPercent[4094]
  #   byte   shieldCurrAlpha[4094]
  #   bit    gatPlasmaTurbo[4094]

  arr {unsigned char} 16376 2 neighboursBits03 {nybble NAME} {}
  arr {unsigned char} 16376 2 neighboursBits47 {nybble NAME} {}
  arr {unsigned char} 16376 2 neighboursBits8B {nybble NAME} {}

  # TODO
  # (For now, just do nothing so it compiles)
  construct {}
}
