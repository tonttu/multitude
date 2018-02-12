/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "BinaryData.hpp"

#include "DateTime.hpp"
#include "StringUtils.hpp"

#include <Radiant/BinaryStream.hpp>
#include <Radiant/Trace.hpp>
#include <Radiant/ConfigReader.hpp>
#include <Radiant/Timer.hpp>

#include <Nimble/Math.hpp>

#include <string.h>
#include <stdlib.h>

#ifdef _MSC_VER
#define strtoll _strtoi64
#endif

namespace Radiant {

  /// @todo What is this? Shouldn't this be done somewhere else. Debug-prints coming from SDK
  ///       that can't be avoided should be strongly discouraged.
  static bool __verbose = true;

  static void badmarker(const char * func, int32_t marker)
  {
    if(!__verbose)
      return;
    char a = marker & 0xFF;
    char b = (marker >> 8) & 0xFF;
    char c = (marker >> 16) & 0xFF;
    char d = (marker >> 24) & 0xFF;
    Radiant::error("%s # bad marker %c%c%c%c", func, a, b, c, d);
  }

  inline int32_t makeMarker(int32_t a, int32_t b, int32_t c, int32_t d)
  {
    return a | (b << 8) | (c << 16) | (d << 24);
  }

  const int32_t BinaryData::FLOAT_MARKER  = makeMarker(',', 'f', '\0', '\0');
  const int32_t BinaryData::DOUBLE_MARKER  = makeMarker(',', 'd', '\0', '\0');
  const int32_t BinaryData::VECTOR2F_MARKER  = makeMarker(',', 'f', '2', '\0');
  const int32_t BinaryData::VECTOR2I_MARKER  = makeMarker(',', 'i', '2', '\0');
  const int32_t BinaryData::VECTOR3F_MARKER  = makeMarker(',', 'f', '3', '\0');
  const int32_t BinaryData::VECTOR3I_MARKER  = makeMarker(',', 'i', '3', '\0');
  const int32_t BinaryData::VECTOR4F_MARKER  = makeMarker(',', 'f', '4', '\0');
  const int32_t BinaryData::VECTOR4I_MARKER  = makeMarker(',', 'i', '4', '\0');
  const int32_t BinaryData::INT32_MARKER  = makeMarker(',', 'i', '\0', '\0');
  const int32_t BinaryData::INT64_MARKER  = makeMarker(',', 'l', '\0', '\0');
  const int32_t BinaryData::TS_MARKER     = makeMarker(',', 't', '\0', '\0');
  const int32_t BinaryData::STRING_MARKER = makeMarker(',', 's', '\0', '\0');
  const int32_t BinaryData::BLOB_MARKER   = makeMarker(',', 'b', '\0', '\0');

  BinaryData::BinaryData()
    : m_current(0),
    m_total(0),
    m_size(0),
    m_shared(false),
    m_buf(0)
  {}

  BinaryData::BinaryData(const BinaryData & that)
    : m_current(0),
    m_total(0),
    m_size(0),
    m_shared(false),
    m_buf(0)
  {
    *this = that;
  }

  BinaryData::~BinaryData()
  {
    if(!m_shared)
      free(m_buf);
  }

  void BinaryData::writeFloat32(float v)
  {
    ensure(sizeof(int32_t) + sizeof(float));
    getRef<int32_t>() = FLOAT_MARKER;
    getRef<float>()   = v;
  }

  void BinaryData::writeFloat64(double v)
  {
    ensure(sizeof(int32_t) + sizeof(double));
    getRef<int32_t>() = DOUBLE_MARKER;
    getRef<double>()  = v;
  }

  void BinaryData::writeInt32(int32_t v)
  {
    ensure(sizeof(int32_t) + sizeof(int32_t));
    getRef<int32_t>() = INT32_MARKER;
    getRef<int32_t>() = v;
  }

  void BinaryData::writeInt64(int64_t v)
  {
    ensure(sizeof(int32_t) + sizeof(int64_t));
    getRef<int32_t>() = INT64_MARKER;
    getRef<int64_t>() = v;
  }

  void BinaryData::writeTimeStamp(Radiant::TimeStamp v)
  {
    ensure(sizeof(int32_t) + sizeof(int64_t));
    getRef<int32_t>() = TS_MARKER;
    getRef<int64_t>() = v.value();
  }

