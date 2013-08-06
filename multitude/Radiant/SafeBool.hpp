/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#if !defined (RADIANT_SAFEBOOL_HPP)
#define RADIANT_SAFEBOOL_HPP

namespace Radiant
{
  /// @cond

  /// Class for the safe bool idiom implementation
  /// NOTE: This class will be obsolete once all compilers implement C++11 explicit bool operator()
  ///
  /// Usage:
  /// class Foo : public SafeBool<Foo>
  /// {
  /// public:
  ///   bool boolean_test() const { }
  /// };
  class SafeBoolBase {
  protected:
    typedef void (SafeBoolBase::*bool_type)() const;
    void this_type_does_not_support_comparisons() const {}

    SafeBoolBase() {}
    SafeBoolBase(const SafeBoolBase&) {}
    SafeBoolBase& operator=(const SafeBoolBase&) {return *this;}
    ~SafeBoolBase() {}
  };

  template <typename T=void> class SafeBool : public SafeBoolBase {
  public:
    operator bool_type() const {
      return (static_cast<const T*>(this))->boolean_test()
        ? &SafeBool<T>::this_type_does_not_support_comparisons : 0;
    }
  protected:
    ~SafeBool() {}
  };

  template <typename T, typename U>
  bool operator==(const SafeBool<T>& lhs,const SafeBool<U>&) {
    lhs.this_type_does_not_support_comparisons();	
    return false;
  }

  template <typename T,typename U> 
  bool operator!=(const SafeBool<T>& lhs,const SafeBool<U>&) {
    lhs.this_type_does_not_support_comparisons();
    return false;	
  }

  /// @endcond
}
#endif // RADIANT_SAFEBOOL_HPP
