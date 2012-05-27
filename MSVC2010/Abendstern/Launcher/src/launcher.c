#ifndef WINVER
#define WINVER 0x0510
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT WINVER
#endif

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS WINVER
#endif

//#ifndef _WIN32_IE                       // Specifies that the minimum required platform is Internet Explorer 7.0.
//#define _WIN32_IE 0x0700        // Change this to the appropriate value to target other versions of IE.
//#endif

#include <windows.h>
#include <SDL.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main(int argc, const char* argv[]) {
  char cmdline[4096], logfile[1024];
  char exe[] = "bin\\abw32gl14.exe";
  int i;
  STARTUPINFOA sinfo = {
    sizeof(STARTUPINFO),
    0 //Init rest with zeros as well
  };
  PROCESS_INFORMATION info;
  FILE* truncator;
  // Truncate the log files before starting
  sprintf(logfile, "%s\\.abendstern\\log.txt", getenv("APPDATA"));
  truncator = fopen(logfile, "w");
  if (truncator) fclose(truncator);
  sprintf(logfile, "%s\\.abendstern\\launchlog.txt", getenv("APPDATA"));
  truncator = fopen(logfile, "w");
  if (truncator) fclose(truncator);
  strcpy(cmdline, "bin\\abw32gl14.exe");
  strcat(cmdline, " ");
  for (i=1; i<argc; ++i) {
    strcat(cmdline, argv[i]);
    strcat(cmdline, " ");
  }
  printf("cmdline: %s\n", cmdline);
  CreateProcessA(exe, cmdline, NULL, NULL, FALSE, CREATE_BREAKAWAY_FROM_JOB, NULL, NULL, &sinfo, &info);
  CloseHandle(info.hProcess);
  CloseHandle(info.hThread);
  return 0;
}
