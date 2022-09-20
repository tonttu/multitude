/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#pragma once

#include "Export.hpp"

#include <Radiant/SymbolRegistry.hpp>

#include <QString>

namespace Valuable
{
  VALUABLE_API extern Radiant::SymbolRegistry g_symbolRegistry;

  /// A symbol is used instead of QByteArray for performance reasons in CSS
  /// selectors and matching fields in Styleable. See Radiant::SymbolRegistry.
  ///
  /// A symbol always represents a lowercase string, but in debug mode the
  /// original string is available with debugStr().
  class Symbol
  {
  public:
    inline Symbol(const QString & str)
    {
      QByteArray id = str.toUtf8();
      m_id = g_symbolRegistry.lookupOrDefine(id.toLower(), id);
    }

    inline Symbol(const QByteArray & str)
      : m_id(g_symbolRegistry.lookupOrDefine(str.toLower(), str))
    {}

    inline Symbol(const char * str)
    {
      QByteArray id{str};
      m_id = g_symbolRegistry.lookupOrDefine(id.toLower(), id);
    }

    inline Symbol(uint32_t id = Radiant::SymbolRegistry::EmptySymbol)
      : m_id(id)
    {}

    inline uint32_t id() const { return m_id; }

    /// Convert the symbol back to lowercase string
    inline QByteArray str() const
    {
      return g_symbolRegistry.lookup(m_id);
    }

    /// Returns the original string (not converted to lowercase).
    inline QByteArray debugStr() const
    {
      return g_symbolRegistry.lookupOriginal(m_id);
    }

    inline bool operator==(Symbol o) const { return m_id == o.m_id; }
    inline bool operator!=(Symbol o) const { return m_id != o.m_id; }
    inline bool operator<(Symbol o) const { return m_id < o.m_id; }
    inline bool operator<=(Symbol o) const { return m_id <= o.m_id; }
    inline bool operator>(Symbol o) const { return m_id > o.m_id; }
    inline bool operator>=(Symbol o) const { return m_id >= o.m_id; }

    /// Same as str() == "" but faster.
    inline bool isEmptyStr() const { return m_id == Radiant::SymbolRegistry::EmptySymbol; }

    inline bool isValid() const { return m_id != Radiant::SymbolRegistry::InvalidSymbol; }

  private:
    uint32_t m_id = Radiant::SymbolRegistry::EmptySymbol;
  };
  static_assert(sizeof(Symbol) == sizeof(uint32_t), "Symbol size check");
}
