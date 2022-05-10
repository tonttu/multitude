# Multitude library collection

Multitude is a collection of C++ libraries. It contains components for a 2D
OpenGL rendering engine, event and attribute system, PDF processing library,
video decoding system using Ffmpeg, audio processing library, asynchronous
programming tools and various platform-specific utilities.

Multitude is part of MultiTaction Cornerstone SDK, a framework for building
interactive applications for large touch walls. While Cornerstone is a
proprietary framework, Multitude is released under LGPLv3 license.

Large part of the code is there for legacy reasons and most of the code it
not super useful without Cornerstone, but it's all here mainly for archival
purposes.

# Compilation

Multitude is intended to be used as a part of a third-party software, so that the third-party
software defines the critical qmake compilation parameters. To do this, you need to pass argument
CONFIG_PATH=/some/path/to/my/config to qmake when building multitude, for example:

qmake CONFIG_PATH=/home/joe/special-project/my-multitude.pri

Multitude build system will include the configuration file. The configuration file is expected
to setup the build environment. The minimum requirements are:

* Update INCLUDEPATH to include directory where external libraries (libsndfile etc.) have their headers.
* Define internal library names LIB_NIMBLE, LIB_RADIANT, LIB_VALUABLE, LIB_PATTERNS etc.

Beyond this, you can set up various build settings as necessary. Here are some common settings:

* Update DESTDIR and DLLDESTDIR to point to the target location for libraries / applications
* Set VERSION to match your software version
* Setup compiler cache systems like ccache
* Update C/C++ build flags
* Set variable CONFIG_LIB_PATH to define a qmake settings file for libraries.

If you set up CONFIG_LIB_PATH, you can add extra definitions that only apply to libraries
(different installation paths etc.)
