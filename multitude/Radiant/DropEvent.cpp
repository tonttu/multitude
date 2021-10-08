/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "DropEvent.hpp"

#include "Mutex.hpp"

#include <QUrl>
#include <QMimeData>

#include <set>

namespace Radiant
{
  DropListener::~DropListener()
  {
    DropEvent::removeDropListener(this);
  }

  class DropEvent::D
  {
  public:
    D() {}

    QList<QUrl> m_urls;
    Nimble::Vector2 m_location;
    std::optional<QDropEvent> m_qevent;
  };

  DropEvent::DropEvent(const QList<QUrl> & urls, Nimble::Vector2 loc)
    : m_d(new D())
  {
    m_d->m_urls = urls;
    m_d->m_location = loc;
  }

  DropEvent::DropEvent(const QDropEvent & de, Nimble::Vector2 loc)
    : m_d(new D())
  {
    if(de.mimeData()->hasUrls()) {
      m_d->m_urls = de.mimeData()->urls();
    }
    m_d->m_location = loc;
    m_d->m_qevent = de;
  }

  DropEvent::~DropEvent()
  {
    delete m_d;
  }

  bool DropEvent::hasUrls() const
  {
    return !m_d->m_urls.empty();
  }

  QList<QUrl>	DropEvent::urls() const
  {
    return m_d->m_urls;
  }

  static QSet<DropListener *> s_listeners;
  static Mutex s_mutex;

  Nimble::Vector2 DropEvent::location() const
  {
    return m_d->m_location;
  }

  std::optional<QDropEvent> DropEvent::qDropEvent() const
  {
    return m_d->m_qevent;
  }

  void DropEvent::addDropListener(DropListener * l)
  {
    Radiant::Guard g(s_mutex);

    s_listeners.insert(l);
  }

  void DropEvent::removeDropListener(DropListener *l)
  {
    Radiant::Guard g(s_mutex);

    if(s_listeners.contains(l))
      s_listeners.erase(s_listeners.find(l));
  }

  bool DropEvent::deliverDropToListeners(const DropEvent & e)
  {
    Radiant::Guard g(s_mutex);

    for(auto it = s_listeners.begin(); it != s_listeners.end(); ++it) {
      if((*it)->dropEvent(e))
        return true;
    }

    return false;
  }

}
