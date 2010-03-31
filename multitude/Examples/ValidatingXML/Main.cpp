#include <Valuable/Valuable.hpp>
#include <Valuable/DOMDocument.hpp>

#include <stdio.h>
#include <string.h>

using namespace Valuable;

int main(int argc, char ** argv)
{
  if(argc != 2) { 
    fprintf(stderr, "Usage: %s <file>\n", argv[0]);
    return 1;
  }

  Valuable::initialize();

  Radiant::RefPtr<DOMDocument> doc = DOMDocument::createDocument();

  if(!doc->readFromFile(argv[1], true))
    fprintf(stderr, "Failed to load '%s'\n", argv[1]);
  return 1;

  Valuable::terminate();

  return 0;
}
