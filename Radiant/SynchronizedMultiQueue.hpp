#ifndef RADIANT_SYNCHRONIZEDMULTIQUEUE_HPP
#define RADIANT_SYNCHRONIZEDMULTIQUEUE_HPP

#include <cstdint>
#include <cstddef>
#include <vector>
#include <atomic>
#include <cassert>

#include <Radiant/Mutex.hpp>
#include <Radiant/Condition.hpp>

namespace Radiant
{

  namespace BitUtils
  {
    /// first set bit index counting from LSB
    /// if no bits are set, returns -1
    RADIANT_API int firstSetBit(uint32_t bits);
  }

  /// Single consumer, multiple producers
  /// Each producer has a separate queue and
  /// producing to individual queues in not thread safe
  ///
  ///  - Maximum number of producers is 32
  ///  - The queue elements are reused
  ///  - The element type T needs to have a default constructor
  ///
  /// Main benefit in using this class compared to completely separate queues
  /// is that you can wait for any specified set of queues to become nonempty.
  template<typename T>
  class SynchronizedMultiQueue
  {
  public:
    /// Set of queues represented as a bitmask
    /// Bit i set <=> queue i is in the set
    typedef uint32_t QueueSet;

    /// @param producerCount Number of producers
    /// @param queueSize Queue size per producer
    ///
    /// Each producer has their own queue internally, so number of items
    /// allocated is producerCount*queueSize
    SynchronizedMultiQueue(size_t producerCount=1, size_t queueSize=32)
    {
      reset(producerCount, queueSize);
    }

    /// Not thread safe, invalidates existing data
    void reset(size_t producerCount, size_t queueSize)
    {
      m_nonEmpty = 0;

      m_writers = std::vector<std::atomic<int64_t>>(producerCount);
      m_readers = std::vector<std::atomic<int64_t>>(producerCount);

      m_queues.resize(producerCount);
      m_queueNotFull = std::vector<Radiant::Condition>(producerCount);

      m_queueSize = queueSize;
      for(auto & q: m_queues) {
        q.resize(queueSize);
      }
    }

    /// Clear the queues. Not thread safe.
    void clear()
    {
      reset(producerCount(), queueSize());
    }

    size_t producerCount() const
    {
      return m_queues.size();
    }

    /// @return capacity of individual queue
    size_t queueSize() const
    {
      return m_queueSize;
    }

    int64_t approxItemsQueued(std::size_t id) const
    {
      return m_writers[id] - m_readers[id];
    }

    ///
    /// @brief nextFillItem
    /// @param id
    /// @param timeoutMs
    /// @return pointer to the next item in queue id
    ///
    ///
    T * currentFillItem(int id, unsigned timeoutMs)
    {
      auto & q = m_queues[id];
      // Check that the next item is not still in use
      int lastItem = m_writers[id] - m_queueSize;
      if(m_readers[id] <= lastItem) {
        unsigned int timeLeft = timeoutMs;
        m_mutex.lock();
        while(timeLeft > 0 && m_readers[id] <= lastItem) {
          m_queueNotFull[id].wait2(m_mutex, timeLeft);
        }
        m_mutex.unlock();

        if(timeLeft > 0)
          return &q[m_writers[id] % m_queueSize];
        return nullptr;
      }

      return &q[m_writers[id] % m_queueSize];
    }

    ///
    /// @brief fillItemReady
    /// @param id
    ///
    void fillItemReady(int id)
    {
      m_mutex.lock();
      m_writers[id]++;
      m_nonEmpty |= 1 << id;
      m_mutex.unlock();

      m_queuesNotEmpty.wakeOne();
    }

    /// @brief Get the front item in any of the specified queues
    ///
    /// @param queues bit mask of the queues you want to enqueue from
    /// @param timeoutMs how long to wait if no items are available from the specified queues
    /// @param sourceId which queue the item is from
    ///
    /// @return pointer to the item in front of the queue or nullptr if timeout was hit
    ///
    /// Calling this function multiple times is not guaranteed to return the same item
    /// if the set of queues has more than one item.
    ///
    /// When you are done with the item, call popItem.
    T * peekItem(QueueSet queues, unsigned int timeoutMs, int * sourceId = 0)
    {
      auto timeLeft = timeoutMs;
      QueueSet nonEmptyInteresting = m_nonEmpty & queues;

      if(nonEmptyInteresting == 0) {
        m_mutex.lock();
        nonEmptyInteresting = m_nonEmpty & queues;

        while(timeLeft > 0 && nonEmptyInteresting == 0) {
          m_queuesNotEmpty.wait2(m_mutex, timeLeft);
          nonEmptyInteresting = m_nonEmpty & queues;
        }
        m_mutex.unlock();
      }

      int id = BitUtils::firstSetBit(nonEmptyInteresting);
      if(id == -1)
        return nullptr;

      if(sourceId)
        *sourceId = id;

      return &m_queues[id][m_readers[id] % m_queueSize];
    }

    /// Release the item in front given queue
    void popItem(int id)
    {
      m_mutex.lock();
      m_readers[id]++;

      assert(m_readers[id] <= m_writers[id]);
      // Check if queue is empty
      if(m_readers[id] == m_writers[id]) {
        m_nonEmpty &= ~(1 << id);
      }

      m_mutex.unlock();
      m_queueNotFull[id].wakeOne();
    }

  private:
    typedef std::vector<T> Queue;
    std::vector<Queue> m_queues;
    // m_writers[i] = next item to be written to queue i
    std::vector<std::atomic<int64_t>> m_writers;
    // m_readers[i] = last read item from queue i
    std::vector<std::atomic<int64_t>> m_readers;

    size_t m_queueSize;

    // Guard for m_nonEmpty
    Radiant::Mutex m_mutex;
    std::atomic<uint32_t> m_nonEmpty;
    Radiant::Condition m_queuesNotEmpty;
    std::vector<Radiant::Condition> m_queueNotFull;
  };

} // namespace Radiant

#endif // RADIANT_SYNCHRONIZEDMULTIQUEUE_HPP
