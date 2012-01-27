/**
 * @file
 * @author Jason Lingle
 *
 * @brief Defines a number of sounds used by the in-game UI.
 *
 * These are used by the HUD and HumanController.
 */

#ifndef UI_SOUNDS_HXX_
#define UI_SOUNDS_HXX_

#include "audio.hxx"
#include "synth.hxx"

namespace audio {
  /** ChangeTarget is a square wave at 2600 Hz (Bell's [in]famous
   * line-reset tone) in two pulses.
   */
  typedef Virtual<Duplex<
          Seq<Duration<Playtab<squaretab,2600>,AUDIO_FRQ/10>,
          Seq<Duration<Silence,AUDIO_FRQ/20>,
              Duration<Playtab<squaretab,2600>,AUDIO_FRQ/10> > > > > ChangeTarget;

  /** Sound to play when activating stealth mode. */
  typedef Virtual<Duplex<
          Mult<Modulate<Playtab<squaretab,1024>,Playtab<decaytab,3> >,
               Playtab<decaytab,3,false> > > > StealthOn;
  /** Sound to play when leaving stealth mode. */
  typedef Virtual<Duplex<
          Mult<Modulate<Playtab<squaretab,1024>,Playtab<buildtab,3> >,
               Playtab<buildtab,3,false> > > > StealthOff;

  /* All weapons-selection tones use a system based off of DTMF
   * using prime numbers, according to this grid:
   *      1009 2111 3499 4001
   * 307  1    2    3    4
   * 659  Q    W    E    R
   * 827  A    S    D    F
   */
  #define DDTMF(H,V,name) \
  typedef Virtual<Duplex<Duration<Mix2<Playtab<squaretab,H>, Playtab<squaretab,V> >,AUDIO_FRQ/8> > > name
  DDTMF(1009,307,Dtmf00);
  DDTMF(2111,307,Dtmf01);
  DDTMF(3499,307,Dtmf02);
  DDTMF(4001,307,Dtmf03);
  DDTMF(1009,659,Dtmf10);
  DDTMF(2111,659,Dtmf11);
  DDTMF(3499,659,Dtmf12);
  DDTMF(4001,659,Dtmf13);
  DDTMF(1009,827,Dtmf20);
  DDTMF(2111,827,Dtmf21);
  DDTMF(3499,827,Dtmf22);
  DDTMF(4001,827,Dtmf23);
  #undef DDTMF

  /** Weapon-not-found is a harsh, low-pitched buzz in two pulses */
  typedef Virtual<Duplex<Modulate<
            Seq<Duration<Playtab<squaretab,330>,32>,
            Seq<Duration<Silence, 24>,
                Duration<Playtab<squaretab,330>,32> > >,
                         /* modulate by */
                         Playtab<decaytab,30> > > >
      WeaponNotFound;

  /** MapBoundaryWarning is a simple 12%-volume high-pitched tone dropping to 4% after 1 second.
   */
  typedef Virtual<Duplex<Seq<Duration<VolumeDown<Playtab<squaretab,1760>,0x2000>, AUDIO_FRQ >,
                             VolumeDown<Playtab<squaretab,1760>,0x0500> > > > MapBoundaryWarning;

  /** CapacitorWarning is a simple A4 tone */
  typedef Virtual<Duplex<Playtab<squaretab,440> > > CapacitorWarning;

  /** PowerWarning is is a 1-second C5 tone followed by infinite silence. */
  typedef Virtual<Duplex<Duration<Playtab<squaretab,523>, AUDIO_FRQ> > > PowerWarning;

  /** HeatWarning is a beeping G5 tone */
  typedef Virtual<Duplex<Loop<Seq<Duration<Playtab<squaretab,783>,AUDIO_FRQ/4>,
                                  Duration<Silence,AUDIO_FRQ/4> > > > > HeatWarning;
  /** HeatDanger is a continuous G5 tone */
  typedef Virtual<Duplex<Playtab<squaretab,783> > > HeatDanger;

  /** ShieldUp is the tones E5-F5; ShieldDown is the reverse.
   * Both are followed by infinite silence.
   */
  typedef Virtual<Duplex<Seq<Duration<Playtab<squaretab,659>,AUDIO_FRQ/8>,
                             Duration<Playtab<squaretab,698>,AUDIO_FRQ/8> > > > ShieldUp;
  /** ShieldUp is the tones E5-F5; ShieldDown is the reverse.
   * Both are followed by infinite silence.
   */
  typedef Virtual<Duplex<Seq<Duration<Playtab<squaretab,698>,AUDIO_FRQ/8>,
                             Duration<Playtab<squaretab,659>,AUDIO_FRQ/8> > > > ShieldDown;
}

#endif /* UI_SOUNDS_HXX_ */