  void BinaryData::writeString(const char * s)
  {
    size_t len = strlen(s);
    size_t space = stringSpace(s);
    ensure(sizeof(int32_t) + space);

    getRef<int32_t>() = STRING_MARKER;
    char * ptr =  getPtr<char>(space);
    memcpy(ptr, s, len);
    memset(ptr + len, 0, space - len);
  }

  void BinaryData::writeBlob(const void * ptr, int n)
  {
    ensure(2*sizeof(int32_t) + n);

    getRef<int32_t>() = BLOB_MARKER;
    getRef<int32_t>() = n;
    char * dest =  getPtr<char>(n);
    if(n)
      memcpy(dest, ptr, n);
  }

  void BinaryData::writeVector2Float32(Nimble::Vector2f v)
  {
    ensure(sizeof(int32_t) + sizeof(float) * 2);
    getRef<int32_t>() = VECTOR2F_MARKER;
    getRef<float>() = v[0];
    getRef<float>() = v[1];
  }

  void BinaryData::writeVector2Int32(Nimble::Vector2i v)
  {
    ensure(sizeof(int32_t) + sizeof(int) * 2);
    getRef<int32_t>() = VECTOR2I_MARKER;
    getRef<int>() = v[0];
    getRef<int>() = v[1];
  }

  void BinaryData::writeVector3Float32(Nimble::Vector3f v)
  {
    ensure(sizeof(int32_t) + sizeof(float) * 3);
    getRef<int32_t>() = VECTOR3F_MARKER;
    getRef<float>() = v[0];
    getRef<float>() = v[1];
    getRef<float>() = v[2];
  }

  void BinaryData::writeVector3Int32(Nimble::Vector3i v)
  {
    ensure(sizeof(int32_t) + sizeof(int) * 3);
    getRef<int32_t>() = VECTOR3I_MARKER;
    getRef<int>() = v[0];
    getRef<int>() = v[1];
    getRef<int>() = v[2];
  }

  void BinaryData::writeVector4Int32(const Nimble::Vector4i & v)
  {
    ensure(sizeof(int32_t) + sizeof(int) * 4);
    getRef<int32_t>() = VECTOR4I_MARKER;
    getRef<int>() = v[0];
    getRef<int>() = v[1];
    getRef<int>() = v[2];
    getRef<int>() = v[3];
  }

  void BinaryData::writeVector4Float32(const Nimble::Vector4f & v)
  {
    ensure(sizeof(int32_t) + sizeof(float) * 4);
    getRef<int32_t>() = VECTOR4F_MARKER;
    getRef<float>() = v[0];
    getRef<float>() = v[1];
    getRef<float>() = v[2];
    getRef<float>() = v[3];
  }

  void BinaryData::append(const BinaryData & that)
  {
    ensure(that.m_total);
    memcpy(getPtr<char>(that.m_total), that.data(), that.m_total);
  }

  float BinaryData::readFloat32(bool * ok)
  {
    if(ok)
      *ok = true;

    if(!available(8)) {
      if(ok) * ok = false;
      unavailable("BinaryData::readFloat32");
      return 0.0f;
    }

    int32_t marker = getRef<int32_t>();

    if(marker == FLOAT_MARKER)
      return getRef<float>();
    else if(marker == DOUBLE_MARKER)
      return getRef<double>();
    else if(marker == INT32_MARKER)
      return float(getRef<int32_t>());
    else if(marker == INT64_MARKER)
      return float(getRef<int64_t>());
    else if (marker == STRING_MARKER) {
      const char * source = & m_buf[m_current];
      m_current += (unsigned) stringSpace(source);
      return QByteArray(source).toFloat(ok);
    }
    else if(ok)
      *ok = false;

    skipParameter(marker);

    return 0.0f;
  }

  double BinaryData::readFloat64(bool * ok)
  {
    if(ok)
      *ok = true;

    if(!available(8)) {
      if(ok) * ok = false;
      unavailable("BinaryData::readFloat64");
      return 0.0f;
    }

    int32_t marker = getRef<int32_t>();

    if(marker == FLOAT_MARKER)
      return getRef<float>();
    else if(marker == DOUBLE_MARKER)
      return getRef<double>();
    else if(marker == INT32_MARKER)
      return float(getRef<int32_t>());
    else if(marker == INT64_MARKER)
      return float(getRef<int64_t>());
    else if (marker == STRING_MARKER) {
      const char * source = & m_buf[m_current];
      m_current += (unsigned) stringSpace(source);
      return QByteArray(source).toDouble(ok);
    }

    else if(ok)
      *ok = false;

    skipParameter(marker);

    return 0.0;
  }

