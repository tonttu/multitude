/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_VERTEXARRAYGL_HPP
#define LUMINOUS_VERTEXARRAYGL_HPP

#include "ResourceHandleGL.hpp"

#include <QRegion>

#include <set>
#include <memory>

namespace Luminous
{
  /// This class represents the VertexArray in GPU memory.
  /// @sa VertexArray
  class VertexArrayGL : public ResourceHandleGL
  {
  public:
    /// Constructor
    /// @param state OpenGL state
    LUMINOUS_API VertexArrayGL(StateGL & state);
    /// Destructor
    LUMINOUS_API ~VertexArrayGL();

    /// Move constructor
    /// @param t vertex array to move
    LUMINOUS_API VertexArrayGL(VertexArrayGL && t);
    /// Move assignment operator
    /// @param t vertex array to move
    LUMINOUS_API VertexArrayGL & operator=(VertexArrayGL && t);

    /// Bind the vertex array
    LUMINOUS_API void bind();

    /// Unbind the vertex array
    LUMINOUS_API void unbind();

    /// Upload he vertex array given specification to the GPU.
    /// @param vertexArray vertex array
    /// @param program shader program to use with the vertex array
    LUMINOUS_API void upload(const VertexArray & vertexArray, ProgramGL * program);

    /// Get the generation count of the vertex array. This counter is used to
    /// keep the CPU and GPU objects synchronized.
    /// @return generation count
    LUMINOUS_API int generation() const { return m_generation; }

  private:
    void setVertexAttributes(const VertexArray & vertexArray, ProgramGL * program);
    void setVertexDescription(const VertexDescription & description, ProgramGL * program);

    int m_generation;
  };
}

#endif
