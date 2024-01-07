echo off
echo "C:\Program Files (x86)\Microsoft Visual Studio\Shared\Python39_64\python.exe" build.py --root=c:\Development\yaget\Dependencies --metafile=.\Sample.build %1 %2 %3
pause

REM optional parameters
REM 	--silent
REM 	--clean
REM 	--filter=<reg_expresion>
REM 	--display
