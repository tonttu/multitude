# Temporary always generated header file.
cmake_path(SET temp NORMALIZE
  ${CMAKE_BINARY_DIR}/VersionGenerated.hpp
)

# File we really care about. Changes if it is different than temp.
cmake_path(SET header NORMALIZE
  ${CMAKE_BINARY_DIR}/multitude/Radiant/VersionGenerated.hpp
)

# Include path for generated header.
cmake_path(SET include NORMALIZE ${CMAKE_BINARY_DIR}/multitude)

# Always generate temp VersionGenerated.hpp. Copy it to destination,
# if it is different than the one in destination. Then remove temp to
# guarantee it is always generated.
add_custom_command(
  OUTPUT ${temp}
  COMMAND ruby ARGS ${CMAKE_CURRENT_LIST_DIR}/generate-version.rb ${temp}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${temp} ${header}
  COMMAND ${CMAKE_COMMAND} -E remove ${temp}
  COMMENT "Generating version header"
)

set(GENERATE VersionGen)
set(LIBRARY VersionHdr)

# We need real target for generation that header only library can depend on.
add_custom_target(${GENERATE} DEPENDS ${temp})

# Header only library.
add_library(${LIBRARY} INTERFACE)

target_include_directories(${LIBRARY} INTERFACE ${include})

add_dependencies(${LIBRARY} ${GENERATE})
