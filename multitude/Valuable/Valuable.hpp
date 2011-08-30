/* COPYRIGHT
 *
 * This file is part of Valuable.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Valuable.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef VALUABLE_VALUABLE_HPP
#define VALUABLE_VALUABLE_HPP

/**
    @page valuable Value objects

    ValueObjects are serializable variant objects that can be accessed from native code or from external files such as stylesheets.
    Valuable has ValueObjects for a lot of various types (bool, float, int, string, vector, matrix, color) and new ones can easily be made. You can see them in the Valuable documentation

    <b>Example 1: Creating and using ValueObjects</b>

    In this example we'll look at how ValueObjects can be used in for example a slider widget. Our slider will be a horizontal line with labels
    on both ends (showing the minimum and maximum value). Somewhere on the line there will be a button with a label to indicate the current value

    @code
    // Output widget should look a bit like this:
    //
    //  [min] -----|-------- [max]
    //          current
    //
    class SliderWidget : public MultiWidgets::Widget
    {
    public:
      SliderWidget::SliderWidget(MultiWidgets::Widget *parent)
        : MultiWidgets::Widget(parent)
        , m_minValue(this, "minimum-value", 0.0f)           // Set the host, the CSS name and the initial value for the ValueObjects
        , m_maxValue(this, "maximum-value", 100.0f)
        , m_currentValue(this, "current-value", 0.0f)
      {
      }

      void renderContent(Luminous::RenderContext & ctx)
      {
        float min = m_minValue.asFloat();
        float max = m_maxValue.asFloat();
        float current = m_currentValue.asFloat();

        // ... Draw the slider ...
      }

      void processFingers(MultiWidgets::GrabManager & gm, MultiWidgets::FingerArray & fingers, float dt)
      {
        Vector2 delta = calculateDelta(gm, fingers);  // Calculate if any of the fingers have moved the slider
        float newValue = calculateValue();            // Get the new value of the slider (in the range [minimum-value...maximum-value])
        m_currentValue = newValue;                    // Assign it to the ValueObject
      }

    private:
      Valuable::ValueFloat m_minValue;
      Valuable::ValueFloat m_maxValue;
      Valuable::ValueFloat m_currentValue;
    };


    @endcode

    <h2> Value Listeners</h2>
    ValueListeners are observers that can be attached to a ValueObject. These observers will be notified of
    any changes to the ValueObject, such as a change of value or when they get deleted.

    <b>Example 2: Using ValueListeners</b>

    We'll extend the SliderWidget a little bit to make it easy to use for other widgets.

    @code
    class SliderWidget : public MultiWidgets::Widget
    {
    public:
      ..
      // Attaches a listener to the current value of the slider
      void addListener(Valuable::ValueListener * listener) { m_currentValue.addListener(listener); }

      // Returns the current value of the slider
      // m_currentValue is a ValueFloat object, so it also has a cast operator defined for float. We could have also done "return (float)m_currentValue;" here
      float getValue() const { return m_currentValue.asFloat(); }
    }
    @endcode


    Now we can use the SliderWidget to make for example an audiomixer panel. Our panel will have a number of sliders that should update the audiosystem if they are moved.

    Widget objects are ValueListeners already, so we can just pass 'this' to addListener and overload the valueChanged method.

    @code
    class AudioPanelWidget : public MultiWidgets::Widget
    {
      enum { channel_count = 8 };

    public:

      AudioPanelWidget(MultiWidgets::Widget * parent)
        : MultiWidgets::Widget(parent)
      {
        for (int i =0 ; i < channel_count; ++i) {
          m_channel[i] = new SliderWidget(this);    // Create the slider widget
          m_channel[i]->addListener(this);          // We attach the AudioPanelWidget to the SliderWidget so we get notified of changes
          m_channel[i]->setClass("audioslider");    // We set the CSS class to 'audioslider' so we can set values in an external CSS file

          // We can also set other things like size, position, etc
        }
      }

      void valueChanged(Valuable::ValueObject * object)
      {
        // Update the audio system with the new slider values
        updateVolumes();
      }

      void updateVolumes()
      {
        for (int i = 0; i < channel_count; ++i) {
          float volume = m_channel[i]->getValue();

          // Update our audiosystem by setting the correct volume for this channel
          m_audioSystem->setChannelVolume(i, volume);
        }
      }

    private:
      SliderWidget * m_channel[channel_count];
      AudioSystem * m_audioSystem;
    }
    @endcode

    The CSS code below would set all sliders ranging from 0 to 11 and initially set to 5
    @code
    .audioslider {
      minimum-value: 0;
      maximum-value: 11;
      current-value: 5;
    }
    @endcode

    <h2>Accessing ValueObjects by name</h2>
    ValueObjects can be accessed by name as well. If we want a 'maxOut' method in our panel that would move all the sliders
    up to whatever the maximum value might be at that moment, we can easily do the following:

    @code
    class AudioPanelWidget : public MultiWidgets::Widget
    {
    public:
      ..
      void maxOut()
      {
        for (int i = 0; i < channel_count; ++i)
        {
          // Retrieve the maximum and current value object for this slider
          ValueObject * max = m_channel[i]->getValue("maximum-value");
          ValueObject * current = m_channel[i]->setValue("current-value");

          if (max && current) {
            // Set the new current value to that of max
            current->set( max->asFloat() ); 
          }
        }
      }
     
    @endcode
*/

#define debugValuable(...) (Radiant::trace("Valuable", Radiant::DEBUG, __VA_ARGS__))
/** A library for automatically saving and loading class member values.

    The purpose of this framework is to handle:


    * Saving classes with members to XML files
    
    * Loading classes with members from XML files

    * Set/get parameter member values dynamically by string name
 */

#include <Valuable/Export.hpp>

namespace Valuable
{

}

#endif

