#include <stdio.h>
#include <process.h>


int main(int argc, const char **argv)
{
  char command[1024];

  if (argc != 4) {
    fprintf(stderr, "Invalid arg count %d\n", argc);
    return 1;
  }

  snprintf(command, sizeof(command), "cmd.exe /C \"\"%s\" \"%s\" \"%s\"\"", argv[1], argv[2], argv[3]);
  fprintf(stderr, "Running test file command: '%s'\n", command);

  return system(command);
}
