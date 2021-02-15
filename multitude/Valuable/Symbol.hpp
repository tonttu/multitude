#pragma once

#include "Export.hpp"

#include <Radiant/SymbolRegistry.hpp>

#include <QString>

namespace Valuable
{
  VALUABLE_API extern Radiant::SymbolRegistry g_symbolRegistry;

  /// A symbol is used instead of QByteArray for performance reasons in CSS
  /// selectors and matching fields in Styleable. See Radiant::SymbolRegistry.
  class Symbol
  {
  public:
    inline Symbol(const QString & str)
      : m_id(g_symbolRegistry.lookupOrDefine(str.toUtf8()))
    {}

    inline Symbol(const QByteArray & str)
      : m_id(g_symbolRegistry.lookupOrDefine(str))
    {}

    inline Symbol(const char * str)
      : m_id(g_symbolRegistry.lookupOrDefine(str))
    {}

    inline Symbol(uint32_t id = 0)
      : m_id(id)
    {}

    inline uint32_t id() const { return m_id; }

    /// Convert the symbol back to string
    inline QByteArray str() const
    {
      return g_symbolRegistry.lookup(m_id);
    }

    inline operator uint32_t() const { return m_id; }

    /// Same as str() == "" but faster.
    inline bool isEmptyStr() const { return m_id == Radiant::SymbolRegistry::EmptySymbol; }

    inline bool isValid() const { return m_id != Radiant::SymbolRegistry::InvalidSymbol; }

  private:
    uint32_t m_id = Radiant::SymbolRegistry::InvalidSymbol;
  };
  static_assert(sizeof(Symbol) == sizeof(uint32_t), "Symbol size check");

  /// A symbol that is converted to lower-case if created from a string. Does
  /// not ensure the symbol is lower-case if constructed from an existing
  /// Symbol/uint32_t.
  ///
  /// You would typically use Symbol to store things, but use LowercaseSymbol
  /// in a setter to convert symbol to lowercase when created from a string:
  ///
  /// class MyClass
  /// {
  ///   Symbol m_type;
  /// public:
  ///   void setType(LowercaseSymbol type) { m_type = type; }
  ///   Symbol type() const { return m_type; }
  /// };
  ///
  /// MyClass foo;
  /// foo.setType("Foo");
  /// assert(foo.type().str() == "foo")
  class LowercaseSymbol : public Symbol
  {
  public:
    inline LowercaseSymbol(const QString & str)
      : Symbol(str.toLower().toUtf8())
    {}

    inline LowercaseSymbol(const QByteArray & str)
      : Symbol(str.toLower())
    {}

    inline LowercaseSymbol(const char * str)
      : Symbol(QByteArray(str).toLower())
    {}

    inline LowercaseSymbol(uint32_t id = 0)
      : Symbol(id)
    {}

    inline LowercaseSymbol(Symbol s)
      : Symbol(s)
    {}
  };
  static_assert(sizeof(LowercaseSymbol) == sizeof(uint32_t), "LowercaseSymbol size check");
}
