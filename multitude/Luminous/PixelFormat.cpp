/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include <Luminous/PixelFormat.hpp>

#include <sstream>

namespace Luminous
{
  /*
     PixelFormat::PixelFormat() 
     : m_layout(LAYOUT_UNKNOWN),
     m_type(TYPE_UNKNOWN)
     {}
     */
  PixelFormat::PixelFormat(const PixelFormat& pf) 
  {
    m_layout = pf.m_layout;
    m_type = pf.m_type;
  }

  PixelFormat::PixelFormat(ChannelLayout layout, ChannelType type):
    m_layout(layout), 
    m_type(type)
  {}

  PixelFormat::~PixelFormat()
  {}

  int PixelFormat::numChannels() const 
  {
    switch(m_layout) {
      case LAYOUT_COLOR_INDEX:
      case LAYOUT_STENCIL_INDEX:
      case LAYOUT_DEPTH_COMPONENT:
      case LAYOUT_RED:
      case LAYOUT_GREEN:
      case LAYOUT_BLUE:
      case LAYOUT_ALPHA:
      case LAYOUT_LUMINANCE:
        return 1;
        break;
      case LAYOUT_LUMINANCE_ALPHA:
        return 2;
        break;
      case LAYOUT_RGB:
      case LAYOUT_BGR:
        return 3;
        break;
      case LAYOUT_RGBA:
      case LAYOUT_BGRA:
        return 4;
        break;          
      default:
        return 0;
    }
  }

  int PixelFormat::bytesPerPixel() const
  {
    int nc = numChannels();

    switch(m_type) {
      case TYPE_BYTE:
      case TYPE_UBYTE:
        return 1 * nc;
        break;
      case TYPE_SHORT:
      case TYPE_USHORT:
        return 2 * nc;
        break;
      case TYPE_INT:
      case TYPE_UINT:
      case TYPE_FLOAT:
        return 4 * nc;
        break;
      case TYPE_DOUBLE:
        return 8 * nc;
        break;
      default:
        return 0;
    }
  }

  static std::string typeToString(PixelFormat::ChannelType type) 
  {
    switch(type)
    {
      case PixelFormat::TYPE_UNKNOWN:
        return "TYPE_UNKNOWN";
      case PixelFormat::TYPE_BYTE:
        return "TYPE_BYTE";
      case PixelFormat::TYPE_UBYTE:
        return "TYPE_UBYTE";
      case PixelFormat::TYPE_SHORT:
        return "TYPE_SHORT";
      case PixelFormat::TYPE_USHORT:
        return "TYPE_USHORT";
      case PixelFormat::TYPE_INT:
        return "TYPE_INT";
      case PixelFormat::TYPE_UINT:
        return "TYPE_UINT";
      case PixelFormat::TYPE_FLOAT:
        return "TYPE_FLOAT";
      case PixelFormat::TYPE_DOUBLE:
        return "TYPE_DOUBLE";
      default:
        return "Invalid value (should never happen";
    }

  }

  static std::string layoutToString(PixelFormat::ChannelLayout layout)
  {
    switch(layout)
    {
      case PixelFormat::LAYOUT_UNKNOWN:
        return "LAYOUT_UNKNOWN";
      case PixelFormat::LAYOUT_COLOR_INDEX:
        return "LAYOUT_COLOR_INDEX";
      case PixelFormat::LAYOUT_STENCIL_INDEX:
        return "LAYOUT_STENCIL_INDEX";
      case PixelFormat::LAYOUT_DEPTH_COMPONENT:
        return "LAYOUT_DEPTH_COMPONENT";
      case PixelFormat::LAYOUT_RED:
        return "LAYOUT_RED";
      case PixelFormat::LAYOUT_GREEN:
        return "LAYOUT_GREEN";
      case PixelFormat::LAYOUT_BLUE:
        return "LAYOUT_BLUE";
      case PixelFormat::LAYOUT_ALPHA:
        return "LAYOUT_ALPHA";
      case PixelFormat::LAYOUT_RGB:
        return "LAYOUT_RGB";
      case PixelFormat::LAYOUT_RGBA:
        return "LAYOUT_RGBA";
      case PixelFormat::LAYOUT_BGR:
        return "LAYOUT_BGR";
      case PixelFormat::LAYOUT_BGRA:
        return "LAYOUT_BGRA";
      case PixelFormat::LAYOUT_LUMINANCE:
        return "LAYOUT_LUMINANCE";
      case PixelFormat::LAYOUT_LUMINANCE_ALPHA:
        return "LAYOUT_LUMINANCE_ALPHA";
      default:
        return "Invalid value (should never happen)";

    }
  }

  std::string PixelFormat::toString() const
  {
    std::ostringstream ss;
    ss << "PixelFormat(" << layoutToString(m_layout) << ", " << typeToString(m_type) << ")";
    return ss.str();
  }

}
