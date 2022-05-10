/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Mime.hpp"
#include "Thread.hpp"
#include "Trace.hpp"

#include <fstream>
#include <sstream>
#include <cassert>

#include <QFileInfo>
#include <QSet>
#include <QRegExp>
#include <QStringList>

namespace Radiant
{

  MimeType::MimeType(const QString & mime)
    : m_toplevel(mime.section("/", 0, 0)),
      m_subtype(mime.section("/", 1))
  {}

  MimeType::MimeType(const QString & toplevel, const QString & subtype) :
    m_toplevel(toplevel), m_subtype(subtype)
  {}

  MimeType::MimeType(const MimeType & t)
    : m_toplevel(t.m_toplevel)
    , m_subtype(t.m_subtype)
  {}

  MimeType & MimeType::operator=(const MimeType & t)
  {
    m_toplevel = t.m_toplevel;
    m_subtype = t.m_subtype;
    return *this;
  }

  void MimeManager::initialize()
  {
    MULTI_ONCE {

      QFileInfo fi("cornerstone:Mime/mime.types");
      if(!fi.exists()) {
        Radiant::error("MimeManager::initialize # failed to find %s", fi.absoluteFilePath().toUtf8().data());
        return;
      }

      std::ifstream fileStream(fi.absoluteFilePath().toUtf8().data());
      if (!fileStream) {
        Radiant::info("FileLoader : Could not load mime.types");
        return;
      }

      std::string row;

      std::vector<QString> extensions;
      std::string mime;

      while (std::getline(fileStream, row)) {

        // skip comments and empty rows
        if (row.length() == 0 || row[0] == '#')
          continue;

        extensions.clear();
        std::istringstream rowStream(row);
        rowStream >> mime;

        std::string extension;
        while (rowStream >> extension)
          extensions.push_back(extension.c_str());

        Radiant::MimeType mimeType(mime.c_str());
        for (unsigned int i=0; i < extensions.size(); ++i) {
          s_sharedExtensions.insert(std::make_pair(extensions[i], mimeType));
        }
      }
    }
  }

  MimeManager::MimeManager() {
    initialize();
  }

  MimeManager::~MimeManager() {
  }

  void MimeManager::insertSharedExtension(const QString & extension, const Radiant::MimeType& type)
  {
    if (s_sharedExtensions.find(extension.toLower()) != s_sharedExtensions.end()) {
      Radiant::info("Overriding shared extension->mime mapping for: %s -> %s", extension.toUtf8().data(), type.typeString().toUtf8().data());
    }
    s_sharedExtensions.insert(std::make_pair(extension.toLower(), type));
  }

  /// add or replace a shared mapping from file extension to mime type
  void MimeManager::insertExtension(const QString & extension, const Radiant::MimeType& type)
  {
    if (m_extensions.find(extension.toLower()) != m_extensions.end()) {
      Radiant::info("Overriding extension->mime mapping for: %s -> %s", extension.toLower().toUtf8().data(), type.typeString().toUtf8().data());
    }
    m_extensions.insert(std::make_pair(extension.toLower(), type));
  }

  const MimeType * MimeManager::mimeTypeByExtension(const QString & ext) const
  {
    ExtensionMap::const_iterator it = m_extensions.find(ext.toLower());
    if (it != m_extensions.end())
      return &it->second;

    it = s_sharedExtensions.find(ext.toLower());
    if (it != s_sharedExtensions.end())
      return &it->second;

    return 0;
  }

  QStringList MimeManager::extensionsByMimeRegexp(const QString & mime) const
  {
    QRegExp r(mime);
    QSet<QString> extensions;
    int step = 0;
    for(ExtensionMap::const_iterator it = m_extensions.begin(),
        end = m_extensions.end(); ; ++it) {
      if (it == end && step == 0) {
        it = s_sharedExtensions.begin();
        end = s_sharedExtensions.end();
        ++step;
      }
      if (it == end && step == 1) break;

      if(r.indexIn(it->second.typeString()) >= 0) {
        extensions << it->first;
      }
    }

    return extensions.toList();
  }

  MimeManager::ExtensionMap MimeManager::s_sharedExtensions;
}
