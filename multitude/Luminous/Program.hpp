#if !defined (LUMINOUS_PROGRAM_HPP)
#define LUMINOUS_PROGRAM_HPP

#include "Luminous/Luminous.hpp"
#include "Luminous/RenderResource.hpp"
#include "Luminous/UniformDescription.hpp"

#include <Valuable/Node.hpp>
#include <Radiant/RefPtr.hpp>

#include <QString>

namespace Luminous
{
  /// A single shader (vertex, fragment, etc)
  class ShaderGLSL
  {
  public:
    enum Type
    {
      Vertex,
      Fragment,
      Geometry,
    };
  public:
    LUMINOUS_API ShaderGLSL(Type type);
    LUMINOUS_API ~ShaderGLSL();

    LUMINOUS_API ShaderGLSL(ShaderGLSL &&);
    LUMINOUS_API ShaderGLSL & operator=(ShaderGLSL &&);

    LUMINOUS_API void loadText(const QString & filename);
    LUMINOUS_API void setText(const QByteArray & text);
    LUMINOUS_API const QByteArray & text() const;

    LUMINOUS_API const QString & filename() const;

    LUMINOUS_API Type type() const;

    LUMINOUS_API RenderResource::Hash hash() const;

  private:
    ShaderGLSL(const ShaderGLSL &);
    ShaderGLSL & operator=(const ShaderGLSL &);

    /*
    friend class Program;*/
    class D;
    D * m_d;
  };

  /// A shader program, combining multiple ShaderGLSL objects into one runnable program
  class Program
    : public RenderResource
    , public Valuable::Node
  {
  public:
    LUMINOUS_API Program();
    LUMINOUS_API ~Program();

    LUMINOUS_API Program(Program && prog);
    LUMINOUS_API Program & operator=(Program && prog);

    LUMINOUS_API ShaderGLSL & addShader(const QByteArray & code, ShaderGLSL::Type type);
    LUMINOUS_API ShaderGLSL & loadShader(const QString & filename, ShaderGLSL::Type type);

    LUMINOUS_API void removeShader(const ShaderGLSL & shader);

    LUMINOUS_API QStringList shaderFilenames() const;

    LUMINOUS_API ShaderGLSL & shader(size_t index) const;
    LUMINOUS_API size_t shaderCount() const;

    LUMINOUS_API Hash hash() const;

    template <typename T> void addShaderUniform(const QString & name, const T & value);
    template <typename T> void addShaderUniform(const QString & name, T & value);

    LUMINOUS_API void removeShaderUniform(const QString & name);

    LUMINOUS_API size_t uniformCount() const;
    LUMINOUS_API ShaderUniform & uniform(size_t index) const;

    LUMINOUS_API const VertexDescription & vertexDescription() const;
    LUMINOUS_API void setVertexDescription(const VertexDescription & description);

    LUMINOUS_API const UniformDescription & uniformDescription() const;
    LUMINOUS_API void setUniformDescription(const UniformDescription & description);

    LUMINOUS_API bool translucent() const;
    LUMINOUS_API void setTranslucency(bool translucency);

  private:
    class D;
    D * m_d;
  };
}
#endif // LUMINOUS_PROGRAM_HPP