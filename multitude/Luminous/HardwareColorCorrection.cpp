#include "HardwareColorCorrection.hpp"
#include "ColorCorrection.hpp"
#include "VM1.hpp"

namespace Luminous
{
  class HardwareColorCorrection::Private : public Valuable::Node
  {
  public:
    Private() : m_cc(0), m_ok(true)
    {
      eventAddIn("sync");
    }

    void processMessage(const QByteArray & type, Radiant::BinaryData & data)
    {
      if(type == "sync") doSync();
      else Node::processMessage(type, data);
    }

    void doSync() {
      if(m_cc && m_vm1.detected()) {
        m_vm1.setColorCorrection(*m_cc);
        m_ok = true;
      } else {
        m_ok = false;
      }
    }

    VM1 m_vm1;
    ColorCorrection * m_cc;
    bool m_ok;
  };

  HardwareColorCorrection::HardwareColorCorrection() : m_p(new Private)
  {
  }

  HardwareColorCorrection::~HardwareColorCorrection()
  {
    delete m_p;
  }

  void HardwareColorCorrection::syncWith(ColorCorrection * cc)
  {
    if(m_p->m_cc == cc) return;
    if(m_p->m_cc) {
      m_p->m_cc->eventRemoveListener(m_p);
    }
    m_p->m_cc = cc;
    if(m_p->m_cc) {
      m_p->m_cc->eventAddListener("changed", "sync", m_p, Valuable::Node::AFTER_UPDATE_ONCE);
      m_p->doSync();
    }
  }

  bool HardwareColorCorrection::ok() const
  {
    return m_p->m_ok;
  }

  DEFINE_SINGLETON(HardwareColorCorrection);
}