  int32_t BinaryData::readInt32(bool * ok)
  {
    if(ok)
      *ok = true;

    if(!available(4)) {
      if(ok) * ok = false;
      unavailable("BinaryData::readInt32");
      return 0;
    }

    int32_t marker = getRef<int32_t>();

    if(marker == INT32_MARKER && available(4))
      return getRef<int32_t>();
    else if(marker == INT64_MARKER && available(8))
      return int32_t(getRef<int64_t>());
    else if(marker == FLOAT_MARKER && available(4))
      return Nimble::Math::Round(getRef<float>());
    else if(marker == DOUBLE_MARKER && available(8))
      return Nimble::Math::Round(getRef<double>());
    else if (marker == STRING_MARKER) {
      const char * source = & m_buf[m_current];
      char * end = (char *) source;
      long d = strtol(m_buf + m_current, & end, 10);
      if(end == (char *) source) {
        if(ok)
          *ok = false;
      }
      else {
        // m_current = end - m_buf;
        m_current += (unsigned) stringSpace(m_buf + m_current);
        return d;
      }
    }
    else if(ok) {
      badmarker("BinaryData::readInt32", marker);
      *ok = false;
    }

    skipParameter(marker);
    return 0;
  }

  int64_t BinaryData::readInt64(bool * ok)
  {
    if(ok)
      *ok = true;

    if(!available(8)) {
      if(ok) * ok = false;
      unavailable("BinaryData::readInt64");
      return 0;
    }

    int32_t marker = getRef<int32_t>();

    if(marker == INT64_MARKER)
      return getRef<int64_t>();
    else if(marker == INT32_MARKER)
      return getRef<int32_t>();
    else if(marker == FLOAT_MARKER)
      return Nimble::Math::Round(getRef<float>());
    else if(marker == DOUBLE_MARKER)
      return Nimble::Math::Round(getRef<double>());
    else if (marker == STRING_MARKER) {
      const char * source = & m_buf[m_current];
      char * end = (char *) source;
      long long d = strtoll(m_buf + m_current, & end, 10);
      if(end == (char *) source) {
        Radiant::error("BinaryData::readInt64 # in strtoll");
        if(ok)
          *ok = false;
      }
      else {
        // m_current = end - m_buf;
        m_current += (unsigned) stringSpace(m_buf + m_current);
        return d;
      }
    }
    else if(ok) {
      badmarker("BinaryData::readInt64", int32_t(marker));
      *ok = false;
    }

    skipParameter(int(marker));
    return 0;
  }

  TimeStamp BinaryData::readTimeStamp(bool * ok)
  {
    if(ok)
      *ok = true;

    if(!available(4)) {
      if(ok) * ok = false;
      unavailable("BinaryData::readTimeStamp");
      return TimeStamp(0);
    }

    int32_t marker = getRef<int32_t>();

    if(marker == TS_MARKER && available(8))
      return TimeStamp(getRef<int64_t>());
    else if (marker == STRING_MARKER && available(10)) {
      const char * source = & m_buf[m_current];
      DateTime dt;
      bool dtok = dt.fromString(source);

      if(!dtok && ok)
        *ok = false;
      else
        return dt.asTimeStamp();
    }
    else if(ok)
      *ok = false;

    skipParameter(marker);
    return TimeStamp(0);
  }

  bool BinaryData::readString(char * str, size_t maxbytes)
  {
    if(!available(sizeof(int32_t))) {
      *str = '\0';
      unavailable("BinaryData::readString");
      return false;
    }

    int32_t marker = getRef<int32_t>();

    if(marker != STRING_MARKER) {
      skipParameter(marker);
      return false;
    }
    const char * source = & m_buf[m_current];
    size_t len = strlen(source);

    skipParameter(marker);

    if(len >= maxbytes) {
      str[0] = 0;
      return false;
    }

    memcpy(str, source, len);
    str[len] = '\0';

    return true;
  }

  bool BinaryData::readString(QString & str)
  {
    if(!available(sizeof(int32_t))) {
      str.clear();
      unavailable("BinaryData::readString");
      return false;
    }

    int32_t marker = getRef<int32_t>();

    if(marker == STRING_MARKER) {
      str = QString::fromUtf8(m_buf + m_current);
      skipParameter(marker);
    } else {
      skipParameter(marker);
      return false;
    }
    return true;
  }

