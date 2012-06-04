#if !defined (LUMINOUS_TEXTUREARRAY_HPP)
#define LUMINOUS_TEXTUREARRAY_HPP

namespace Luminous
{
  class TextureArray : public Texture
  {
  public:
    TextureArray(RenderResource::Id id, RenderDriver & driver);
    ~TextureArray();

    // Sets the number of levels (textures) in the array
    void setLevelCount(unsigned int levels);
    // Returns the number of levels (textures)
    unsigned int levelCount() const;

    // Sets the currently active level
    void setLevel(unsigned int level);
    // Returns the currently active level
    unsigned int level() const;
  private:
    class D;
    D * m_d;
  };
}
#endif // LUMINOUS_TEXTUREARRAY_HPP
