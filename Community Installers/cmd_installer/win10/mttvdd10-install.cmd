:: A simpel install script for mttvdd, needs a path to install to as parameter
:: first it checks for elevation, needed for driverinstal and systemwide powershell modules
:: mttvdd-install.cmd d:\my_test\folder
:: extracts the sfx 7z into the suggested folder and sets a permanent system Env.Varibale for options.txt
:: installs the provided cert and then the driver
:: last it adds the ps-repo and needed moduels from it

@echo off
if [%1]==[] goto usage
REM Check if running as administrator
>nul 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"

REM If not running as administrator, restart script with elevated privileges
if %errorlevel% neq 0 (
    echo Requesting administrator rights...
    powershell.exe -Command "Start-Process -Verb RunAs -FilePath '%~0'"
    exit /b
)

md %~f1
setx IDD_SAMPLE_DRIVER_CONFIG %~f1 /m
%~d1:
cd %~p1
%~dp0\mttvdd10.exe -o"%~f1" -y
timeout 5
installCert.bat
bin\nefconw --install-driver --inf-path "IddSampleDriver.inf"
powershell.exe -ExecutionPolicy Bypass -Command "Register-PSRepository -Default -InstallationPolicy Trusted"
powershell.exe -ExecutionPolicy Bypass -Command "Install-Module -Name DisplayConfig -Force -Scope CurrentUser -Repository PSGallery"
powershell.exe -ExecutionPolicy Bypass -Command "Install-Module -Name MonitorConfig -Force -Scope CurrentUser -Repository PSGallery"
goto eof

:usage
@echo Usage: %0 ^<path to install to^>
exit /B 1

:eof
Echo Thank you
