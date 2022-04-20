#include "SymbolRegistry.hpp"
#include "Trace.hpp"

#include <cassert>

namespace Radiant
{
  SymbolRegistry::SymbolRegistry()
  {
    QByteArray empty{""};
    define(empty, empty, EmptySymbol);
  }

  SymbolRegistry::Symbol SymbolRegistry::lookupOrDefineImpl(
      const QByteArray & name, const QByteArray & originalName)
  {
    QWriteLocker locker(&m_lock);
    auto it = m_nameToSymbol.find(name);
    if(it == m_nameToSymbol.end()) {
      // +1 because symbol of 0 is not valid
      Symbol symbol = static_cast<Symbol>(m_symbolToName.size() + 1);
      m_symbolToName.emplace(symbol, SymbolName{name, originalName});
      m_nameToSymbol.emplace(name, symbol);
      return symbol;
    }
    return it->second;
  }

  bool SymbolRegistry::define(const QByteArray & name, const QByteArray & originalName, Symbol symbol)
  {
    if(symbol == InvalidSymbol) {
      Radiant::error("SymbolRegistry::define # Trying to assign symbol %d to name %s. Symbol %d is reserved.",
                     InvalidSymbol, name.data(), InvalidSymbol);
      return false;
    }
    QWriteLocker locker(&m_lock);
    auto it = m_symbolToName.find(symbol);
    if(it != m_symbolToName.end()) {
      Radiant::error("SymbolRegistry::define # Trying to define symbol %u to %s, but it already is assigned to %s ",
                     symbol, name.data(), it->second.name.data());
      return false;
    }
    m_symbolToName.emplace(symbol, SymbolName{name, originalName});
    m_nameToSymbol.emplace(name, symbol);
    return true;
  }
}
