@echo off
setlocal

:: Check if parameter is given
if [%1]==[] goto usage

:: Ensure the script is running as administrator
>nul 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"
if %errorlevel% neq 0 (
    echo Requesting administrator rights...
    powershell.exe -Command "Start-Process -Verb RunAs -FilePath '%~0' -ArgumentList '%*'"
    exit /b
)

:: Splitting the parameter into its components
set "startpath=%~dp0"
set "fullpath=%~f1"
set "drive=%~d1"

:: Create target folder
md %fullpath%

:: Variables for PowerShell installation
set "pwshInstaller=https://github.com/PowerShell/PowerShell/releases/download/v7.4.3/PowerShell-7.4.3-win-x64.msi"
set "pwshInstallerName=PowerShell-7.4.3-win-x64.msi"
set "pwshInstallPath=%ProgramFiles%\PowerShell\7"
set "pwshExe=%pwshInstallPath%\pwsh.exe"
set "pwshdest=%startpath%\%pwshInstallerName%"

:: Check if PowerShell 7 is already installed
if exist "%pwshExe%" (
    echo PowerShell 7 is already installed.
) else (
    echo Downloading and installing PowerShell 7...
    bitsadmin /transfer "pwshDownloadJob" /priority FOREGROUND /DOWNLOAD "%pwshInstaller%" "%pwshdest%"
    msiexec /i "%pwshInstallerName%" /quiet /norestart

    :: Verify PowerShell installation
    if exist "%pwshExe%" (
        echo PowerShell installed successfully.
    ) else (
        echo PowerShell installation failed.
        goto :eof
    )
)

:: Set the execution policy to Bypass for the current process to install modules systemwide
"%pwshExe%" -NoProfile -ExecutionPolicy Bypass -Command "Set-ExecutionPolicy -ExecutionPolicy Bypass -Scope Process -Force; Install-Module -Name DisplayConfig -Scope CurrentUser -Force -AllowClobber; Install-Module -Name MonitorConfig -Scope CurrentUser -Force -AllowClobber"

:: Set system variable to target folder
setx IDD_SAMPLE_DRIVER_CONFIG %fullpath% /m
setx VDDPATH %fullpath% /m

:: Change working dir for batch script
%drive%
cd %fullpath%

:: Unzipping the 7zip-sfx that contains, driver, cert, scripts and needed files for proper CLI-based installation.
START /B /WAIT cmd /c "%startpath%\mttvdd_win10.exe -o%fullpath% -y"

:: Install certificate
call .\installCert.bat

:: Sign PowerShell scripts
echo Signing PowerShell scripts...
"%pwshExe%" -ExecutionPolicy Bypass -Command "Get-ChildItem -Path '%fullpath%\scripts' -Filter *.ps1 |
ForEach-Object { Set-AuthenticodeSignature -FilePath $_.FullName -Certificate
%fullpath%\VDD.cer }"

:: Install part one of the driver
pnputil -i -a %fullpath%\VDD.inf

:: Install the other part of the driver, depends on vddcon.exe in bin folder (static compiled devcon.exe)
%fullpath%\bin\vddcon.exe install %fullpath%\VDD.inf ROOT\VDDbyMTT

goto eof

:usage
@echo Usage: %0 ^<path to install to^>
exit /B 1

:eof
echo Thank you
endlocal
