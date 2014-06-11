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
  class RESONANT_API DSPNetwork : public AudioLoop
  {
    DECLARE_SINGLETON(DSPNetwork);
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
      Connection() : m_channel(0),m_buf(0) { }
      /// Creates a connection object
      /** @param moduleId The id of the module that we are connecting to.
          @param channel The channel to connect to.
      */
      Connection(const QByteArray & moduleId, int channel)
        : m_moduleId(moduleId), m_channel(channel),m_buf(0)
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
      Buf        *m_buf;
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
      void setModule(Module *m) { m_module = m; }
      /// Returns a pointer to the DSP module
      Module * module() { return m_module; }

      /// Sets the default target channel of the module
      void setTargetChannel(int channel)
      {
        m_targetChannel = channel;
      }

      /// Deletes the module.
      void deleteModule();

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

      Module * m_module;

      std::vector<Connection> m_inputs;

      std::list<NewConnection> m_connections;
      std::vector<float *> m_ins;
      std::vector<float *> m_outs;

      bool m_compiled;
      bool m_done;
      bool m_usePanner;

      int  m_targetChannel;
    };

    /// @cond
    typedef std::list<Item> container;
    typedef container::iterator iterator;
    /// @endcond

    virtual ~DSPNetwork();

    /** Starts the DSPNetwork, using given audio device.

        To get a list of possible sound device names we recommend you use utility application
        ListPortAudioDevices.
        @param device Device name or empty string for default device
        @return False on error
    */
    bool start(const QString & device = "");

    /// Adds a DSP #Resonant::Module to the signal processing graph
    /** This function does not perform the actual addition, but puts the module into a FIFO,
        for the signal processing thread. */
    void addModule(Item &);
    /** Marks a given DSP module as finished. Once this function has been called the
        DSP thread will remove the module from the graph, and delete it. */
    void markDone(Item &);
    /** Marks a given DSP module as finished. Once this function has been called the
        DSP thread will remove the module from the graph, and delete it. */
    void markDone(Module &);

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
    /// This is not thread-safe
    /// @return Default sampley player object
    ModuleSamplePlayer * samplePlayer();

    /// Finds an item that holds a module with given id
    /// @param id Module id, @see Module::id()
    /// @return Pointer to the item inside DSPNetwork or NULL
    Item * findItem(const QByteArray & id);
    /// Finds a module with name id inside one of the items in DSPNetwork
    /// @param id Module id, @see Module::id()
    /// @return Pointer to the module or NULL
    Module * findModule(const QByteArray & id);

    /// @cond
    void dumpInfo(FILE *f);
    /// @endcond

    bool hasPanner() const { return m_panner != nullptr; }

    std::size_t itemCount() const { return m_items.size(); }

  private:
    /// Creates an empty DSPNetwork object.
    DSPNetwork();

    virtual int callback(const void *in, void *out,
                         unsigned long framesPerBuffer,
                         int streamid,
                         const PaStreamCallbackTimeInfo & time,
                         unsigned long flags);

    void doCycle(int, const CallbackTime & time);

    void checkNewControl();
    void checkNewItems();
    void checkDoneItems();
    void deliverControl(const QByteArray & moduleid, const QByteArray & commandid,
                        Radiant::BinaryData &);

    bool uncompile(Item &);
    bool compile(Item &);
    bool compile(Item &, int);
    Buf & findFreeBuf(int);
    bool bufIsFree(int, int);
    void checkValidId(Item &);
    float * findOutput(const QByteArray & id, int channel);
    long countBufferBytes();
    void duDumpInfo(FILE *f);

    container m_items;

    container m_newItems;

    std::vector<Buf> m_buffers;

    /// @todo remove these special hacks
    ModuleOutCollect *m_collect;
    ModulePanner   *m_panner;

    Radiant::BinaryData m_controlData;
    Radiant::BinaryData m_incoming;
    Radiant::BinaryData m_incopy;
    Radiant::Mutex m_inMutex;

    QString m_devName;
    // bool        m_continue;
    long        m_frames;
    int         m_doneCount;

    struct
    {
      Radiant::TimeStamp baseTime;
      int framesProcessed;
    } m_syncinfo;

    Radiant::Mutex m_newMutex;

    Radiant::Mutex m_startupMutex;
  };

}

#endif
