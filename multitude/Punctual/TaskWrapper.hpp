#ifndef TASKWRAPPER_HPP
#define TASKWRAPPER_HPP

#include <Radiant/BGThread.hpp>
#include <Radiant/Task.hpp>

#include <folly/futures/Promise.h>
#include <boost/expected/expected.hpp>

#include <boost/variant/variant.hpp>
#include <boost/variant/get.hpp>

namespace Punctual
{

  struct NotReadyYet {};

  template <typename T>
  using WrappedTaskReturnType = boost::variant<NotReadyYet, boost::expected<T>>;

  /// Signaling is done using the return value that can either provide
  /// result or reason, the result couldn't produced or 'Not ready yet, try
  /// again later'-status. This is encoded in boost::variant
  template <typename T>
  using WrappedTaskFunc = std::function<WrappedTaskReturnType<T>(void)>;

  /// Creates a task and places it to BGThread. Future is fulfilled when
  /// the function has returned boost::excepted<T> that contains value or
  /// exception_ptr
  template <typename T>
  folly::Future<T> createWrappedTask(WrappedTaskFunc<T>&& func);

  template <typename T>
  folly::Future<T> createWrappedTask(std::function<boost::expected<T>(void)>&& func);

  template <typename T>
  folly::Future<T> createWrappedTask(std::function<T(void)>&& func);

  // ---------------------- Implementation below -----------------------------
  // -------------------------------------------------------------------------

  template <typename T>
  class WrappedTask : public Radiant::Task
  {
  public:
    WrappedTask(WrappedTaskFunc<T>&& f, folly::Promise<T>&& promise);
    virtual void doTask() override;

  private:
    WrappedTaskFunc<T> m_func;
    folly::Promise<T> m_promise;
  };

  // ---------------------------------------------------------------------------

  template <typename T>
  folly::Future<T> createWrappedTask(WrappedTaskFunc<T>&& func)
  {
    folly::Promise<T> promise;
    auto future = promise.getFuture();
    auto task = std::make_shared<WrappedTask<T>>(std::move(func), std::move(promise));
    Radiant::BGThread::instance()->addTask(task);
    return future;
  }

  template <typename T>
  folly::Future<T> createWrappedTask(std::function<boost::expected<T>(void)>&& func)
  {
    folly::MoveWrapper<std::function<boost::expected<T>(void)>> wrap(std::move(func));
    auto wrapFunc = [wrap] () -> WrappedTaskReturnType<T> {
      return (*wrap)();
    };
    return createWrappedTask<T>(std::move(wrapFunc));
  }

  template <typename T>
  folly::Future<T> createWrappedTask(std::function<T(void)>&& func)
  {
    folly::MoveWrapper<std::function<T(void)>> wrap(std::move(func));
    auto wrapFunc = [wrap] () -> WrappedTaskReturnType<T> {
      return (*wrap)();
    };
    return createWrappedTask<T>(std::move(wrapFunc));
  }

  // ---------------------------------------------------------------------------

  template <typename T>
  WrappedTask<T>::WrappedTask(WrappedTaskFunc<T>&& f, folly::Promise<T>&& promise)
    : m_func(std::move(f)),
      m_promise(std::move(promise))
  {
    m_promise.setInterruptHandler([this] (const folly::exception_wrapper&) {
      setCanceled();
    });
  }

  template <typename T>
  void WrappedTask<T>::doTask()
  {
    WrappedTaskReturnType<T> optional = std::move(m_func());
    if(boost::get<NotReadyYet>(&optional)) {
      return;
    } else if (boost::expected<T>* result = boost::get<boost::expected<T>>(&optional)) {
      setFinished();
      if(result->valid()) {
        m_promise.setValue(std::move(result->value()));
      } else {
        m_promise.setException(folly::exception_wrapper(result->get_unexpected().value()));
      }
    }
  }

}

#endif // TASKWRAPPER_HPP