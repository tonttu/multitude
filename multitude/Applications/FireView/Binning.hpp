/* COPYRIGHT
 *
 * This file is part of FireView.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef BINNING_HPP
#define BINNING_HPP

#include <Nimble/Vector2.hpp>

#include <QString>
#include <QMap>

namespace FireView {

  class Quadrangle {
  public:
    Quadrangle();
    Quadrangle(Nimble::Vector2 a, Nimble::Vector2 b, Nimble::Vector2 c, Nimble::Vector2 d);

    bool inside(Nimble::Vector2 p) const;

    Nimble::Vector2 m_p[4];
  };

  class Binning
  {
  public:

    enum Layout {
      BINNING_ANSI_C78_377,
      BINNING_CREE,
      BINNING_TACTION7,
      BINNING_TACTION7AB
    };

    Binning();

    void defineBin(const QString & name, const Quadrangle & region);

    const QString & classify(Nimble::Vector2 p) const;


    void defineBins_ANSI_C78_377();
    void defineBins_CREE();
    void defineBins_TACTION7();
    void defineBins_TACTION7AB();

    void defineBins(Layout type)
    {
      if(type == BINNING_ANSI_C78_377)
        defineBins_ANSI_C78_377();
      else if(type == BINNING_CREE)
        defineBins_CREE();
      else if(type == BINNING_TACTION7)
        defineBins_TACTION7();
      else if(type == BINNING_TACTION7AB)
        defineBins_TACTION7AB();
    }

    void debugVisualize(int sx, int sy);
    mutable Nimble::Vector2 m_debugLastPoint;

    typedef QMap<QString, Quadrangle> Regions;

    Regions m_regions;
  };

  }

#endif // BINNING_HPP
