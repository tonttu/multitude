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

#ifndef LUMINOUS_GARBAGE_COLLECTOR_HPP
#define LUMINOUS_GARBAGE_COLLECTOR_HPP

#include <Luminous/Export.hpp>

#include <Radiant/Mutex.hpp>

#include <set>

namespace Luminous
{

  class Collectable;

  /// This class is used to keep track of objects that have been deleted.

  /** The general usage pattern is as follows:

      <pre>

      // Application main loop:
      while(true) {

        // Clean up the collector:
        GarbageCollector::clear();

        // When Collectable objects are deleted, they store their pointers here
        updateLogic();

    // Go set the OpenGL context
    setOpenGLContext1();

    // Remove the deleted resources:
    GLResources * rsc1 = getResources1();
    rsc1->eraseResources();
    renderOpenGL();

    // Then another OpenGL context:
    setOpenGLContext2();

    // Remove the deleted resources:
    GLResources * rsc2 = getResources2();
    rsc2->eraseResources();
    renderOpenGL();
      }
      </pre>

      This code snippet is for the single-threaded case, with two
      OpenGL contexts.
   */
  /// @todo Rename??
  class LUMINOUS_API GarbageCollector
  {
  public:

    typedef std::set<Collectable *> container;
    typedef container::iterator iterator;

    /// Empties the garbage list.
    static void clear();

    /// Adds the obj to the list of deleted objects
    static void objectDeleted(Collectable * obj);

    /// Iterator to the beginning of the list
    static iterator begin() { return m_items.begin(); }
    /// Iterator to the end of the list
    static iterator end() { return m_items.end(); }

    /// Gets an object from an interator
    /** By using this method to access the object, you can guarantee
      that your code will work even if the container type is
      changed. */
    static Collectable * getObject(iterator & it) { return (*it); }

    static int size() { return (int) m_items.size(); }

    static Radiant::MutexStatic & mutex();
  private:

    GarbageCollector();
    ~GarbageCollector();

    static container m_items;

  };
}

#endif
