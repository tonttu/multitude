include(../multitude.pri)

TEMPLATE = lib
TARGET = Dummy

# Doxygen file to use
DOX = multitude.dox

###############################################################################
#                                   TOOLS
###############################################################################

# Compiler for doxygen
dox_builder.name = doxygen
dox_builder.input = DOX
dox_builder.output = .dummy
dox_builder.commands = doxygen ${QMAKE_FILE_IN}
dox_builder.CONFIG += target_predeps
dox_builder.variable_out =
dox_builder.clean = .dummy
QMAKE_EXTRA_COMPILERS += dox_builder

###############################################################################
#                                 INSTALL
###############################################################################

doxs.path = $$PREFIX/share/doc/multitude/api
doxs.files = doxygen/html
doxs.CONFIG += no_check_exist

INSTALLS += doxs
