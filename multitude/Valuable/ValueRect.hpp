/* COPYRIGHT
 */

#ifndef VALUABLE_VALUE_RECT_HPP
#define VALUABLE_VALUE_RECT_HPP

#include <Valuable/Export.hpp>
#include <Valuable/ValueObject.hpp>

#include <Nimble/Rect.hpp>

namespace Valuable
{

  /// A valuable object holding a Nimble::Rect object
  template <class T>
  class VALUABLE_API AttributeRectT : public AttributeT<Nimble::RectT<T> >
  {
    typedef AttributeT<Nimble::RectT<T> > Base;
  public:
    using Base::operator =;

    /// @copydoc Attribute::Attribute(HasValues *, const std::string &, bool transit)
    /// @param r The rectangle to be stored in the AttributeRect
    AttributeRectT(HasValues * host, const QString & name, const Nimble::RectT<T> & r, bool transit = false);

    const char * type() const;

    QString asString(bool * const ok = 0) const;

    bool deserialize(const ArchiveElement & element);

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
}

#endif
