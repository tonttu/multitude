#ifndef POSTPROCESSFILTER_HPP
#define POSTPROCESSFILTER_HPP

#include <Valuable/Node.hpp>

#include <Luminous/Style.hpp>

namespace Luminous
{
  class PostProcessContext;
  class RenderContext;

  class LUMINOUS_API PostProcessFilter : public Valuable::Node
  {
  public:
    PostProcessFilter(Valuable::Node * host = 0, const QByteArray & name = "");
    virtual ~PostProcessFilter();

    virtual void update(float dt);

    virtual void initialize(Luminous::RenderContext & rc,
                            Luminous::PostProcessContext & pc) const;

    virtual void filter(Luminous::RenderContext & rc,
                        Luminous::PostProcessContext & pc,
                        Luminous::Style style = Luminous::Style()) const;

    bool enabled() const;
    void setEnabled(bool enabled);

  private:
    class D;
    D * m_d;
  };

  typedef std::shared_ptr<PostProcessFilter> PostProcessFilterPtr;
  typedef std::map<unsigned, PostProcessFilterPtr> PostProcessFilters;
}
#endif // POSTPROCESSFILTER_HPP
