#ifndef UNITTEST_DEFERREDTESTRESULT_H
#define UNITTEST_DEFERREDTESTRESULT_H

#include "Config.h"
#ifndef UNITTEST_NO_DEFERRED_REPORTER

#include "HelperMacros.h"
#include <string>
#include <vector>

namespace UnitTest
{

   class DeferredTestFailure
   {
   public:
      UNITTEST_LINKAGE DeferredTestFailure();
      UNITTEST_LINKAGE DeferredTestFailure(int lineNumber_, const char* failureStr_);

      int lineNumber;
      std::string failureStr;
   };

}

namespace UnitTest
{

   class DeferredTestResult
   {
   public:
      UNITTEST_LINKAGE DeferredTestResult();
      UNITTEST_LINKAGE DeferredTestResult(char const* suite, char const* test);
      UNITTEST_LINKAGE ~DeferredTestResult();

      std::string suiteName;
      std::string testName;
      std::string failureFile;

      typedef std::vector< DeferredTestFailure > FailureVec;
      FailureVec failures;

      float timeElapsed;
      bool failed;
   };

}

#endif
#endif
