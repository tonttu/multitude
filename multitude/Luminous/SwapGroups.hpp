#ifndef LUMINOUS_SWAPGROUPS_HPP
#define LUMINOUS_SWAPGROUPS_HPP

#include "Luminous.hpp"

namespace Luminous
{

  /// This class provides the capability to synchronize the buffer swaps of
  /// group of OpenGL windows. Windows belonging to the same swap group will
  /// have their buffer swaps take place concurrently.
  /// Swap barriers can be used to synchronize buffer swaps of different swap
  /// groups, which may reside on distributed systems on a network.
  class LUMINOUS_API SwapGroups
  {
  public:
    /// @param screen X screen number. Ignored on Windows.
    SwapGroups(int screen);
    ~SwapGroups();

    /// Check if the GLX_SWAP_GROUP_NV extension is supported
    /// @return true if the extension is supported; otherwise false
    bool isExtensionSupported() const;

    /// Get the maximum number of swap groups and barriers supported by the
    /// current active OpenGL context.
    /// @param maxGroups maximum number of swap groups
    /// @param maxBarriers maximum number of barriers
    /// @return true if the query was successful; otherwise false
    bool queryMaxSwapGroup(GLuint& maxGroups, GLuint& maxBarriers) const;

    /// Joins the current OpenGL context to the specified swap group. If the
    /// context is already a member of a swap group, it is removed from that
    /// group first. If group is zero, the context is unbound from its current
    /// swap group, if any.
    /// @param group swap group to join
    /// @return true join was successful; otherwise false
    bool joinSwapGroup(GLuint group) const;

    /// Bind the specified swap group to given barrier. If barrier is zero, the
    /// group is unbound from its current barrier, if any.
    /// @param group swap group to bind
    /// @param barrier swap barrier to bind to
    /// @return true if successful; otherwise false
    bool bindSwapBarrier(GLuint group, GLuint barrier) const;

    /// Query the swap group and barrier of the current OpenGL context.
    /// @param group swap group the current context is bound to
    /// @param barrier swap barrier the current context is bound to
    /// @return true if query was successful; otherwise false
    bool querySwapGroup(GLuint& group, GLuint& barrier) const;

    /// Returns the current frame counter for swapGroup.
    /// @param[out] count frame counter
    /// @return true if query was successful; otherwise false
    bool queryFrameCount(GLuint & count) const;

    /// Resets the frame counter of swapGroup to zero.
    /// @return true if successful; otherwise false
    bool resetFrameCount() const;

  private:
    class D;
    std::unique_ptr<D> m_d;
  };

}

#endif // LUMINOUS_SWAPGROUPS_HPP
