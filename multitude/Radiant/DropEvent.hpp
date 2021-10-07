/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_DROPEVENT_HPP
#define RADIANT_DROPEVENT_HPP

#include <Radiant/Export.hpp>

#include "Defines.hpp"

#include <Nimble/Vector2.hpp>

#include <Patterns/NotCopyable.hpp>

#include <QDropEvent>

#include <optional>

namespace Radiant
{
  class DropEvent;

  /// Interface for DropEvent listeners
  /// If a Widget - or any other object - needs to receive operating system drag-and-drop
  /// events, it needs to be inherited from the DropEvent class.
  class RADIANT_API DropListener
  {
  public:
    /// Destructor
    virtual ~DropListener();
    /// A virtual function for receiving drop events.
    /// This function must be implemented by derived classes.
    /// @param de DropEvent to handle.
    /// @return This function should return true if the drop event is consumed by the object.
    ///         If the drop event is ignored it should return false.
    virtual bool dropEvent(const Radiant::DropEvent & de) = 0;
  };

  /// This class abstracts drag and drop events.
  /// Instances of this class are generated when a drag and drop action is completed.
  /// The DropEvent is an experimental part of Cornerstone, and its API may change yet.
  class RADIANT_API DropEvent : public Patterns::NotCopyable
  {
  public:
    /// Creates a drop event that contains a collection of URLs
    /// @param urls List of urls to store with event
    /// @param loc Location of drop in Application's scene coordinates
    DropEvent(const QList<QUrl> & urls, Nimble::Vector2 loc);

    /// Create a drop event from QDropEvent
    /// @param de QDropEvent to copy
    /// @param loc drop location in Application's scene coordinates
    DropEvent(const QDropEvent & de, Nimble::Vector2 loc);
    /// Deletes the drop event
    ~DropEvent();

    /// Check whether the event contains URLs
    /// @return True if event contains URLs
    bool hasUrls() const;
    /// Return the list of URLs
    /// @return URLs related to event
    QList<QUrl>	urls() const;

    /// Returns the location of the drop event in scene coordinates.
    /// In most cases, you need to convert this location to object coordinates.
    /// For Widgets, this can be done with MultiWidgets::Widget::mapFromScene function -
    /// assuming that there is no complex view hierarchy that causes extra transformations.
    /// @return Location of the drop event in scene coordinates.
    Nimble::Vector2 location() const;

    /// Returns the event as QDropEvent if it was originally created from one
    std::optional<QDropEvent> qDropEvent() const;

    /// Registers a DropListener object. After calling this function, the listener is active
    /// and will receive drop events.
    /// This function is thread-safe.
    /// @param l The listener to be registered.
    static void addDropListener(DropListener * l);

    /// Removes a DropListener object from the list of registered listeners.
    /// After calling this function, the listener is no longer active and will
    /// receive no further drop events.
    /// This function is thread-safe.
    /// @param l The listener to be removed.
    static void removeDropListener(DropListener * l);

    /// Deliver a DropEvent to all the registered listeners.
    /// This function iterates through the list of DropListener object that have
    /// been registered with #addDropListener. For each listener it will call the
    /// #Radiant::DropListener::dropEvent, until the the list is exhausted or one of the
    /// function calls returns true.
    /// This function is thread-safe.
    /// @param e The DropEvent to be delivered
    /// @return True if some listeners accepted event, false otherwise.
    static bool deliverDropToListeners(const DropEvent & e);

  private:
    class D;
    D * m_d;
  };

  }
#endif // DROPEVENT_HPP
