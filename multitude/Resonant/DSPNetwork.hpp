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

#ifndef RESONANT_DSPNETWORK_HPP
#define RESONANT_DSPNETWORK_HPP

#include <Resonant/AudioLoop.hpp>
#include <Resonant/Module.hpp>

#include <Radiant/BinaryData.hpp>
#include <Radiant/Mutex.hpp>

#include <Radiant/RefPtr.hpp>

#include <list>
#include <vector>
#include <cassert>

#include <string.h>

namespace Resonant {

  using Radiant::RefPtr;

  class ModuleOutCollect;
  class ModulePanner;
  class ModuleSamplePlayer;

  /* Stuff that needs implementing:

      - When a module is put into the graph (that's running) it can
      end up with getting input from a source that is invalidated by
      later modules. This should be fixed once we get to having
      trouble...
  */

  /** An audio signal processing engine.

      DSPNetwork implements a simple signal processing graph
      driver. The graph has hot-plug -feature so new modules can be
      added in run-time, and the engine will re-wire the connetions as
      necessary.

      DSPNetwork follows a semi-singleton approach: It is possible to create multiple
      DSPNetwork objects, that use different audio devices. In reality this is seldom practical
      and thus there is usually exactly one DSPNetwork per application. When the first
      DSPNetwork is created it becomes the default network, and calls to #instance() will
      return pointer to the network. It is strongly recommended that you do not delete the
      defualt DSPNetwork before application is ready exit, as doing so may invalidate
      pointers that are held to it.
   */
  class RESONANT_API DSPNetwork : public AudioLoop
  {
  public:

    /** Holds audio sample buffers for inter-module transfer.

    Note the lack of destructor. You need to call "clear"
    manually. */
    class Buf
    {
    public:

      Buf() : m_data(0), m_size(0)
      {}

      /// Allocates n samples to the storage buffer
      void allocate(int n)
      {
        if(n != m_size) {
          delete [] m_data;
          m_data = new float [n];
          m_size = n;
        }
      }

      /// Allocates #Module::MAX_CYCLE samples for buffer space
      void init() { allocate(Module::MAX_CYCLE); }

      /// Frees the buffer data
      void clear() { delete [] m_data; m_data = 0; m_size = 0; }

      float * m_data;
      int     m_size;
    };

    /** Holds connection information between the DSP modules.*/
    class RESONANT_API Connection
    {
    public:
      Connection() : m_channel(0),m_buf(0) { m_moduleId[0] = '\0'; }
      Connection(const char * moduleId, int channel)
          : m_channel(channel),m_buf(0)
      {
        setModuleId(moduleId);
      }

      void setModuleId(const char * id)
      {
        if(id)
#ifdef WIN32
          strcpy_s(m_moduleId, id);
#else
        strcpy(m_moduleId, id);
#endif
        else
          m_moduleId[0] = '\0';
      }

      inline bool operator == (const Connection & that) const
      {
        return (strcmp(m_moduleId, that.m_moduleId) == 0) &&
            (m_channel == that.m_channel);
      }

      char        m_moduleId[Module::MAX_ID_LENGTH];
      int         m_channel;
      Buf        *m_buf;
    };

    /** Objects that store the information necessary to create new connections.

    @see Connection
     */
    class RESONANT_API NewConnection
    {
    public:
      NewConnection() : m_sourceChannel(0), m_targetChannel(0) {}
      /** The id of the audio source module. */
      char        m_sourceId[Module::MAX_ID_LENGTH];
      /** The id of the audio destination module. */
      char        m_targetId[Module::MAX_ID_LENGTH];
      /** The channel index in the source module (where the signal is coming from). */
      int         m_sourceChannel;
      /** The channel index in the target module (where the signal is going to). */
      int         m_targetChannel;
    };

    /** Stores a sinple audio processing #Resonant::Module.*/
    class RESONANT_API Item
    {
      friend class DSPNetwork;

