/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_STYLE_VALUE_HPP
#define VALUABLE_STYLE_VALUE_HPP

#include "Export.hpp"
#include "Attribute.hpp"
#include "SimpleExpression.hpp"

#include <QString>
#include <QVector>
#include <Radiant/Color.hpp>

#include <variant>

namespace Valuable
{
  /// CSS attribute value is a list of variant components.
  /// StyleValue is the part between ':' and ';' in CSS, example:
  ///   background: "image.png" top left;
  /// This value is a list of three components, one string and two keywords.
  /// CSS parser will generate a StyleValue instance from a value in CSS declaration.
  /// @sa http://www.w3.org/TR/CSS21/syndata.html#values
  class VALUABLE_API StyleValue
  {
  public:
    /// A separator between components
    enum Separator
    {
      SEPARATOR_WHITE_SPACE,  ///< Components are separated by white space (newline, tab, space)
      SEPARATOR_COMMA,        ///< Components are separated by comma (,)
      SEPARATOR_SLASH         ///< Components are separated by slash (/)
    };

    /// Component variant type. This need to be in the same order as Component::m_data.
    enum ValueType
    {
      TYPE_NONE,    ///< Null type
      TYPE_FLOAT,   ///< Floating point value, example: 10.0
      TYPE_INT,     ///< Integer value, example:        10
      TYPE_COLOR,   ///< Color value, example:          #fff
      TYPE_COLOR_PMA, ///< Color value (pre-multiplied alpha), example: 0.7, 0.4, 0.7, 0.9
      TYPE_STRING,  ///< Quoted string, example:        "image.png"
      TYPE_KEYWORD, ///< Unquoted string, example:      transparent
      TYPE_EXPR     ///< Simple expression, example:    calc(100%/3+30px)
    };

    /// One part of StyleValue list, variant / tagged union type with some convertions
    class VALUABLE_API Component
    {
    public:
      /// Create a null component
      Component();
      /// Create a floating point type with optional unit
      /// @param f value
      /// @param unit unit of f
      Component(float f, Attribute::ValueUnit unit = Attribute::VU_UNKNOWN);
      /// Create a integer component
      /// @param i value
      Component(int i);
      /// Create a color component
      /// @param color value
      Component(const Radiant::Color & color);
      /// Create a color component
      /// @param color value
      Component(const Radiant::ColorPMA & color);
      /// Create a string component
      /// @param string value
      Component(const QString & string);
      /// Create a keyword component
      /// @param keyword value
      Component(const QByteArray & keyword);
      /// Create a expression component
      /// @param expr value
      Component(const SimpleExpression & expr);
      /// Delete the component
      ~Component();

      /// Make a copy of the component
      /// @param component component to copy
      Component(const Component & component);
      /// Make a copy of the component
      /// @param component component to copy
      /// @return reference to this
      Component & operator=(const Component & component);

      /// Move a component
      /// @param component component to move
      Component(Component && component);
      /// Move a component
      /// @param component component to move
      /// @return reference to this
      Component & operator=(Component && component);

      /// @returns the component value as integer, works with int and float types,
      ///          otherwise prints an error and returns zero
      int asInt() const;
      /// @returns the component value as float, works with int and float types,
      ///          otherwise prints an error and returns zero
      float asFloat() const;
      /// @returns the component value as string, works with string and keyword types,
      ///          otherwise prints an error and returns a null string
      QString asString() const;
      /// @returns the component value as keyword, works with string and keyword types,
      ///          otherwise prints an error and returns a null bytearray
      QByteArray asKeyword() const;
      /// @returns the component value as color. If the type is not color,
      ///          prints an error and returns a default Radiant::Color()
      Radiant::Color asColor() const;
      /// @returns the component value as color. If the type is not color,
      ///          prints an error and returns a default Radiant::ColorPMA()
      Radiant::ColorPMA asColorPMA() const;
      /// @returns the component value as expression. If the type is not simple
      ///          expression, prints an error and returns a zero
      SimpleExpression asExpr() const;

      /// @param t try to convert this component to this type
      /// @returns true if this component could be converted to type t
      bool canConvert(StyleValue::ValueType t) const;

      /// @returns true if component is either int or float
      bool isNumber() const;

      /// @returns the type of the component
      ValueType type() const { return static_cast<ValueType>(m_data.index()); }
      /// @returns component type, VU_UNKNOWN if the type is not a number
      Attribute::ValueUnit unit() const { return m_unit; }
      /// @returns separator token preceding this component in a StyleValue
      Separator separator() const { return m_separator; }

      /// @param separator new separator
      void setSeparator(Separator separator) { m_separator = separator; }

      /// @returns string representation of the type
      const char * typeName() const;

      /// Compares two components, does no type conversions, types need to be exactly the same
      /// @param v other component to compare to
      /// @return true if components are identical
      bool operator==(const Component & v) const;
      /// Compares this component string / keyword to the given bytearray
      /// @param str UTF-8 string
      /// @return true if component string or keyword equals to str
      bool operator==(const QByteArray & str) const;

    private:
      /// This need to be in the same order as ValueType.
      std::variant<nullptr_t, float, int, Radiant::Color, Radiant::ColorPMA, QString, QByteArray, SimpleExpression> m_data;
      Attribute::ValueUnit m_unit;
      Separator m_separator;
    };
    typedef QVector<Component> ComponentList;

