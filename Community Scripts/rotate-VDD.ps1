# Self-elevate the script if required
if (-Not ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] 'Administrator')) {
    if ([int](Get-CimInstance -Class Win32_OperatingSystem | Select-Object -ExpandProperty BuildNumber) -ge 6000) {
        $CommandLine = "-File `"" + $MyInvocation.MyCommand.Path + "`" " + $MyInvocation.UnboundArguments
        Start-Process -FilePath PowerShell.exe -Verb Runas -ArgumentList $CommandLine
        Exit
    }
}

$repoName = "PSGallery"
$moduleName1 = "DisplayConfig"
$moduleName2 = "MonitorConfig"
if (!(Get-PSRepository -Name $repoName) -OR !(Find-Module -Name $moduleName1 | Where-Object {$_.Name -eq $moduleName1}) -OR !(Find-Module -Name $moduleName2 | Where-Object {$_.Name -eq $moduleName2})) {
    & .\dep_fix.ps1
}


# Import the local module
$dpth = Get-Module -ListAvailable $moduleName1 | select path | Select-Object -ExpandProperty path
import-module -name $dpth -InformationAction:Ignore -Verbose:$false -WarningAction:SilentlyContinue -Confirm:$false

# Import the other localmodule
$mpth = Get-Module -ListAvailable $moduleName2 | select path | Select-Object -ExpandProperty path
import-module -name $mpth -InformationAction:Ignore -Verbose:$false -WarningAction:SilentlyContinue -Confirm:$false

# Check if there are enough arguments
$numArgs = $args.Count
switch ($numArgs) {
	0 { Write-Error "This script requires at least 1, rotation in degrees ex. 90/180/270."; break }
	1 { $disp = Get-Monitor | Select-String -Pattern "LNX0000" | Select-Object LineNumber | Select-Object -ExpandProperty LineNumber
	    switch ($args[0]) {
		    90  { $degree = "Rotate90"
			      continue
			    }
			180 { $degree = "Rotate180"
			      continue
			    }
			270 { $degree = "Rotate270"
			      continue
			    }
			default	{ Write-Error "Invalid rotation angle(valid 90,180,270)."; break }
	}
    default { Write-Error "Invalid number of arguments: $numArgs"; break }
}
# Setting the resolution on the correct display.
Get-DisplayConfig | Set-DisplayRotation -DisplayId $disp -Rotation $degree | Use-DisplayConfig