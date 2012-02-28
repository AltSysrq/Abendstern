verbatimc {
//MSVC++ doesn't handle inherited members accessed by friends correctly.
//This hack injects an appropriate friends list into GameObject
#ifdef WIN32
#define TclGameObject \
  TclGameObject; \
  friend class INO_EnergyCharge;\
  friend class ENO_EnergyCharge
#endif
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
