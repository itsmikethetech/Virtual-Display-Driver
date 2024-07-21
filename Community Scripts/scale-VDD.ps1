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
	0 { Write-Error "This script requires a scale 'reset' or an accepted percfentage value (100, 125, 150, 175, 200, 225, 250, 300, 350, 400, 450, 500) limited by the device max scale."; break }
	1 { $disp = Get-Monitor | Select-String -Pattern "MTT1337" | Select-Object LineNumber | Select-Object -ExpandProperty LineNumber
	    $maxscale = Get-DisplayScale -DisplayId $disp | Select-Object -ExpandProperty MaxScale
		if ( $args[0] -is [int] ) {
		    if ( $args[0] -le $maxscale -And $args[0] -ge 100 ) {
                Set-DisplayScale -DisplayId $disp -Scale $args[0]
				break
			}
			else { 
			    Write-Error "Invalid scale percentage." 
			    break
			}
		}
		else {
		    if ( $args[0] -eq "reset" ) {
		       Set-DisplayScale -DisplayId $disp -Recommended
			   break
		    }
			else {
			    Write-Error "Invalid arguments: 'reset' or a valid percentage value like 150 is acceptable."
				break
			}
        }
	}
    default { Write-Error "Invalid number of arguments: $numArgs"; break }
}
