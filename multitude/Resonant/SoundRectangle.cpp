#include "SoundRectangle.hpp"

#define DEFAULT_LOC           Nimble::Vector2i(0, 0)
#define DEFAULT_SIZE          Nimble::Vector2i(1920, 1080)
#define DEFAULT_PAN           0.3f
#define DEFAULT_FADE          100
#define DEFAULT_LEFT_CHANNEL  0
#define DEFAULT_RIGHT_CHANNEL 1

namespace Resonant
{

  SoundRectangle::SoundRectangle()
    :HasValues(0, "SoundRectangle"),
    m_location(this, "location", DEFAULT_LOC),
    m_size(this, "size", DEFAULT_SIZE),
    m_stereoPan(this, "stereo-pan", DEFAULT_PAN),
    m_fadeWidth(this, "fade-width", DEFAULT_FADE),
    m_leftChannel(this, "left-channel", DEFAULT_LEFT_CHANNEL),
    m_rightChannel(this, "right-channel", DEFAULT_RIGHT_CHANNEL)
  {
  }

  SoundRectangle::SoundRectangle(Nimble::Vector2i loc, Nimble::Vector2i size, float stereoPan, int fadeWidth, int leftChannel, int rightChannel)
    :HasValues(0, "SoundRectangle"),
    m_location(this, "location", loc),
    m_size(this, "size", size),
    m_stereoPan(this, "stereo-pan", stereoPan),
    m_fadeWidth(this, "fade-width", fadeWidth),
    m_leftChannel(this, "left-channel", leftChannel),
    m_rightChannel(this, "right-channel", rightChannel)
  {
  }

}
