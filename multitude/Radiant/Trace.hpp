/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef RADIANT_TRACE_HPP
#define RADIANT_TRACE_HPP

#include "Export.hpp"
#include "Flags.hpp"

#include <QString>

#include <memory>
#include <functional>
#include <map>

namespace Radiant
{
  namespace Trace
  {
    /// Error severity levels
    enum Severity
    {
      /// Debug information, that is usually not useful for the end user
      /** Debug messages are printed out only if verbose output is
          enabled. */
      DEBUG,
      /// Useful information to all users.
      /** Info messages are printed out always. */
      INFO,
      /// Something bad may or may not had happened
      WARNING,
      /// An error occurred
      FAILURE,
      /// Fatal error, causes application shutdown
      FATAL,
    };

    enum InitFlags
    {
      /// No init flags defined
      INIT_NO_FLAGS               = 0,

      /// Process all messages that were sent before Trace was initialized.
      /// If not set, queued messages are just dropped.
      PROCESS_QUEUED_MESSAGES     = 1 << 0,

      /// Create default filters that limit the messages based on their
      /// severity and print the messages to stdout / stderr
      INITIALIZE_DEFAULT_FILTERS  = 1 << 1,
    };
    MULTI_FLAGS(InitFlags)

    /// @cond

    struct Message
    {
      Severity severity;
      QByteArray module;
      QString text;
    };

    inline bool operator==(const Message & a, const Message & b)
    {
      return a.severity == b.severity && a.module == b.module && a.text == b.text;
    }

    /// Single filter on a static filter chain. When Radiant::info or any other
    /// trace function is called, the message is passed to the filter chain by
    /// calling Filter::trace(msg) to the first filter in the chain. The return
    /// value determines if the message is passed to the next filter on the
    /// chain.
    /// Filters typically either drop messages based on the message parameters
    /// or content, or output the messages to other systems like stdout/stderr,
    /// log files, syslog or windows debug console.
    /// The filters are processed in order based on order (float) parameter.
    /// Typically filters that drop messages should have order close to
    /// ORDER_DEFAULT_FILTERS, while filters that just output the message to
    /// some other systems have order close to ORDER_OUTPUT.
    class Filter
    {
    public:
      enum Order
      {
        ORDER_BEGIN = 0,
        ORDER_DEFAULT_FILTERS = 1000,
        ORDER_OUTPUT = 2000,
        ORDER_END = 3000,
      };

    public:
      Filter(float order = ORDER_OUTPUT) : m_order(order) {}
      virtual ~Filter() {}

      /// @return true to drop the message, false to pass the message to the
      ///         next filter in the chain
      virtual bool trace(Message & msg) = 0;

      inline float order() const { return m_order; }

    private:
      const float m_order;
    };
    typedef std::shared_ptr<Filter> FilterPtr;

    typedef std::function<bool(Message & message)> FilterFunc;

    RADIANT_API void addFilter(const FilterPtr & filter);
    /// Add a lambda as filter
    RADIANT_API FilterPtr addFilter(const FilterFunc & filter, float order = Filter::ORDER_OUTPUT);
    RADIANT_API bool removeFilter(const FilterPtr & filter);
    RADIANT_API std::multimap<float, FilterPtr> filters();

    template <typename FilterT, typename ... Args>
    inline std::shared_ptr<FilterT> findOrCreateFilter(Args && ... args);

    template <typename FilterT>
    inline std::shared_ptr<FilterT> findFilter();

    template <typename FilterT>
    inline std::shared_ptr<FilterT> replaceFilter(std::shared_ptr<FilterT> newFilter);

    RADIANT_API QByteArray severityText(Severity severity);

    /// @endcond

    /// Initializes the logging system. Before this is called, no messages are
    /// processed. If you call Radiant::info or other trace functions, the
    /// messages are stored to a static queue that can be either cleared or
    /// processed in initialize().
    /// If the application is closed before initialize is called, all buffered
    /// messages are printed to stderr.
    /// This is typically called automatically by MultiWidgets::Application.
    RADIANT_API void initialize(Radiant::FlagsT<InitFlags> flags =
        PROCESS_QUEUED_MESSAGES | INITIALIZE_DEFAULT_FILTERS);


