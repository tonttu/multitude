/* COPYRIGHT
 */

#include "ValueRect.hpp"
#include "DOMElement.hpp"

#include <Radiant/StringUtils.hpp>

namespace Valuable
{

  template <class T>
  ValueRectT<T>::ValueRectT(HasValues * parent, const std::string & name, const Nimble::RectT<T> & r, bool transit)
    : Base(parent, name, r, transit)
  {}

  template <class T>
  bool ValueRectT<T>::deserialize(const ArchiveElement & element)
  {
    std::stringstream in(element.get());

    Nimble::Vector2T<T> lo, hi;

    in >> lo[0];
    in >> lo[1];
    in >> hi[0];
    in >> hi[1];

    this->m_value.setLow(lo);
    this->m_value.setHigh(hi);

    this->emitChange();
    return true;
  }

  template <class T>
  std::string ValueRectT<T>::asString(bool * const ok) const
  {
    if(ok) *ok = true;

    const Nimble::Vector2T<T> & lo = this->m_value.low();
    const Nimble::Vector2T<T> & hi = this->m_value.high();

    std::string r = Radiant::StringUtils::stringify(lo[0]);
    r += std::string(" ") + Radiant::StringUtils::stringify(lo[1]);
    r += std::string(" ") + Radiant::StringUtils::stringify(hi[0]);
    r += std::string(" ") + Radiant::StringUtils::stringify(hi[1]);

    return r;
  }

  template <>
  const char * ValueRectT<float>::type() const
  { return "rect"; }

  template <>
  const char * ValueRectT<double>::type() const
  { return "rectd"; }

  template <>
  const char * ValueRectT<int>::type() const
  { return "recti"; }



  template class ValueRectT<float>;
  template class ValueRectT<double>;
  template class ValueRectT<int>;
/*
  bool ValueRect::set(const Nimble::Vector4f & v)
  {
    m_color = v;
    return true;
  }
*/
}

