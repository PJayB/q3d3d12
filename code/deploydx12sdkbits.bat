@echo off

if "%DX12SDKDIR%"=="" goto skip
if not exist "%DX12SDKDIR%" goto skip

xcopy /D /Y /S "%DX12SDKDIR%\Bin\x64\*.*" %1
goto end

:skip
echo Skipping DX12 bits deploy.

:end
