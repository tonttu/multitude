/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RESONANT_DSPNETWORK_HPP
#define RESONANT_DSPNETWORK_HPP

#include <Resonant/AudioLoop.hpp>
#include <Resonant/Module.hpp>

#include <Radiant/BinaryData.hpp>
#include <Radiant/Singleton.hpp>

#include <list>
#include <vector>
#include <cassert>

#include <string.h>

namespace Resonant {

  using std::shared_ptr;

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

      DSPNetwork implements a simple signal processing graph driver. The graph
      has hot-plug -feature so new modules can be added in run-time, and the
      engine will re-wire the connetions as necessary.

      DSPNetwork is a singleton. The instance is kept alive as long as there is
      a reference to the shared pointer returned by the DSPNetwork::instance()
      function. It is strongly recommended that you keep a reference to it
      during the lifetime of your application.*/
  class RESONANT_API DSPNetwork
  {
    DECLARE_SINGLETON(DSPNetwork);
  public:

    /// Which AudioLoop backend to use
    enum AudioLoopBackend
    {
      AUDIO_LOOP_DEFAULT,

      AUDIO_LOOP_PORT_AUDIO,
      AUDIO_LOOP_PULSE_AUDIO
    };

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
          m_data = new float [n] ();
          m_size = n;
        }
      }

      /// Allocates #Resonant::Module::MAX_CYCLE samples for buffer space
      void init() { allocate(Module::MAX_CYCLE); }

      /// Frees the buffer data
      void clear() { delete [] m_data; m_data = 0; m_size = 0; }
    private:
      friend class DSPNetwork;

      float * m_data;
      int     m_size;
    };

    /** Holds connection information between the DSP modules.*/
    class RESONANT_API Connection
    {
    public:
      /// Creates an empty connection object, with undefined connections.
      Connection() : m_channel(0) { }
      /// Creates a connection object
      /** @param moduleId The id of the module that we are connecting to.
          @param channel The channel to connect to.
      */
      Connection(const QByteArray & moduleId, int channel)
        : m_moduleId(moduleId), m_channel(channel)
      {
      }

      /// Sets the id of the connected module
      void setModuleId(const QByteArray & id)
      {
        m_moduleId = id;
      }

      /// Compare two Connection objects
      inline bool operator == (const Connection & that) const
      {
        return (m_moduleId == that.m_moduleId) &&
            (m_channel == that.m_channel);
      }

    private:
      friend class DSPNetwork;

      /// @cond
      QByteArray m_moduleId;
      int         m_channel;
      /// @endcond
    };

    /** Objects that store the information necessary to create new connections.

    @see Connection
     */
    class RESONANT_API NewConnection
    {
    public:
      NewConnection() : m_sourceChannel(0), m_targetChannel(0) {}
      /** The id of the audio source module. */
      QByteArray m_sourceId;
      /** The id of the audio destination module. */
      QByteArray m_targetId;
      /** The channel index in the source module (where the signal is coming from). */
      int         m_sourceChannel;
      /** The channel index in the target module (where the signal is going to). */
      int         m_targetChannel;
    };

    /** Stores a simple audio processing #Resonant::Module.*/
    class RESONANT_API Item
    {
      friend class DSPNetwork;

    public:
      Item();
      ~Item();
      /// Sets the DSP #Resonant::Module that this Item contains.
      void setModule(Module *m) { m_module = ModulePtr(m); }
      void setModule(ModulePtr m) { m_module = m; }
      /// Returns a pointer to the DSP module
      ModulePtr & module() { return m_module; }

      /// Sets the default target channel of the module
      void setTargetChannel(int channel)
      {
        m_targetChannel = channel;
      }

      /// Resets the module pointer. May also delete the module.
      void resetModule();

      /// Sets if the item should use panner
      void setUsePanner(bool usePanner)
      {
        m_usePanner = usePanner;
      }

      /// Returns if the item use panner
      bool usePanner()
      {
        return m_usePanner;
      }

      void addConnection(const NewConnection & c) { m_connections.push_back(c); }

    private:

      // Process n samples
      inline void process(int n, const CallbackTime & time)
      {
        assert(m_compiled != false);
        float ** in = m_ins.empty() ? 0 : &m_ins[0];
        float ** out = m_outs.empty() ? 0 : &m_outs[0];
        m_module->process(in, out, n, time);
      }

      void eraseInput(const Connection & c);
      void eraseInputs(const QByteArray & moduleId);
      int findInInput(float * ptr) const;
      int findInOutput(float * ptr) const;
      void removeInputsFrom(const QByteArray & id);

      ModulePtr m_module;

      std::vector<Connection> m_inputs;

      std::list<NewConnection> m_connections;
      std::vector<float *> m_ins;
      std::vector<float *> m_outs;

      bool m_compiled;
      bool m_done;
      bool m_usePanner;

      int  m_targetChannel;
    };

    typedef std::shared_ptr<Item> ItemPtr;

    /// @cond
    typedef std::list<ItemPtr> container;
    typedef container::iterator iterator;
    /// @endcond

    virtual ~DSPNetwork();

    /** Starts the DSPNetwork, using given audio device.

        To get a list of possible sound device names we recommend you use utility application
        ListPortAudioDevices.
        @param backend AudioLoop backend
        @return False on error
    */
    bool start(AudioLoopBackend backend = AUDIO_LOOP_DEFAULT);

    /// Adds a DSP #Resonant::Module to the signal processing graph
    /** This function does not perform the actual addition, but puts the module into a FIFO,
        for the signal processing thread. */
    void addModule(ItemPtr item);
    /** Marks a given DSP module as finished. Once this function has been called the
        DSP thread will remove the module from the graph, and delete it. */
    void markDone(ItemPtr );
    /** Marks a given DSP module as finished. Once this function has been called the
        DSP thread will remove the module from the graph, and delete it. */
    void markDone(ModulePtr module);

    /** Send binary control data to the DSP network.
        When sending messages, the BinaryData object should contain an identifier string
        in the beginning. The DSPNetwork will read this string, and pass the command to the
        corresponding #Resonant::Module. Typical example of use could be:

        \code
Radiant::BinaryData control;
control.writeString("moviegain/gain");
control.writeFloat32(0.3);
DSPNetwork::instance().send(control);
        \endcode
        @param control Identifier string and correct commands for corresponding Module
    */
    void send(Radiant::BinaryData & control);

    /// Returns the default sample player object.
    /// If the object does not exis yet, it is created on the fly.
    /// @return Default sampley player object
    std::shared_ptr<ModuleSamplePlayer> samplePlayer();

    /// @cond

    ModuleSamplePlayer * javascriptSamplePlayer();

    /// @endcond

    /// Finds an item that holds a module with given id
    /// @param id Module id, @see Module::id()
    /// @return Pointer to the item inside DSPNetwork or NULL
    // ItemPtr findItem(const QByteArray & id);
    ItemPtr findItem(const QByteArray & id);
    /// Finds a module with name id inside one of the items in DSPNetwork
    /// @param id Module id, @see Module::id()
    /// @return Pointer to the module or NULL
    ModulePtr findModule(const QByteArray & id);

    /// @cond
    void dumpInfo(FILE *f);
    /// @endcond

    bool hasPanner() const { return m_panner != nullptr; }
    /// Returns the panner module or NULL
    std::shared_ptr<ModulePanner> panner() { return m_panner; }

    std::shared_ptr<ModuleOutCollect> collect() const { return m_collect; }

    std::size_t itemCount() const { Radiant::Guard g(m_itemMutex); return m_items.size(); }

    AudioLoop * audioLoop() { return m_audioLoop.get(); }

    bool isRunning() const { return m_audioLoop ? m_audioLoop->isRunning() : false; }

    /// @cond

    void doCycle(int framesPerBuffer, const CallbackTime & time);

    /// @endcond

  private:
    /// Creates an empty DSPNetwork object.
    DSPNetwork();

    void checkNewControl();
    void checkNewItems();
    void checkDoneItems(std::vector<ModulePtr> & modulesToDelete);
    void deliverControl(const QByteArray & moduleid, const QByteArray & commandid,
                        Radiant::BinaryData &);

    bool uncompile(ItemPtr item);
    bool compile(ItemPtr item);
    bool compile(ItemPtr item, int location);
    Buf & findFreeBuf(int);
    bool bufIsFree(int, int);
    void checkValidId(ItemPtr item);
    float * findOutput(const QByteArray & id, int channel);
    long countBufferBytes();
    void doDumpInfo(FILE *f);

    /// m_itemMutex must be locked in order to call this function
    ItemPtr findItemUnsafe(const QByteArray & id);

    container m_items;

    container m_newItems;

    std::vector<Buf> m_buffers;

    /// @todo remove these special hacks
    std::shared_ptr<ModuleOutCollect> m_collect;
    std::shared_ptr<ModulePanner>     m_panner;

    Radiant::BinaryData m_controlData;
    Radiant::BinaryData m_incoming;
    Radiant::BinaryData m_incopy;
    Radiant::Mutex m_inMutex;

    std::unique_ptr<AudioLoop> m_audioLoop;

    // bool        m_continue;

    int         m_doneCount; // Protected by m_newMutex

    Radiant::Mutex m_newMutex;
    mutable Radiant::Mutex m_itemMutex;

    Radiant::Mutex m_startupMutex;
  };

}

#endif
