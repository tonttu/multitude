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
#include <Radiant/TCPServerSocket.hpp>
#include <Radiant/TCPSocket.hpp>

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
int iterations = 1;

const char * appname = 0;

float __duration = 10000000.0f;
Radiant::TimeStamp __began;


void runServer(const char * host, int port, bool withBlocking)
{
  printf("Setting up a server socket to %s:%d\n", host, port);

  char buf[1024];

  TCPServerSocket server;
  int i, err = server.open(host, port, 5);
// err = server.open(0, port, 5)
  if(err) {
    const int  msgSize = 128;
    char  msgBuf[msgSize] = "";
    printf("%s cannot open server socket to %s:%d -> %s\n", 
	   appname, host, port, strerror_s(msgBuf, msgSize, err));
    return;
  }
  
  for(i = 0; i < 10; i++) {
    puts("Waiting for a connection");

    while(withBlocking && !server.isPendingConnection(1000000)) {
      printf(".");
      fflush(0);
    }

    TCPSocket * socket = server.accept();

    if(!socket) {
      Radiant::error("Could not create accept a socket connection.");
      return;
    }

    socket->setNoDelay(true);

    printf("Got a new socket %p\n", socket);
    fflush(0);

    for(int j = 0; j < iterations; j++) {
      int32_t len = 0;
      buf[0] = '\0';
      int n = socket->read( & len, 4);
      
      if(n != 4) {
	Radiant::error("Could not read 4 header bytes from the socket, got %d", n);
	delete socket;
	return;
      }
      
      n = socket->read(buf, len);
      
      if(n != len) {
	Radiant::error("Could not read %d data bytes from the socket, got %d",
		       len, n);
      }
      
      printf("Received \"%s\"\n", buf);

      if(withreplies) {
	socket->write( & len, 4);
	socket->write( & buf, len);
      }
    }

    delete socket;
  }
  
  fflush(0);

  info("%s %d clients handled, returning", appname, i);
}

void runClient(const char * host, int port, const char * message)
{
  printf("Setting up a client socket to %s:%d\n", host, port);

  TCPSocket socket;
  int err = socket.open(host, port);

  if(err) {
    const int  msgSize = 128;
    char  msgBuf[msgSize] = "";
    printf("%s cannot open client socket to %s:%d -> %s\n", 
	   appname, host, port, strerror_s(msgBuf, msgSize, err));
    return;
  }

  socket.setNoDelay(true);

  char buf[1024];

  for(int i = 0; i < iterations; i++) {

    puts("Sending message");

    sprintf(buf, "%s %d", message, i + 1);
    
    int32_t len = strlen(buf) + 1;
    socket.write( & len, 4);
    socket.write(buf, len);

    if(withreplies) {
      len = 0;
      socket.read( & len, 4);
      bzero(buf, sizeof(buf));
      socket.read(buf, len);
      printf("Received reply \"%s\"\n", buf);
    }
  }

  puts("Closing");

  socket.close();
}

void runListener(const char * host, int port, const char *)
{
  printf("Setting up a listener socket to %s:%d\n", host, port);

  TCPSocket socket;
  int err;
  if((err = socket.open(host, port)) != 0) {
    printf("%s cannot open client socket to %s:%d -> %s\n", 
	   appname, host, port, strerror(err));
    return;
  }

  while(__began.since().secondsD() < __duration) {
    char buf[2048];
    bzero(buf, sizeof(buf));

    socket.read(buf, sizeof(buf) - 1, false);
    
    // buf[n] = 0;

    printf("%s", buf);
    fflush(0);
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
    runListener(host, port, message);
  else if(isclient)
    runClient(host, port, message);
  else
    runServer(host, port, withBlocking);

#ifdef WIN32
  // WinPort::exitSockets();
#endif

  printf("%s took %.2lf seconds\n", appname, startTime.since().secondsD());

  return 0;
}

