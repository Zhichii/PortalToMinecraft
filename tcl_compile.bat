echo Thanks to Python! Build environments: x86, amd64, x86_amd64
echo.
set _ARGS=%*
if NOT DEFINED _ARGS set _ARGS=amd64

if not exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" goto :skip_vswhere
set VSTOOLS=
for /F "tokens=*" %%i in ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -property installationPath -latest -prerelease -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64') DO @(set VSTOOLS=%%i\VC\Auxiliary\Build\vcvarsall.bat)
if not defined VSTOOLS goto :skip_vswhere
call "%VSTOOLS%" %_ARGS%
goto :compile

:skip_vswhere
if not defined VSTOOLS set VSTOOLS=%VS140COMNTOOLS%
if not defined VSTOOLS set VSTOOLS=%VS120COMNTOOLS%
if not defined VSTOOLS set VSTOOLS=%VS110COMNTOOLS%
if not defined VSTOOLS set VSTOOLS=%VS100COMNTOOLS%
call "%VSTOOLS%..\..\VC\vcvarsall.bat" %_ARGS%

:compile
set _TCL_SOURCE=H:\Libraries\tcl-sources
set _TK_SOURCE=H:\Libraries\tcl-sources\tk-sources
set _OPTIONS=static
set _PATH=%cd%
set _INSTALL_DIR=H:\Libraries\Tcl

pause 
cls
cd /d %_TCL_SOURCE%\win
nmake -f makefile.vc all OPTS=%_OPTIONS%
nmake -f makefile.vc install OPTS=%_OPTIONS% INSTALLDIR=%_INSTALL_DIR%
pause
cls
cd /d %_TK_SOURCE%\win
nmake -f makefile.vc all OPTS=%_OPTIONS% TCLDIR=%_TCL_SOURCE%
nmake -f makefile.vc install OPTS=%_OPTIONS% TCLDIR=%_TCL_SOURCE% INSTALLDIR=%_INSTALL_DIR%
pause