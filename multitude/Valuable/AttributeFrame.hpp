/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_ATTRIBUTE_FRAME_HPP
#define VALUABLE_ATTRIBUTE_FRAME_HPP

#include <Nimble/Frame4.hpp>

#include "AttributeTuple.hpp"
#include "StyleValue.hpp"

#include <QStringList>

#include <array>

namespace Valuable
{
  /// This class provides an attribute that stores a two-dimensional frame. The
  /// frame width can be individually defined for top, right, bottom, and left
  /// edges of the frame. This class is used by the CSS engine.
  template <>
  class AttributeT<Nimble::Frame4f> :
      public AttributeTuple<Nimble::Frame4f, AttributeT<Nimble::Frame4f>>
  {
  public:
    typedef AttributeTuple<Nimble::Frame4f, AttributeT<Nimble::Frame4f>> Base;
    using Base::operator=;
    using Base::set;

    typedef AttributeT<Nimble::Frame4f> AttributeType;


    AttributeT(Node * host, const QByteArray & name,
               const Nimble::Frame4f & v = Nimble::Frame4f())
      : AttributeTuple<Nimble::Frame4f, AttributeType>(host, name, v)
    {
    }

    static QString priv_elementName(int tupleIndex, QString baseName)
    {
      static const char* suffixes[] = {"-top", "-right", "-bottom", "-left"};
      return baseName.append(suffixes[tupleIndex]);
    }

    bool set(const Nimble::Frame4f& frame, AttributeType::Layer layer,
             QList<ValueUnit> units = QList<ValueUnit>())
    {
      Nimble::Vector4f v(frame.x,frame.y,frame.z,frame.w);
      return set(v, layer, units);
    }

    virtual int priv_t2r(int tupleIndex, int range) const OVERRIDE
    {
      if(tupleIndex != 3 || range != 3)
        return tupleIndex % range;
      else
        return 1;
    }

    virtual void priv_setWrapped(Nimble::Frame4f &v, int index, ElementType elem) const OVERRIDE
    {
      v[index] = elem;
    }

  };
  typedef AttributeT<Nimble::Frame4f> AttributeFrame;
}

#endif // VALUABLE_ATTRIBUTE_FRAME_HPP
