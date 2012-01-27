/**
 * @file
 * @author Jason Lingle
 *
 * @brief Implementation of src/audio/ship_effects.hxx.
 */

#include <algorithm>

#include "audio.hxx"
#include "synth.hxx"
#include "wavload.hxx"
#include "src/ship/ship.hxx"
#include "ship_effects.hxx"
#include "ship_effects_common.hxx"

using namespace std;

namespace audio {
  ShipAmbient shipBackground(new Virtual<WhiteNoise<1,32> >, 1.0f, 4096, 0, 0);
  ShipAmbient fissionPower(new Virtual<
      Mix2<
        Modulate<Mult<Playtab<sinetab,1440>,VolumeDown<Playtab<sinetab,1024>,0x6000> >,
                 Add<VolumeDown<MakePositive<Playtab<sinetab,64> >,0x1000>, Value<0x5000> >
                >,
        VolumeDown<Mult<Playtab<sawtab,8000>,WhiteNoise<8,3> >, 0x3800>
       >
      >, 1.0f, 5512, 0, 0);
  ShipAmbient particleAccelerator(new Virtual<Mix2<
                                    NSeq<Mult<WhiteNoise<3,1>,Playtab<decaytab,3> >, 2204, Silence>,
                                    Mix2<WhiteNoise<8,4>, Mult<BrownNoise<1,16>,
                                                               Playtab<ptritab,440> > > > >,
                                  1.0f, 7350, 2204, 1);
  ShipAmbient superParticleAccelerator(new Virtual<Mix2<
                                    NSeq<Mult<WhiteNoise<3,1>,Playtab<decaytab,3> >, 2204, Silence>,
                                    Mix2<WhiteNoise<6,4>, Mult<BrownNoise<1,16>,
                                                               Playtab<ptritab,550> > > > >,
                                  1.0f, 7350, 2204, 1);

