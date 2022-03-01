@echo off

set action=%1
set input=%2
set output=%3
set extra=%4 %5 %6 %7 %8 %9

set qabi=%~dp0\qabigen.exe

pushd %~dp0..\tools\QAbiGen

if "%action%" == "build" goto build
if "%action%" == "rebuild" goto rebuild
if "%action%" == "clean" goto clean
echo Unknown action: %action%
exit 1

:build
mkdir %output% 2> nul
%qabi% %input% /o %output% %extra%
goto end

:rebuild
mkdir %output% 2> nul
%qabi% %input% /o %output% /f %extra%
goto end

:clean
del /Q %output%\*.h
goto end

:end
popd
