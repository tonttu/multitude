/* COPYRIGHT
 *
 * This file is part of VideoDisplay.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include "AVDecoder.hpp"

/// @todo this include is just for create(), should be removed
#include "AVDecoderFFMPEG.hpp"

namespace VideoDisplay
{
  AVDecoder::AVDecoder()
  {
    eventAddOut("error");
    eventAddOut("ready");
    eventAddOut("finished");
  }

  AVDecoder::~AVDecoder()
  {
  }

  std::unique_ptr<AVDecoder> AVDecoder::create(const Options & options, const QString & /*backend*/)
  {
    /// @todo add some great factory registry thing here
    std::unique_ptr<AVDecoder> decoder(new AVDecoderFFMPEG());
    decoder->load(options);
    return decoder;
  }
}
