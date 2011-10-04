
#include <Radiant/DateTime.hpp>
#include <Radiant/Trace.hpp>

int main()
{
  Radiant::TimeStamp nowts = Radiant::TimeStamp::getTime();

  Radiant::DateTime tmp, nowdate(nowts);

  Radiant::info("Current date: %.2d.%.2d.%.4d\nCurrent time: %.2d:%.2d:%.2d",
                nowdate.monthDay() + 1, nowdate.month() + 1, nowdate.year(),
                nowdate.hour(), nowdate.minute(), nowdate.second());

  const char * datestr1 = "2011-10-12";
  const char * datestr2 = "2011-10--3";

  bool ok1 = tmp.fromString(datestr1, Radiant::DateTime::DATE_ISO);
  bool ok2 = tmp.fromString(datestr2, Radiant::DateTime::DATE_ISO);

  if(ok1)
    Radiant::info("Parsing good date-time string passed ok");
  else
    Radiant::error("Parsing good date-time string failed");

  if(!ok2)
    Radiant::info("Parsing corrupt date-time string failed (which is good)");
  else
    Radiant::error("Parsing corrupt date-time string passed (which is bad)");

  return 0;
}
