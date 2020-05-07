#ifndef VALUABLE_ATTRIBUTEVECTORCONTAINER_HPP
#define VALUABLE_ATTRIBUTEVECTORCONTAINER_HPP

#include "Attribute.hpp"
#include "AttributeEvent.hpp"

namespace Valuable
{
  /// Non-template base class for all AttributeContainers, like AttributeVectorContainer
  class AttributeContainerBase : public Attribute
  {
  public:
    AttributeContainerBase(Valuable::Node * parent = nullptr,
                           const QByteArray & name = QByteArray())
      : Attribute(parent, name)
    {}

    AttributeEventListenerList & eventListenerList() { return m_eventListeners; }
    const AttributeEventListenerList & eventListenerList() const { return m_eventListeners; }

  protected:
    AttributeEventListenerList m_eventListeners;
  };

  /// An attribute vector of immutable objects.
  ///
  /// Changes to the vector can be monitored by adding a new listener with addListener
  ///
  /// @tparam T needs to be serializable class, i.e.
  ///           Valuable::Serializer::serialize<T> and deserialize must work.
  ///
  /// @sa MutableAttributeVectorContainer
  template <typename T, typename Allocator = typename std::vector<T>::allocator_type>
  class AttributeVectorContainer : public AttributeContainerBase
  {
  public:
    // types:
    typedef T value_type;
    typedef value_type & reference;
    typedef const value_type & const_reference;
    typedef std::vector<T> container;
    typedef typename container::const_iterator const_iterator;
    typedef typename container::size_type size_type;
    typedef typename container::difference_type difference_type;
    typedef Allocator allocator_type;
    typedef typename std::allocator_traits<allocator_type>::pointer pointer;
    typedef typename std::allocator_traits<allocator_type>::const_pointer const_pointer;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    // Constructors, destructor and copying

    AttributeVectorContainer(Valuable::Node * parent = nullptr,
                             const QByteArray & name = QByteArray(), const Allocator & allocator = Allocator())
      : AttributeContainerBase(parent, name)
      , m_vector(allocator)
    {}

    ~AttributeVectorContainer()
    {
      m_eventListeners.send(AttributeEvent::Type::DELETED);
    }

    // Copy vector contents, doesn't copy listeners or anything Attribute-specific
    AttributeVectorContainer & operator=(const AttributeVectorContainer & container)
    {
      const bool wasEmpty = empty();
      m_vector = container.m_vector;
      if (!wasEmpty || !empty()) {
        m_eventListeners.send(AttributeEvent::Type::CHANGED);
      }
      return *this;
    }

    AttributeVectorContainer & operator=(const container & vector)
    {
      const bool wasEmpty = empty();
      m_vector = vector;
      if (!wasEmpty || !empty())
        m_eventListeners.send(AttributeEvent::Type::CHANGED);
      return *this;
    }

    AttributeVectorContainer & operator=(container && vector)
    {
      const bool wasEmpty = empty();
      m_vector = std::move(vector);
      if (!wasEmpty || !empty())
        m_eventListeners.send(AttributeEvent::Type::CHANGED);
      return *this;
    }

    // Move vector contents, doesn't move listeners or anything Attribute-specific
    AttributeVectorContainer & operator=(AttributeVectorContainer && container)
    {
      if (container.empty()) {
        clear();
      } else {
        m_vector = std::move(container.m_vector);
        container.m_vector.clear();
        container.m_eventListeners.send(AttributeEvent::Type::CHANGED);
        m_eventListeners.send(AttributeEvent::Type::CHANGED);
      }
      return *this;
    }

    AttributeVectorContainer & operator=(std::initializer_list<T> list)
    {
      const bool wasEmpty = empty();
      m_vector = list;
      if (!wasEmpty || !empty()) {
        m_eventListeners.send(AttributeEvent::Type::CHANGED);
      }
      return *this;
    }

    template <class InputIterator>
    void assign(InputIterator first, InputIterator last)
    {
      const bool wasEmpty = empty();
      m_vector.assign(first, last);
      if (!wasEmpty || !empty()) {
        m_eventListeners.send(AttributeEvent::Type::CHANGED);
      }
    }

    void assign(size_type n, const T & t)
    {
      const bool wasEmpty = empty();
      m_vector.assign(n, t);
      if (!wasEmpty || !empty()) {
        m_eventListeners.send(AttributeEvent::Type::CHANGED);
      }
    }

