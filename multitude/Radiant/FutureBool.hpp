#ifndef RADIANT_FUTUREBOOL_HPP
#define RADIANT_FUTUREBOOL_HPP

#include <future>

namespace Radiant {

  /// This class wraps a std::future boolean and provides implicit conversion
  /// to boolean type. The class is used to provide asynchronous return values
  /// from functions.
  class FutureBool
  {
  public:
    /// Construct a new FutureBool by taking ownership of the given std::future
    /// @param future boolean future to wrap
    explicit FutureBool(std::future<bool> && future)
      : m_future(std::move(future))
    {}

    /// Implicit conversion to boolean. This function will block until the
    /// value of the future has been set.
    /// @return boolean value
    operator bool()
    {
      try {
        return m_future.get();
      } catch(std::future_error &) {
        return false;
      }
    }

  private:
    std::future<bool> m_future;
  };

} // namespace Radiant

#endif // RADIANT_FUTUREBOOL_HPP
