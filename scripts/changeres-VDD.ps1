# Self-elevate the script if required
if (-Not ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] 'Administrator')) {
    if ([int](Get-CimInstance -Class Win32_OperatingSystem | Select-Object -ExpandProperty BuildNumber) -ge 6000) {
        $CommandLine = "-File `"" + $MyInvocation.MyCommand.Path + "`" " + $MyInvocation.UnboundArguments
        Start-Process -FilePath PowerShell.exe -Verb Runas -ArgumentList $CommandLine
        Exit
    }
}

# Import the necessary modules
Import-Module -Name "PowerShellGet"
Import-Module -Name "PackageManagement"

# Define the repository name and URL
$repoName = "PSGallery"
$repoUrl = "https://www.powershellgallery.com/api/v2"

# Check if the repository is registered
if (-not (Get-PSRepository -Name $repoName)) {
    # Register the repository
    Register-PSRepository -Name $repoName -SourceLocation $repoUrl -InstallationPolicy Trusted
}

# Define the module name and version
$moduleName1 = "DisplayConfig"
$version1 = "1.0.5"

# Define the module name and version
$moduleName2 = "MonitorConfig"
$version2 = "1.0.3"

# Check if the module is installed
if (-not (Get-InstalledModule -Name $moduleName1 -MinimumVersion $version1)) {
    # Install the module
    Install-Module -Name $moduleName1 -Repository $repoName -RequiredVersion $version1
}

# Check if the module is installed
if (-not (Get-InstalledModule -Name $moduleName2 -MinimumVersion $version2)) {
    # Install the module
    Install-Module -Name $moduleName2 -Repository $repoName -RequiredVersion $version2
}

# Import the module
Import-Module -Name $moduleName1 -RequiredVersion $version1

# Import the module
Import-Module -Name $moduleName2 -RequiredVersion $version2

# Use the module's functions and cmdlets in your script
# Check if there are enough arguments
$numArgs = $args.Count
switch ($numArgs) {
	0 { Write-Error "This script requires at least 2 arguments Xres Yres."; break }
	1 { Write-Error "This script requires at least 2 arguments Xres Yres."; break }
	2 { $disp = Get-Monitor | Select-String -Pattern "LNX" | Select-Object LineNumber | Select-Object -ExpandProperty LineNumber
	    $vres = $args[0]
		$hres = $args[1]
		$rate = 60
		continue
	}
    default { Write-Error "Invalid number of arguments: $numArgs"; break }
}
# Setting the resolution on the correct display.
Get-DisplayConfig | Set-DisplayResolution -DisplayId $disp -Width $vres -Height $hres | Set-DisplayRefreshRate -DisplayId $disp -RefreshRate $rate | Use-DisplayConfig