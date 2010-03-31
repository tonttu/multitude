/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#ifndef RADIANT_BINARY_DATA_HPP
#define RADIANT_BINARY_DATA_HPP

#include <Nimble/Vector2.hpp>
#include <Nimble/Vector4.hpp>

#include <Radiant/Export.hpp>

#include <stdint.h>
#include <vector>

namespace Radiant {
  class BinaryStream;
}

namespace Radiant {

  /// OSC-like binary data storage
  /** This class encapsulates control messages in a binary buffer. The
      data is stored in pretty much the same way as in Open Sound
      Control (OSC). This is done to ensure easy control data
      conversion between Radiant and OSC-compliant applications.

      \b Differences between Radiant and OSC:

      <UL>

      <LI>In ControlData the byte order is machine native byte
      order, for performance reasons.

      <LI> Time-stamp for is slightly different. In Radiant the the
      timestamps are 40+24 bits fixed point while in OSC they are
      32+32 bit fixed point.

      <LI> In Radiant, there is no address matching

      <LI> In Radiant, the type of each parameter is stored right
      before the parameter, to make writing/reading the stream easier.

      <LI> In Radiant strings are padded to 4-byte margin (like OSC),
      but the padding does not need to be zeros.

      </UL>

      OSC basic types are well known, so automatic conversion of all the
      differences should be easy enough.

      \b Wrting functions always put the type marker before the actual
      value.

      \b Reading functions set the optional bool argument "ok" to
      false if the operation fails. The boolean is never set to true,
      so you must do that in our own code.

  */

  class RADIANT_API BinaryData
  {
  public:
    BinaryData();
    BinaryData(const BinaryData & );
    ~BinaryData();

    /// Writes a 32-bit floating point number to the data buffer
    void writeFloat32(float v);
    /// Writes a 32-bit floating point number to the data buffer
    void writeFloat64(double v);
    /// Writes a 32-bit integer to the data buffer
    void writeInt32(int32_t v);
    /// Writes a 64-bit integer to the data buffer
    void writeInt64(int64_t v);
    /// Writes a 64-bit integer to the data buffer
    void writePointer(void * ptr) { writeInt64((int64_t) ptr); }
    /// Writes a 64-bit time-stamp to the data buffer
    /** The timestamp uses Radiant::TimeStamp internal structure (40+24
    bit fixed-point value).*/
    void writeTimeStamp(int64_t v);

    /// Write a null-terminated string to the buffer
    void writeString(const char *);
    void writeString(const std::string & str) { writeString(str.c_str()); }
    /** Writes a wide-string to the buffer. The string is internally
    stored as 32-bit integers, since that is the typical
    wchar_t.*/
    void writeWString(const std::wstring & str);

    /// Writes binary blob to the buffer.
    void writeBlob(const void * ptr, int n);

    /// Writes a 2D 32-bit floating point vector to the data buffer
    void writeVector2Float32(Nimble::Vector2f);
    /// Writes a 3D 32-bit floating point vector to the data buffer
    void writeVector3Float32(Nimble::Vector3f);

    /// Writes a 2D 32-bit integer vector to the data buffer
    void writeVector2Int32(Nimble::Vector2i);
    /// Writes a 3D 32-bit integer vector to the data buffer
    void writeVector3Int32(Nimble::Vector3i);

    /// Writes a 4D 32-bit integer vector to the data buffer
    void writeVector4Int32(const Nimble::Vector4i &);

    /// Writes a 4D 32-bit float vector to the data buffer
    void writeVector4Float32(const Nimble::Vector4f &);

    void append(const BinaryData & that);

    template <class T> inline T read(bool * ok = 0);

    /// Reads a 32-bit floating point number from the data buffer
    float readFloat32(bool * ok = 0);
    /// Reads a 64-bit floating point number from the data buffer
    double readFloat64(bool * ok = 0);
    /// Reads a 32-bit integer from the data buffer
    int32_t readInt32(bool * ok = 0);

