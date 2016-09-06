#include "Checks.h"
#include <cstring>

#include <QByteArray>

namespace UnitTest {

namespace {

void CheckStringsEqual(TestResults& results, char const* expected, char const* actual, 
                       TestDetails const& details)
{
	using namespace std;

    if ((expected && actual) ? strcmp(expected, actual) : (expected || actual))
    {
        UnitTest::MemoryOutStream stream;
        stream << "Expected " << (expected ? expected : "<NULLPTR>") << " but was " << (actual ? actual : "<NULLPTR>");

        results.OnTestFailure(details, stream.GetText());
    }
}

}

void CheckEqual(TestResults& results, const QByteArray & expected,
                const QByteArray & actual, TestDetails const& details)
{
  CheckStringsEqual(results, expected.constData(), actual.constData(), details);
}

void CheckEqual(TestResults& results, const char * expected,
                const QByteArray & actual, TestDetails const& details)
{
  CheckStringsEqual(results, expected, actual.constData(), details);
}

void CheckEqual(TestResults& results, const QByteArray & expected,
                const char * actual, TestDetails const& details)
{
  CheckStringsEqual(results, expected.constData(), actual, details);
}


void CheckEqual(TestResults& results, char const* expected, char const* actual,
                TestDetails const& details)
{
    CheckStringsEqual(results, expected, actual, details);
}

void CheckEqual(TestResults& results, char* expected, char* actual,
                TestDetails const& details)
{
    CheckStringsEqual(results, expected, actual, details);
}

void CheckEqual(TestResults& results, char* expected, char const* actual,
                TestDetails const& details)
{
    CheckStringsEqual(results, expected, actual, details);
}

void CheckEqual(TestResults& results, char const* expected, char* actual,
                TestDetails const& details)
{
    CheckStringsEqual(results, expected, actual, details);
}


}
