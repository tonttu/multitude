#ifndef UNITTEST_DEFERREDTESTREPORTER_H
#define UNITTEST_DEFERREDTESTREPORTER_H

#include "Config.h"

#ifndef UNITTEST_NO_DEFERRED_REPORTER

#include "TestReporter.h"
#include "DeferredTestResult.h"

#include <vector>

namespace UnitTest
{

   class DeferredTestReporter : public TestReporter
   {
   public:
      UNITTEST_LINKAGE virtual void ReportTestStart(TestDetails const& details);
      UNITTEST_LINKAGE virtual void ReportFailure(TestDetails const& details, char const* failure);
      UNITTEST_LINKAGE virtual void ReportTestFinish(TestDetails const& details, float secondsElapsed);

      typedef std::vector< DeferredTestResult > DeferredTestResultList;
      UNITTEST_LINKAGE DeferredTestResultList& GetResults();

   private:
      DeferredTestResultList m_results;
   };

}

#endif
#endif
