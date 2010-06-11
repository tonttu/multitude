#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char * argv[])
{
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    return 1;
  }
  char *tmp = malloc(strlen(argv[1])+5);
  strcpy(tmp, "src=");
  strcat(tmp, argv[1]);
  return execlp("nspluginplayer-mt", "nspluginplayer-mt", "--fullscreen", tmp, NULL);
}