    /// Display useful output.
    /** This function prints out given message, based on current verbosity level.

      Radiant includes a series of functions to write debug output on the
      terminal.

      The functions @ref info, @ref debug, @ref error and @ref fatal print output to the
      sceen in standardized format. The debug function only writes data to
      the screen if verbose reporting is enabled with @ref enableVerboseOutput
      (see also @ref MultiWidgets::Application::verbose). These functions are basically
      wrappers around printf.

      The terminal output is protected by mutex lock so that multiple
      threads can write to the same terminal without producing corrupted
      output. This was also the reason why the output is done with
      functions, rather than std::cout etc. With the std streams one
      cannot organize a mutex lock around the text output, which easily
      results in corrupted (and rather useless) output.

      @param s severity of the message
      @param msg message format string */
    RADIANT_API void trace(Severity s, const char * msg, ...) RADIANT_PRINTF_CHECK(2, 3);

    /// @copydoc trace
    /// @param module outputting module from which this message originates
    RADIANT_API void trace(const char * module, Severity s, const char * msg, ...)
      RADIANT_PRINTF_CHECK(3, 4);

    /// Used from JS
    RADIANT_API void traceMsg(Severity s, const QByteArray & msg);

    /// Display debug output
    /** This function calls trace to do the final work and it is
      effectively the same as calling trace(DEBUG, ...).

      @param msg message
      @see trace
  */
    RADIANT_API void debug(const char * msg, ...) RADIANT_PRINTF_CHECK(1, 2);

    /// Display information output
    /** This function calls trace to do the final work and it is
      effectively the same as calling trace(INFO, ...).

      @param msg message
      @see trace
  */
    RADIANT_API void info(const char * msg, ...) RADIANT_PRINTF_CHECK(1, 2);

    /// Display error output, with a warning message
    /** This function calls trace to do the final work and it is
      effectively the same as calling trace(WARNING, ...).
      @param msg message
      @see trace
  */
    RADIANT_API void warning(const char * msg, ...) RADIANT_PRINTF_CHECK(1, 2);

    /// Display error output
    /** This function calls trace to do the final work and it is
      effectively the same as calling trace(FAILURE, ...).

      @param msg message
      @see trace
  */
    RADIANT_API void error(const char * msg, ...) RADIANT_PRINTF_CHECK(1, 2);

    /// Display error output, with a fatal message
    /** This function calls trace to do the final work and it is
      effectively the same as calling trace(FATAL, ...).
      @param msg message
      @see trace
  */
    RADIANT_API void fatal(const char * msg, ...) RADIANT_PRINTF_CHECK(1, 2);


    template <typename FilterT, typename ... Args>
    inline std::shared_ptr<FilterT> findOrCreateFilter(Args && ... args)
    {
      auto filter = findFilter<FilterT>();
      if (!filter) {
        filter = std::make_shared<FilterT>(std::forward<Args>(args)...);
        addFilter(filter);
      }
      return filter;
    }

    template <typename FilterT>
    inline std::shared_ptr<FilterT> findFilter()
    {
      for (auto & p: filters()) {
        if (auto filter = std::dynamic_pointer_cast<FilterT>(p.second)) {
          return filter;
        }
      }

      return nullptr;
    }

    template <typename FilterT>
    inline std::shared_ptr<FilterT> replaceFilter(std::shared_ptr<FilterT> newFilter)
    {
      auto filter = findFilter<FilterT>();
      if (filter) removeFilter(filter);
      addFilter(newFilter);
    }

  } // namespace Trace

  using Trace::trace;
  using Trace::debug;
  using Trace::info;
  using Trace::warning;
  using Trace::error;
  using Trace::fatal;

} // namespace Radiant

#endif