  bool BinaryData::readString(QByteArray & str)
  {
    if(!available(sizeof(int32_t))) {
      str.clear();
      unavailable("BinaryData::readString");
      return false;
    }

    int32_t marker = getRef<int32_t>();

    if(marker == STRING_MARKER) {
      str = m_buf + m_current;
      skipParameter(marker);
    } else {
      skipParameter(marker);
      return false;
    }
    return true;
  }

  bool BinaryData::readBlob(void * ptr, int n)
  {
    if(!available(sizeof(int32_t))) {
      unavailable("BinaryData::readBlob");
      return false;
    }

    int32_t marker = getRef<int32_t>();

    if(marker != BLOB_MARKER) {
      skipParameter(marker);
      return false;
    }

    int32_t recv = getRef<int32_t>();

    const char * source = & m_buf[m_current];

    m_current += recv;

    memcpy( ptr, source, std::min(n, recv));

    return n == recv;
  }

  bool BinaryData::readBlob(std::vector<uint8_t> & buf)
  {
    if(!available(sizeof(int32_t))) {
      unavailable("BinaryData::readBlob");
      return false;
    }

    int32_t marker = getRef<int32_t>();

    if(marker != BLOB_MARKER) {
      skipParameter(marker);
      return false;
    }

    int32_t recv = getRef<int32_t>();

    const char * source = & m_buf[m_current];

    m_current += recv;

    buf.resize(recv);

    if(recv > 0)
      memcpy( & buf[0], source, recv);

    return true;

  }

  int BinaryData::readBlobPtr(void *& ptr)
  {
    if(!available(sizeof(int32_t))) {
      unavailable("BinaryData::readBlobPtr");
      return -1;
    }

    int32_t marker = getRef<int32_t>();

    if(marker != BLOB_MARKER) {
      skipParameter(marker);
      return -1;
    }

    int32_t recv = getRef<int32_t>();

    ptr = &m_buf[m_current];

    m_current += recv;

    return recv;
  }

#define BD_STR_TO_VEC(type, n, ok) \
  const char * source = & m_buf[m_current]; \
                        Radiant::Variant v(source); \
                        skipParameter(marker); \
                        type vect; \
                        vect.clear(); \
                        if(v.getFloats(vect.data(), n) == n) \
                        return vect; \
                        else { \
                               if(ok) \
                               *ok = false; \
                                     vect.clear(); \
                                     return vect; \
                                   }

  Nimble::Vector2f BinaryData::readVector2Float32(bool * ok)
  {
    if(ok)
      *ok = true;

    if(!available(4)) {
      if(ok) * ok = false;
      unavailable("BinaryData::readVector2Float32");
      return Nimble::Vector2f(0, 0);
    }

    int32_t marker = getRef<int32_t>();

    if(marker == VECTOR2F_MARKER) {

      if(!available(8)) {
        if(ok) * ok = false;
        unavailable("BinaryData::readVector2Float32");
        return Nimble::Vector2f(0, 0);
      }

      return getRef<Nimble::Vector2f>();
    }
    else if(marker == VECTOR2I_MARKER) {

      if(!available(8)) {
        if(ok) * ok = false;
        unavailable("BinaryData::readVector2Float32");
        return Nimble::Vector2f(0, 0);
      }

      Nimble::Vector2i r = getRef<Nimble::Vector2i>();

      return Nimble::Vector2f(r.x, r.y);
    }
    else if(marker == STRING_MARKER) {
      BD_STR_TO_VEC(Nimble::Vector2f, 2, ok);
    }
    else {
      skipParameter(marker);
      if(ok)
        *ok = false;

      return Nimble::Vector2f(0, 0);
    }

  }

  Nimble::Vector3f BinaryData::readVector3Float32(bool * ok)
  {
    if(ok)
      *ok = true;

    if(!available(16)) {
      if(ok) * ok = false;
      unavailable("BinaryData::readVector3Float32");
      return Nimble::Vector3f(0, 0, 0);
    }

    int32_t marker = getRef<int32_t>();

    if(marker == VECTOR3F_MARKER) {
      return getRef<Nimble::Vector3f>();
    }
    else if(marker == VECTOR3I_MARKER) {
      Nimble::Vector3i r = getRef<Nimble::Vector3i>();

      return Nimble::Vector3f(r.x, r.y, r.z);
    }
    else if(marker == STRING_MARKER) {
      BD_STR_TO_VEC(Nimble::Vector3f, 3, ok);
    }
    else {
      skipParameter(marker);
      if(ok)
        *ok = false;

      return Nimble::Vector3f(0, 0, 0);
    }

  }

