/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */
#ifndef RADIANT_RESOURCE_LOCATOR_HPP
#define RADIANT_RESOURCE_LOCATOR_HPP

#include "Export.hpp"
#include "Singleton.hpp"

#include <QString>
#include <QStringList>

namespace Radiant
{

  /// This class provides resource location utilities. This class contains a
  /// list of search paths that get searched for files whenever any Qt I/O
  /// operation is performed. This includes QFile, QDir, QFileInfo, etc. Other I/O
  /// APIs are not affected.
  class RADIANT_API ResourceLocator
  {
    /// @cond

    DECLARE_SINGLETON(ResourceLocator);

    /// @endcond

    public:
      /// Constructs a new ResourceLocator
      ResourceLocator();
      /// Destroys the ResourceLocator
      ~ResourceLocator();

      /// This enum describes filtering options available to ResourceLocator;
      /// e.g. for ResourceLocator::locate(). The filter value is specified by
      /// combining values from the following list using bitwise OR operator:
      enum Filter {
          FILES     = (1 << 0)        ///< List files
        , DIRS      = (1 << 1)        ///< List directories
        , WRITEABLE = (1 << 2)        ///< List files which the application has write access. Must be combined with Dirs or Files.
        , ALL_ENTRIES = FILES & DIRS   ///< List directories and files
      };

      /// Get the defined search paths in the resource locator
      /// @return list of search paths
      const QStringList & searchPaths() const;

      /// Add a search path
      /// @param path path to add
      /// @param inFront if true, add to the front of the search list; otherwise add to the end
      void addSearchPath(const QString & path, bool inFront = false);
      /// Add a list of search paths
      /// @param paths paths to add
      /// @param inFront if true, add to the front of the search list; otherwise add to the end
      void addSearchPaths(const QStringList & paths, bool inFront = false);

      /// Locate a path. Returns a list of matching paths or an empty list if no matches are found.
      /// @param path path to search for
      /// @param filter filter the search results
      /// @return list of matching paths or an empty list
      QStringList locate(const QString & path, Filter filter = ALL_ENTRIES) const;

    private:
      class D;
      D * m_d;
  };

}

#endif
