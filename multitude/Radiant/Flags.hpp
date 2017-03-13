/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_FLAGS_HPP
#define RADIANT_FLAGS_HPP

#include <cstdint>

namespace Radiant
{
  /// @cond
  template <int T> struct IntOfSize { typedef uint32_t Type; };
  template <> struct IntOfSize<1> { typedef uint8_t Type; };
  template <> struct IntOfSize<2> { typedef uint16_t Type; };
  template <> struct IntOfSize<4> { typedef uint32_t Type; };
  template <> struct IntOfSize<8> { typedef uint64_t Type; };
  /// @endcond

  struct Flags {};

  /// This class implements type-safe flags.
  /// @tparam T Type of the flag type (enum)
  /// @tparam S Internal type for storing the flags
  template <typename T, typename S = typename IntOfSize<sizeof(T)>::Type>
  class FlagsT : public Flags
  {
  public:
    /// Internal type used to store the flags
    typedef S Int;
    typedef T Enum;

    /// Construct empty flags object
    FlagsT() : m_value(0) {}
    /// Construct flags
    /// @param t initial values
    FlagsT(Enum t) : m_value(Int(t)) {}
    /// Construct a copy
    /// @param f flags to copy
    FlagsT(const FlagsT & f) : m_value(f.m_value) {}

    /// Copy flags
    /// @param b flags to copy
    /// @return reference to this flags object
    FlagsT & operator=(const FlagsT & b) { m_value = b.m_value; return *this; }

    // These operators are inside the class, since something like
    // template <typename Enum> FlagsT<Enum> operator|(const FlagsT<Enum> & a, const FlagsT<Enum> & b);
    // doesn't get called if you write code "flag | enum", apparently since FlagsT
    // is a template function. So since we want to have some of these as a
    // member functions, we also define some of the operators as friends so
    // that we can define them here in the same place.

    /// Compare if two flags are equal
    /// @param b flags to compare
    /// @return true if the flags are equal; otherwise false
    bool operator==(const FlagsT & b) const { return m_value == b.m_value; }
    friend bool operator==(Enum a, const FlagsT & b) { return a == b.m_value; }

    /// Compare if two flags are inequal
    /// @param b flags to compare
    /// @return true if the flags are inequal; otherwise false
    bool operator!=(const FlagsT & b) const { return m_value != b.m_value; }
    friend bool operator!=(Enum a, const FlagsT & b) { return a != b.m_value; }

    /// Check if no flags are raised
    /// @return true if all flags are zero; otherwise false
    bool operator!() const { return !m_value; }
    /// Flip all the flags
    /// @return flipped flags
    FlagsT operator~() const { return ~m_value; }

    /// Do a bit-wise AND of flags
    /// @param b flags to AND
    /// @return combined flags
    FlagsT operator&(const FlagsT & b) const { return m_value & b.m_value; }
    friend FlagsT operator&(Enum a, const FlagsT & b) { return a & b.m_value; }
    /// Do a bit-wise OR of flags
    /// @param b flags to AND
    /// @return combined flags
    FlagsT operator|(const FlagsT & b) const { return m_value | b.m_value; }
    friend FlagsT operator|(Enum a, const FlagsT & b) { return a | b.m_value; }
    /// Do a bit-wise XOR of flags
    /// @param b flags to XOR
    /// @return combined flags
    FlagsT operator^(const FlagsT & b) const { return m_value ^ b.m_value; }
    friend FlagsT operator^(Enum a, const FlagsT & b) { return a ^ b.m_value; }

    /// Do a bit-wise AND of flags and assign the result to this
    /// @param b flags to AND
    /// @return reference to this
    FlagsT & operator&=(const FlagsT & b) { m_value &= b.m_value; return *this; }
    /// Do a bit-wise OR of flags and assign the result to this
    /// @param b flags to OR
    /// @return reference to this
    FlagsT & operator|=(const FlagsT & b) { m_value |= b.m_value; return *this; }
    /// Do a bit-wise XOR of flags and assign the result to this
    /// @param b flags to XOR
    /// @return reference to this
    FlagsT & operator^=(const FlagsT & b) { m_value ^= b.m_value; return *this; }

    /// @cond

    // We are not using explicit boolean conversion operator, since this class
    // is typically used as a drop-in replacement for some unsigned int type,
    // and expected to work like "return m_grabFlags & MultiTouch::TYPE_PEN;"
    // - that wouldn't compile with explicit boolean operator
    typedef void (FlagsT<Enum>::*bool_type)();
    operator bool_type() const { return m_value ? &FlagsT<Enum>::clear : nullptr; }

    /// @endcond

    /// Clear all flags to zero
    void clear() { m_value = 0; }

    /// Convert the flags to integer
    /// @return flags as integer
    Int asInt() const { return m_value; }

    /// Converts int to Flags. If you end up using this function,
    /// you better have a real good reason to do so.
    /// @param i integers to convert from
    /// @return converted flags
    static FlagsT<Enum> fromInt(Int i) { return FlagsT<Enum>(i); }

    bool operator<(const FlagsT & t) const { return m_value < t.m_value; }

  private:
    FlagsT(Int s) : m_value(s) {}
    Int m_value;
  };
}

#define MULTI_FLAGS(T) \
  inline Radiant::FlagsT<T> operator|(T a, T b) { return Radiant::FlagsT<T>(a) | b; } \
  inline Radiant::FlagsT<T> operator&(T a, T b) { return Radiant::FlagsT<T>(a) & b; } \
  inline Radiant::FlagsT<T> operator^(T a, T b) { return Radiant::FlagsT<T>(a) ^ b; } \
  inline Radiant::FlagsT<T> operator~(T t) { return ~Radiant::FlagsT<T>(t); }

#endif // RADIANT_FLAGS_HPP
