#if !defined (RADIANT_SAFEBOOL_HPP)
#define RADIANT_SAFEBOOL_HPP

/* Class for the safe bool idiom implementation
 * NOTE: This class will be obsolete once all compilers implement C++11 explicit bool operator()
 *
 * Usage:
 * class Foo : public SafeBool<Foo>
 * {
 * public:
 *   bool boolean_test() const { }
 * };
 */
namespace Radiant
{
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

  template<> class SafeBool<void> : public SafeBoolBase {
  public:
    operator bool_type() const {
      return boolean_test()==true ? 
        &SafeBool<void>::this_type_does_not_support_comparisons : 0;
    }
  protected:
    virtual bool boolean_test() const=0;
    virtual ~SafeBool() {}
  };

  template <typename T, typename U> 
  bool operator==(const SafeBool<T>& lhs,const SafeBool<U>& rhs) {
    lhs.this_type_does_not_support_comparisons();	
    return false;
  }

  template <typename T,typename U> 
  bool operator!=(const SafeBool<T>& lhs,const SafeBool<U>& rhs) {
    lhs.this_type_does_not_support_comparisons();
    return false;	
  }
}
#endif // RADIANT_SAFEBOOL_HPP