  public:
    /// Creates an empty StyleValue
    StyleValue() {}
    /// Creates a new StyleValue with one float component
    /// @param v float value
    /// @param unit unit of v
    StyleValue(float v, Attribute::ValueUnit unit = Attribute::VU_UNKNOWN);
    /// Creates a new StyleValue with one int component
    /// @param v int value
    StyleValue(int v);
    /// Creates a new StyleValue with one color component
    /// @param color color value
    StyleValue(const Radiant::Color & color);
    /// Creates a new StyleValue with one color component
    /// @param color color value
    StyleValue(const Radiant::ColorPMA & color);
    /// Creates a new StyleValue with one string component
    /// @param string string value
    StyleValue(const QString & string);
    /// Creates a new StyleValue with one keyword component
    /// @param keyword keyword value
    StyleValue(const QByteArray & keyword);
    /// Creates a new StyleValue with one component
    /// @param component component to add to the new StyleValue
    StyleValue(const Component & component);

    /// Shorthand constructor for creating a StyleValue from a map
    /// @param map map that is converted to StyleValue, see asMap() for exact format
    StyleValue(const QMap<QString, QString> & map);

    StyleValue(const SimpleExpression & expr);

    /// Deletes StyleValue and its components
    ~StyleValue();

    StyleValue(const StyleValue & v)
      : m_isUniform(v.m_isUniform)
      , m_components(v.m_components)
    {}

    StyleValue & operator=(const StyleValue & v)
    {
      m_isUniform = v.m_isUniform;
      m_components = v.m_components;
      return *this;
    }

    StyleValue(StyleValue && v)
      : m_isUniform(v.m_isUniform),
        m_components(std::move(v.m_components))
    {}

    StyleValue & operator=(StyleValue && v)
    {
      m_isUniform = v.m_isUniform;
      m_components = std::move(v.m_components);
      return *this;
    }

    /// @todo remove?
    int asInt(int idx = 0) const;
    float asFloat(int idx = 0) const;
    QString asString(int idx = 0) const;
    QByteArray asKeyword(int idx = 0) const;
    Radiant::Color asColor(int idx = 0) const;
    Radiant::ColorPMA asColorPMA(int idx = 0) const;
    SimpleExpression asExpr(int idx = 0) const;
    ValueType type(int idx = 0) const;
    Attribute::ValueUnit unit(int idx = 0) const;

    /// Concatenates another StyleValue to this object
    /// @param v StyleValue to append
    void append(StyleValue v);
    /// Concatenates another StyleValue to this object with given separator
    /// @param v StyleValue to append
    /// @param separator separator to use between this and v
    void append(StyleValue v, Separator separator);
    /// Appends one component to end of this StyleValue
    /// @param c component to append
    void append(const Component & c);
    /// Appends one component to end of this StyleValue with given separator
    /// @param c component to append
    /// @param separator separator to use between this and c
    void append(const Component & c, Separator separator);

    /// @returns the number of components
    int size() const { return m_components.size(); }

    /// @returns true if size() == 0
    bool isEmpty() const { return m_components.isEmpty(); }

    /// In uniform StyleValue all components have same separators
    /// and types that can be converted to each other. Empty StyleValue is also
    /// uniform type.
    /// @returns true if StyleValue is uniform
    bool isUniform() const { return m_isUniform; }

    /// @returns String representation that can be used in a CSS
    QString stringify() const;

    /// @param idx index of the value in the list
    /// @return true if value is integer or floating point number
    bool isNumber(int idx = 0) const;

    /// Access one component in the StyleValue
    /// @param idx 0 <= idx < size()
    /// @returns const reference to component at index idx
    const Component & operator[](int idx) const;

    /// @returns const references to component list
    const ComponentList & components() const { return m_components; }

    /// Checks if StyleValue objects are different
    /// @param v other StyleValue object
    /// @returns true if objects differ
    inline bool operator!=(const StyleValue & v) const { return !operator==(v); }

    /// Checks if StyleValue objects are identical
    /// @param v other StyleValue object
    /// @returns true if objects are identical
    bool operator==(const StyleValue & v) const;

    /// Splits StyleValue to parts with a separator.
    /// @param sep separator that marks the points where to split
    /// @returns this StyleValue splitted to smaller parts
    QList<StyleValue> split(Separator sep) const;

    /// CSS value "aaa bbb, ccc ddd eee, fff" will be converted to map:
    /// "aaa" => "bbb", "ccc" => "ddd eee", "fff" => ""
    /// @returns StyleValue converted to map
    QMap<QString, QString> asMap() const;

    /// @cond

    /// @todo this should be removed
    QList<Attribute::ValueUnit> units() const;

    /// @endcond

  private:
    bool m_isUniform = true;
    ComponentList m_components;
  };

  /// Converts StyleValue to CSS string and writes that to stream
  /// @param[out] os output stream
  /// @param[in] value StyleValue to write
  /// @returns os
  VALUABLE_API std::ostream & operator<<(std::ostream & os, const StyleValue & value);
  /// Reads one line from stream and parses that with CSS parser to StyleValue
  /// @param[in] is stream to read the line
  /// @param[out] value StyleValue to fill
  /// @returns is
  /// @note This needs full CSS parser to work, so its implemented in Stylish
  STYLISH_API std::istream & operator>>(std::istream & is, StyleValue & value);
}

#endif