    public:
      Item();
      ~Item();
      /// Sets the DSP #Module that this Item contains.
      void setModule(Module *m) { m_module = m; }
      /// Returns a pointer to the DSP module
      Module * module() { return m_module; }

      /// Sets the defualt target channel of the module
      void setTargetChannel(int channel)
      {
        m_targetChannel = channel;
      }

      /// Deletes the module.
      void deleteModule()
      {
        delete m_module;
        m_module = 0;
      }

    private:

      // Process n samples
      inline void process(int n)
      {
        assert(m_compiled != false);
        float ** in = m_ins.empty() ? 0 : &m_ins[0];
        float ** out = m_outs.empty() ? 0 : &m_outs[0];
        m_module->process(in, out, n);
      }

      void eraseInput(const Connection & c);
      void eraseInputs(const std::string & moduleId);
      int findInInput(float * ptr) const;
      int findInOutput(float * ptr) const;
      void removeInputsFrom(const char * id);

      Module * m_module;

      std::vector<Connection> m_inputs;

      std::list<NewConnection> m_connections;
      std::vector<float *> m_ins;
      std::vector<float *> m_outs;

      bool m_compiled;
      bool m_done;

      int  m_targetChannel;
    };

    typedef std::list<Item> container;
    typedef container::iterator iterator;

    /// Creates an empty DSPNetwork object.
    DSPNetwork();
    virtual ~DSPNetwork();

    /** Starts the DSPNetwork, using given audio device.

        To get a list of possible sound device names we recommend you use utility application
        ListPortAudioDevices.
    */
    bool start(const char * device = 0);

    /// Adds a DSP #Module to the signal processing graph
    /** This function does not perform the actual addition, but puts the module into a FIFO,
        for the signal processing thread. */
    void addModule(Item &);
    /** Marks a given DSP module as finished. Once this function has been called the
        DSP thread will remove the module from the graph, and delete it. */
    void markDone(Item &);

    /** Send binary control data to the DSP network.
        When sending messages, the BinaryData object should contain an identifier string
        in the beginning. The DSPNetwork will read this string, and pass the command to the
        corresponding #Module. Typical example of use could be:

        \code
Radiant::BinaryData control;
control.writeString("moviegain/gain");
control.writeFloat32(0.3);
DSPNetwork::instance().send(control);
        \endcode
    */
    void send(Radiant::BinaryData & control);

    /// Returns the default sample player object.
    /** If the object does not exis yet, it is created on the fly. */
    ModuleSamplePlayer * samplePlayer();
    /// Returns the DSPNetwork instance.
    /**  */
    static DSPNetwork * instance();

  private:

    virtual int callback(const void *in, void *out,
                         unsigned long framesPerBuffer
                         //       , const PaStreamCallbackTimeInfo* time,
                         //			 PaStreamCallbackFlags status
                         );

    void doCycle(int);

    void checkNewControl();
    void checkNewItems();
    void checkDoneItems();
    void deliverControl(const char * moduleid, const char * commandid,
                        Radiant::BinaryData &);

    bool uncompile(Item &);
    bool compile(Item &);
    bool compile(Item &, int);
    Buf & findFreeBuf(int);
    bool bufIsFree(int, int);
    void checkValidId(Item &);
    Item * findItem(const char * id);
    Module * findModule(const char * id);
    float * findOutput(const char * id, int channel);
    long countBufferBytes();

    container m_items;

    container m_newItems;

    std::vector<Buf> m_buffers;

    ModuleOutCollect *m_collect;
    ModulePanner   *m_panner;

    Radiant::BinaryData m_controlData;
    Radiant::BinaryData m_incoming;
    Radiant::BinaryData m_incopy;
    Radiant::MutexAuto m_inMutex;

    char        m_devName[128];
    // bool        m_continue;
    long        m_frames;
    int         m_doneCount;

    Radiant::MutexAuto m_newMutex;

    static DSPNetwork * m_instance;
  };

}

#endif
