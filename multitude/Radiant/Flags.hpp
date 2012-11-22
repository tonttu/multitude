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

  /// This class implements type-safe flags.
  template <typename T, typename S = typename IntOfSize<sizeof(T)>::Type>
  class FlagsT
  {
  public:
    /// Internal type used to store the flags
    typedef S Int;

    /// Construct empty flags object
    FlagsT() : m_value(0) {}
    /// Construct flags
    /// @param t initial values
    FlagsT(T t) : m_value(t) {}
    /// Construct a copy
    /// @param f flags to copy
    FlagsT(const FlagsT & f) : m_value(f.m_value) {}

    /// Copy flags
    /// @param b flags to copy
    /// @return reference to this flags object
    FlagsT & operator=(const FlagsT & b) { m_value = b.m_value; return *this; }

    /// Compare if two flags are equal
    /// @param b flags to compare
    /// @return true if the flags are equal; otherwise false
    bool operator==(const FlagsT & b) const { return m_value == b.m_value; }
    /// Compare if two flags are inequal
    /// @param b flags to compare
    /// @return true if the flags are inequal; otherwise false
    bool operator!=(const FlagsT & b) const { return m_value != b.m_value; }

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
    /// Do a bit-wise OR of flags
    /// @param b flags to AND
    /// @return combined flags
    FlagsT operator|(const FlagsT & b) const { return m_value | b.m_value; }
    /// Do a bit-wise XOR of flags
    /// @param b flags to XOR
    /// @return combined flags
    FlagsT operator^(const FlagsT & b) const { return m_value ^ b.m_value; }

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

    // best you can do to emulate c++0x explicit boolean conversion operator
    typedef void (FlagsT<T>::*bool_type)();
    operator bool_type() const { return m_value ? &FlagsT<T>::clear : 0; }

    /// @endcond

    /// Clear all flags to zero
    void clear() { m_value = 0; }

    /// Convert the flags to integer
    /// @return flags as integer
    S asInt() { return m_value; }

    /// Converts int to Flags. If you end up using this function,
    /// you better have a real good reason to do so.
    /// @param i integers to convert from
    /// @return converted flags
    static FlagsT<T> fromInt(Int i) { return FlagsT<T>(i); }

  private:
    FlagsT(S s) : m_value(s) {}
    S m_value;
  };
}

#define MULTI_FLAGS(T) \
  inline Radiant::FlagsT<T> operator|(T a, T b) { return Radiant::FlagsT<T>(a) | b; } \
  inline Radiant::FlagsT<T> operator~(T t) { return ~Radiant::FlagsT<T>(t); }

//  FlagsT<T> operator^(T a, T b) const { return FlagsT<T>(a) ^ b; }
//  FlagsT<T> operator&(T a, T b) const { return FlagsT<T>(a) & b; }



#endif // RADIANT_FLAGS_HPP
