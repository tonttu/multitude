/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RESONANT_SOUNDRECTANGLE_HPP
#define RESONANT_SOUNDRECTANGLE_HPP

#include "Export.hpp"

#include <Nimble/Rect.hpp>

#include <Valuable/Node.hpp>
#include <Valuable/AttributeVector.hpp>
#include <Valuable/AttributeFloat.hpp>

namespace Resonant
{

  /** SoundRectangle defines a rectangular area in screen coordinates for stereo
sound output. The rectangle has two output channels (left & right) defined at
the middle or the left and right edge of the rectangle.

The audio is panned between the left and right channels inside the rectangle.
The amount of loss in gain on the right channel when the sound source is in the
left of the rectangle can be adjusted. The rectangle also has an extra border
where the gain falls to zero if the sound source moves outside the rectangle.
  */
  class RESONANT_API SoundRectangle : public Valuable::Node
  {
  public:
    /// Constructs a sound rectangle
    SoundRectangle();
    /** Constructs a sound rectangle with given parameters
     @param loc location of the upper-left corner of the rectangle
     @param size size of the rectangle
     @param stereoPan defines how much the gain will change inside the
    rectangle between the left and right channels. For example, a value of 0.3
    will cause the right channel to output audio at 70% of volume when the
    sound source is located at the left edge of the rectangle. Valid values are
    between 0 and 1.
    @param fadeWidth the width of the extra border outside the rectangle in which the volume falls linearly to zero
    @param leftChannel id of the channel on the left side of the rectangle
    @param rightChannel id of the channel on the right side of the rectangle
    */
    SoundRectangle(Nimble::Vector2i loc, Nimble::Vector2i size, float stereoPan, int fadeWidth, int leftChannel, int rightChannel);

    /// Sets the location of the rectangle
    void setLocation(Nimble::Vector2i loc) { m_location = loc; }
    /// Sets the size of the rectangle
    void setSize(Nimble::Vector2i size) { m_size = size; }
    /// Sets the amount of stereo panning inside the rectangle
    void setStereoPan(float pan) { m_stereoPan = pan; }
    /// Sets the width of the border outside the rectangle that fades volume to zero
    void setFadeWidth(int fade) { m_fadeWidth = fade; }
    /// Sets the ids of the two channels
    void setChannels(int leftChannel, int rightChannel) { m_leftChannel = leftChannel; m_rightChannel = rightChannel; }

    /// Returns the location of the rectangle
    Nimble::Vector2i location() const { return m_location.asVector(); }
    /// Returns the size of the rectangle
    Nimble::Vector2i size() const { return m_size.asVector(); }
    /// Returns amount of stereo panning. See the following table for examples
    /// values and how it affects source volume levels:
    /// <table>
    /// <tr><th>stereo-pan</th><th>source location</th><th>gain at the left speaker</th><th>gain at the right speaker</th></tr>
    /// <tr><td>1.0</td>       <td>left</td>           <td>1.0</td>                     <td>0.0</td></tr>
    /// <tr><td>1.0</td>       <td>center</td>         <td>0.5</td>                     <td>0.5</td></tr>
    /// <tr><td>1.0</td>       <td>right</td>          <td>0.0</td>                     <td>1.0</td></tr>
    /// <tr><td>0.5</td>       <td>left</td>           <td>1.0</td>                     <td>0.5</td></tr>
    /// <tr><td>0.5</td>       <td>center</td>         <td>0.75</td>                    <td>0.75</td></tr>
    /// <tr><td>0.5</td>       <td>right</td>          <td>0.5</td>                     <td>1.0</td></tr>
    /// <tr><td>0.0</td>       <td>left</td>           <td>1.0</td>                     <td>1.0</td></tr>
    /// <tr><td>0.0</td>       <td>center</td>         <td>1.0</td>                     <td>1.0</td></tr>
    /// <tr><td>0.0</td>       <td>right</td>          <td>1.0</td>                     <td>1.0</td></tr>
    /// </table>
    float stereoPan() const { return m_stereoPan; }
    /// Returns the width of the fade border
    int fade() const { return m_fadeWidth; }
    /// Returns the left channel id
    int leftChannel() const { return m_leftChannel; }
    /// Returns the right channel id
    int rightChannel() const { return m_rightChannel; }

    /// Returns the SoundRectangle as Nimble::Recti
    Nimble::Recti rect() const { return Nimble::Recti(location(), Nimble::Size(size())); }

  private:
    // Corner location of the rectangle in screen coordinates
    Valuable::AttributeVector2i m_location;
    // Rectangle size in screen coordinates
    Valuable::AttributeVector2i m_size;
    // Percentage value [0,1] of stereo panning inside the rectangle
    Valuable::AttributeFloat m_stereoPan;
    // Width in screen coordinates outside the rectangle where volume fades linearly to zero
    Valuable::AttributeInt m_fadeWidth;
    // Left audio channel
    Valuable::AttributeInt m_leftChannel;
    // Right audio channel
    Valuable::AttributeInt m_rightChannel;
  };

}

#endif // SOUNDRECTANGLE_HPP
