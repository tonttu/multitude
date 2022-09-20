/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef RADIANT_SYMBOLREGISTRY_HPP
#define RADIANT_SYMBOLREGISTRY_HPP

#include "Export.hpp"

#include <cstdint>
#include <unordered_map>
#include <map>

#include <QByteArray>
#include <QReadWriteLock>

namespace Radiant
{
  /// Symbol is a mapping between a string and uint32. It’s similar concept to
  /// Atoms/Symbols in Lisp, Ruby, X11 etc. The purpose of symbol is to replace
  /// often used strings with a plain number to reduce required memory /
  /// network bandwidth / storage space.
  ///
  /// Symbol 0 means not defined null symbol
  class RADIANT_API SymbolRegistry
  {
  public:
    typedef uint32_t Symbol;
    static constexpr Symbol InvalidSymbol = 0;
    static constexpr Symbol EmptySymbol = 1;

  public:
    SymbolRegistry();

    /// @param name typically a lowercase string
    /// @param originalName the original string, for debug purposes
    inline Symbol lookupOrDefine(const QByteArray & name, const QByteArray & originalName);
    inline Symbol lookupOrDefine(const QByteArray & name);

    inline bool define(const QByteArray & name, Symbol symbol);
    bool define(const QByteArray & name, const QByteArray & originalName, Symbol symbol);

    /// @return QByteArray() if not found
    inline QByteArray lookup(Symbol symbol) const;

    /// @return QByteArray() if not found
    inline QByteArray lookupOriginal(Symbol symbol) const;

    /// @return 0 if the symbol wasn't found
    inline Symbol lookup(const QByteArray & name) const;

  private:
    Symbol lookupOrDefineImpl(const QByteArray & name, const QByteArray & originalName);

  private:
    struct SymbolName
    {
      QByteArray name;
      QByteArray originalName;
    };

    std::map<QByteArray, Symbol> m_nameToSymbol;
    std::unordered_map<Symbol, SymbolName> m_symbolToName;
    mutable QReadWriteLock m_lock;
  };


  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  SymbolRegistry::Symbol SymbolRegistry::lookupOrDefine(const QByteArray & name)
  {
    return lookupOrDefine(name, name);
  }

  SymbolRegistry::Symbol SymbolRegistry::lookupOrDefine(
      const QByteArray & name, const QByteArray & originalName)
  {
    if (auto symbol = lookup(name))
      return symbol;
    return lookupOrDefineImpl(name, originalName);
  }

  bool SymbolRegistry::define(const QByteArray & name, SymbolRegistry::Symbol symbol)
  {
    return define(name, name, symbol);
  }

  QByteArray SymbolRegistry::lookup(Symbol symbol) const
  {
    QReadLocker locker(&m_lock);
    auto it = m_symbolToName.find(symbol);
    return it == m_symbolToName.end() ? QByteArray() : it->second.name;
  }

  QByteArray SymbolRegistry::lookupOriginal(Symbol symbol) const
  {
    QReadLocker locker(&m_lock);
    auto it = m_symbolToName.find(symbol);
    return it == m_symbolToName.end() ? QByteArray() : it->second.originalName;
  }

  SymbolRegistry::Symbol SymbolRegistry::lookup(const QByteArray & name) const
  {
    QReadLocker locker(&m_lock);
    auto it = m_nameToSymbol.find(name);
    return it == m_nameToSymbol.end() ? InvalidSymbol : it->second;
  }
} // namespace Radiant

#endif // RADIANT_SYMBOLREGISTRY_HPP
