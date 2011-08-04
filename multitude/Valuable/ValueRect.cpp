/* COPYRIGHT
 */

#include "ValueRect.hpp"
#include "DOMElement.hpp"

#include <Radiant/StringUtils.hpp>

namespace Valuable
{

  template <class T>
  ValueRectT<T>::ValueRectT(HasValues * host, const QString & name, const Nimble::Rect & r, bool transit)
    : Base(host, name, r, transit)
  {}

  template <class T>
  bool ValueRectT<T>::deserialize(ArchiveElement & element) {
    std::stringstream in(element.get().toUtf8().data());

    Nimble::Vector2T<T> lo, hi;

    in >> lo[0];
    in >> lo[1];
    in >> hi[0];
    in >> hi[1];

    *this = Nimble::RectT<T>(lo, hi);
    return true;
  }

  template <class T>
  QString ValueRectT<T>::asString(bool * const ok) const {
    if(ok) *ok = true;

    const Nimble::Rect & rect = value();
    const Nimble::Vector2f & lo = rect.low();
    const Nimble::Vector2f & hi = rect.high();

    QString r = Radiant::StringUtils::stringify(lo[0]);
    r += QString(" ") + Radiant::StringUtils::stringify(lo[1]);
    r += QString(" ") + Radiant::StringUtils::stringify(hi[0]);
    r += QString(" ") + Radiant::StringUtils::stringify(hi[1]);

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
}

