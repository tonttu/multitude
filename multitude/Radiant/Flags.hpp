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
    typedef S Int;

    FlagsT() : m_value(0) {}
    FlagsT(T t) : m_value(t) {}
    FlagsT(const FlagsT & f) : m_value(f.m_value) {}

    FlagsT & operator=(const FlagsT & b) { m_value = b.m_value; return *this; }

    bool operator==(const FlagsT & b) const { return m_value == b.m_value; }
    bool operator!=(const FlagsT & b) const { return m_value != b.m_value; }

    bool operator!() const { return !m_value; }
    FlagsT operator~() const { return ~m_value; }

    FlagsT operator&(const FlagsT & b) const { return m_value & b.m_value; }
    FlagsT operator|(const FlagsT & b) const { return m_value | b.m_value; }
    FlagsT operator^(const FlagsT & b) const { return m_value ^ b.m_value; }

    FlagsT & operator&=(const FlagsT & b) { m_value &= b.m_value; return *this; }
    FlagsT & operator|=(const FlagsT & b) { m_value |= b.m_value; return *this; }
    FlagsT & operator^=(const FlagsT & b) { m_value ^= b.m_value; return *this; }

    /// best you can do to emulate c++0x explicit boolean conversion operator
    typedef void (FlagsT<T>::*bool_type)();
    operator bool_type() const { return m_value ? &FlagsT<T>::clear : 0; }

    void clear() { m_value = 0; }

    S asInt() { return m_value; }

    /// Converts int to Flags. If you end up using this function,
    /// you better have a real good reason to do so.
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
