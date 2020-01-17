namespace Valuable
{
  template <typename... Args>
  struct Event<Args...>::Listener
  {
    Listener(int id_, Cb cb_, folly::Executor * executor_, Valuable::Node * receiver_)
      : id(id_)
      , cb(std::move(cb_))
      , executor(executor_)
      , receiver(receiver_)
      , hasReceiver(receiver_ != nullptr)
    {}

    int id;
    Cb cb;
    folly::Executor * executor;
    Valuable::WeakNodePtrT<Valuable::Node> receiver;
    bool hasReceiver;
    // Set to false if removeListener was called while the event was raised
    bool valid = true;
  };

  template <typename... Args>
  struct Event<Args...>::D
  {
    std::vector<Listener> listeners;
    std::vector<Listener> newListeners;
    int raising = 0;
    int nextId = 0;

    bool addedDuringraise:1;
    bool removedDuringRaise:1;
    bool removeCurrentListener:1;

    D()
      : addedDuringraise(false)
      , removedDuringRaise(false)
      , removeCurrentListener(false)
    {}
  };


  template <typename... Args>
  int Event<Args...>::addListener(Cb cb)
  {
    return addListener(nullptr, nullptr, std::move(cb));
  }

  template <typename... Args>
  int Event<Args...>::addListener(Valuable::Node * receiver, Cb cb)
  {
    return addListener(receiver, nullptr, std::move(cb));
  }

  template <typename... Args>
  int Event<Args...>::addListener(folly::Executor * executor, Cb cb)
  {
    return addListener(nullptr, executor, std::move(cb));
  }

  template <typename... Args>
  int Event<Args...>::addListener(Valuable::Node * receiver, folly::Executor * executor, Cb cb)
  {
    if (!m_d)
      m_d.reset(new D());
    int id = m_d->nextId++;
    if (m_d->raising) {
      m_d->newListeners.emplace_back(id, std::move(cb), executor, receiver);
      m_d->addedDuringraise = true;
    } else {
      m_d->listeners.emplace_back(id, std::move(cb), executor, receiver);
    }
    return id;
  }

  template <typename... Args>
  void Event<Args...>::removeCurrentListener()
  {
    if (!m_d)
      return;
    m_d->removeCurrentListener = true;
  }

  template <typename... Args>
  bool Event<Args...>::removeListener(int id)
  {
    if (!m_d)
      return false;

    D & d = *m_d;
    for (auto it = d.listeners.begin(); it != d.listeners.end(); ++it) {
      if (it->id == id) {
        if (d.raising) {
          it->valid = false;
          d.removedDuringRaise = true;
        } else {
          d.listeners.erase(it);
        }
        return true;
      }
    }
    return false;
  }

  template <typename... Args>
  void Event<Args...>::raise(Args... args)
  {
    if (!m_d)
      return;

    D & d = *m_d;
    ++d.raising;

    // Don't use iterators, since they could be invalidated if a listener adds a new listener
    for (size_t i = 0, size = d.listeners.size(); i < size;) {
      Listener& l = d.listeners[i];
      if (!l.valid) {
        ++i;
        continue;
      }

      d.removeCurrentListener = false;
      if (l.executor) {
        if (l.hasReceiver) {
          if (*l.receiver) {
            l.executor->add([receiver = l.receiver, cb = l.cb, args...] {
              if (*receiver)
                cb(args...);
            });
          } else {
            d.removeCurrentListener = true;
          }
        } else {
          l.executor->add(std::bind(l.cb, args...));
        }
      } else {
        if (l.hasReceiver) {
          if (*l.receiver)
            l.cb(args...);
          else
            d.removeCurrentListener = true;
        } else {
          l.cb(args...);
        }
      }

      if (d.removeCurrentListener) {
        d.listeners.erase(d.listeners.begin() + i);
        --size;
      } else {
        ++i;
      }
    }

    if (--d.raising == 0) {
      if (d.removedDuringRaise) {
        for (auto it = d.listeners.begin(); it != d.listeners.end();) {
          if (it->valid)
            ++it;
          else
            it = d.listeners.erase(it);
        }
        d.removedDuringRaise = false;
      }

      if (d.addedDuringraise) {
        d.listeners.insert(d.listeners.end(),
                           std::make_move_iterator(d.newListeners.begin()),
                           std::make_move_iterator(d.newListeners.end()));
        d.newListeners.clear();
        d.addedDuringraise = false;
      }
    }
  }
}
