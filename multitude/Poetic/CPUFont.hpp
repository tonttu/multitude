/* COPYRIGHT
 */
#ifndef POETIC_CPU_FONT_HPP
#define POETIC_CPU_FONT_HPP

#include "Export.hpp"
#include "GlyphContainer.hpp"
#include "BBox.hpp"

#include <Luminous/Collectable.hpp>
#include <Luminous/GLResources.hpp>

#define POETIC_DEFAULT_RESOLUTION 72

namespace Poetic
{

  class GPUFont;

  /// An abstract base class providing a common interface for all fonts residing
  /// in CPU memory.
  /// @todo Doc
  class POETIC_API CPUFont : public::Luminous::Collectable
  {
  public:
    virtual ~CPUFont() {}
    /// Returns the cursor advance for the given string, i.e. how long the rendered string is
    /// @param str string to process
    /// @param n maximum number of characters to process. When using -1 here it will process until it reaches a \0 character
    /// @return the cursor advance (length) of the rendered string
    virtual float advance(const char * str, int n = -1) = 0;
    /// @copydoc advance
    virtual float advance(const wchar_t * str, int n = -1) = 0;
    /// Calculates the cursor advance for every separate character
    /// @param str input string
    /// @param advances list of floats for calculated results
    /// @param n maximum number of characters to process. When using -1 here it will process until it reaches a \0 character
    virtual void advanceList(const wchar_t * str, float * advances, int n = -1) = 0;

    /// @copybrief advance
    float advance(const QString & str) {
      std::wstring wstr = str.toStdWString();
      return advance(wstr.c_str(), int(wstr.length()));
    }

    /// Returns the face size of the font
    virtual int faceSize() const = 0;
    /// Sets the face size of the font
    virtual bool setFaceSize(int size, int resolution = POETIC_DEFAULT_RESOLUTION) = 0;

    /// Returns the ascender height
    virtual float ascender() const = 0;
    /// Returns the descender height
    virtual float descender() const = 0;
    /// Returns the line height
    virtual float lineHeight() const = 0;

    /// Computes the bounding box for the given string
    /// @param str the string to calculate the box for
    /// @param bbox the computed bounding box
    virtual void bbox(const char * str, BBox & bbox) = 0;
    /// @copydoc bbox
    virtual void bbox(const wchar_t * str, BBox & bbox) = 0;

    /// Loads a font from the given .ttf file
    virtual bool load(const char * fontFilePath) = 0;

    /// Creates a matching GPU font
    virtual GPUFont * createGPUFont() = 0;
    /// Returns a GPU font for the given CPU font
    GPUFont * getGPUFont();
  };

}

#endif
