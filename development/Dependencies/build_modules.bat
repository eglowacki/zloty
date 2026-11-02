echo off
REM Build all libraries in this folder using Modules.build configuration. Yaget (c)2023

c:\EdgarDev\zloty\development\Dependencies\YagetTools\DependencyBuild\build.py --metafile=Modules --silent

C:\EdgarDev\zloty\development\Dependencies\sol3>python single\single.py --output ..\builds\sol3\include\sol\sol.hpp
