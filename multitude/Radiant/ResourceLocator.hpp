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
#ifndef RADIANT_RESOURCE_LOCATOR_HPP
#define RADIANT_RESOURCE_LOCATOR_HPP

#include <Radiant/Export.hpp>

#include <string>

namespace Radiant
{

  /// Class for locating resources. Basically it searches for a given filename
  /// through a set of paths and returns the first path that contains the given
  /// file.

  /// @todo Documentation, examples
  class RADIANT_API ResourceLocator
  {
    public:
      ResourceLocator();
      ~ResourceLocator();

      /// Character that separates paths.
      static std::string  separator;

      /// Return the paths.
      const std::string & paths() const { return  m_paths; }

      /** Add a path to the list to search though.

          @param path The directory path to be added to the search paths.

          @param front If true, then the new path is placed in front of the path list, and
          future searches will start from it. Otherwise the new path is placed at the end
          of the path list.

       */
      void addPath(const std::string & path, bool front = false);
      /** @copydoc addPath*/
      /// @param module The name of the module for which we are looking for some data.
      void addModuleDataPath(const std::string & module, bool front = false);

      /// Locate a file
      std::string locate(const std::string & file) const;
      /// Locate a file that can be written
      std::string locateWriteable(const std::string & file) const;
      /// Locate an existing file that can be written
      std::string locateOverWriteable(const std::string & file) const;

      /// Returns a ResourceLocator instance
      static ResourceLocator & instance() { return s_instance; }

    private:
      std::string m_paths;

      static ResourceLocator s_instance;
  };

}

#endif
