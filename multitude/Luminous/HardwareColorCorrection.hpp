#ifndef LUMINOUS_HARDWARECOLORCORRECTION_HPP
#define LUMINOUS_HARDWARECOLORCORRECTION_HPP

#include "Export.hpp"

#include <Patterns/NotCopyable.hpp>
#include <Radiant/Singleton.hpp>

/// @cond

namespace Luminous
{

  class ColorCorrection;

  // This class is internal to MultiTouch Ltd. Do not use this class.
  // It will be removed in future revisions.
  class LUMINOUS_API HardwareColorCorrection : public Patterns::NotCopyable
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

/// @endcond

#endif // LUMINOUS_HARDWARECOLORCORRECTION_HPP
