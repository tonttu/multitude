/* COPYRIGHT
 */

#ifndef VALUABLE_VALUE_RECT_HPP
#define VALUABLE_VALUE_RECT_HPP

#include <Radiant/StringUtils.hpp>
#include <Valuable/Export.hpp>
#include <Valuable/AttributeObject.hpp>

#include <Nimble/Rect.hpp>

namespace Valuable
{

  /// A valuable object holding a Nimble::Rect object
  template <class T>
  class AttributeRectT : public AttributeT<Nimble::RectT<T> >
  {
    typedef AttributeT<Nimble::RectT<T> > Base;
  public:
    using Base::operator =;

    /// @copydoc Attribute::Attribute(Node *, const std::string &, bool transit)
    /// @param r The rectangle to be stored in the AttributeRect
    AttributeRectT(Node * host, const QString & name, const Nimble::RectT<T> & r, bool transit = false);

    virtual const char * type() const OVERRIDE;

    virtual QString asString(bool * const ok = 0) const OVERRIDE;

    virtual bool deserialize(const ArchiveElement & element) OVERRIDE;

    /// Converts the object to rectangle
    Nimble::RectT<T> asRect() const { return this->value(); }
  };

  /// Default floating point AttributeRectT typedef
  typedef AttributeRectT<float> AttributeRect;
  /// AttributeRectT of floats
  typedef AttributeRectT<float> AttributeRectf;
  /// AttributeRectT of doubles
  typedef AttributeRectT<double> AttributeRectd;
  /// AttributeRectT of ints
  typedef AttributeRectT<int> AttributeRecti;


  template <class T>
  AttributeRectT<T>::AttributeRectT(Node * host, const QString & name, const Nimble::RectT<T> & r, bool transit)
    : Base(host, name, r, transit)
  {}

  template <class T>
  bool AttributeRectT<T>::deserialize(const ArchiveElement & element) {
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
  QString AttributeRectT<T>::asString(bool * const ok) const {
    if(ok) *ok = true;

    const Nimble::RectT<T> & rect = this->value();
    const auto lo = rect.low();
    const auto hi = rect.high();

    QString r = Radiant::StringUtils::stringify(lo[0]);
    r += QString(" ") + Radiant::StringUtils::stringify(lo[1]);
    r += QString(" ") + Radiant::StringUtils::stringify(hi[0]);
    r += QString(" ") + Radiant::StringUtils::stringify(hi[1]);

    return r;
  }
}

#endif
