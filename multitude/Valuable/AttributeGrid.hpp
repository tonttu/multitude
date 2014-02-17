/* Copyright (C) 2007-2014: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_VALUE_GRID_HPP
#define VALUABLE_VALUE_GRID_HPP

#include <Valuable/Export.hpp>
#include <Valuable/Attribute.hpp>

#include <Radiant/Grid.hpp>
#include <QString>

namespace Valuable
{
  template<class T>
  class VALUABLE_API AttributeGrid : public Attribute
  {
    typedef Radiant::GridT<T, Radiant::GridMemT<T>> GridType;

  public:
    AttributeGrid()
    {}

    AttributeGrid(Node * host, const QByteArray & name, bool transit = false)
      : Attribute(host, name, transit)
    {}

    virtual bool deserialize(const Valuable::ArchiveElement&)
    {
      return false;
    }

    template<typename U>
    AttributeGrid & operator=(const U & that)
    {
      m_grid.copy(that.data(), that.width(), that.height());
      emitChange();
      return *this;
    }

    const GridType & operator*() const
    {
      return m_grid;
    }

  private:
    GridType m_grid;
  };
}

#endif
