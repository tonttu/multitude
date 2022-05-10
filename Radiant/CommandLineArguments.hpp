#ifndef RADIANT_COMMANDLINEARGUMENTS_HPP
#define RADIANT_COMMANDLINEARGUMENTS_HPP

#include "Export.hpp"

#include <Patterns/NotCopyable.hpp>

#include <QStringList>

#include <vector>

namespace Radiant
{
  /// Helper class for building argc and argv dynamically. Used with libraries
  /// that have initialization code that expects argc and argv-pointer.
  class CommandLineArguments : public Patterns::NotCopyable
  {
  public:
    /// args[0] is the application name, args[1..] are the arguments
    RADIANT_API CommandLineArguments(const QStringList & args);
    RADIANT_API ~CommandLineArguments();

    inline int & argc() { return m_argc; }
    inline char ** argv() { return m_argv.data(); }

  private:
    int m_argc;
    std::vector<char*> m_argv;
    QByteArray m_data;
  };

} // namespace Radiant

#endif // RADIANT_COMMANDLINEARGUMENTS_HPP
