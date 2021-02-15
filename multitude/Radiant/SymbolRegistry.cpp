#include "SymbolRegistry.hpp"
#include "Trace.hpp"

#include <cassert>

namespace Radiant
{
  SymbolRegistry::SymbolRegistry()
  {
    define("", EmptySymbol);
  }

  SymbolRegistry::Symbol SymbolRegistry::lookupOrDefineImpl(const QByteArray & name)
  {
    QWriteLocker locker(&m_lock);
    auto it = m_nameToSymbol.find(name);
    if(it == m_nameToSymbol.end()) {
      // +1 because symbol of 0 is not valid
      Symbol symbol = static_cast<Symbol>(m_symbolToName.size() + 1);
      m_symbolToName.insert(std::make_pair(symbol, name));
      m_nameToSymbol.insert(std::make_pair(name, symbol));
      return symbol;
    }
    return it->second;
  }

  bool SymbolRegistry::define(const QByteArray & name, Symbol symbol)
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
                     symbol, name.data(), it->second.data());
      return false;
    }
    m_symbolToName.insert(std::make_pair(symbol, name));
    m_nameToSymbol.insert(std::make_pair(name, symbol));
    return true;
  }
}
