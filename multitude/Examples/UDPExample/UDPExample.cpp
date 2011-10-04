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
#include <cassert>
#include <string.h>
#include <strings.h>

using namespace Radiant;

int g_iterations = 30;

const char * appname = 0;

float g_duration = 10000000.0f;
Radiant::TimeStamp g_began;


void runRead(int port)
{
  info("read mode at port %d", port);

  char buf[1024];

  UDPSocket socket;

  int err = socket.openServer(port);
  if(err) {
    error("failed to bind to %d (%s)", port, strerror(err));
    return;
  }

  int totalBytes = 0;
  for(int i = 0; i < g_iterations; i++) {
    // QString fromAddr(host);
    // uint16_t fromPort(port);

    buf[0] = 0;
    int gotBytes = socket.read(buf, 1024, true);

    info("Got %d bytes (%s)", gotBytes, buf);
    if(gotBytes > 0) totalBytes += gotBytes;
  }

  info("done, received %d bytes", totalBytes);
}

void runSend(const char * host, int port, const char * message)
{
  info("send mode to %s:%d", host, port);
  UDPSocket socket;

  int err = socket.openClient(host, port);
  if(err) {
    error("failed to connect to %s:%d (%s)",
          host, port, strerror(err));
    return;
  }

  char buf[1024];

  // Write
  int totalBytes = 0;
  for(int i = 0; i < g_iterations; i++) {
    memset(buf, 0, 1024);

    info("Sending message '%s'", message);

    sprintf(buf, "%s %d", message, i + 1);

    int32_t len = strlen(buf) + 1;

    int written = socket.write(buf, len);
    assert(written == len);

    info("wrote %d bytes (%s) to %s:%d", written, buf, host, port);

    if(written <= 0) {

    } else totalBytes += written;

    //Radiant::Sleep::sleepMs(500);
  }

  info("done, sent %d bytes", totalBytes);
}

int main(int argc, char ** argv)
{
  Radiant::TimeStamp startTime(Radiant::TimeStamp::getTime());

  const char * host = "127.0.0.1";
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
    runRead(port);
  else
    runSend(host, port, message);


  printf("%s took %.2lf seconds\n", appname, startTime.since().secondsD());

  return 0;
}

