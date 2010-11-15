/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: Helsinki University of Technology, MultiTouch Oy and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include <Radiant/CSVDocument.hpp>
#include <Radiant/Trace.hpp>

#include <Valuable/CmdParser.hpp>
#include <Valuable/HasValues.hpp>
#include <Valuable/ValueString.hpp>

int main(int argc, char ** argv)
{
  Valuable::HasValues opts;
  Valuable::ValueString filename( & opts, "filename", "test.csv");

  Valuable::CmdParser::parse(argc, argv, opts);

  Radiant::CSVDocument doc;

  if(!doc.load(filename.str().c_str(), ",")) {
    Radiant::error("Could not load CSV file \"%s\"", filename.str().c_str());
    return -1;
  }

  Radiant::info("Loaded %s with %d rows, printing first 10 rows:",
                filename.str().c_str(), doc.rowCount());

  int i = 0;
  for(Radiant::CSVDocument::Rows::iterator it = doc.begin();
  (it != doc.end()) && (i < 10); it++) {

    Radiant::CSVDocument::Row & r = *it;

    printf("Row %d/%d: ", i+1, (int) doc.rowCount());

    for(Radiant::CSVDocument::Row::iterator it2 = r.begin(); it2 != r.end(); it2++) {
      std::string printable = Radiant::StringUtils::stdWstringAsUtf8(*it2);
      printf("[%s] ", printable.c_str());
    }

    printf("\n");

    i++;
  }

  return 0;
}

