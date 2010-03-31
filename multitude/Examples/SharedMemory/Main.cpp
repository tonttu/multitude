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

#include <Radiant/SHMPipe.hpp>
#include <Radiant/Sleep.hpp>
#include <Radiant/Trace.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <WinPort.h>
#endif

static const char * appname = 0;

static int bytes = 10000;
static int times = 10000;
static key_t key = 100;


void sendTest()
{    
  Radiant::info("Setting up shared memory buffer for writing");

  Radiant::SHMPipe * shm = new Radiant::SHMPipe(key, bytes);

  Radiant::info("Writing strings to the buffer");

  const char * strings[] = {
    "foo",
    "and",
    "bar",
    "are",
    "coder",
    "dreams",
    "Priests may not eat meat in secret. Priests may eat meat in public "
    "if they want (old Syriac canon).",
    0
  };
  
  int i, index = 0;

  Radiant::BinaryData bd;

  for(i = 0; i < times; i++) {
    const char * tmp = strings[index++];
    if(!tmp) {
      tmp = strings[0];
      index = 1;
    }

    bd.rewind();
    bd.writeString(tmp);

    
    int avail = shm->writeAvailable(200);
    if(avail < 200) {
      Radiant::info("Only %d bytes available for writing", avail);
    }
    else {
      shm->write(bd);
      shm->flush();
    }
    if(i % 100 == 0) {
      printf("+");
      fflush(0);
    }
  }

  Radiant::info("Deleting shared memory buffer after %d writes", i);
  
  delete shm;

  Radiant::info("Done");
}

void listenTest()
{    
  Radiant::info("Setting up shared memory buffer for listening");

  Radiant::SHMPipe * shm = new Radiant::SHMPipe(key, 0);

  Radiant::BinaryData bd;

  int i, fails = 0;

  std::string str;

  for(i = 0; true; i++) {
    
    if(shm->read(bd) <= 0) {
      fails++;

      if(fails > 10) {
	Radiant::info("SHMPipe read failed 10 times, maybe the sender is done");
	break;
      }

      Radiant::Sleep::sleepMs(50);
    }
    else
      fails = 0;

    if(i % 100 == 0) {
      bd.rewind();

      bd.readString(str);
      printf("str = %s\n", str.c_str());
      fflush(0);
    }
  }

  Radiant::info("Deleting shared memory buffer after %d reads", i);
  
  delete shm;

  Radiant::info("Done");
}

int main(int argc, char ** argv)
{

  bool sender = true;

  appname = argv[0];
  
  for(int i = 1; i < argc; i++) {
    if(strcmp(argv[i], "--key") == 0 && (i + 1) < argc)
      key = atoi(argv[++i]);
    else if(strcmp(argv[i], "--listener") == 0)
      sender = false;
    else if(strcmp(argv[i], "--verbose") == 0)
      Radiant::enableVerboseOutput(true);
    else
      printf("%s # Unknown argument \"%s\"\n", appname, argv[i]);
  }

  if(sender)
    sendTest();
  else
    listenTest();

  return 0;
}
