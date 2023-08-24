@echo off

set COMMAND=%1
set TESTFILE=%2

%COMMAND% %TESTFILE% || goto :error
%COMMAND% stdin <%TESTFILE% || goto :error
type %TESTFILE% | %COMMAND% stdin || goto :error
%COMMAND% stdout >%TESTFILE% || goto :error
%COMMAND% stdout | %COMMAND% stdin || goto :error

:error
exit /b %errorlevel%