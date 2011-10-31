#include "Platform.hpp"

#ifdef RADIANT_WINDOWS

#include "CallStack.hpp"
#include "Mutex.hpp"
#include "Trace.hpp"

#include <stdint.h>
#include <sstream>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <DbgHelp.h>
#pragma comment(lib, "DbgHelp.lib")

static Radiant::Mutex s_dbgHelpMutex;

namespace Radiant
{

#if defined (RADIANT_X86)
#pragma optimize( "g", off )
#pragma warning( push )
#pragma warning( disable : 4748 )
#endif
  int backtrace(stackptr_t *buffer, int max_frames)
  {
    HANDLE processHandle = GetCurrentProcess();

    DWORD MachineType;
    CONTEXT context;
    STACKFRAME64 stackFrame;

    ZeroMemory( &stackFrame, sizeof( STACKFRAME64 ) );

#if defined (RADIANT_WIN32)
    ZeroMemory( &context, sizeof( CONTEXT ) );
    context.ContextFlags = CONTEXT_CONTROL;
    __asm
    {
    Label:
       mov [context.Ebp], ebp;
       mov [context.Esp], esp;
       mov eax, [Label];
       mov [context.Eip], eax;
    }

    MachineType                 = IMAGE_FILE_MACHINE_I386;
    stackFrame.AddrPC.Offset    = context.Eip;
    stackFrame.AddrPC.Mode      = AddrModeFlat;
    stackFrame.AddrFrame.Offset = context.Ebp;
    stackFrame.AddrFrame.Mode   = AddrModeFlat;
    stackFrame.AddrStack.Offset = context.Esp;
    stackFrame.AddrStack.Mode   = AddrModeFlat;

#elif defined (RADIANT_WIN64)
    RtlCaptureContext( &context );
 #if defined (RADIANT_AMD64)
    MachineType                 = IMAGE_FILE_MACHINE_AMD64;
    stackFrame.AddrPC.Offset    = context.Rip;
    stackFrame.AddrPC.Mode      = AddrModeFlat;
    stackFrame.AddrFrame.Offset = context.Rsp;
    stackFrame.AddrFrame.Mode   = AddrModeFlat;
    stackFrame.AddrStack.Offset = context.Rsp;
    stackFrame.AddrStack.Mode   = AddrModeFlat;
 #elif defined (RADIANT_IA64)
    MachineType                 = IMAGE_FILE_MACHINE_IA64;
    stackFrame.AddrPC.Offset    = context.StIIP;
    stackFrame.AddrPC.Mode      = AddrModeFlat;
    stackFrame.AddrFrame.Offset = context.IntSp;
    stackFrame.AddrFrame.Mode   = AddrModeFlat;
    stackFrame.AddrBStore.Offset= context.RsBSP;
    stackFrame.AddrBStore.Mode  = AddrModeFlat;
    stackFrame.AddrStack.Offset = context.IntSp;
    stackFrame.AddrStack.Mode   = AddrModeFlat;
 #endif
#endif // RADIANT_WIN32/WIN64

    int frames = 0;
    while ( frames < max_frames )
    {
       if ( ! StackWalk64(
          MachineType,
          processHandle,
          GetCurrentThread(),
          &stackFrame,
          (MachineType == IMAGE_FILE_MACHINE_I386 ? NULL : &context),
          NULL,
          SymFunctionTableAccess64,
          SymGetModuleBase64,
          NULL ) )
       {
          // Maybe it failed, maybe we have finished walking the stack. No real way to tell
          break;
       }

       if ( stackFrame.AddrPC.Offset != 0 )
       {
          buffer[ frames++ ] = stackFrame.AddrPC.Offset;
       }
       else
       {
          break;
       }
    }

    return frames;
  }
#if defined (RADIANT_X86)
   #pragma warning( pop )
   #pragma optimize( "g", on )
#endif

  CallStack::CallStack()
  {
    // DbgHelp is single-threaded
    Radiant::Guard lock(s_dbgHelpMutex);

    // Need to initialize DbgHelp symbols
    static bool initialized = false;
    if (!initialized)
    {
      DWORD Options = SymGetOptions();
      ::SymSetOptions( Options | SYMOPT_DEBUG | SYMOPT_LOAD_LINES );
      ::SymInitialize (GetCurrentProcess(), NULL, TRUE);
      initialized = true;
    }

    m_frameCount = backtrace(m_frames, max_frames);
  }

  CallStack::~CallStack()
  {
  }

  void CallStack::print() const
  {
      Radiant::Guard lock(s_dbgHelpMutex);

    HANDLE handle = GetCurrentProcess();
    DWORD64 offset;

    const size_t buffersize = (sizeof(SYMBOL_INFO) + MAX_SYM_NAME*sizeof(TCHAR) + sizeof(ULONG64) - 1) / sizeof(ULONG64);
    ULONG64 buffer[buffersize];

    PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
    pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    pSymbol->MaxNameLen = MAX_SYM_NAME;

    std::stringstream s;

    for (size_t i = 0; i < size(); ++i)
    {
      DWORD64 ptr = m_frames[i];

      // Retrieve symbol name
      if (!SymFromAddr(handle, ptr, &offset, pSymbol))
        Radiant::warning("Unable to get symbol information: err %d\n", GetLastError());

      // Retrieve filename and linenumber
      DWORD displacement = 0;
      IMAGEHLP_LINE64 sym_line;
      sym_line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
      if (!SymGetLineFromAddr64(handle, ptr, &displacement, &sym_line))
      {
        // TODO: Not sure why this sometimes fails with INVALID_ADDRESS
        if (GetLastError() != ERROR_INVALID_ADDRESS)
          Radiant::warning("Unable to get symbol line information: err %d\n", GetLastError());
      }
      Radiant::info("#%d %s at %s:%d", i, pSymbol->Name, sym_line.FileName, sym_line.LineNumber);
    }
  }
}

#endif
