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
    ShaderGLSL(RenderResource::Id id, ShaderType type, RenderDriver & driver);
    ~ShaderGLSL();

    void setText(const QString & text);
    const QString & text() const;

    ShaderType type() const;
  private:
    class D;
    D * m_d;
  };

  /// A shader program, combining multiple ShaderGLSL objects into one runnable program
  class ShaderProgram
    : public RenderResource
  {
  public:
    ShaderProgram(RenderResource::Id id, RenderDriver & driver);
    ~ShaderProgram();

    void addShader(const std::shared_ptr<ShaderGLSL> & shader);
    void removeShader(const std::shared_ptr<ShaderGLSL> & shader);

    const std::shared_ptr<ShaderGLSL> & shader(size_t index) const;
    size_t shaderCount() const;

  private:
    class D;
    D * m_d;
  };
}
#endif // LUMINOUS_SHADERPROGRAM_HPP
