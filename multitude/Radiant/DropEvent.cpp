#include "DropEvent.hpp"

#include <QUrl>

namespace Radiant
{

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


  }
