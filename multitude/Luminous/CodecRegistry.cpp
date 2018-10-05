/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "CodecRegistry.hpp"
#include "ImageCodec.hpp"
#include "Luminous.hpp"

#include <Radiant/StringUtils.hpp>
#include <Radiant/FileUtils.hpp>
#include <Radiant/Trace.hpp>

#include <typeinfo>

#include <QStringList>

namespace Luminous
{

  CodecRegistry::CodecRegistry()
  {}

  CodecRegistry::~CodecRegistry()
  {}

  std::shared_ptr<ImageCodec> CodecRegistry::getCodec(const QString & filename, QFile * file)
  {
    Luminous::initDefaultImageCodecs();

    std::shared_ptr<ImageCodec> codec;

    // Try a codec that matches the extension first
    const QString ext = Radiant::FileUtils::suffix(filename);
    Aliases::iterator alias = m_aliases.find(ext);

    if(alias != m_aliases.end())
      codec = alias->second;

    if(file) {
     
      // Verify our choice
      if(codec && codec->canRead(*file))
        return codec;

      // No codec matched the extension, go through all registered codecs and
      // see if they match
      for(Codecs::iterator it = m_codecs.begin(); it != m_codecs.end(); ++it) {
        auto candidate = *it;

        // We already tried this
        if(candidate == codec)
          continue;

        if(candidate->canRead(*file)) 
          return candidate;
      }

      return nullptr;
    }

    return codec;
  }

  void CodecRegistry::registerCodec(std::shared_ptr<ImageCodec> codec)
  {
    m_codecs.push_back(codec);

    // Associate extensions with this codec
    Q_FOREACH(QString ext, codec->extensions().split(" ", QString::SkipEmptyParts))
      m_aliases.insert(std::make_pair(ext, codec));
  }

  
}
