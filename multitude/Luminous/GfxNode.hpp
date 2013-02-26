#ifndef LUMINOUS_GFXNODE_HPP
#define LUMINOUS_GFXNODE_HPP

#include "Export.hpp"

#include <Valuable/Node.hpp>

namespace Luminous
{
  class LUMINOUS_API GfxNode : public Valuable::Node
  {
  public:
    typedef std::function<void(Luminous::GfxNode*)> CallbackType;

  public:
    /// @copydoc Valuable::Node::Node
    GfxNode();
    /// @copydoc Valuable::Node::Node
    GfxNode(Node * host, const QByteArray & name = "", bool transit = false);

    virtual ~GfxNode();

    GfxNode(GfxNode && node);
    GfxNode & operator=(GfxNode && node);

    /// Is the object ready to be displayed on screen. Objects that require some
    /// initialization or data that is provided using asynchronous methods may
    /// not be ready for rendering before the initialization is complete. This
    /// function should be overriden in derived classes if they require time to
    /// setup before they can be rendered.
    /// @sa MultiWidgets::Widget::isReady(bool);
    /// @return true for the default implementation
    virtual bool isReady() const;

    /// Callback is called when the node is ready or immediately if the node is already ready
    /// This is needed for ensuring that callbacks get called. For example the following
    /// code is not thread-safe:
    /// if(n.isReady()) {
    ///   /// do something
    /// } else {
    ///   n.eventAddListener("ready", "do smth", ..);
    /// }
    template <typename T>
    void onReady(std::function<void(T*)> readyCallback, bool once = true, ListenerType type = AFTER_UPDATE)
    {
      onReady([readyCallback](Luminous::GfxNode * n) { readyCallback(static_cast<T*>(n)); }, once, type);
    }

    template <typename T, template <typename> class Ptr >
    void onReady(std::function<void(Ptr<T>)> readyCallback, bool once = true, ListenerType type = AFTER_UPDATE)
    {
      onReady([readyCallback](Luminous::GfxNode * n) {
        T* ptr = static_cast<T*>(n);
        readyCallback(Ptr<T>(ptr));
      }, once, type);
    }

    void clearReadyCallbacks();

    void onReady(CallbackType readyCallback, bool once = true, ListenerType type = AFTER_UPDATE);

    /// Same as other onReady-functions but supports functions taking none parameters
    //  In perfect world compiler could deduce the types more correctly but at least g++ 4.6.3
    //  had serious problems with type inference when function below was named "onReady"
    void onReadyVoid(std::function<void(void)> readyCallback, bool once = true, ListenerType type = AFTER_UPDATE);

  private:
    class D;
    std::unique_ptr<D> m_d;
  };
} // namespace Luminous

#endif // LUMINOUS_GFXNODE_HPP
