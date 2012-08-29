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

#include <Patterns/NotCopyable.hpp>

#include <QDropEvent>

namespace Radiant
{
  class DropEvent;

  class DropListener
  {
  public:

    virtual bool dropEvent(const DropEvent & ) = 0;
  };

  /// An object representing drop-data from a drag-and-drop operation.
  class RADIANT_API DropEvent : public Patterns::NotCopyable
  {
  public:
    /// Creates a drop event that contains a collection of URLs
    DropEvent(const QList<QUrl> & urls);
    DropEvent(const QDropEvent & de);
    /// Deletes the drop event
    ~DropEvent();

    /// Returns true if the event contains URLs
    bool hasUrls() const;
    /// Return the list of URLs
    QList<QUrl>	urls() const;

    static void addDropListener(DropListener *);
    static void removeDropListener(DropListener *);
    static bool deliverDropToListeners(const DropEvent &);

  private:
    class D;
    D * m_d;
  };

  }
#endif // DROPEVENT_HPP
