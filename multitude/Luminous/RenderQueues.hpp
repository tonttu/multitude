/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_RENDERQUEUES_HPP
#define LUMINOUS_RENDERQUEUES_HPP

/// @cond

#include "PipelineCommand.hpp"

#include <tuple>
#include <vector>

namespace Luminous
{

  struct RenderState
  {
    Luminous::ProgramGL * program;
    VertexArrayGL * vertexArray;
    BufferGL * uniformBuffer;
    std::array<TextureGL*, 8> textures;

    bool operator<(const RenderState & o) const
    {
      if(program != o.program)
        return program < o.program;
      if(vertexArray != o.vertexArray)
        return vertexArray < o.vertexArray;
      if(uniformBuffer != o.uniformBuffer)
        return uniformBuffer < o.uniformBuffer;
      for(std::size_t i = 0; i < textures.size(); ++i)
        if((!textures[i] || !o.textures[i]) || (textures[i] != o.textures[i]))
          return textures[i] < o.textures[i];

      return false;
    }

    bool operator!=(const RenderState & o) const
    {
      if(program != o.program || vertexArray != o.vertexArray || uniformBuffer != o.uniformBuffer)
        return true;
      for(std::size_t i = 0; i < textures.size(); ++i)
        if(!textures[i] || textures[i] != o.textures[i])
          return textures[i] != o.textures[i];

      return false;
    }
  };

  // A segment of the master render queue. A segment contains two separate
  // command queues, one for opaque draw calls and one for translucent draw
  // calls. The translucent draw calls are never re-ordered in order to
  // guarantee correct output. The opaque queue can be re-ordered to maximize
  // performance by minimizing state-changes etc. The segments themselves are
  // never re-ordered to guarantee correct output.
  struct RenderQueueSegment
  {
    RenderQueueSegment(PipelineCommand * cmd, unsigned int opaqueCmdBegin_, unsigned int translucentCmdBegin_)
      : pipelineCommand(cmd)
      , opaqueCmdBegin(opaqueCmdBegin_)
      , opaqueCmdEnd(opaqueCmdBegin_)
      , translucentCmdBegin(translucentCmdBegin_)
      , translucentCmdEnd(translucentCmdBegin_)
    {}

    std::unique_ptr<PipelineCommand> pipelineCommand;
    unsigned int opaqueCmdBegin;
    unsigned int opaqueCmdEnd;
    unsigned int translucentCmdBegin;
    unsigned int translucentCmdEnd;
  };

} //namespace Luminous

/// @endcond

#endif