    void assign(std::initializer_list<T> list)
    {
      const bool wasEmpty = empty();
      m_vector.assign(list);
      if (!wasEmpty || !empty()) {
        m_eventListeners.send(AttributeEvent::Type::CHANGED);
      }
    }

    allocator_type get_allocator() const noexcept { return m_vector.get_allocator(); }


    // Const iterators:

    const_iterator begin() const noexcept { return m_vector.begin(); }
    const_iterator end() const noexcept { return m_vector.end(); }

    const_reverse_iterator rbegin() const noexcept { return m_vector.rbegin(); }
    const_reverse_iterator rend() const noexcept { return m_vector.rend(); }

    const_iterator cbegin() noexcept { return m_vector.cbegin(); }
    const_iterator cend() noexcept { return m_vector.cend(); }
    const_reverse_iterator crbegin() const noexcept { return m_vector.crbegin(); }
    const_reverse_iterator crend() const noexcept { return m_vector.crend(); }


    // Capacity:

    size_type size() const noexcept { return m_vector.size(); }
    size_type max_size() const noexcept { return m_vector.max_size(); }
    size_type capacity() const noexcept { return m_vector.capacity(); }
    bool empty() const noexcept { return m_vector.empty(); }
    void reserve(size_type n) { m_vector.reserve(n); }
    void shrink_to_fit() { m_vector.shrink_to_fit(); }


    // Const element access:

    const_reference operator[](size_type n) const { return m_vector[n]; }
    const_reference at(size_type n) const { return m_vector.at(n); }
    const_reference front() const { return m_vector.front(); }
    const_reference back() const { return m_vector.back(); }


    // Const data access:

    const T* data() const noexcept { return m_vector.data(); }
    const container & operator *() const { return m_vector; }
    const container & value() const { return m_vector; }


    // Modifiers:

    template <class... Args>
    void emplace_back(Args&&... args)
    {
      const auto idx = m_vector.size();
      m_vector.emplace_back(std::forward<Args>(args)...);
      m_eventListeners.send(AttributeEvent::Type::ELEMENT_INSERTED, idx);
    }

    template <typename Y>
    void push_back(Y && y)
    {
      const auto idx = m_vector.size();
      m_vector.push_back(std::forward<Y>(y));
      m_eventListeners.send(AttributeEvent::Type::ELEMENT_INSERTED, idx);
    }

    void pop_back()
    {
      if (m_vector.empty()) return;
      m_vector.pop_back();
      m_eventListeners.send(AttributeEvent::Type::ELEMENT_ERASED, m_vector.size());
    }

    template <class... Args>
    const_iterator emplace(const_iterator position, Args&&... args)
    {
      // libstdc++-4.8 doesn't have modifier functions that take
      // const_iterator like specified in C++11. Work around it.
      const auto index = position - begin();
      const_iterator it = m_vector.emplace(m_vector.begin() + index, std::forward<Args>(args)...);
      m_eventListeners.send(AttributeEvent::Type::ELEMENT_INSERTED, index);
      return it;
    }

    template <typename Y>
    const_iterator insert(const_iterator position, Y && y)
    {
      const auto index = position - begin();
      auto it = m_vector.insert(m_vector.begin() + index, std::forward<Y>(y));
      m_eventListeners.send(AttributeEvent::Type::ELEMENT_INSERTED, index);
      return it;
    }

    // std::vector::erase returns iterator, not const_iterator
    const_iterator erase(const_iterator position)
    {
      const auto index = position - begin();
      auto it = m_vector.erase(m_vector.begin() + index);
      m_eventListeners.send(AttributeEvent::Type::ELEMENT_ERASED, index);
      return it;
    }

    void swap(AttributeVectorContainer & other)
    {
      if (empty() && other.empty()) {
        return;
      }
      m_vector.swap(other.m_vector);
      other.m_eventListeners.send(AttributeEvent::Type::CHANGED);
      m_eventListeners.send(AttributeEvent::Type::CHANGED);
    }

    void swap(container & other)
    {
      if (empty() && other.empty()) {
        return;
      }
      m_vector.swap(other);
      m_eventListeners.send(AttributeEvent::Type::CHANGED);
    }

    void clear() noexcept
    {
      if (empty()) {
        return;
      }
      m_vector.clear();
      m_eventListeners.send(AttributeEvent::Type::CHANGED);
    }


    // Element write-access:

