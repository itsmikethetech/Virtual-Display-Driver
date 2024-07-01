@echo off
setlocal enabledelayedexpansion
:: needs three params: your local VDD-git repos path _-_ "Type of cpompile" D for debug | R for release _-_ OS version 10|11
:: example: mk_install_sfx.cmd c:\git\vdd R 11
:: needs 7zip instaleld at default path

:: Check if three params are given
if [%3]==[] goto usage

:: Splitting the parameter into its components
set "startpath=%~dp0"
set "fullpath=%~f1"
set "sdrive=%~d0"
set "drive=%~d1"
set "typ=%2"
set "wos=%3"
set "util=Community Installers\CMD Installer\util"
set "tool=Community Installers\CMD Installer\tool"
set "scripts=Community Scripts"

if /I "%typ%"=="D" (
    set "kind=x64\Debug"
) else if /I "%typ%"=="R" (
    set "kind=x64\Release"
) else (
    echo Invalid type parameter. Use D for debug or R for release.
    goto usage
)

if /I "%wos%"=="10" (
    set "osdir=Virtual Display Driver (Non-HDR)"
) else if /I "%wos%"=="11" (
    set "osdir=Virtual Display Driver (HDR)"
) else (
    echo Invalid OS version parameter. Use 10 or 11.
    goto usage
)

:creating temp directories
md "%startpath%ziptemp"
md "%startpath%ziptemp\bin"
md "%startpath%ziptemp\scripts"
md "%startpath%ziptemp\utils"
md "%startpath%ziptemp\utils\onoff_at_loginout"

:: changing workdir to git repo
%drive%
cd "%fullpath%"

:: copy the driver files and other stuff into a temporary folder for zipping
copy "!osdir!\option.txt" "%startpath%ziptemp"
copy "!osdir!\!kind!\IddSampleDriver\*" "%startpath%ziptemp"
copy "!osdir!\!kind!\IddSampleDriver.cer" "%startpath%ziptemp"
copy "%fullpath%\%util%\installCert.bat" "%startpath%ziptemp"
copy "%fullpath%\%util%\onoff_at_loginout\*" "%startpath%ziptemp\utils\onoff_at_loginout"
copy "%fullpath%\%tool%\vddcon.exe.rename" "%startpath%ziptemp\bin\vddcon.exe"
copy "%fullpath%\%scripts%\*.*" "%startpath%ziptemp\scripts"

%sdrive%
cd "%startpath%ziptemp"
:: will create a 7zip sfx that you can install on other computers with the install script
"c:\Program Files\7-Zip\7z.exe" a -r -sfx -t7z "%startpath%mttvdd_win%wos%.exe" *
goto eof

:usage
@echo Usage: %0 ^<path to local git-repo^> ^<D or R^>  ^<10 or 11^>
@echo "1. d:\git\vdd"
@echo "2. D for the last debug compile / R for last release compile"
@echo "3. 10 for NON-HDR compile or 11 for a HDR compile"
exit /B 1

:eof
echo Thank you
endlocal
