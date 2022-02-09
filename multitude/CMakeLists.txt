add_subdirectory(ThirdParty/expected)

if(ENABLE_PUNCTUAL)
  add_subdirectory(Punctual)
endif()

if(ENABLE_SMTP)
  add_subdirectory(ThirdParty/SMTPEmail)
  add_subdirectory(EmailSending)
endif()

if(ENABLE_LUMINOUS)
  add_subdirectory(Squish)
  add_subdirectory(Luminous)
endif()

if(ENABLE_RESONANT OR ENABLE_MUSHY)
  add_subdirectory(Nimble)
  add_subdirectory(Radiant)
  add_subdirectory(Valuable)
endif()

if(ENABLE_RESONANT)
  add_subdirectory(Resonant)
endif()

if(ENABLE_VIDEODISPLAY)
  add_subdirectory(VideoDisplay)
endif()

if(ENABLE_APPLICATIONS)
  add_subdirectory(Applications/ListPortAudioDevices)
endif()

if(ENABLE_TESTS)
  add_subdirectory(ThirdParty/UnitTest++)
endif()
