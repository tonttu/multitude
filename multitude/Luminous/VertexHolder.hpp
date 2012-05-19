#ifndef LUMINOUS_VERTEXHOLDER_HPP
#define LUMINOUS_VERTEXHOLDER_HPP

#include <vector>
#include <typeinfo>

#include "GLSLProgramObject.hpp"
#include "VertexBuffer.hpp"

#include <Radiant/Trace.hpp>

#include <functional>

//@cond
namespace Luminous
{
  class RenderPacket;
  // typedef std::function< void (RenderContext &) > PacketLoaderFunction;

  // This class is internal to the Luminous library, it is not exported or anything like that
  class VertexHolder
  {
  public:
    friend class RenderContext;
    friend class RenderPacket;

    VertexHolder();
    ~VertexHolder();

    void clear() { m_buffer.clear(); }

    bool empty() const { return m_buffer.empty(); }

    template <class S>
    void addVertex(const S & v)
    {
      {
        // Once everything is stable, we can comment this out
        const char * stype = typeid(S).name();
        if(empty())
          strcpy(m_vertextype, stype);
        else if(strcmp(stype, m_vertextype) != 0) {
          Radiant::fatal("VertexHolder::addVertex # Expected \"%s\" got \"%s\"", m_vertextype, stype);
        }
      }

      size_t now = m_buffer.size();
      m_buffer.resize(now + sizeof(v));
      * (S*) & m_buffer[now] = v;
    }

    /// Number of elements of kind S in the buffer
    template <class S>
    size_t count() const { return m_buffer.size() / sizeof(S); }

    /// Number of bytes in the buffer
    size_t bytes() const { return m_buffer.size(); }

  private:

    std::vector<uint8_t> m_buffer;
    // Effectively the class name of the current type, for manual type safety checks
    char m_vertextype[256];
  };

  class RenderPacket
  {
  public:

    typedef void(*RenderFunction)(RenderContext &, RenderPacket &);

    RenderPacket();
    ~RenderPacket();

    void clear() { m_vertices.clear(); }
    bool empty() const { return m_vertices.empty(); }

    template <class S>
    void addVertex(const S & a)
    {
      m_vertices.addVertex<S>(a);
    }

    template <class S>
    S * vertexData() { return (S*) & m_vertices.m_buffer[0]; }

    VertexHolder & vertices() { return m_vertices; }

    VertexBuffer & vbo() { return  m_vbo; }

    void setPacketRenderFunction(RenderFunction func) { m_func = func; }
    RenderFunction renderFunction() const { return m_func;  }

    void setProgram(GLSLProgramObject * prog) { m_program = prog; }
    GLSLProgramObject * program() { return m_program; }



  private:

    GLSLProgramObject * m_program;
    VertexHolder        m_vertices;
    VertexBuffer        m_vbo;
    RenderFunction      m_func;
  };


  class RectVertex
  {
  public:
    RectVertex() { memset(this, 0, sizeof (*this)); }

    static void render(RenderContext &, RenderPacket &);

    Nimble::Vector2 m_location;
    Nimble::Vector2 m_texCoord;
    Nimble::Vector4 m_color;
    float           m_useTexture;
    //to help debug
    friend std::ostream& operator<<(std::ostream& s, RectVertex& r)
    {
        s<<"[ m_location ="<<r.m_location
         <<", m_texCoord="<<r.m_texCoord
         <<", m_useTexture="<<r.m_useTexture
         <<" ]"<<std::endl;
        return s;
    }
  };


  }

//@endcod
#endif // VERTEXHOLDER_HPP
