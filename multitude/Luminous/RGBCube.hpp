/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RGBCUBE_HPP
#define RGBCUBE_HPP

#include <Luminous/Texture.hpp>

#include <Nimble/Vector3.hpp>

#include <Valuable/Node.hpp>

namespace Luminous
{
  class ColorCorrection;

  /**
    This class is used to perform RGB color correction using 3D color lookup
    grid. This means that the whole RGB color cube can be remapped to attain
    desired color changes.
  */
  class LUMINOUS_API RGBCube : public Valuable::Node
  {
  public:
    RGBCube(Node * host = 0, const QByteArray & name = "");
    virtual ~RGBCube();

    /// Creates a default lookup grid with the specified division
    /// @param division Grid division
    void createDefault(int division = 3);

    /// Returns the division of the lookup grid. The parameter describes how
    /// many sample points are used per dimension.
    /// @return number of divisions
    int division() const;
    /// Sets the number of divisions. Calling this will resize the lookup grid
    /// but not populate it with any data.
    /// @param division Number of divisions
    void setDivision(int division);

    /// Checks if the RGBCube is defined, ie. if it contains any valid samples.
    /// @return true if the cube is defined, false otherwise
    bool isDefined() const;

    /// Returns the number of values in the lookup grid.
    size_t rgbCount() const;

    /// Returns the white color of the color lookup grid.
    Nimble::Vector3 white() const;

    /// Returns a 3D texture that represents the color lookup grid.
    /// @return 3D texture
    const Luminous::Texture & asTexture() const;

    /// Sets all sample points of the grid to the specified color
    /// @param rgb Color to populate the grid with
    void setAll(Nimble::Vector3 rgb);

    /// Sets the color for the specified index.
    /// @param index Linear index for the lookup grid
    /// @param rgb Color for the index
    void setIndex(size_t index, Nimble::Vector3 rgb);
    /// Returns the color at the specified linear index
    /// @param index Linear index for the lookup grid
    /// @return Color at index
    Nimble::Vector3 getIndex(size_t index) const;

    /// Sets the color for the specified index
    void setRGB(int rindex, int gindex, int bindex, Nimble::Vector3 rgb);
    /// Returns the color with the specified index
    Nimble::Vector3 getRGB(int rindex, int gindex, int bindex) const;

    /// Sets the error for the sample at the specified index
    void setError(size_t index, float error);

    /// Finds the linear index to the color that most closely resembles the
    /// specified color
    /// @return Linear index to the lookup grid
    int findClosestRGBIndex(Nimble::Vector3 color) const;

    /// Checks if the configuration has changed
    bool deserialize(const Valuable::ArchiveElement & element) OVERRIDE;

    /// Converts and recalculates the lookup grid from color correction splines
    void fromColorSplines(const Luminous::ColorCorrection & cc);

  private:
    class D;
    D * m_d;
  };
}
#endif // RGBCUBE_HPP
