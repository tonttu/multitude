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

#ifndef RADIANT_DROPEVENT_HPP
#define RADIANT_DROPEVENT_HPP

#include <Radiant/Export.hpp>

#include "Defines.hpp"

#include <Nimble/Vector2.hpp>

#include <Patterns/NotCopyable.hpp>

#include <QDropEvent>

namespace Radiant
{
  class DropEvent;

  /// Interface for DropEvent listeners
  /** If a Widget - or any other object - needs to receive operating system drag-and-drop
      events, it needs to be inherited from the DropEvent class. */
  class RADIANT_API DropListener
  {
  public:
    virtual ~DropListener();
    /** A virtual function for receiving drop events.
        This function must be implemented by derived classes.

        @return This function should return true if the drop event is consumed by the object.
        If the drop event is ignored it should return false.
    */
    virtual bool dropEvent(const Radiant::DropEvent & ) = 0;
  };

  /** This class abstracts drag and drop events. It is generated when a drag
      and drop action is completed.

      The DropEvent is an experimental part of Cornerstone, and its API may change yet.
  */
  class RADIANT_API DropEvent : public Patterns::NotCopyable
  {
  public:
    /// Creates a drop event that contains a collection of URLs
    DropEvent(const QList<QUrl> & urls, Nimble::Vector2 loc);
    /// Create a drop event from QDropEvent
    /// @param de QDropEvent to copy
    /// @param loc drop location
    /// @todo location in what coordinates?
    DropEvent(const QDropEvent & de, Nimble::Vector2 loc);
    /// Deletes the drop event
    ~DropEvent();

    /// Returns true if the event contains URLs
    bool hasUrls() const;
    /// Return the list of URLs
    QList<QUrl>	urls() const;

    /// Returns the location of the drop event, in window coordinates
    /** In most cases, you need to convert this location to object coordinates.
        For Widgets, this can be done with MultiWidgets::Widget::mapFromScene function -
        assuming that there is no complex view hierarchy that causes extra transformations.
    */
    Nimble::Vector2 location() const;

    /** Registers a DropListener object. After calling this function, the listener is active
        and will receive drop events.

        This function is thread-safe.

        @param l The listener to be registered.
    */
    static void addDropListener(DropListener * l);
    /** Removes a DropListener object from the list of registered listeners.
        After calling this function, the listener is no longer active and will
        receive no further drop events.

        This function is thread-safe.

        @param l The listener to be removed.
    */
    static void removeDropListener(DropListener * l);
    /** Deliver a DropEvent to all the registered listeners.
        This function iterates through the list of DropListener object that have
        been registered with #addDropListener. For each listener it will call the
        #Radiant::DropListener::dropEvent, until the the list is exhausted or one of the
        function calls returns true.

        This function is thread-safe.

        @param e The DropEvent to be delivered
    */
    static bool deliverDropToListeners(const DropEvent & e);

  private:
    class D;
    D * m_d;
  };

  }
#endif // DROPEVENT_HPP
