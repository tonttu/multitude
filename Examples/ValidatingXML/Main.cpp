/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

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

  std::shared_ptr<DOMDocument> doc(DOMDocument::createDocument());

  if(!doc->readFromFile(argv[1], true))
    fprintf(stderr, "Failed to load '%s'\n", argv[1]);
  return 1;

  return 0;
}
