/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_FUTUREBOOL_HPP
#define RADIANT_FUTUREBOOL_HPP

#include "Task.hpp"

namespace Radiant
{

  /// Interface class for FutureBool backends.
  class FutureBoolI
  {
  public:
    virtual ~FutureBoolI() {}
    /// Are all Radiant::Tasks finished that are associated with this object
    /// If the job we are waiting for doesn't use tasks, return true
    virtual bool isReady() const = 0;
    /// Return next task that need to be executed to get the job done
    virtual Radiant::TaskPtr task() const = 0;
    /// Once all tasks are ready, this returns the actual boolean value we want.
    /// This is only called once and might block.
    virtual bool validate() = 0;
  };
  typedef std::unique_ptr<FutureBoolI> FutureBoolIPtr;

  /// Conjunction of two FutureBool -implementations.
  /// Enables combining of two FutureBools similar short circuited way
  /// as logical operators work in C++.
  class RADIANT_API FutureBoolConjunction : public FutureBoolI
  {
  public:
    /// Constructor of conjunction
    /// @param lhs Left side operand of 'and'
    /// @param rhs Right side operand of 'and'
    FutureBoolConjunction(FutureBoolIPtr lhs, FutureBoolIPtr rhs);

    /// Destructor
    virtual ~FutureBoolConjunction();

    /// Is ready if all of the tasks associated to left side operand are finished
    /// and it evaluates to false, or if all of the tasks are finished.
    /// @param Can boolean value represented by this class be evaluated immediately
    virtual bool isReady() const OVERRIDE;

    /// Return the next task associated with this value.
    /// First the tasks of left side operand are returned.
    /// @param Next task to execute
    virtual Radiant::TaskPtr task() const OVERRIDE;

    /// Once this object is ready this will return the actual boolean value we want.
    /// @return Final value of the expression represented this object
    virtual bool validate() OVERRIDE;

    /// Construct pointer by taking a conjunction of two FutureBool-objects
    /// @return Pointer to FutureBoolConjunction-object
    static FutureBoolIPtr conjunction(FutureBoolI *lhs, FutureBoolI *rhs);

  private:
    FutureBoolIPtr m_lhs;
    FutureBoolIPtr m_rhs;
  };


  /// This class provides implicit conversion to boolean type. The class is
  /// used to provide asynchronous return values from functions. By invoking
  /// a function that returns a FutureBool-instance, the invoker can choose
  /// if he wants to wait for the return value by evaluating the value of the
  /// FutureBool. In case the value is never evaluated the function is run
  /// asynchronously.
  ///
  /// This behaviour and usage pattern is very similar to std::future.
  /// See also @ref MultiWidgets::ImageWidget::load
  class FutureBool
  {
  public:
    /// Constructs a FutureBool object whose value is already known at this point.
    /// @param value Value to return when converting this object to boolean
    FutureBool(bool value)
      : m_cached(value)
      , m_cacheSet(true)
    {
    }

    /// Constructs a FutureBool object from FutureBoolIPtr
    /// @param future Class-specific implementation of FutureBoolI-interface.
    FutureBool(FutureBoolIPtr future)
      : m_future(std::move(future))
      , m_cached(false)
      , m_cacheSet(false)
    {}

    /// Move the given FutureBool object
    /// @param f FutureBool to move
    FutureBool(FutureBool && f)
      : m_future(std::move(f.m_future))
      , m_cached(f.m_cached)
      , m_cacheSet(f.m_cacheSet)
    {}

    /// Move the given FutureBool object
    /// @param f FutureBool to move
    FutureBool & operator=(FutureBool && f)
    {
      std::swap(m_future, f.m_future);
      m_cached = f.m_cached;
      m_cacheSet = f.m_cacheSet;
      return *this;
    }

    /// Implicit conversion to boolean. This function will block until the
    /// value of the future has been set.
    /// @return boolean value
    inline operator bool()
    {
      if (m_cacheSet)
        return m_cached;

      m_cached = run();
      m_cacheSet = true;
      return m_cached;
    }

  private:
    bool run() const
    {
      while (!m_future->isReady()) {
        auto task = m_future->task();
        if (!task)
          break;
        task->runNow(true);
      }

      return m_future->validate();
    }

  private:
    FutureBoolIPtr m_future;
    bool m_cached;
    bool m_cacheSet;
  };

} // namespace Radiant

#endif // RADIANT_FUTUREBOOL_HPP
