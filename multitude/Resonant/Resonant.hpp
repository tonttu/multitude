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

#ifndef RESONANT_RESONANT_HPP
#define RESONANT_RESONANT_HPP

#include <Radiant/Trace.hpp>

#define debugResonant(...) (Radiant::trace("Resonant", Radiant::DEBUG, __VA_ARGS__))

/**
    @page sound-config Sound configuration

    Multi-channel audio setup can be configured with either using the
    Resonant C++ API directly or specifying the config with an external XML
    file.

    The XML file can be given as a parameter to any cornerstone application
    with <b>--audio-config</b> -parameter or with a C++ function
    MultiWidgets::Application::loadAudioConfig. When this file is used,
    Application creates a new instance of Resonant::ModulePanner and sets all
    parameters from the XML to the panner.

    It is also possible to save the current audio setup back to XML file using
    Serializer:
    @code
    Valuable::Serializer::serializeXML("audio-config.xml", modulePannerInstance);
    @endcode

    This page includes a full C++ example using either radial or rectangle
    (depending which file is given as a parameter to the application)
    panning modes with a sample player. The same example can be found under
    Examples/AudioExample inside the SDK package.

    example-speakers.xml - Start the example with
    <em>--audio-config example-speakers.xml</em>:

    @include "../../Examples/AudioExample/example-speakers.xml"

    example-rectangles.xml - Start the example with
    <em>--audio-config example-rectangles.xml</em>:

    @include "../../Examples/AudioExample/example-rectangles.xml"

    <em>AudioExample/Main.cpp</em>:
    @include "../../Examples/AudioExample/Main.cpp"
*/


/// Resonant library is a collection of C++ classes for sound processing.

/** It is heavily in work-in-progress.
    
    \b Copyright: The Resonant library has been developed by Helsinki
    Institute for Information Technology (HIIT, 2006-2007) and
    MultiTouch Oy (2007).
    
    Resonant is released under the GNU Lesser General Public License
    (LGPL), version 2.1.

    @author Tommi Ilmonen, Esa Nuuros
    
*/

namespace Resonant {

}

#endif
