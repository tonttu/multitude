#include "GfxNode.hpp"

namespace Luminous
{
  class GfxNode::D
  {
  public:
    D();

  public:
    /// List of all listeners added in onReady()
    QList<QPair<GfxNode::CallbackType, GfxNode::ListenerType>> m_readyCallbacks;
    QList<QPair<GfxNode::CallbackType, GfxNode::ListenerType>> m_readyOnceCallbacks;
    QList<QPair<GfxNode::CallbackType, GfxNode::ListenerType>> m_headerReadyCallbacks;
    QList<QPair<GfxNode::CallbackType, GfxNode::ListenerType>> m_headerReadyOnceCallbacks;
    /// Protects m_readyCallbacks and isReady() call in onReady()
    std::shared_ptr<Radiant::Mutex> m_readyCallbacksMutex;
    /// Used by onReady to check if there already is "ready" event listener
    int m_hasReadyListener;
    int m_hasHeaderReadyListener;
  };

  GfxNode::D::D()
    : m_readyCallbacksMutex(new Radiant::Mutex()),
      m_hasReadyListener(0),
      m_hasHeaderReadyListener(0)
  {}

  GfxNode::GfxNode()
    : m_d(new D())
  {}

  GfxNode::GfxNode(Node * host, const QByteArray & name, bool transit)
    : Node(host, name, transit)
    , m_d(new D())
  {}

  GfxNode::~GfxNode()
  {
    // Make sure that any of the callbacks doesn't get access to this
    clearReadyCallbacks();
    m_d->m_readyCallbacksMutex.reset();
  }

  GfxNode::GfxNode(GfxNode && node)
    : Node(std::move(node)),
      m_d(std::move(node.m_d))
  {
  }

  GfxNode & GfxNode::operator=(GfxNode && node)
  {
    Node::operator=(std::move(node));
    std::swap(m_d, node.m_d);
    return *this;
  }

  bool GfxNode::isReady() const
  {
    return true;
  }

  bool GfxNode::isHeaderReady() const
  {
    return true;
  }

  void GfxNode::clearReadyCallbacks()
  {
    Radiant::Guard g(*m_d->m_readyCallbacksMutex);
    m_d->m_readyCallbacks.clear();
    m_d->m_readyOnceCallbacks.clear();
    m_d->m_headerReadyCallbacks.clear();
    m_d->m_headerReadyOnceCallbacks.clear();
  }

  void GfxNode::onReady(CallbackType callback, bool once, ListenerType type)
  {
    Radiant::Guard g(*m_d->m_readyCallbacksMutex);
    std::weak_ptr<Radiant::Mutex> weak = m_d->m_readyCallbacksMutex;

    if ((m_d->m_hasReadyListener & type) == 0) {
      m_d->m_hasReadyListener |= type;
      eventAddListener("ready", [=] {
        std::shared_ptr<Radiant::Mutex> mptr = weak.lock();
        if(!mptr) return;
        Radiant::Guard g(*mptr);
        for (auto p: m_d->m_readyCallbacks)
          if (p.second == type)
            p.first(this);
        for (auto it = m_d->m_readyOnceCallbacks.begin(); it != m_d->m_readyOnceCallbacks.end();) {
          if (it->second == type) {
            it->first(this);
            it = m_d->m_readyOnceCallbacks.erase(it);
          } else ++it;
        }
      }, type);
    }

    if (isReady()) {
      callback(this);
    } else if (once) {
      m_d->m_readyOnceCallbacks << qMakePair(callback, type);
    }
    if(!once) m_d->m_readyCallbacks << qMakePair(callback, type);
  }

  void GfxNode::onHeaderReady(CallbackType callback, bool once, ListenerType type)
  {
    Radiant::Guard g(*m_d->m_readyCallbacksMutex);
    std::weak_ptr<Radiant::Mutex> weak = m_d->m_readyCallbacksMutex;

    if ((m_d->m_hasHeaderReadyListener & type) == 0) {
      m_d->m_hasHeaderReadyListener |= type;
      eventAddListener("header-ready", [=] {
        std::shared_ptr<Radiant::Mutex> mptr = weak.lock();
        if(!mptr) return;
        Radiant::Guard g(*mptr);
        for (auto p: m_d->m_headerReadyCallbacks)
          if (p.second == type)
            p.first(this);
        for (auto it = m_d->m_headerReadyOnceCallbacks.begin(); it != m_d->m_headerReadyOnceCallbacks.end();) {
          if (it->second == type) {
            it->first(this);
            it = m_d->m_headerReadyOnceCallbacks.erase(it);
          } else ++it;
        }
      }, type);
    }

    if (isHeaderReady()) {
      callback(this);
    } else if (once) {
      m_d->m_headerReadyOnceCallbacks << qMakePair(callback, type);
    }
    if(!once) m_d->m_headerReadyCallbacks << qMakePair(callback, type);
  }

  void GfxNode::onReadyVoid(std::function<void(void)> readyCallback, bool once, ListenerType type)
  {
    onReady([readyCallback](Luminous::GfxNode*) { readyCallback(); }, once, type);
  }

  void GfxNode::onHeaderReadyVoid(std::function<void(void)> readyCallback, bool once, ListenerType type)
  {
    onHeaderReady([readyCallback](Luminous::GfxNode*) { readyCallback(); }, once, type);
  }

} // namespace Luminous