  Nimble::Vector2i BinaryData::readVector2Int32(bool * ok)
  {
    if(ok)
      *ok = true;

    if(!available(12)) {
      if(ok) * ok = false;
      unavailable("BinaryData::readVector2Int32");
      return Nimble::Vector2i(0, 0);
    }

    int32_t marker = getRef<int32_t>();

    if(marker == VECTOR2I_MARKER) {
      return getRef<Nimble::Vector2i>();
    }
    else if(marker == VECTOR2F_MARKER) {
      Nimble::Vector2f r = getRef<Nimble::Vector2f>();

      return Nimble::Vector2i(r.x, r.y);
    }
    else {
      skipParameter(marker);
      if(ok)
        *ok = false;

      return Nimble::Vector2i(0, 0);
    }

  }

  Nimble::Vector3i BinaryData::readVector3Int32(bool * ok)
  {
    if(ok)
      *ok = true;

    if(!available(16)) {
      if(ok) * ok = false;
      unavailable("BinaryData::readVector3Int32");
      return Nimble::Vector3i(0, 0, 0);
    }

    int32_t marker = getRef<int32_t>();

    if(marker == VECTOR3I_MARKER) {
      return getRef<Nimble::Vector3i>();
    }
    else if(marker == VECTOR3F_MARKER) {
      Nimble::Vector3f r = getRef<Nimble::Vector3f>();

      return Nimble::Vector3i(r.x, r.y, r.z);
    }
    else {
      skipParameter(marker);
      if(ok)
        *ok = false;

      return Nimble::Vector3i(0, 0, 0);
    }

  }

  Nimble::Vector4i BinaryData::readVector4Int32(bool * ok)
  {
    if(ok)
      *ok = true;

    if(!available(12)) {
      if(ok) * ok = false;
      unavailable("BinaryData::readVector4Int32");
      return Nimble::Vector4i(0, 0, 0, 1);
    }

    int32_t marker = getRef<int32_t>();

    if(marker == VECTOR4I_MARKER) {
      return getRef<Nimble::Vector4i>();
    }
    else if(marker == VECTOR4F_MARKER) {
      Nimble::Vector4f r = getRef<Nimble::Vector4f>();

      return Nimble::Vector4i(r.x, r.y, r.z, r.w);
    }
    else {
      skipParameter(marker);
      if(ok)
        *ok = false;

      return Nimble::Vector4i(0, 0, 0, 1);
    }

  }


  Nimble::Vector4f BinaryData::readVector4Float32(bool * ok)
  {
    if(ok)
      *ok = true;

    if(!available(12)) {
      if(ok) * ok = false;
      unavailable("BinaryData::readVector4Float32");
      return Nimble::Vector4f(0, 0, 0, 1);
    }

    int32_t marker = getRef<int32_t>();

    if(marker == VECTOR4I_MARKER) {
      Nimble::Vector4i r = getRef<Nimble::Vector4i>();

      return Nimble::Vector4f(r.x, r.y, r.z, r.w);
    }
    else if(marker == VECTOR4F_MARKER) {
      return getRef<Nimble::Vector4f>();
    }
    else if(marker == STRING_MARKER) {
      BD_STR_TO_VEC(Nimble::Vector4f, 4, ok);
    }
    else {
      skipParameter(marker);
      if(ok)
        *ok = false;

      return Nimble::Vector4f(0, 0, 0, 1);
    }

  }

  int32_t BinaryData::peekMarker(bool * ok) const
  {
    if (!available(4)) {
      if (ok)
        *ok = false;
      return 0;
    }
    if (ok)
      *ok = true;
    return *reinterpret_cast<int32_t*>(&m_buf[m_current]);
  }

  bool BinaryData::write(Radiant::BinaryStream & stream) const
  {
    int32_t s = pos();
    if(stream.write(&s, 4) != 4)
      return false;

    return stream.write( & m_buf[0], s) == s;
  }

