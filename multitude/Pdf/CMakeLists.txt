set(LIBRARY Pdf)

cornerstone_add_library(${LIBRARY} SHARED)

target_sources(${LIBRARY} PRIVATE PDFManager.cpp)

target_link_libraries(${LIBRARY}
  PUBLIC OurExpected Punctual MultiFolly
)

cornerstone_add_pdfium_libs(${LIBRARY} PRIVATE)

target_compile_definitions(${LIBRARY} PRIVATE -DPDF_EXPORT)

cornerstone_install_lib(${LIBRARY})
