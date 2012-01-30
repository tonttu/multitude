#ifndef LUMINOUS_HARDWARECOLORCORRECTION_HPP
#define LUMINOUS_HARDWARECOLORCORRECTION_HPP

#include <Patterns/NotCopyable.hpp>
#include <Radiant/Singleton.hpp>

namespace Luminous
{
  class ColorCorrection;
  class HardwareColorCorrection : public Patterns::NotCopyable
  {
    DECLARE_SINGLETON(HardwareColorCorrection);
  public:
    HardwareColorCorrection();
    ~HardwareColorCorrection();
    void syncWith(ColorCorrection * cc);
    bool ok() const;

  private:
    class Private;
    Private * m_p;
  };

}

#endif // LUMINOUS_HARDWARECOLORCORRECTION_HPP
