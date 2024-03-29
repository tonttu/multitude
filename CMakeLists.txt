add_subdirectory(ThirdParty/expected)
add_subdirectory(Patterns)

if(ENABLE_PUNCTUAL)
  add_subdirectory(Punctual)
endif()

if(ENABLE_SMTP)
  add_subdirectory(ThirdParty/SMTPEmail)
  add_subdirectory(EmailSending)
endif()

if(ENABLE_LUMINOUS)
  add_subdirectory(ThirdParty/nvml)
  add_subdirectory(ThirdParty/adl_sdk)
  add_subdirectory(Squish)
  add_subdirectory(Luminous)
endif()

if(ENABLE_PDF)
  add_subdirectory(Pdf)
endif()

if(ENABLE_RESONANT OR ENABLE_MUSHY)
  add_subdirectory(Nimble)
  add_subdirectory(Radiant)
  add_subdirectory(Valuable)

  include(Radiant/Version.cmake)
  include(Radiant/RadiantHdr.cmake)
endif()

if(ENABLE_RESONANT)
  add_subdirectory(Resonant)
endif()

if(ENABLE_VIDEODISPLAY)
  add_subdirectory(VideoDisplay)
  add_subdirectory(VideoDisplay/rgbeasy-sdk-v7.14.1)
endif()

if(ENABLE_APPLICATIONS)
  add_subdirectory(Applications/ListPortAudioDevices)
endif()

if(ENABLE_UNITTEST++)
  add_subdirectory(ThirdParty/UnitTest++)
endif()