    template <typename Y>
    void set(const_iterator position, Y && y)
    {
      set(position - begin(), std::forward<Y>(y));
    }

    template <typename Y>
    void set(std::size_t index, Y && y)
    {
      T & dest = m_vector[index];
      if (dest == y) return;

      dest = std::forward<Y>(y);
      m_eventListeners.send(AttributeEvent::Type::ELEMENT_CHANGED, index);
    }


    // Listeners:

    /// @copydoc EventListenerList::addListener
    template <class... Args>
    AttributeEventListenerList::ListenerId addListener(Args&&... args)
    {
      return m_eventListeners.addListener(std::forward<Args>(args)...);
    }

    /// @copydoc EventListenerList::removeListener
    bool removeListener(AttributeEventListenerList::ListenerId listener)
    {
      return m_eventListeners.removeListener(listener);
    }


    // Serialization:

    void setClearOnDeserialize(bool v)
    {
      m_clearOnDeserialize = v;
    }

    bool clearOnDeserialize() const
    {
      return m_clearOnDeserialize;
    }

    virtual ArchiveElement serialize(Archive & archive) const override
    {
      QString elementName = name().isEmpty() ? "AttributeVectorContainer" : name();
      ArchiveElement elem = archive.createElement(elementName);

      for (auto & value: m_vector) {
        auto e = Serializer::serialize<T>(archive, value);
        if (!e.isNull()) {
          elem.add(e);
        }
      }

      return elem;
    }

    virtual bool deserialize(const ArchiveElement & element) override
    {
      bool wasEmpty = empty();

      if (m_clearOnDeserialize) {
        m_vector.clear();
      }

      for (ArchiveElement::Iterator it = element.children(); it; ++it) {
        /// @todo should detect deserialization errors
        m_vector.emplace_back(Serializer::deserialize<T>(*it));
      }

      if (!wasEmpty || !empty()) {
        m_eventListeners.send(AttributeEvent::Type::CHANGED);
      }

      return true;
    }

    virtual QByteArray type() const override
    {
      return "vector:" + Radiant::StringUtils::type<T>();
    }

    virtual QString asString(bool * const ok, Layer) const override
    {
      if (ok)
        *ok = true;
      QString out;
      for (size_t i = 0; i < m_vector.size(); ++i) {
        if (i != 0)
          out += ", ";
        out += Radiant::StringUtils::toString(m_vector[i]);
      }
      return out;
    }

  protected:
    container m_vector;

  private:
    bool m_clearOnDeserialize = true;
  };

  /// Attribute vector of mutable elements. Unlike in AttributeVectorContainer,
  /// in this version it's possible to have non-const references to objects and
  /// modify them without triggering change events.
  template <typename T, typename Allocator = typename std::vector<T>::allocator_type>
  class MutableAttributeVectorContainer : public AttributeVectorContainer<T, Allocator>
  {
  public:
    typedef AttributeVectorContainer<T, Allocator> Base;
    typedef typename Base::reference reference;
    typedef typename Base::size_type size_type;
    typedef typename Base::container::iterator iterator;
    typedef typename Base::const_iterator const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;


    // Iterators:

    using Base::begin;
    using Base::end;
    using Base::rbegin;
    using Base::rend;

    iterator begin() noexcept { return Base::m_vector.begin(); }
    iterator end() noexcept { return Base::m_vector.end(); }

    reverse_iterator rbegin() noexcept { return Base::m_vector.rbegin(); }
    reverse_iterator rend() noexcept { return Base::m_vector.rend(); }


    // Element access:

    using Base::operator[];
    using Base::at;
    using Base::front;
    using Base::back;

    reference operator[](size_type n) { return Base::m_vector[n]; }
    reference at(size_type n) { return Base::m_vector.at(n); }
    reference front() { return Base::m_vector.front(); }
    reference back() { return Base::m_vector.back(); }

    // Base version returns const_iterator, this version returns iterator like std::vector does
    iterator erase(const_iterator position)
    {
      const auto index = position - Base::cbegin();
      auto it = Base::m_vector.erase(Base::m_vector.begin() + index);
      Base::m_eventListeners.send(AttributeEvent::Type::ELEMENT_ERASED, index);
      return it;
    }


    // Data access:

    using Base::data;

    T * data() noexcept { return Base::m_vector.data(); }
  };


} // namespace Valuable

#endif // VALUABLE_ATTRIBUTEVECTORCONTAINER_HPP
