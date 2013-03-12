/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include <portaudio.h>

#include <stdio.h>


int main(int, char **)
{
  Pa_Initialize();

  printf("API list:\n");

  for(int i = 0; i < Pa_GetHostApiCount(); i++) {
    const PaHostApiInfo* api = Pa_GetHostApiInfo(i);

    printf("API %d: %s\n", i, api->name);

  }

  printf("Audio device list:\n");

  for(int i = 0; i < Pa_GetDeviceCount(); i++) {
    const PaDeviceInfo * info = Pa_GetDeviceInfo(i);
    const PaHostApiInfo* api = Pa_GetHostApiInfo(info->hostApi);

    printf("Audio device %d: [%s], channels = %d-%d, API = %s\n", i, info->name,
           info->maxInputChannels, info->maxOutputChannels, api->name);
  }


  return 0;
}
