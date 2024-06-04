:: A simpel install script for mttvdd, needs a path to install to as parameter
:: First it checks for elevation, needed for driverinstall "and systemwide powershell modules"
:: mttvdd-install.cmd d:\my_test\folder
:: extracts the sfx 7z into the suggested folder and sets a permanent system Env.Varibale for options.txt
:: installs the provided cert and then the driver
:: last it can add the ps-repo and needed moduels from it

@echo off
:: Check if parameter is given
if [%1]==[] goto usage

:: Check if running as administrator
>nul 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"

:: If not running as administrator, restart script with elevated privileges
if %errorlevel% neq 0 (
    echo Requesting administrator rights...
    powershell.exe -Command "Start-Process -Verb RunAs -FilePath '%~0'"
    exit /b
)

:: splitting the parameter into it's components
set "startpath=%~dp0"
set "fullpath=%~f1"
set "drive=%~d1"

:: Create target folder
md %fullpath%

:: Set systemvariable to target folder
setx IDD_SAMPLE_DRIVER_CONFIG %fullpath% /m

:changeing working dir for batchscript
%drive%
cd %fullpath%

:: unzipping the 7zip-sfx that contains, driver, cert, scripts and needed files for propper CLI based installation.
START /B /WAIT cmd /c "%startpath%\mtt-vdd-gpu11.exe -o%fullpath% -y"

:: Install certificate
call .\installCert.bat

:install part one of the drier
pnputil -i -a %fullpath%\IddSampleDriver.inf

:: Installs the other part of the driver, depends on vddcon.exe in bin folder(static compiled devcon.exe)
%fullpath%\bin\vddcon.exe install %fullpath%\iddsampledriver.inf ROOT\IddSampleDriver

:: Powershell settings and components needed for the scripts, works but looks ugly with red text
:: powershell.exe -ExecutionPolicy Bypass -Command "Register-PSRepository -Default -InstallationPolicy Trusted"
:: powershell.exe -ExecutionPolicy Bypass -Command "Install-Module -Name DisplayConfig -Force -Scope CurrentUser -Repository PSGallery"
:: powershell.exe -ExecutionPolicy Bypass -Command "Install-Module -Name MonitorConfig -Force -Scope CurrentUser -Repository PSGallery"
goto eof

:usage
@echo Usage: %0 ^<path to install to^>
exit /B 1

:eof
Echo Thank you
