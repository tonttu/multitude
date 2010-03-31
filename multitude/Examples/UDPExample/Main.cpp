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

#include <Radiant/FileUtils.hpp>
#include <Radiant/Sleep.hpp>
#include <Radiant/TimeStamp.hpp>
#include <Radiant/Trace.hpp>
#include <Radiant/UDPSocket.hpp>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>


#ifndef WIN32

const char * strerror_s(const char *, int, int err)
{
  return strerror(err);
}

#endif

using namespace Radiant;

bool withreplies = false;
int iterations = 10;

const char * appname = 0;

float __duration = 10000000.0f;
Radiant::TimeStamp __began;

void sendTest(const char * host, int port, const char * message)
{
  printf("Setting up a UDP socket to %s:%d\n", host, port);

  UDPSocket socket;
  int err = socket.openClient(host, port);

  if(err) {
    const int  msgSize = 128;
    char  msgBuf[msgSize] = "";
    printf("%s cannot open UDP socket to %s:%d -> %s\n", 
	   appname, host, port, strerror_s(msgBuf, msgSize, err));
    return;
  }

  char buf[1024];

  printf("UDP socket ready %d\n", socket.isOpen());

  for(int i = 0; i < iterations; i++) {

    // Radiant::Sleep::sleepMs(500);

    sprintf(buf, "%s %d", message, i + 1);
    int len = strlen(buf) + 1;

    printf("Sending message: %s\n", buf);
    fflush(0);

    int n = socket.write(buf, len);

    if(n != len) {
      Radiant::error("Error writing to UDP socket (%d %s)", n, strerror(errno));
      break;      
    }

  }

  puts("Closing");

  socket.close();
}

void listenTest(const char * host, int port)
{
  printf("Setting up a listener socket to %s:%d\n", host, port);

  UDPSocket socket;
  int err;
  if((err = socket.openServer(host, port)) != 0) {
    printf("%s cannot open UDP socket to %s:%d -> %s\n", 
	   appname, host, port, strerror(err));
    return;
  }

  while(__began.since().secondsD() < __duration) {
    char buf[2048];
    bzero(buf, sizeof(buf));

    
    int n = socket.read(buf, sizeof(buf) - 1, false);
    
    buf[n] = 0;

    if(n > 0) {
      puts(buf);
      fflush(0);
    }
    else {
      Radiant::Sleep::sleepMs(10);

      if(n < 0) {
	Radiant::error("Error reading from UDP socket");
	// break;
      }
    }
  }

  Radiant::info("%s Closing socket", appname);

  socket.close();
}

int main(int argc, char ** argv)
{
  Radiant::TimeStamp startTime(Radiant::TimeStamp::getTime());

  const char * host = "localhost";
  const char * message = "Here we have a message";
  int port = 3456;
  bool islistener = false;
  bool isclient = true;
  bool withBlocking = true;

  appname = argv[0];

  __began = TimeStamp::getTime();

  for(int i = 1; i < argc; i++) {
    if(strcmp(argv[i], "--server") == 0)
      isclient = false;
    else if(strcmp(argv[i], "--listen") == 0)
      islistener = true;
    else if(strcmp(argv[i], "--host") == 0 && (i + 1) < argc)
      host = argv[++i];
    else if(strcmp(argv[i], "--port") == 0 && (i + 1) < argc)
      port = atoi(argv[++i]);
    else if(strcmp(argv[i], "--iterations") == 0 && (i + 1) < argc)
      iterations = atoi(argv[++i]);
    else if(strcmp(argv[i], "--message") == 0 && (i + 1) < argc)
      message = argv[++i];
    else if(strcmp(argv[i], "--messagefile") == 0 && (i + 1) < argc) {
      char * tmp = Radiant::FileUtils::loadTextFile(argv[++i]);
      if(tmp)
	message = tmp;
    }
    else if(strcmp(argv[i], "--time") == 0 && (i + 1) < argc)
      __duration = atof(argv[++i]);
    else if(strcmp(argv[i], "--withblocking") == 0)
      withBlocking = true;
    else if(strcmp(argv[i], "--withreplies") == 0)
      withreplies = true;
    else
      printf("%s # Unknown argument \"%s\"\n", appname, argv[i]);
  }

#ifdef WIN32
  // WinPort::initSockets();
#endif
  
  if(islistener)
    listenTest(host, port);
  else
    sendTest(host, port, message);

#ifdef WIN32
  // WinPort::exitSockets();
#endif

  printf("%s took %.2lf seconds\n", appname, startTime.since().secondsD());

  return 0;
}