  bool BinaryData::read(Radiant::BinaryStream & stream, int timeoutMs)
  {
    uint32_t s = 0;
    m_current = 0;
    m_total = 0;

    const bool waitForData = timeoutMs < 0;
    double timeout = timeoutMs / 1000.0;

    Radiant::Timer timer;
    int n = 0;
    char * target = (char*) &s;

    if (timeoutMs > 0 && !stream.isPendingInput(timeoutMs * 1000))
      return false;

    for (;;) {
      n += stream.read(&target[n], 4 - n, waitForData);
      if (n >= 4 || !stream.isOpen()) break;

      if (waitForData) continue;

      double timeRemaining = timeout - timer.time();
      if (timeRemaining <= 0) break;

      if (!stream.isPendingInput(timeRemaining * 1000000))
        return false;
    }

    if(n < 4) {
      error("BinaryData::read # Could not read the 4 header bytes");
      return false;
    }

    if(m_size < s) {
      if(s > 500000000) { // Not more than 500 MB at once, please
        Radiant::error("BinaryData::read # Attempting extraordinary read (%d bytes)", s);
        return false;
      }
      ensure(s + 8);
    }

    memset(&m_buf[0], 0, m_size);

    n = 0;
    for (;;) {
      n += stream.read(& m_buf[n], s - n, waitForData);

      if (n >= int(s) || !stream.isOpen()) break;

      if (waitForData) continue;

      double timeRemaining = timeout - timer.time();
      if (timeRemaining <= 0) break;

      if (!stream.isPendingInput(timeRemaining * 1000000))
        return false;
    }

    if(n != (int) s) {
      error("BinaryData::read # buffer read failed (got %d != %d)", n, s);
      return false;
    }

    m_current = 0;
    m_total = s;

    return true;
  }

  void BinaryData::linkTo(void * data, int capacity)
  {
    if(!m_shared && m_buf)
      free(m_buf);

    m_buf = (char *) data;
    m_size = capacity;
    m_shared = true;
  }

  void BinaryData::ensure(size_t bytes)
  {
    size_t need = m_current + bytes;
    if(need > m_size) {
      if(m_shared)
        fatal("BinaryData::ensure # Sharing data, cannot ensure required space");

      m_size = (unsigned) (need + 128 + need / 16);
      m_buf = (char *) realloc(m_buf, m_size);
    }
    m_total = (unsigned) (m_current + bytes);
  }

  void BinaryData::clear()
  {
    rewind();

    memset(data(), 0, m_size);
  }

  bool BinaryData::saveToFile(const char * filename) const
  {
    FILE * f = fopen(filename, "wb");

    if(!f)
      return false;

    uint64_t s = m_current;
    fwrite(& s, 8, 1, f);
    bool ok = fwrite(m_buf, m_current, 1, f) == 1;

    fclose(f);

    return ok;
  }

  bool BinaryData::loadFromFile(const char * filename, size_t maxSize)
  {
    FILE * f = fopen(filename, "rb");

    if(!f)
      return false;

    uint64_t s = 0;
    bool ok = fread( & s, 8, 1, f) == 1;

    if(s > maxSize) {
      ok = false;
    }
    else if(ok) {
      clear();
      ensure(s);
      ok = fread(m_buf, s, 1, f) == 1;
      if(ok) {
        m_total = s;
        rewind();
      }
    }

    fclose(f);

    return ok;
  }

  void BinaryData::skipParameter(int marker)
  {
    if(marker == INT32_MARKER ||
       marker == FLOAT_MARKER)
      m_current += 4;
    else if(marker == INT64_MARKER ||
            marker == DOUBLE_MARKER ||
            marker == TS_MARKER ||
            marker == VECTOR2F_MARKER ||
            marker == VECTOR2I_MARKER)
      m_current += 8;
    else if(marker == STRING_MARKER) {
      const char * str = & m_buf[m_current];
      m_current += (unsigned) stringSpace(str);
    }
    else if(marker == BLOB_MARKER) {
      int n = getRef<int32_t>();
      m_current += n;
    }
  }

  size_t BinaryData::stringSpace(const char * str) const
  {
    size_t len = strlen(str) + 1;

    int rem = len & 0x3;

    int pad = rem ? 4 - rem : 0;
    return len + pad;
  }

  void BinaryData::unavailable(const char * func) const
  {
    Radiant::error("%s # Not enough data available (at %u/%u)",
                   func, m_current, m_total);
  }

}
