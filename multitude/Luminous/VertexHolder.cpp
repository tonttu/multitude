#include "VertexHolder.hpp"

namespace Luminous
{

  VertexHolder::VertexHolder()
  {
    strcpy(m_vertextype, "Undefined");
  }

  VertexHolder::~VertexHolder()
  {}



  RenderPacket::RenderPacket()
    : m_program(0)
  {}

  RenderPacket::~RenderPacket()
  {}


  }
