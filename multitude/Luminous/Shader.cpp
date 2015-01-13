/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Shader.hpp"

#include <Radiant/Mutex.hpp>

#include <QFile>
#include <QCryptographicHash>

#include <map>

namespace
{
  struct ShaderCode
  {
    QByteArray text;
    Luminous::RenderResource::Hash hash;
  };

  static Radiant::Mutex s_shaderCacheMutex;
  static std::map<QString, std::weak_ptr<ShaderCode>> s_shaderCache;

  static std::shared_ptr<ShaderCode> loadFromText(const QByteArray & text)
  {
    std::shared_ptr<ShaderCode> code = std::make_shared<ShaderCode>();
    code->text = text;
    QCryptographicHash hasher(QCryptographicHash::Md5);
    hasher.addData(text);
    memcpy(&code->hash, hasher.result().data(), sizeof(code->hash));
    return code;
  }

  static std::shared_ptr<ShaderCode> loadFromFile(const QString & filename)
  {
    Radiant::Guard g(s_shaderCacheMutex);
    std::weak_ptr<ShaderCode> & weak = s_shaderCache[filename];
    std::shared_ptr<ShaderCode> ptr = weak.lock();
    if(ptr)
      return ptr;

    QFile shaderFile(filename);
    if(!shaderFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
      Radiant::warning("Shader: Unable to open shader file %s", filename.toUtf8().data());
      return ptr;
    }
    ptr = loadFromText(shaderFile.readAll());
    weak = ptr;
    return ptr;
  }
}

namespace Luminous
{
  class Shader::D
  {
  public:
    Shader::Type m_type;
    QString m_filename;
    std::shared_ptr<ShaderCode> m_code;
  };

  Shader::Shader(Type type)
    : m_d(new Shader::D())
  {
    m_d->m_type = type;
  }

  Shader::~Shader()
  {
  }

  Shader::Shader(Shader && s)
    : m_d(std::move(s.m_d))
  {
  }

  Shader & Shader::operator=(Shader && s)
  {
    std::swap(m_d, s.m_d);
    return *this;
  }

  void Shader::setText(const QByteArray & src)
  {
    m_d->m_code = loadFromText(src);
    m_d->m_filename = QString();
  }

  bool Shader::loadText(const QString & filename)
  {
    m_d->m_code = loadFromFile(filename);
    m_d->m_filename = filename;

    return m_d->m_code && !m_d->m_code->text.isEmpty();
  }

  const QByteArray & Shader::text() const
  {
    static QByteArray s_null;
    return m_d->m_code ? m_d->m_code->text : s_null;
  }

  const QString & Shader::filename() const
  {
    return m_d->m_filename;
  }

  Shader::Type Shader::type() const
  {
    return m_d->m_type;
  }

  RenderResource::Hash Shader::hash() const
  {
    return m_d->m_code ? m_d->m_code->hash : RenderResource::Hash();
  }
}
