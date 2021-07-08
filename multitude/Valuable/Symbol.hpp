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
#ifdef RADIANT_DEBUG
    inline Symbol(const QString & str)
      : m_originalString(str.toUtf8())
    {
      m_id = g_symbolRegistry.lookupOrDefine(m_originalString.toLower());
    }

    inline Symbol(const QByteArray & str)
      : m_id(g_symbolRegistry.lookupOrDefine(str.toLower()))
      , m_originalString(str)
    {}

    inline Symbol(const char * str)
      : m_originalString(str)
    {
      m_id = g_symbolRegistry.lookupOrDefine(m_originalString.toLower());
    }
#else
    inline Symbol(const QString & str)
      : m_id(g_symbolRegistry.lookupOrDefine(str.toUtf8().toLower()))
    {}

    inline Symbol(const QByteArray & str)
      : m_id(g_symbolRegistry.lookupOrDefine(str.toLower()))
    {}

    inline Symbol(const char * str)
      : m_id(g_symbolRegistry.lookupOrDefine(QByteArray(str).toLower()))
    {}
#endif

    inline Symbol(uint32_t id = Radiant::SymbolRegistry::EmptySymbol)
      : m_id(id)
    {}

    inline uint32_t id() const { return m_id; }

    /// Convert the symbol back to lowercase string
    inline QByteArray str() const
    {
      return g_symbolRegistry.lookup(m_id);
    }

    /// Returns the original string (not converted to lowercase) in debug mode,
    /// in release mode this is the same as str().
    inline QByteArray debugStr() const
    {
#ifdef RADIANT_DEBUG
      if (!m_originalString.isEmpty())
        return m_originalString;
#endif
      return str();
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
#ifdef RADIANT_DEBUG
    QByteArray m_originalString;
#endif
  };
#ifndef RADIANT_DEBUG
  static_assert(sizeof(Symbol) == sizeof(uint32_t), "Symbol size check");
#endif
}
