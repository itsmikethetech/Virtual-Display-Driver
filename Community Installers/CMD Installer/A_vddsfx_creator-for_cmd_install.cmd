:: needs three params: your local VDD-git repos path _-_ "Type of cpompile" D for debug | R for release _-_ OS version 10|11
:: example: mk_install_sfx.cmd c:\git\vdd R 11
:: needs 7zip instaleld at default path

@echo off

:: Check if three params are given
if [%3]==[] goto usage

:: Splitting the parameter into it's components
set "startpath=%~dp0"
set "fullpath=%~f1"
set "sdrive=%~d0"
set "drive=%~d1"
set "typ=%2"
set "wos=%3"
set "util=Community Installers\CMD Installer\util"
set "scrips=Community Scripts"


if [%typ%]=="D" set "kind=x64\Debug"
if [%typ%]=="R" set "kind=x64\Release"

if [%wos%]=="10" set "osdir=Virtual Display Driver (Non-HDR)"
if [%wos%]=="11" set "osdir=Virtual Display Driver (HDR)"

:creating temp directories
md %startpath%\ziptemp
md %startpath%\ziptemp\bin
md %startpath%\ziptemp\scripts

:: changeing workdir to git repo
%drive%
cd %fullpath%

:: copy the driver files and other stuff into a temporary fodler for zipping
cp %osdir%\%kind%\IddSampleDriver\* %startpath%\ziptemp
cp %osdir%\%kind%\IddSampleDriver.cer %startpath%\ziptemp
cp %fullpath%\%util%\util\installCert.bat %startpath%\ziptemp
cp %fullpath%\%util%\util\onoff_at_loginout %startpath%\ziptemp
cp %fullpath%\%util%\bin\vddcon.exe %startpath%\ziptemp\bin
cp %fullpath%\%scripts%\*.*  %startpath%\ziptemp\scripts

%sdrive%
cd %startpath%\ziptemp
:: will create a 7zip zfx that you can install on other computers with the insatll script
"c:\Program Files\7-Zip\7z.exe" a -r -sfx -t7z %startpath%\mttvdd_win%wos%.exe *

