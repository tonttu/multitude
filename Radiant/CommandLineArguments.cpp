#include "CommandLineArguments.hpp"

namespace Radiant
{
  CommandLineArguments::CommandLineArguments(const QStringList & args)
    : m_argc(args.size())
  {
    std::vector<int> offsets;
    offsets.reserve(args.size());
    m_argv.reserve(args.size()+1);

    for (auto & str: args) {
      QByteArray arg = str.toUtf8();
      offsets.push_back(m_data.size());
      m_data += arg;
      m_data += '\0';
    }

    for (auto offset: offsets) {
      m_argv.push_back(m_data.data() + offset);
    }
    m_argv.push_back(nullptr);
  }

  CommandLineArguments::~CommandLineArguments()
  {
  }

} // namespace Radiant
