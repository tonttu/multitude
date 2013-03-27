/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef CODEC_REGISTERY_HPP
#define CODEC_REGISTERY_HPP

#include "Export.hpp"

#include <vector>
#include <map>
#include <QString>
#include <QFile>
#include <memory>

namespace Luminous
{
  class ImageCodec;

  /** CodecRegistry keeps track of different registered ImageCodecs that can be
   * used to load images. **/
  class LUMINOUS_API CodecRegistry
  {
  public:
    CodecRegistry();
    ~CodecRegistry();

    /// Try to get a codec that could load a given file.
    /// @param filename name of the file to load
    /// @param file the file to query
    /// @return returns a pointer to a codec that reports it can load the given
    /// file or NULL if no codec is found.
    std::shared_ptr<ImageCodec> getCodec(const QString & filename, QFile * file = nullptr);
    /// Register a new codec that can be used to load images
    /// @param codec the new codec
    void registerCodec(std::shared_ptr<ImageCodec> codec);

  private:
    typedef std::vector<std::shared_ptr<ImageCodec> > Codecs;
    typedef std::multimap<QString, std::shared_ptr<ImageCodec> > Aliases;

    Codecs m_codecs;
    Aliases m_aliases;
  };

}

#endif
