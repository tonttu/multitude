/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RESONANT_MODULE_HPP
#define RESONANT_MODULE_HPP

#include <Resonant/Export.hpp>

#include <Valuable/Node.hpp>

namespace  Radiant {

  class BinaryData;

}

namespace Resonant {

  struct CallbackTime;

  /** Base class for #Resonant signal processing blocks. */
  /// @todo Check if the id could be dropped in favor of
  /// Attribute::name
  class RESONANT_API Module : public Valuable::Node
  {
  public:

    enum {
      /// Maximum length of a processing cycle (samples)
      MAX_CYCLE = 1024
    };

    /// Constructs a new module base object.s
    Module();
    virtual ~Module();

    /** Prepare for signal processing.

    The default implementation returns true. Most child classes
    will need to override this method to perform some preparation
    work.

    @param[out] channelsIn The number of desired input channels.  If
    necessary, the number of input and output channels is changed
    (for example if the module is stereo-only, but the host
    requested mono operation).

    @param[out] channelsOut The number of desired output channels.


    @return Returns true if the module prepared successfully.
     */
    virtual bool prepare(int & channelsIn, int & channelsOut);
    /** Sends a control message to the module.

        The default implementation does nothing. Child classes with
        dynamic variable will need to override this method.
        @param id Command name
        @param data Command parameters
     */
    virtual void eventProcess(const QByteArray & id, Radiant::BinaryData & data) OVERRIDE;
    /** Processes one cycle of audio data.

    @param in Input audio data.

    @param out Output audio data.

    @param n Number of samples to process. Guaranteed to be between
    1 and #MAX_CYCLE.
     */
    virtual void process(float ** in, float ** out, int n, const CallbackTime & time) = 0;

    /// Stops the signal processing, freeing any resources necessary.
    /// @return True if stopping succeeded (or was already stopped). False on error.
    virtual bool stop();

    /// Sets the id of the module.
    /// @param id The new id
    void setId(const QByteArray & id);
    /// ID of the module
    /// @return the id of the module.
    const QByteArray & id() const { return m_id; }

  private:
    QByteArray m_id;
  };

  typedef std::shared_ptr<Module> ModulePtr;

}

#endif
