/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include "VideoInput.hpp"

#include <Nimble/Math.hpp>

namespace Radiant {

  static const float table[FPS_COUNT] = {0, 5, 10, 15, 30, 60, 120};

  float asFloat(FrameRate rate)
  {
    if(rate >= FPS_COUNT)
      return table[FPS_IGNORE];

    return table[rate];
  }

  FrameRate closestFrameRate(float fps)
  {
    int best = 0;
    float diff = Nimble::Math::Abs(fps - table[0]);

    for(int i = 0; i < FPS_COUNT; i++) {
      float diff2 = Nimble::Math::Abs(fps - table[i]);
      if(diff > diff2) {
	diff = diff2;
	best = i;
      }
    }
    return (FrameRate) best;
  }

  VideoInput::~VideoInput()
  {}

  void VideoInput::doneImage()
  {}

  const void * VideoInput::captureAudio(int * frameCount)
  {
    * frameCount = 0;
    return 0;
  }

  void VideoInput::getAudioParameters(int * channels, 
				      int * sample_rate,
				      AudioSampleFormat * format)
  {
    * channels = 0;
    * sample_rate = 0;
    * format = ASF_INT16;
  }

  void VideoInput::setGamma(float)
  {}

  void VideoInput::setShutter(float)
  {}

  void VideoInput::setGain(float)
  {}

  void VideoInput::setExposure(float)
  {}

  void VideoInput::setBrightness(float)
  {}

}
