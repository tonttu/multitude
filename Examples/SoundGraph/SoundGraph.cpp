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
#include <Resonant/ModuleGain.hpp>

#include <errno.h>
#include <stdlib.h>

/** A simple application that plays ambient background.


*/

int main(int argc, char ** argv)
{
  const char * directory = 0;
  float gainv = 0.5f;

  for(int i = 1; i < argc; i++) {
    if(strcmp(argv[i], "--dir") == 0 && (i + 1) < argc)
      directory = argv[++i];
    else if(strcmp(argv[i], "--gain") == 0 && (i + 1) < argc)
      gainv = atof(argv[++i]);
    else if(strcmp(argv[i], "--verbose") == 0)
      Radiant::enableVerboseOutput(true);
    else {
      printf("%s # Unknown argument \"%s\"\n", argv[0], argv[i]);
      return EINVAL;
    }
  }

  if (directory == 0)
  {
    printf("Usage: %s --dir <directoryname> --gain [gainvalue] --verbose", argv[0]);
    return 1;
  }

  std::shared_ptr<Resonant::DSPNetwork> dsp = Resonant::DSPNetwork::instance();

  dsp->start();

  {
    Resonant::ModuleSamplePlayer * player = new Resonant::ModuleSamplePlayer(0);

    player->setId("myplayer");
    player->createAmbientBackground(directory, 1.0f);
    player->eventProcessInt("channels", 2);

    Resonant::DSPNetwork::Item playerItem;
    playerItem.setModule(player);
    dsp->addModule(playerItem);
  }

  {
    Resonant::ModuleGain * gain = new Resonant::ModuleGain(0);
    gain->setId("mygain");
    gain->setGainInstant(gainv);

    Resonant::DSPNetwork::Item gainItem;
    gainItem.setModule(gain);
    dsp->addModule(gainItem);
  }

  Radiant::Sleep::sleepS(1000);

  dsp->stop();

  return 0;
}