  #define CAPACFRQ(ix) ((ix)*220)
  #define CAPACAMP(ix) ((ix) < 31? (ix)*48 : (ix)*8)
  //We put eight waves, rounded down, into the buffer
  //We must also ensure that this is an even number.
  #define CAPACLEN(ix)  ((AUDIO_FRQ*8/CAPACFRQ(ix)) &~1)
  #define CAPACITOR(ix) \
    static ShipAmbient capacRing##ix(new Virtual<VolumeDown<Playtab<sinetab,CAPACFRQ(ix)>, \
                                                            CAPACAMP(ix)> >, \
                                     1.0f, CAPACLEN(ix), 0, 0)

  CAPACITOR( 1);
  CAPACITOR( 2);
  CAPACITOR( 3);
  CAPACITOR( 4);
  CAPACITOR( 5);
  CAPACITOR( 6);
  CAPACITOR( 7);
  CAPACITOR( 8);
  CAPACITOR( 9);
  CAPACITOR(10);
  CAPACITOR(11);
  CAPACITOR(12);
  CAPACITOR(13);
  CAPACITOR(14);
  CAPACITOR(15);
  CAPACITOR(16);
  CAPACITOR(17);
  CAPACITOR(18);
  CAPACITOR(19);
  CAPACITOR(20);
  CAPACITOR(21);
  CAPACITOR(22);
  CAPACITOR(23);
  CAPACITOR(24);
  CAPACITOR(25);
  CAPACITOR(26);
  CAPACITOR(27);
  CAPACITOR(28);
  CAPACITOR(29);
  CAPACITOR(30);
  CAPACITOR(31);

  ShipAmbient*const capacitorRing[32] = {
    NULL,         &capacRing1,  &capacRing2,  &capacRing3,
    &capacRing4,  &capacRing5,  &capacRing6,  &capacRing7,
    &capacRing8,  &capacRing9,  &capacRing10, &capacRing11,
    &capacRing12, &capacRing13, &capacRing14, &capacRing15,
    &capacRing16, &capacRing17, &capacRing18, &capacRing19,
    &capacRing20, &capacRing21, &capacRing22, &capacRing23,
    &capacRing24, &capacRing25, &capacRing26, &capacRing27,
    &capacRing28, &capacRing29, &capacRing30, &capacRing31,
  };

  typedef
      Add<VolumeDown<Add<VolumeDown<Playtab<sinetab,880>,0x4000>,
                         VolumeDown<Playtab<sawtab,1180>,0x38000> >, 0x4000>,
          VolumeDown<Add<VolumeDown<Playtab<sinetab,881>,0x4000>,
                         VolumeDown<Playtab<sawtab,1184>,0x38000> >, 0x4000> >
    miniGravwaveDrive_inner_t;
  ShipAmbient miniGravwaveDrive(new Virtual<
      Mult<NSeq<Playtab<buildtab,2>, AUDIO_FRQ/2,
           NSeq<Value<0x7FFF>, 8820,
                Playtab<decaytab,2> > >,
           miniGravwaveDrive_inner_t> >,
    1.0f, 8820, AUDIO_FRQ/2, AUDIO_FRQ/2);

  typedef
      Add<VolumeDown<Add<VolumeDown<Playtab<sinetab,990>,0x4000>,
                         VolumeDown<Playtab<sawtab,1280>,0x38000> >, 0x4000>,
          VolumeDown<Add<VolumeDown<Playtab<sinetab,991>,0x4000>,
                         VolumeDown<Playtab<sawtab,1284>,0x38000> >, 0x4000> >
    miniGravwaveDriveII_inner_t;
  ShipAmbient miniGravwaveDriveII(new Virtual<
      Mult<NSeq<Playtab<buildtab,2>, AUDIO_FRQ/2,
           NSeq<Value<0x7FFF>, 8820,
                Playtab<decaytab,2> > >,
           miniGravwaveDriveII_inner_t> >,
    1.0f, 8820, AUDIO_FRQ/2, AUDIO_FRQ/2);

  static Waveform bramjet("bramjet");
  ShipAmbient bussardRamjet(bramjet.get(), 1.0f, 1*AUDIO_FRQ, AUDIO_FRQ/2, AUDIO_FRQ/2);

  static Waveform relionacc("relionacc");
  ShipAmbient relIonAccelerator(relionacc.get(), 1.5f, relionacc.size());

  static Waveform fuspow("fuspow");
  ShipAmbient fusionPower(fuspow.get(), 1.0f, fuspow.size(), 0, 0);

  static Waveform antipow("antipow");
  ShipAmbient antimatterPower(antipow.get(), 1.1f, antipow.size(), 0, 0);

  static Waveform shieldAmb("shielda");
  ShipAmbient shieldGenerator(shieldAmb.get(), 1.0f, shieldAmb.size());

  static Waveform gpblnorm("gpblnorm"), gpblturb("gpblturb");
  ShipAmbient gatPlasmaBurstLauncherRevNorm (gpblnorm.get(), 7.0f, gpblnorm.size()),
              gatPlasmaBurstLauncherRevTurbo(gpblturb.get(), 8.0f, gpblturb.size());

  ShipStaticEvent energyChargeLauncher0(new Virtual<
      Mult<Modulate<Modulate<Playtab<sinetab,2000>,MakePositive<Playtab<sawtab,1000> > >,
                             Playtab<decaytab,5> >,
                    NSeq<Playtab<decaytab,5>, 4410, Silence> > >, 1.0f, 4410);
  ShipStaticEvent energyChargeLauncher1(new Virtual<VolumeDown<
      Mult<Modulate<Playtab<sinetab,4000>,Playtab<decaytab,2> >,
           NSeq<Playtab<decaytab,3>, 7350, Silence> >, 0x2000 > >, 1.0f, 7350);
  bool energyChargeLauncherOK = true;

  static Waveform magbombp1("magbombp1"),
                  magbombp2("magbombp2"),
                  magbombop("magbombop"),
                  smgbombax("smgbombax");
  ShipStaticEvent magnetoBombLauncher0(magbombp1.get(), 4.5f, magbombp1.size()),
                  magnetoBombLauncher1(magbombp2.get(), 4.5f, magbombp2.size()),
                  magnetoBombLauncherOP(magbombop.get(), 4.5f, magbombop.size());
  bool magnetoBombLauncherOK = true;
  ShipStaticEvent sgBombLauncher0(magbombp1.get(), 4.5f, magbombp1.size()),
                  sgBombLauncher1(magbombp2.get(), 4.5f, magbombp2.size()),
                  sgBombLauncher2(smgbombax.get(), 8.5f, smgbombax.size()),
                  sgBombLauncherOP(magbombop.get(), 4.5f, magbombop.size());
  bool sgBombLauncherOK = true;

  typedef Virtual<Mult<Mult<Playtab<sinetab,1>,
                            Playtab<decaytab,2> >,
                       WhiteNoise<16,1> > > PlasmaBurstLaunchSound;
  ShipStaticEvent plasmaBurstLauncher(new PlasmaBurstLaunchSound,
                                      4.0f, 11024);
  bool plasmaBurstLauncherOK = true;
  ShipStaticEvent gatPlasmaBurstLauncherNorm (new PlasmaBurstLaunchSound, 5.0f, 11024),
                  gatPlasmaBurstLauncherTurbo(new PlasmaBurstLaunchSound, 6.0f, 11024);
  bool gatPlasmaBurstLauncherNormOK = true,
       gatPlasmaBurstLauncherTurboOK= true;
  //This is reset to 0 whenever either of the above are false;
  //each frame, elapsed time is added to it.
  //When less than 512, volume for the respective ambient is 0x7FFF;
  //between 512 and 1024, it decays to 0x4000; above 1024, the ambient
  //is set to zero.
  static float gpblNRevTime = 10000.0f, gpblTRevTime = 10000.0f;

  static Waveform missile("missile");
  ShipStaticEvent missileLauncher(missile.get(), 5.0f, missile.size());
  bool missileLauncherOK = true;

  static Waveform monophas("monophas");
  ShipStaticEvent monophasicEnergyEmitter(monophas.get(), 3.0f, monophas.size());
  bool monophasicEnergyEmitterOK = true;

  static Waveform partbeam("partbeam");
  ShipStaticEvent particleBeamLauncher(partbeam.get(), 2.5f, partbeam.size());
  bool particleBeamLauncherOK = true;

  static Waveform shieldImpact("shieldh");
  AudioSource* shieldImpactSound() { return shieldImpact.get(); }
  unsigned shieldImpactLength() { return shieldImpact.size(); }

  void shipSoundEffects(float et,Ship* ship) {
    SDL_LockAudio();

    if (!gatPlasmaBurstLauncherNormOK ) gpblNRevTime=0;
    else                                gpblNRevTime+=et;
    if (!gatPlasmaBurstLauncherTurboOK) gpblTRevTime=0;
    else                                gpblTRevTime+=et;

    shipBackground.set(ship->isStealth()? 0x0800 : 0x2000);
    fissionPower.set   (ship->isStealth()? 0 : (Sint16)(min(32767.0f, 0x5000*ship->getPowerUsagePercent())));
    fusionPower.set    (ship->isStealth()? 0 : (Sint16)(min(32767.0f, 0x7FFF*ship->getPowerUsagePercent())));
    antimatterPower.set(ship->isStealth()? 0 : (Sint16)(min(32767.0f, 0x7FFF*ship->getPowerUsagePercent())));
    particleAccelerator.set     (ship->isStealth()? 0 : (Sint16)(0x5000*ship->getThrust()));
    bussardRamjet.set           (ship->isStealth()? 0 : (Sint16)(0x7FFF*ship->getThrust()));
    superParticleAccelerator.set(ship->isStealth()? 0 : (Sint16)(0x7FFF*ship->getThrust()));
    relIonAccelerator       .set(ship->isStealth()? 0 : (Sint16)(0x7FFF*ship->getThrust()));
    miniGravwaveDrive.set  ((Sint16)(0x3000*ship->getThrust()));
    miniGravwaveDriveII.set((Sint16)(0x4000*ship->getThrust()));
    shieldGenerator.set(ship->shieldsDeactivated || ship->stealthMode? 0 : 0x7FFF);
    gatPlasmaBurstLauncherRevNorm .set(gpblNRevTime < 512? 0x7FFF
                                      :gpblNRevTime < 1024? 0x4000+0x3FFF/512.0f*(1024.0f - gpblNRevTime)
                                      :0);
    gatPlasmaBurstLauncherRevTurbo.set(gpblTRevTime < 512? 0x7FFF
                                      :gpblTRevTime < 1024? 0x4000+0x3FFF/512.0f*(1024.0f - gpblTRevTime)
                                      :0);

    float currentCapacitance = ship->getCapacitancePercent();
    for (unsigned i=0; i<lenof(capacitorRing); ++i) if (capacitorRing[i]) {
      float min = i/(float)lenof(capacitorRing);
      float max = (i < lenof(capacitorRing)-1? min + 1.0f/lenof(capacitorRing) : 1.1f);
      capacitorRing[i]->set(min < currentCapacitance && currentCapacitance <= max? 0x5000 : 0x0000);
    }

    energyChargeLauncherOK = true;
    magnetoBombLauncherOK = true;
    sgBombLauncherOK = true;
    plasmaBurstLauncherOK = true;
    gatPlasmaBurstLauncherNormOK = true;
    gatPlasmaBurstLauncherTurboOK= true;
    missileLauncherOK = true;
    monophasicEnergyEmitterOK = true;
    particleBeamLauncherOK = true;
    SDL_UnlockAudio();
  }
}
