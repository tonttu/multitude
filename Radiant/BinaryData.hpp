/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_BINARY_DATA_HPP
#define RADIANT_BINARY_DATA_HPP

#include <Nimble/Vector2.hpp>
#include <Nimble/Vector4.hpp>
#include <Nimble/Frame4.hpp>

#include "Export.hpp"
#include "TimeStamp.hpp"
#include "Color.hpp"

#include <cstdint>
#include <vector>

#include <QString>

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

      \b Reading functions set the optional bool argument "ok" to true
      if the operation is successful and to false if it fails.

  */

  class RADIANT_API BinaryData
  {
  public:
    static const int32_t FLOAT_MARKER;
    static const int32_t DOUBLE_MARKER;
    static const int32_t VECTOR2F_MARKER;
    static const int32_t VECTOR2I_MARKER;
    static const int32_t VECTOR3F_MARKER;
    static const int32_t VECTOR3I_MARKER;
    static const int32_t VECTOR4F_MARKER;
    static const int32_t VECTOR4I_MARKER;
    static const int32_t INT32_MARKER;
    static const int32_t INT64_MARKER;
    static const int32_t TS_MARKER;
    static const int32_t STRING_MARKER;
    static const int32_t BLOB_MARKER;

  public:
    /// Constructs empty data container
    BinaryData();
    /// Copy constructor
    BinaryData(const BinaryData & );
    /// Destructor
    ~BinaryData();

    /// Writes a 32-bit floating point number to the data buffer
    /// @param v Value to write
    void writeFloat32(float v);
    /// Writes a 32-bit floating point number to the data buffer
    /// @param v Value to write
    void writeFloat64(double v);
    /// Writes a 32-bit integer to the data buffer
    /// @param v Value to write
    void writeInt32(int32_t v);
    /// Writes a 64-bit integer to the data buffer
    /// @param v Value to write
    void writeInt64(int64_t v);
    /// Writes a 64-bit integer to the data buffer
    /// @param ptr Value to write
    void writePointer(void * ptr) { writeInt64((int64_t) ptr); }
    /// Write a 64-bit time-stamp to the data buffer
    /// The timestamp uses Radiant::TimeStamp internal structure (40+24
    /// bit fixed-point value).
    /// @param v time-stamp to write
    void writeTimeStamp(TimeStamp v);

    /// Write a null-terminated string to the buffer
    /// @param str String to write
    void writeString(const char *str);
    /// Write a string to the buffer
    /// @param str string to write
    void writeString(const QString & str) { writeString(str.toUtf8().data()); }
    /// Write a string to the buffer
    /// @param str string to write
    void writeString(const QByteArray & str) { writeString(str.data()); }

    /// Writes binary blob to the buffer.
    /// @param ptr Pointer to blob
    /// @param n Bytes to write
    void writeBlob(const void * ptr, int n);

    /// Writes a 2D 32-bit floating point vector to the data buffer
    /// @param v Value to write
    void writeVector2Float32(Nimble::Vector2f v);
    /// Writes a 3D 32-bit floating point vector to the data buffer
    /// @param v Value to write
    void writeVector3Float32(Nimble::Vector3f v);

    /// Writes a 2D 32-bit integer vector to the data buffer
    /// @param v Value to write
    void writeVector2Int32(Nimble::Vector2i v);
    /// Writes a 3D 32-bit integer vector to the data buffer
    /// @param v Value to write
    void writeVector3Int32(Nimble::Vector3i v);

    /// Writes a 4D 32-bit integer vector to the data buffer
    /// @param v Value to write
    void writeVector4Int32(const Nimble::Vector4i &v);

    /// Writes a 4D 32-bit float vector to the data buffer
    /// @param v Value to write
    void writeVector4Float32(const Nimble::Vector4f &v);

    /// Writes a 32-bit floating point number to the data buffer
    /// @param v Value to write
    inline void write(float v) { writeFloat32(v); }
    /// Writes a 32-bit floating point number to the data buffer
    /// @param v Value to write
    inline void write(double v) { writeFloat64(v); }
    /// Writes a bool to the data buffer
    /// @param v Value to write
    inline void write(bool v) { writeInt32(v); }
    /// Writes a 32-bit integer to the data buffer
    /// @param v Value to write
    inline void write(int32_t v) { writeInt32(v); }
    /// Writes a 64-bit integer to the data buffer
    /// @param v Value to write
    inline void write(int64_t v) { writeInt64(v); }
    /// Write a 64-bit time-stamp to the data buffer
    /// The timestamp uses Radiant::TimeStamp internal structure (40+24
    /// bit fixed-point value).
    /// @param ts time-stamp to write
    inline void write(const Radiant::TimeStamp & ts) { writeTimeStamp(ts); }

    /// Write a null-terminated string to the buffer
    /// @param str String to write
    inline void write(const char * str) { writeString(str); }
    /// Write a string to the buffer
    /// @param str string to write
    inline void write(const QString & str) { writeString(str); }
    /// Write a string to the buffer
    /// @param str string to write
    inline void write(const QByteArray & str) { writeString(str.data()); }
    /// Writes a 2D 32-bit floating point vector to the data buffer
    /// @param v Value to write
    inline void write(Nimble::Vector2f v) { writeVector2Float32(v); }
    /// Writes a 3D 32-bit floating point vector to the data buffer
    /// @param v Value to write
    inline void write(Nimble::Vector3f v) { writeVector3Float32(v); }

    /// Writes a 2D 32-bit integer vector to the data buffer
    /// @param v Value to write
    inline void write(Nimble::Vector2i v) { writeVector2Int32(v); }
    /// Writes a 3D 32-bit integer vector to the data buffer
    /// @param v Value to write
    inline void write(Nimble::Vector3i v) { writeVector3Int32(v); }

    /// Writes a 4D 32-bit integer vector to the data buffer
    /// @param v Value to write
    inline void write(const Nimble::Vector4i & v) { writeVector4Int32(v); }

    /// Writes a 4D 32-bit float vector to the data buffer
    /// @param v Value to write
    inline void write(const Nimble::Vector4f & v) { writeVector4Float32(v); }

    inline void write(const Radiant::Color & c) { write(c.toVector()); }
    inline void write(const Radiant::ColorPMA & c) { write(c.toVector()); }

    /// Appends another BinaryData object to this
    /// @param that BinaryData to append
    void append(const BinaryData & that);

    /// Read a value from the data
    /// @param[out] ok If read was succesful true is writtten to this, otherwise false.
    /// @tparam T Type of value to read
    template <class T> inline T read(bool * ok = 0);

    /// Read a value from the buffer
    /// Tries to read the matching value type from the buffer. If successful
    /// the value is returned and the buffer read position is incremented. If the
    /// read fails, zero is returned and the optional ok flag is set to false.
    /// @param[out] ok check if the read was successful
    /// @return requested value or zero on failure
    float readFloat32(bool * ok = 0);
    /// @copydoc readFloat32
    double readFloat64(bool * ok = 0);
    /// @copydoc readFloat32
    int32_t readInt32(bool * ok = 0);
    /// @copydoc readFloat32
    int64_t readInt64(bool * ok = 0);
    /// @copydoc readFloat32
    TimeStamp readTimeStamp(bool * ok = 0);

    /// Read a null-terminated string from the buffer
    /// @param[out] str string buffer to write to
    /// @param maxbytes maximum number of bytes to read
    /// @return true on success
    bool readString(char * str, size_t maxbytes);
    /// Read a string from the buffer
    /// @param[out] str string to write to
    /// @return true on success
    bool readString(QString & str);
    /// Read a string from the buffer
    /// @param[out] str string to write to
    /// @return true on success
    bool readString(QByteArray & str);
    /// Reads a blob of expected size
    /// @param[out] ptr buffer to write to
    /// @param n bytes to read
    /// @return true on success
    bool readBlob(void * ptr, int n);
    /// Reads a blob
    /// @param[out] buf buffer to write to. The buffer will be resized to fit the data
    /// @return true on success
    bool readBlob(std::vector<uint8_t> & buf);
    /// Reads a blob without copying the data
    /// @param ptr will be assigned to be the beginning of the blob data
    /// @returns size of blob, or -1 in case of error
    int readBlobPtr(void *& ptr);

    /// Reads a 2D 32-bit floating point vector from the buffer
    /// @param[out] ok check if the read was successful
    Nimble::Vector2f readVector2Float32(bool * ok = 0);
    /// Reads a 2D 32-bit integer vector from the buffer
    /// @param[out] ok check if the read was successful
    Nimble::Vector2i readVector2Int32(bool * ok = 0);

    /// Reads a 3D 32-bit floating point vector from the buffer
    /// @param[out] ok check if the read was successful
    Nimble::Vector3f readVector3Float32(bool * ok = 0);
    /// Reads a 3D 32-bit integer vector from the buffer
    /// @param[out] ok check if the read was successful
    Nimble::Vector3i readVector3Int32(bool * ok = 0);

    /// Reads a 4D 32-bit integer vector from the buffer
    /// @param[out] ok check if the read was successful
    Nimble::Vector4i readVector4Int32(bool * ok = 0);
    /// Reads a 4D 32-bit float vector from the buffer
    /// @param[out] ok check if the read was successful
    Nimble::Vector4f readVector4Float32(bool * ok = 0);

    /// Extracts the next marker code from the stream
    /// @param[out] ok check if the read was successful
    int32_t peekMarker(bool * ok = 0) const;

    /// Tells the current position of the read/write pointer
    /// @return Position of read write pointer in bytes from the begininning.
    inline int pos() const { return m_current; }
    /// Sets the position of the read/write pointer
    /// @param index Bytes from the beginning
    inline void setPos(int index) { m_current = index; }
    /// Rewind the index pointer to the beginning
    inline void rewind() { m_current = 0; }

    /// Returns the total number of bytes used by this buffer
    /// @return Total bytes used
    inline int total() const { return m_total; }
    /// Sets the total number of bytes used by this buffer
    /// @param bytes Use at most this many bytes
    inline void setTotal(unsigned bytes) { if(bytes > m_size) ensure(bytes - m_size); m_total = bytes; }

    /// Writes the buffer into a stream
    /// @param stream Stream to write
    /// @return True if the whole data was written to the stream. false otherwise
    bool write(Radiant::BinaryStream &stream) const;
    /// Reads the buffer from a stream
    /// @param stream Stream to read from
    /// @param timeoutMs How many milliseconds the data is waited for. -1 if waiting indefinitively.
    /// @return False if the data could not completely read, true otherwise
    bool read(Radiant::BinaryStream &stream, int timeoutMs=-1);

    /// Returns a pointer to the buffer
    /// @return Pointer to the raw data used by BinaryData
    inline char * data() { return & m_buf[0]; }
    /// Returns a pointer to the buffer
    /// @return Pointer to the raw data used by BinaryData
    inline const char * data() const { return & m_buf[0]; }

    /// Makes the buffer point to existing memory.
    /// The shared memory will not be freed when the BinaryData object is destroyed.
    /// @param data Pointer to data that is being shared
    /// @param capacity Size of the BinaryData
    void linkTo(void * data, int capacity);

    /// Ensure that at least required amount of bytes is available
    /// @param bytes
    void ensure(size_t bytes);
    /// Rewind the buffer and fill it with zeroes
    void clear();

    /// Copy a buffer object
    /// @param that Buffer to assign
    /// @return Reference to this
    inline BinaryData & operator = (const BinaryData & that)
    { rewind(); append(that); m_current = that.m_current; return * this;}
    /// Saves this buffer to the given file
    /// @param filename Name of the target file
    /// @return True if write was succesful, false otherwise
    bool saveToFile(const char * filename) const;
    /// Reads data from a file
    /// @param filename Name of the file where data is loaded
    /// @param maxSize Maximum size of the read in bytes
    /// @return Did we manage to read the whole file succesfully?
    bool loadFromFile(const char * filename, size_t maxSize = 10000000);

  private:

    template <class T>
    inline T * getPtr(size_t advance = sizeof(T))
    { T * tmp = (T*) & m_buf[m_current]; m_current += (unsigned) advance; return tmp; }
    template <class T>
    inline T & getRef()
    { T * tmp = (T*) & m_buf[m_current]; m_current += sizeof(T); return *tmp; }

    inline bool available(unsigned bytes) const
    { return (m_current + bytes) <= m_total; }
    void skipParameter(int marker);
    size_t stringSpace(const char * str) const;

    void unavailable(const char * func) const;

    unsigned m_current;
    // number of bytes used in the buffer
    unsigned m_total;
    // number of bytes allocated, the capacity of m_buf
    unsigned m_size;
    bool     m_shared;
    char    *m_buf;
  };

  template <> inline float BinaryData::read(bool * ok)            { return readFloat32(ok); }
  template <> inline double BinaryData::read(bool * ok)           { return readFloat64(ok); }
  template <> inline int BinaryData::read(bool * ok)              { return readInt32(ok); }
  template <> inline unsigned int BinaryData::read(bool * ok)     { return readInt32(ok); }
  template <> inline bool BinaryData::read(bool * ok)             { return readInt32(ok) != 0; }
  template <> inline int64_t BinaryData::read(bool * ok)          { return readInt64(ok); }
  template <> inline TimeStamp BinaryData::read(bool * ok)        { return readTimeStamp(ok); }
  template <> inline uint64_t BinaryData::read(bool * ok)         { return readInt64(ok); }
  template <> inline Nimble::Vector2f BinaryData::read(bool * ok) { return readVector2Float32(ok); }
  template <> inline Nimble::Vector3f BinaryData::read(bool * ok) { return readVector3Float32(ok); }
  template <> inline Nimble::Vector4f BinaryData::read(bool * ok) { return readVector4Float32(ok); }
  template <> inline Nimble::Frame4f BinaryData::read(bool * ok)  { return readVector4Float32(ok); }
  template <> inline Nimble::Vector2i BinaryData::read(bool * ok) { return readVector2Int32(ok); }
  template <> inline Nimble::Vector3i BinaryData::read(bool * ok) { return readVector3Int32(ok); }
  template <> inline Nimble::Vector4i BinaryData::read(bool * ok) { return readVector4Int32(ok); }
  template <> inline Color BinaryData::read(bool * ok)            { return readVector4Float32(ok); }

  template <> inline QString BinaryData::read(bool * ok)
  {
    QString tmp;
    bool good = readString(tmp);
    if(ok)
      *ok = good;
    return tmp;
  }

  template <> inline QByteArray BinaryData::read(bool * ok)
  {
    QByteArray tmp;
    bool good = readString(tmp);
    if(ok)
      *ok = good;
    return tmp;
  }
}

#endif
