#include "EventWrapper.hpp"

#include <Valuable/ListenerHolder.hpp>

namespace Valuable
{

  folly::Future<folly::Unit>
  wrapEvent(Valuable::Node* node, const QByteArray &event)
  {
    return wrapEvent(node, event, []{ return true; });
  }

  folly::Future<Radiant::BinaryData>
  wrapBdEvent(Valuable::Node* node, const QByteArray &event)
  {
    return wrapBdEvent(node, event, [] (Radiant::BinaryData){ return true; });
  }


  folly::Future<folly::Unit>
  wrapEvent(Valuable::Node* node, const QByteArray &event,
            std::function<bool()> test)
  {
    struct Context
    {
      std::function<bool()> test;
      folly::Promise<folly::Unit> promise;
    };
    std::shared_ptr<Context> ctx = std::make_shared<Context>();
    ctx->test = std::move(test);
    folly::MoveWrapper<Valuable::ListenerHolder> holder;

    holder->add(node, event, [ctx] () {
      if(ctx->test())
        ctx->promise.setValue();
    });

    return ctx->promise.getFuture().then([holder] {/* Remove listener */});
  }

  folly::Future<Radiant::BinaryData>
  wrapBdEvent(Valuable::Node* node, const QByteArray &event,
              std::function<bool(Radiant::BinaryData)> test)
  {
    struct Context
    {
      std::function<bool(Radiant::BinaryData)> test;
      folly::Promise<Radiant::BinaryData> promise;
    };
    std::shared_ptr<Context> ctx = std::make_shared<Context>();
    ctx->test = std::move(test);
    folly::MoveWrapper<Valuable::ListenerHolder> holder;

    holder->addBd(node, event, [ctx] (Radiant::BinaryData& bd) {
      if(ctx->test(bd))
        ctx->promise.setValue(bd);
    });

    return ctx->promise.getFuture()
        .then([holder](Radiant::BinaryData bd) { return bd; });
  }
}
