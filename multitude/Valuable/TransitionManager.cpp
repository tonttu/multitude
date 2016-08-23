#include "TransitionManager.hpp"
#include "TransitionAnim.hpp"
#include "TransitionImpl.hpp"

#include <algorithm>
#include <queue>
#include <list>
#include <utility>

#include <cassert>

namespace Valuable
{
  static std::vector<TransitionManager*> s_managers;
  /// @todo Bucket size is chosen by Stetson-Harrison method. May be good idea to increase
  static const size_t s_bucketSize = 100;


  class TransitionBucket
  {
  public:
    TransitionBucket(size_t capacity)
      : m_transitions(capacity),
        m_sizeUsed(0)
    {}

    TransitionAnim* add(const TransitionAnim& anim);
    bool full() const;
    size_t capacity() const;
    size_t elements() const;
    void clear();

    void updateTransitions(float dt);
    /// Returns how many was removed
    size_t updateStructure(float dt);

    void merge(TransitionBucket& other);

  private:
    std::vector<TransitionAnim> m_transitions;
    size_t m_sizeUsed;
  };

  // ----------------------------------------------------------------------------

  bool TransitionBucket::full() const
  {
    return m_sizeUsed >= m_transitions.size();
  }

  void TransitionBucket::clear()
  {
    for(size_t i = 0; i < m_sizeUsed; ++i) {
      assert(!m_transitions[i].isNull());
      m_transitions[i].setNull();
    }
    m_sizeUsed = 0;
  }

  size_t TransitionBucket::elements() const
  {
    return m_sizeUsed;
  }

  size_t TransitionBucket::capacity() const
  {
    return m_transitions.size();
  }

  void TransitionBucket::merge(TransitionBucket &other)
  {
    assert(m_sizeUsed + other.m_sizeUsed < capacity());

    size_t j = m_sizeUsed;
    for(size_t i = 0; i < other.m_sizeUsed; ++i, ++j) {
      m_transitions[j] = other.m_transitions[i];
      m_transitions[j].updateTargetAttributePointer();
    }
    m_sizeUsed += other.m_sizeUsed;
  }

  size_t TransitionBucket::updateStructure(float dt)
  {
    std::queue<size_t> freePositions;

    for(size_t i = 0; i < m_sizeUsed; ++i) {
      TransitionAnim& anim = m_transitions[i];
      TransitionTypeData* tData = anim.typeData();

      assert(tData);
      assert(!anim.isNull());

      bool alive = true;
      if(tData->transitionUpdated) {
        tData->transitionUpdated = false;
        anim.updatePosition(dt);

        if(anim.normalizedPos() >= 1.f || tData->deleted) {
          alive = false;
          if (!tData->deleted)
            anim.remove();
          anim.setNull();
          freePositions.push(i);
        }
      }

      if(alive && !freePositions.empty()) {
        assert(freePositions.back() < i);
        size_t pos = freePositions.front();
        freePositions.pop();
        m_transitions[pos] = m_transitions[i];
        freePositions.push(i);
        m_transitions[pos].updateTargetAttributePointer();
      }
    }
    size_t removed = freePositions.size();
    m_sizeUsed -= removed;
    return removed;
  }

  void TransitionBucket::updateTransitions(float dt)
  {
    for(size_t i = 0; i < m_sizeUsed; ++i)
      m_transitions[i].update(dt);
  }

  TransitionAnim* TransitionBucket::add(const TransitionAnim &anim)
  {
    assert(!full());
    m_transitions[m_sizeUsed] = anim;
    return &m_transitions[m_sizeUsed++];
  }

  /// All animations are stored in the following structure. This is automatically cleaned
  /// when tranistions are finished.

  class TransitionContainer
  {
  public:

    void update(float dt);

    void clear();
    TransitionAnim* add(const TransitionAnim& anim);
    bool full() const;

    void updateTransitions(float dt);
    void updateStructure(float dt);

    size_t size() const;

  private:
    std::list<TransitionBucket> m_buckets;
    size_t m_capacity;
    size_t m_capacityUsed;
  };

  // ----------------------------------------------------------------------------

  void TransitionContainer::clear()
  {
    for(auto & bucket : m_buckets)
      bucket.clear();
    m_buckets.clear();
  }

  bool TransitionContainer::full() const
  {
    return m_capacity <= m_capacityUsed;
  }

  TransitionAnim* TransitionContainer::add(const TransitionAnim& anim)
  {
    if(full()) {
      m_buckets.emplace_back(s_bucketSize);
      m_capacity += s_bucketSize;
      ++m_capacityUsed;
      return m_buckets.back().add(anim);
    } else {
      for(auto & bucket : m_buckets) {
        if(!bucket.full()) {
          ++m_capacityUsed;
          return bucket.add(anim);
        }
      }
    }
    assert(false && "Should never reach this");
    return nullptr;
  }

  size_t TransitionContainer::size() const
  {
    return m_capacityUsed;
  }

  void TransitionContainer::updateTransitions(float dt)
  {
    for(auto & bucket : m_buckets) {
      bucket.updateTransitions(dt);
    }
  }

  void TransitionContainer::updateStructure(float dt)
  {
    /// Store buckets and their sizes. After first run, merge the smallest together
    typedef std::pair<size_t, std::list<TransitionBucket>::iterator> P;
    // note reverse comparison for minimum heap
    struct cmpp { bool operator()(const P& p1, const P& p2) const { return p2.first < p1.first;} };

    std::vector<P> tmp;

    for(auto it = m_buckets.begin(); it != m_buckets.end(); ) {
      size_t removed = it->updateStructure(dt);

      m_capacityUsed -= removed;
      size_t elems = it->elements();
      if(elems == 0) { // Just remove the whole bucket
        it = m_buckets.erase(it);
        m_capacity -= s_bucketSize;
      } else {
        tmp.emplace_back(std::make_pair(elems, it));
        ++it;
      }
    }

    cmpp comparison;
    std::priority_queue<P, std::vector<P>, cmpp> heap(comparison, std::move(tmp));

    while(heap.size() >= 2) {
      auto p1 = heap.top();
      heap.pop();
      auto p2 = heap.top();
      heap.pop();

      if(p1.first + p2.first < s_bucketSize) {
        p1.second->merge(*p2.second);
        m_buckets.erase(p2.second);
        p1.first += p2.first;
        heap.push(p1);
        m_capacity -= s_bucketSize;
      }
    }
  }

  // ----------------------------------------------------------------------------

  static TransitionContainer s_transitions;

  TransitionManager::TransitionManager()
  {
    s_managers.push_back(this);
  }

  TransitionManager::~TransitionManager()
  {
    s_managers.erase(std::find(s_managers.begin(), s_managers.end(), this));
    /// This one assumes that all transition managers are destroyed same time
    s_transitions.clear();
  }

  std::size_t TransitionManager::activeTransitions()
  {
    return s_transitions.size();
  }

  TransitionAnim* TransitionManager::createAnim(TransitionManager* manager)
  {
    return s_transitions.add(TransitionAnim(manager));
  }

  void TransitionManager::updateAll(float dt)
  {
    /// Copy to avoid side effects for the transition pipe
    TransitionContainer transitions = s_transitions;

    transitions.updateTransitions(dt);
    s_transitions.updateStructure(dt);
  }


} // namespace Valuable
