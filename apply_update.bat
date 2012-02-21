@ECHO OFF

ECHO Waiting fo Abendstern to exit...
PING -n 10 localhost >NUL 2>NUL

IF NOT EXIST bin\NUL MD bin

COPY /Y deferred\*.dll .
COPY /Y deferred\Abendstern.exe .
COPY /Y deferred\*.exe bin

DEL deferred\*.exe >NUL 2>NUL
DEL deferred\*.dll >NUL 2>NUL
RD  deferred

REM Remove old-named executables
DEL Abendstern_WGL32.exe >NUL 2>NUL
DEL Abendstern_W32GL21.exe >NUL 2>NUL

ECHO Update applied.
ECHO It is now safe to restart Abendstern.
PAUSE
EXIT
