@call H:\Programming\Microsoft\VisualStudio\VC\Auxiliary\Build\vcvars64.bat
H:
cd H:\tcl8.6.14\win
cls
nmake -f makefile.vc all OPTS=static
nmake -f makefile.vc install OPTS=static
pause
cd H:\tcl8.6.14\tk8.6.14\win
cls
nmake -f makefile.vc all OPTS=static TCLDIR=..\..
nmake -f makefile.vc install OPTS=static TCLDIR=..\..
pause