#ifndef BINNING_HPP
#define BINNING_HPP

#include <Nimble/Vector2.hpp>

#include <QString>
#include <QMap>

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
    Binning();

    void defineBin(const QString & name, const Quadrangle & region);

    const QString & classify(Nimble::Vector2 p) const;


    void defineBins_ANSI_C78_377();
    void defineBins_CREE();
    void defineBins_TACTION();

    void debugVisualize(int sx, int sy);
    mutable Nimble::Vector2 m_debugLastPoint;

    typedef QMap<QString, Quadrangle> Regions;

    Regions m_regions;
};

#endif // BINNING_HPP
