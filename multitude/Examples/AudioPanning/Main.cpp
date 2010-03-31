/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: Helsinki University of Technology, MultiTouch Oy and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include <Radiant/Sleep.hpp>
#include <Radiant/Trace.hpp>

#include <Resonant/DSPNetwork.hpp>
#include <Resonant/ModuleSamplePlayer.hpp>
#include <Resonant/ModulePanner.hpp>

#include <sndfile.h>

#include <errno.h>
#include <stdlib.h>

/** A simple application that plays audio samples.
    
*/

int main(int argc, char ** argv)
{
  const char * file = "../test.wav";
  float pitch = 1.0f;
  int repeats = 5;
  bool loop = true;

  Resonant::DSPNetwork::Item item;

  for(int i = 1; i < argc; i++) {
    if(strcmp(argv[i], "--sample") == 0 && (i + 1) < argc)
      file = argv[++i];
    else if(strcmp(argv[i], "--repeat") == 0 && (i + 1) < argc)
      repeats = atoi(argv[++i]);
    else if(strcmp(argv[i], "--targetchannel") == 0 && (i + 1) < argc)
      item.setTargetChannel(atoi(argv[++i]) - 1);
    else if(strcmp(argv[i], "--verbose") == 0)
      Radiant::enableVerboseOutput(true);
    else {
      printf("%s # Unknown argument \"%s\"\n", argv[0], argv[i]);
      return EINVAL;
    }
  }

  if(loop)
    repeats = 1;
  
  SF_INFO info;
  SNDFILE * sndf = sf_open(file, SFM_READ, & info);

  if(!sndf) {
    Radiant::error("Could not open sound file \"%s\"", file);
    return EINVAL;
  }
  
  sf_close(sndf);

  Resonant::DSPNetwork dsp;

  dsp.start();

  Radiant::BinaryData control;

  {
    Resonant::DSPNetwork::Item pitem;
    
    item.setModule(new Resonant::ModulePanner(0));
    item.module()->setId("panner");
    
    control.rewind();
    control.writeInt32(2);
    control.rewind();
    
    item.module()->processMessage("channels", & control);

    control.rewind();
    item.module()->processMessage("fullhdstereo", & control);
    
    dsp.addModule(item);

  }

  item.setModule(new Resonant::ModuleSamplePlayer(0));
  item.module()->setId("sampleplayer");
  
  control.rewind();
  control.writeInt32(2);
  control.rewind();

  item.module()->processMessage("channels", & control);
  
  dsp.addModule(item);


  control.rewind();
  // Id of the receiving module
  control.writeString("sampleplayer/playsample");

  // File name
  control.writeString(file);

  // Gain
  control.writeString("gain");
  control.writeFloat32(0.745f);

  // Relative pitch
  control.writeString("relpitch");
  control.writeFloat32(pitch);

  // Infinite looping;
  control.writeString("loop");
  control.writeInt32(loop);

  control.writeString("end");

  dsp.send(control);

  Radiant::Sleep::sleepMs(500);

  int locx = 0;

  for(int i = 1; i < 100; i++) {

    Radiant::Sleep::sleepS(1);

    if(locx == 0)
      locx = 960;
    else if(locx == 960)
      locx = 1920;
    else if(locx == 1920)
      locx = 0;
    
    control.rewind();
    control.writeString("panner/setsourcelocation");
    control.writeString("sampleplayer-0"); // index
    control.writeVector2Float32(Nimble::Vector2f(locx, 540)); // index    
    dsp.send(control);
  }
  
  dsp.stop();

  return 0;
}

