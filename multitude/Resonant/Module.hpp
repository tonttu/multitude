/* COPYRIGHT
 *
 * This file is part of Resonant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Resonant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#ifndef RESONANT_MODULE_HPP
#define RESONANT_MODULE_HPP

#include <Resonant/Export.hpp>

#include <Valuable/HasValues.hpp>

namespace  Radiant {

  class BinaryData;

}

namespace Resonant {

  class Application;

  /** Base class for #Resonant signal processing blocks. */
  /// @todo Check if the id could be dropped in favor of
  /// ValueObject::name
  class RESONANT_API Module : public Valuable::HasValues
  {
  public:

    enum {
      /// Maximum length of the #id string (bytes)
      MAX_ID_LENGTH = 256,
      /// Maximum length of a processing cycle (samples)
      MAX_CYCLE = 1024
    };

    Module(Application *);
    virtual ~Module();

    /** Prepare for signal processing.

    The default implementation returns true. Most child classes
    will need to override this method to perform some preparation
    work.

    @arg channelsIn The number of desired input channels.  If
    necessary, the number of input and output channels is changed
    (for example if the module is stereo-only, but the host
    requested mono operation).

    @arg channelsOut The number of desired output channels.


    @return Returns true if the module prepared successfully.
     */
    virtual bool prepare(int & channelsIn, int & channelsOut);
    /** Sends a control message to the module.

    The default implementation does nothing. Child classes with
    dynamic variable will need to override this method.
     */
    virtual void processMessage(const char * address, Radiant::BinaryData *);
    /** Processes one cycle of audio data.

    @arg in Input audio data.

    @arg out Output audio data.

    @arg n Number of samples to process. Guaranteed to be between
    1 and #MAX_CYCLE.
     */
    virtual void process(float ** in, float ** out, int n) = 0;
    /** Stops the signal processing, freeing any resources necessary. */
    virtual bool stop();

    /** Sets the id of the module. */
    void setId(const char *);
    /** Returns the id of the module. */
    const char * id() { return m_id; }

  private:
    Application * m_application;
    char m_id[MAX_ID_LENGTH];
  };

}

#endif
