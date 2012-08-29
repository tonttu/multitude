#include "DropEvent.hpp"

#include "Mutex.hpp"

#include <QUrl>

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
    // QDropEvent m_qevent;
  };

  DropEvent::DropEvent(const QList<QUrl> & urls)
    : m_d(new D())
  {
    m_d->m_urls = urls;
  }

  DropEvent::DropEvent(const QDropEvent & de)
    : m_d(new D())
  {
    if(de.mimeData()->hasUrls()) {
      m_d->m_urls = de.mimeData()->urls();
    }
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

    for(auto it = s_listeners.begin(); it != s_listeners.end(); it++) {
      if((*it)->dropEvent(e))
        return true;
    }

    return false;
  }

  }
