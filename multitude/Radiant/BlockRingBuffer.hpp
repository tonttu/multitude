#ifndef RADIANT_BLOCKRINGBUFFER_HPP
#define RADIANT_BLOCKRINGBUFFER_HPP

#include <vector>
#include <atomic>
#include <algorithm>
#include <stdexcept>

namespace Radiant
{
  /// Lock-free and thread-safe fixed-size ring buffer with one producer and one consumer.
  /// The API is optimized for storing bigger blocks (like float arrays)
  /// to the buffer, but it works with all elements that can be copied
  /// and default-constructed. Elements are not destroyed when they are consumed.
  template <typename T>
  class BlockRingBuffer
  {
  public:
    /// Helper class for reading continuous memory segments from the buffer
    /// The data can be used freely while this object is alive, it's actually
    /// consumed in the destructor.
    class Reader
    {
    private:
      Reader(BlockRingBuffer<T> & buffer, T* data, int size)
        : m_buffer(buffer)
        , m_data(data)
        , m_size(size)
      {}

    public:
      Reader(Reader && r)
        : m_buffer(r.m_buffer)
        , m_data(r.m_data)
        , m_size(r.m_size)
      {
        r.m_size = 0;
      }

      Reader(const Reader & r) = delete;

      Reader & operator=(Reader && r)
      {
        if (&m_buffer != &r.m_buffer)
          throw std::invalid_argument("BlockRingBufferReader buffers doesn't match");
        m_data = r.m_data;
        m_size = r.m_size;
        r.m_size = 0;
        return *this;
      }

      ~Reader()
      {
        if (m_size) m_buffer.consume(m_size);
      }

      T* data() const { return m_data; }
      int size() const { return m_size; }

    private:
      friend class BlockRingBuffer<T>;
      BlockRingBuffer<T> & m_buffer;
      T* m_data;
      int m_size;
    };

    class Writer
    {
    private:
      Writer(BlockRingBuffer<T> & buffer, T* data, int size)
        : m_buffer(buffer)
        , m_data(data)
        , m_size(size)
      {}

    public:
      Writer(Writer && r)
        : m_buffer(r.m_buffer)
        , m_data(r.m_data)
        , m_size(r.m_size)
      {
        r.m_size = 0;
      }

      Writer(const Writer & r) = delete;

      ~Writer()
      {
        if (m_size) m_buffer.produce(m_size);
      }

      T* data() const { return m_data; }
      int size() const { return m_size; }

    private:
      friend class BlockRingBuffer<T>;
      BlockRingBuffer<T> & m_buffer;
      T* m_data;
      int m_size;
    };

    /// Creates a new buffer with given capacity
    BlockRingBuffer(int capacity)
      : m_buffer(capacity)
    {}

    /// Copies the object, not thread-safe
    BlockRingBuffer(const BlockRingBuffer & o)
      : m_buffer(o.m_buffer)
      , m_reader(o.m_reader)
      , m_writer(o.m_writer)
      , m_size((int)o.m_size)
    {
    }

    /// Copies the object, not thread-safe
    BlockRingBuffer & operator=(const BlockRingBuffer & o)
    {
      m_buffer = o.m_buffer;
      m_reader = o.m_reader;
      m_writer = o.m_writer;
      m_size = (int)o.m_size;
      return *this;
    }

    /// Write up to count elements from data to the buffer
    /// @returns number of elements written, less than count if the buffer fills up
    int write(const T* input, int count)
    {
      const int capacity = m_buffer.size();
      count = std::min<int>(count, capacity - m_size);

      const int part1 = std::min(count, capacity - m_writer);
      const int part2 = count - part1;

      if (part1) std::copy_n(input, part1, m_buffer.data() + m_writer);
      if (part2) std::copy_n(input + part1, part2, m_buffer.data());

      m_writer = (m_writer + count) % capacity;
      m_size += count;

      return count;
    }

    Writer write(int count)
    {
      const int capacity = m_buffer.size();
      count = std::min(capacity - m_writer, std::min<int>(count, capacity - m_size));

      return Writer(*this, m_buffer.data() + m_writer, count);
    }

    /// Consumes max count elements from the buffer and writes them to output
    /// @returns number of elements consumed and written to output, less than
    ///          count if the buffer doesn't have enough data
    int read(T* output, int count)
    {
      const int capacity = m_buffer.size();

      count = std::min<int>(count, m_size);

      const int part1 = std::min(count, capacity - m_reader);
      const int part2 = count - part1;

      if (part1) std::copy_n(m_buffer.data() + m_reader, part1, output);
      if (part2) std::copy_n(m_buffer.data(), part2, output + part1);

      consume(count);

      return count;
    }

    /// Returns a reader object that has continuous memory segment up to count bytes,
    /// but it can be less if there isn't enough continuous data available
    Reader read(int count)
    {
      const int capacity = m_buffer.size();

      count = std::min(capacity - m_reader, std::min<int>(count, m_size));

      return Reader(*this, m_buffer.data() + m_reader, count);
    }

    /// Returns the number of elements in the buffer. Can be called from all threads.
    int size() const
    {
      return m_size;
    }

    /// Consumes exactly count elements from the buffer. Can be called only
    /// from the reader thread.
    /// @param count number of elements to consume, must not be bigger than size().
    void consume(int count)
    {
      m_reader = (m_reader + count) % m_buffer.size();
      m_size -= count;
    }

    /// Consumes exactly count elements from the buffer. Can be called only
    /// from the reader thread.
    /// @param count number of elements to consume, must not be bigger than
    ///              available buffer size().
    void produce(int count)
    {
      m_writer = (m_writer + count) % m_buffer.size();
      m_size += count;
    }

  private:
    std::vector<T> m_buffer;
    int m_reader = 0;
    int m_writer = 0;
    std::atomic<int> m_size{0};
  };
}

#endif // RADIANT_BLOCKRINGBUFFER_HPP
