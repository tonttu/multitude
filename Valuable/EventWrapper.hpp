#ifndef EVENTWRAPPER_HPP
#define EVENTWRAPPER_HPP

#include "Export.hpp"

// Include order is crucial! (conflict with Qt)
#include <folly/futures/Future.h>

#include <Valuable/Node.hpp>

namespace Valuable
{

  /// @todo should we also have listenerType as parameter?
  ///       Now all callbacks are defaulted to direct

  /// Wraps the next occurence of the given event from the given object
  /// into future. The returned future is fulfilled when the target node
  /// emits the event next time. Note that the future gets fulfilled only
  /// once.
  ///
  /// @param node Node whose event is wrapped
  /// @param event Event that is wrapped
  /// @return Future that is fulfilled next time the node emits the event
  VALUABLE_API folly::Future<folly::Unit>
  wrapEvent(Valuable::Node* node, const QByteArray& event);

  /// Wraps the next occurence of the given event from the given object
  /// into future. The returned future is fulfilled with the event data
  /// when the target node emits the event next time. Note that the future
  /// gets fulfilled only once.
  ///
  /// @param node Node whose event is wrapped
  /// @param event Event that is wrapped
  /// @return Future that is fulfilled with the event data next time the
  ///         node emits the event
  VALUABLE_API folly::Future<Radiant::BinaryData>
  wrapBdEvent(Valuable::Node* node, const QByteArray& event);

  /// Wraps the next occurence of the given event from the given object
  /// into future. The returned future is fulfilled when the target node
  /// emits the event next time and the test function returns true. The
  /// test-function is executed every time the event is sent. Note that
  /// future can be fulfilled and the test can pass only once.
  ///
  /// @param node Node whose event is wrapped
  /// @param event Event that is wrapped
  /// @param test Test function to filter out the events
  /// @return Future that is fulfilled next time the node emits the event
  ///         that is accepted by the test function
  VALUABLE_API folly::Future<folly::Unit>
  wrapEvent(Valuable::Node* node, const QByteArray& event,
            std::function<bool()> test);

  /// Wraps the next occurence of the given event from the given object
  /// into future. The returned future is fulfilled with the event data
  /// when the target node emits the event next time and the test function
  /// returns true. The test-function is executed every time with the event
  /// data when the event is sent. Note that future can be fulfilled and the
  /// test can pass only once.
  ///
  /// @param node Node whose event is wrapped
  /// @param event Event that is wrapped
  /// @param test Test function to filter out the events
  /// @return Future that is fulfilled next time the node emits the event
  ///         that is accepted by the test function
  VALUABLE_API folly::Future<Radiant::BinaryData>
  wrapBdEvent(Valuable::Node* node, const QByteArray& event,
              std::function<bool(Radiant::BinaryData&)> test);

}

#endif // EVENTWRAPPER_HPP
