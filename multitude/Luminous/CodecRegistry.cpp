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

#include "CodecRegistry.hpp"
#include "ImageCodec.hpp"
#include "Luminous.hpp"

#include <Radiant/StringUtils.hpp>

#include <typeinfo>

namespace Luminous
{

  CodecRegistry::CodecRegistry()
  {}

  CodecRegistry::~CodecRegistry()
  {}

  ImageCodec * CodecRegistry::getCodec(const std::string & filename, FILE * file)
  {
    Luminous::initDefaultImageCodecs();

    ImageCodec * codec = 0;

    // Try a codec that matches the extension first
    const std::string ext = filename.substr(filename.rfind(".") + 1);
    Aliases::iterator alias = m_aliases.find(ext);

    if(alias != m_aliases.end())
      codec = alias->second;

    if(file) {
     
      // Verify our choice
      if(codec && codec->canRead(file))
        return codec;

      Radiant::debug("CodecRegistry::getCodec # Default codec failed for %s (%s, %p)",
		     filename.c_str(), ext.c_str(), codec);
      
      // No codec matched the extension, go through all registered codecs and
      // see if they match
      for(Codecs::iterator it = m_codecs.begin(); it != m_codecs.end(); it++) {
        ImageCodec * candidate = *it;

        // We already tried this
        if(candidate == codec) continue;

        if(candidate->canRead(file)) 
          return candidate;
      }
    }

    return codec;
  }

  void CodecRegistry::registerCodec(ImageCodec * codec)
  {
    Radiant::debug("CodecRegistry::registerCodec # %s",
		   typeid(*codec).name());

    namespace su = Radiant::StringUtils;

    m_codecs.push_back(codec);

    // Associate extensions with this codec
    su::StringList exts;
    su::split(codec->extensions(), " ", exts);

    for(su::StringList::iterator it = exts.begin(); it != exts.end(); it++) {
      m_aliases.insert(std::make_pair(*it, codec));
      Radiant::debug("Adding codec %p for file type %s", codec, (*it).c_str());
    }    
  }

  
}
