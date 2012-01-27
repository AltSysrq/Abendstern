/**
 * @file
 * @author Jason Lingle
 * @brief Contains the frame recording interface
 */

/*
 * frame_recorder.hxx
 *
 *  Created on: 19.04.2011
 *      Author: jason
 */

#ifndef FRAME_RECORDER_HXX_
#define FRAME_RECORDER_HXX_

/** If #defined, the frame recorder is enabled.
 * Don't define this in production!
 */
#define FRAME_RECORDER_ENABLED

#ifdef FRAME_RECORDER_ENABLED

/** Contains an interface to Abendstern's built-in
 * frame recorder. The recorder, when active, captures the current
 * screen at a framerate of exactly 30 FPS and stores them to
 * images with filenames fitting the pattern of
 * &nbsp;  <code>recorder/XX.NNNNNNNN.bmp</code>
 * where XX is the recording run (starting at 01) and NNNNNNNN is
 * the frame number.
 * It assumes that the directory "recorder" already exists.
 * The images may be converted to a video with the command
 * &nbsp;  <code>ffmpeg -r 30 -b 1200 -i %08d.bmp out.mpg</code>
 *
 * THIS SHOULD NEVER BE ENABLED IN A PRODUCTION BUILD.
 */
namespace frame_recorder {
  /** Sets internal variables up. This MUST be called
   * before any of the other functions.
   */
  void init();
  /** Begins a new capture run. */
  void begin();
  /** Ends the current capture run. */
  void end();
  /** Returns true if currently capturing. */
  bool on();
  /** Updates the frame recorder. */
  void update(float);
}

#endif /* FRAME_RECORDER_ENABLED */

#endif /* FRAME_RECORDER_HXX_ */
