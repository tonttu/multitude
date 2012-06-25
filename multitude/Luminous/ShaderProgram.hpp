#if !defined (LUMINOUS_SHADERPROGRAM_HPP)
#define LUMINOUS_SHADERPROGRAM_HPP

#include "Luminous/RenderResource.hpp"
#include <Radiant/RefPtr.hpp>

#include <QString>

namespace Luminous
{
  /// A single shader (vertex, fragment, etc)
  class ShaderGLSL
    : public RenderResource
  {
  public:
    LUMINOUS_API ShaderGLSL(ShaderType type);
    LUMINOUS_API ~ShaderGLSL();

    LUMINOUS_API void loadText(const QString & filename);
    LUMINOUS_API void setText(const QString & text);
    LUMINOUS_API const QString & text() const;

    LUMINOUS_API ShaderType type() const;
  private:
    friend class ShaderProgram;
    class D;
    D * m_d;
  };

  /// A shader program, combining multiple ShaderGLSL objects into one runnable program
  class ShaderProgram
    : public RenderResource
  {
  public:
    LUMINOUS_API ShaderProgram();
    LUMINOUS_API ~ShaderProgram();

    LUMINOUS_API void addShader(const ShaderGLSL & shader);
    LUMINOUS_API void removeShader(const ShaderGLSL & shader);

    LUMINOUS_API const ShaderGLSL & shader(size_t index) const;
    LUMINOUS_API size_t shaderCount() const;

  private:
    class D;
    D * m_d;
  };
}
#endif // LUMINOUS_SHADERPROGRAM_HPP
