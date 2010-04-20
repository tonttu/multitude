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

#include <Radiant/TimeStamp.hpp>
#include <Radiant/Trace.hpp>
#include <Radiant/UDPSocket.hpp>
#include <Radiant/Sleep.hpp>

#include <stdlib.h>
#include <string.h>
#include <strings.h>

using namespace Radiant;

int g_iterations = 1;

const char * appname = 0;

float g_duration = 10000000.0f;
Radiant::TimeStamp g_began;


void runRead(const char * host, int port)
{
    info("read mode");

  char buf[1024];

  UDPSocket socket;

  if(!socket.bind(host, port)) {
      error("failed to bind to %s %d", host, port);
      return;
  }

  for(int i = 0; i < g_iterations; i++) {
      std::string fromAddr;
      uint16_t fromPort;

      int gotBytes = socket.readDatagram(buf, 1024, &fromAddr, &fromPort);

      info("Got %d bytes (%s) from %s:%d", gotBytes, buf, fromAddr.c_str(), fromPort);

      Radiant::Sleep::sleepMs(500);
  }
}

void runSend(const char * host, int port, const char * message)
{
    info("send mode");
  UDPSocket socket;

  char buf[1024];

  // Write
  for(int i = 0; i < g_iterations; i++) {
      memset(buf, 0, 1024);

    info("Sending message '%s'", message);

    sprintf(buf, "%s %d", message, i + 1);

    int32_t len = strlen(buf) + 1;

    int written = socket.writeDatagram(buf, len, host, port);

    info("wrote %d bytes (%s) to %s:%d", written, buf, host, port);

    Radiant::Sleep::sleepMs(500);
  }

}

int main(int argc, char ** argv)
{
  Radiant::TimeStamp startTime(Radiant::TimeStamp::getTime());

  const char * host = "localhost";
  const char * message = "Here we have a message";
  int port = 3456;
  bool isread = false;

  appname = argv[0];

  g_began = TimeStamp::getTime();

  for(int i = 1; i < argc; i++) {
    if(strcmp(argv[i], "--read") == 0)
      isread = true;
    else if(strcmp(argv[i], "--host") == 0 && (i + 1) < argc)
      host = argv[++i];
    else if(strcmp(argv[i], "--port") == 0 && (i + 1) < argc)
      port = atoi(argv[++i]);
    else if(strcmp(argv[i], "--iterations") == 0 && (i + 1) < argc)
      g_iterations = atoi(argv[++i]);
    else if(strcmp(argv[i], "--message") == 0 && (i + 1) < argc)
      message = argv[++i];
    else if(strcmp(argv[i], "--time") == 0 && (i + 1) < argc)
      g_duration = atof(argv[++i]);
    else
      printf("%s # Unknown argument \"%s\"\n", appname, argv[i]);
  }

  if(isread)
    runRead(host, port);
  else
    runSend(host, port, message);


  printf("%s took %.2lf seconds\n", appname, startTime.since().secondsD());

  return 0;
}

