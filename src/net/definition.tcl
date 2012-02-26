verbatimh {
  #include "object_geraet.hxx"
}
verbatimc {
  #include "object_geraet.hxx"
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
  float vx {
    extract { vx = X->vx; }
    update { X->vx = vx; }
    compare {
      float delta = fabs(x_vx - y_vx);
      //Consider 1 screen/sec to be too much at 10 screens
      near += delta*10000;
      far += delta*10000;
    }
  }
  float vy {
    extract { vy = X->vy; }
    update { X->vy = vy; }
    compare {
      float delta = fabs(x_vy - y_vy);
      near += delta*10000;
      far += delta*10000;
    }
  }
  float x {
    extract { x = X->x; }
    update { X->x = max(0.0f, min(field->width, x + T*vx)); }
    compare {
      float delta = fabs(x_x - y_x);
      near += delta*32;
      far += delta*32;
    }
  }
  float y {
    extract { y = X->y; }
    update { X->y = max(0.0f, min(field->height, y + T*vy)); }
    compare {
      float delta = fabs(x_y + y_y);
      near += delta*32;
      far += delta*32
    }
  }
  string 128 tag {
    extract { strncpy(tag, X->tag.c_str(), sizeof(tag-1)); }
    update { if (!X->ignoreRemoteTag) X->tag = tag; }
    compare {
      if (strcmp(x_tag, y_tag)) return true; //Must send update
    }
  }
}

type EnergyCharge {
  # Never send updates
  void { compare { return false; } }
  extension GameObject
  float intensity {
    extract { intensity = X->intensity; }
    update { X->intensity = max(0.0f,min(1.0f,intensity)); }
  }
  float theta {
    extract { theta = X->theta; }
    update { X->theta = theat; }
  }

  bit 1 exploded {
    type bool
    extract { exploded = X->exploded; }
    update {
      if (!X->exploded && exploded)
        X->explode(NULL);
    }
  }

  constructor {
     X = new EnergyCharge(field, x, y, vx, vy, theta, intensity);
  }
}
