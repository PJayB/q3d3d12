@echo off

set branchBaseDir=%~dp0\..
set q3BaseDir=%Q3INSTALLDIR%
set layoutDir=%~dp0..\projects\Build\Layout\Image\Loose

rem Validate the paths
if not exist "%branchBaseDir%" goto branchBaseDirFail
if not exist "%branchBaseDir%\baseq3" goto branchBaseDirFail
if not exist "%q3BaseDir%" goto q3BaseDirFail
if not exist "%q3BaseDir%\baseq3" goto q3BaseDirFail
if not exist "%layoutDir%" goto layoutDirFail

rem Create the layout directory if it doesn't exist
if not exist "%layoutDir%" mkdir "%layoutDir%"
if not exist "%layoutDir%" goto layoutDirFail

echo Q3 Installation Directory: %q3BaseDir%
echo Code Branch Assets Directory: %branchBaseDir%
echo Layout Directory: %layoutDir%

rem Copy the base assets to the layout directory (and only copy PK3s because 
rem we assume configs and editor content aren't required.)
xcopy "%q3BaseDir%\baseq3\*.pk3" "%layoutDir%\baseq3" /E /R /Y /D

rem Optionally copy missionpack
if exist "%q3BaseDir%\missionpack" (
  xcopy "%q3BaseDir%\missionpack\*.pk3" "%layoutDir%\missionpack" /E /R /Y /D
  )
  
rem For the files and folders in the branch we want to copy everything
xcopy "%branchBaseDir%\baseq3" "%layoutDir%" /E /R /Y /D /I
xcopy "%branchBaseDir%\missionpack" "%layoutDir%" /E /R /Y /D /I

echo Copy complete.

goto end

:branchBaseDirFail
echo Can't find the branch base dir (the baseq3 dir above 'code')
echo The path can't be found: %branchBaseDir%
goto end

:q3BaseDirFail
echo Can't find the assets directory using the Q3INSTALLDIR environment variable.
if "%Q3INSTALLDIR%"=="" ( 
  echo The environment variable isn't set.
  goto end
  )
echo The path can't be found: %q3BaseDir%
goto end

:layoutDirFail
echo Can't create or find the layout directory: %layoutDir%
goto end

:end
