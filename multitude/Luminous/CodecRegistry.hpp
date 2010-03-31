/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef CODEC_REGISTERY_HPP
#define CODEC_REGISTERY_HPP

#include <vector>
#include <map>
#include <string>

namespace Luminous
{
  class ImageCodec;

  /** CodecRegistry keeps track of different registered ImageCodecs that can be
   * used to load images. **/
  class CodecRegistry
  {
  public:
    CodecRegistry();
    ~CodecRegistry();

    /// Try to get a codec that could load a given file.
    /// @param filename name of the file to load
    /// @param file the file to query
    /// @return returns a pointer to a codec that reports it can load the given
    /// file or NULL if no codec is found.
    ImageCodec * getCodec(const std::string & filename, FILE * file = 0);
    /// Register a new codec that can be used to load images
    /// @param codec the new codec
    void registerCodec(ImageCodec * codec);

  private:
    typedef std::vector<ImageCodec *> Codecs;
    typedef std::multimap<std::string, ImageCodec *> Aliases;

    Codecs m_codecs;
    Aliases m_aliases;
  };

}

#endif