    /// Reads a 64-bit integer from the data buffer
    int64_t readInt64(bool * ok = 0);
    /// Reads a 64-bit time-stamp from the data buffer
    int64_t readTimeStamp(bool * ok = 0);
    /// Read a null-terminated string from the buffer
    bool readString(char * str, size_t maxbytes);
    bool readString(std::string & str);
    /// Reads a wide string from the buffer
    bool readWString(std::wstring & str);
    /// Reads a blob of expected size
    bool readBlob(void * ptr, int n);

    /// Reads a 2D 32-bit floating point vector from the buffer
    Nimble::Vector2f readVector2Float32(bool * ok = 0);
    /// Reads a 2D 32-bit integer vector from the buffer
    Nimble::Vector2i readVector2Int32(bool * ok = 0);

    /// Reads a 3D 32-bit floating point vector from the buffer
    Nimble::Vector3f readVector3Float32(bool * ok = 0);
    /// Reads a 3D 32-bit integer vector from the buffer
    Nimble::Vector3i readVector3Int32(bool * ok = 0);

    /// Reads a 4D 32-bit integer vector from the buffer
    Nimble::Vector4i readVector4Int32(bool * ok = 0);
    /// Reads a 4D 32-bit float vector from the buffer
    Nimble::Vector4f readVector4Float32(bool * ok = 0);

    /// Tells the current position of the read/write pointer
    inline int pos() const { return m_current; }
    /// Sets the position of the read/write pointer
    inline void setPos(int index) { m_current = index; }
    /// Rewind the index pointer to the beginning
    inline void rewind() { m_current = 0; }

    inline int total() const { return m_total; }
    inline void setTotal(int bytes) { m_total = bytes; }

    bool write(Radiant::BinaryStream *);
    bool read(Radiant::BinaryStream *);

    inline char * data() { return & m_buf[0]; }
    inline const char * data() const { return & m_buf[0]; }

    void linkTo(void * data, int capacity);

    /// Ensure that at least required amount of bytes is available
    void ensure(size_t bytes);
    /// Rewind the buffer and fill it with zeroes
    void clear();

    inline BinaryData & operator = (const BinaryData & that)
    { rewind(); append(that); return * this;}

  private:

    template <class T>
    inline T * getPtr(int advance = sizeof(T))
    { T * tmp = (T*) & m_buf[m_current]; m_current += advance; return tmp; }
    template <class T>
    inline T & getRef()
    { T * tmp = (T*) & m_buf[m_current]; m_current += sizeof(T); return *tmp; }

    inline bool available(unsigned bytes)
    { return (m_current + bytes) <= m_total; }
    void skipParameter(int marker);
    size_t stringSpace(const char * str);

    void unavailable(const char * func);

    unsigned m_current;
    unsigned m_total;
    unsigned m_size;
    bool     m_shared;
    char    *m_buf;
  };


  template <> inline float BinaryData::read(bool * ok)
  { return readFloat32(ok); }

  template <> inline double BinaryData::read(bool * ok)
  { return readFloat64(ok); }

  template <> inline Nimble::Vector2f BinaryData::read(bool * ok)
  { return readVector2Float32(ok); }

  template <> inline Nimble::Vector3f BinaryData::read(bool * ok)
  { return readVector3Float32(ok); }

  template <> inline Nimble::Vector4f BinaryData::read(bool * ok)
  { return readVector4Float32(ok); }

  template <> inline Nimble::Vector2i BinaryData::read(bool * ok)
  { return readVector2Int32(ok); }

  template <> inline Nimble::Vector3i BinaryData::read(bool * ok)
  { return readVector3Int32(ok); }

  template <> inline Nimble::Vector4i BinaryData::read(bool * ok)
  { return readVector4Int32(ok); }

  template <> inline std::string BinaryData::read(bool * ok)
  {
      std::string tmp;
      bool good = readString(tmp);
      if(ok)
            *ok = good;
    return tmp;
  }

  template <> inline std::wstring BinaryData::read(bool * ok)
  {
      std::wstring tmp;
      bool good = readWString(tmp);
      if(ok)
            *ok = good;
    return tmp;
  }

}

#endif
