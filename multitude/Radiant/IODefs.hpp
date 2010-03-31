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

#ifndef RADIANT_IODEFS_HPP
#define RADIANT_IODEFS_HPP

// @todo move to Resonant (if needed) -> VideoInput uses some of these, so moving to Resonant
// is not really practical or desireable.

namespace Radiant {

  /** Input/output modes. */
  enum IoMode
    {
      /// All bits zero.
      IO_NONE = 0x0,
      /// First bit set.
      IO_INPUT = 0x1,
      /// Second bit set.
      IO_OUTPUT = 0x2,
      /// Both first and second bit set.
      IO_INPUT_OUTPUT = 0x3
    };

  /** Audio sample types. */
  enum AudioSampleFormat
    {
      /// 16 bit two's complement audio samples.
      ASF_INT16,
      /// 24 bit two's complement audio samples.
      ASF_INT24,
      /** 32 bit two's complement audio samples.

      If this format is used for 24 bit samples, the samples must be
      MSB aligned.
      */
      ASF_INT32,
      /// 32 bit floating point audio samples in the range [-1.0,1.0].
      ASF_FLOAT32,
      /// 64 bit floating point audio samples in the range [-1.0,1.0].
      ASF_FLOAT64
    };
  /** Returns the number of bytes a particular sample type uses. */
  inline int sampleWidth(AudioSampleFormat format)
  {
    switch(format) {
    case ASF_INT16:
      return 2;
    case ASF_INT24:
      return 3;
    case ASF_INT32:
    case ASF_FLOAT32:
      return 4;
    case ASF_FLOAT64:
      return 8;
    }
    return 0;
  }

}


#endif
