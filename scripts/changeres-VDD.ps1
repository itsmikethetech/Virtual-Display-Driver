# Self-elevate the script if required
if (-Not ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] 'Administrator')) {
    if ([int](Get-CimInstance -Class Win32_OperatingSystem | Select-Object -ExpandProperty BuildNumber) -ge 6000) {
        $CommandLine = "-File `"" + $MyInvocation.MyCommand.Path + "`" " + $MyInvocation.UnboundArguments
        Start-Process -FilePath PowerShell.exe -Verb Runas -ArgumentList $CommandLine
        Exit
    }
}
#Register PSGallery if not regitered
if (! (Get-PSRepository | where-object {$_.Name -eq "PSGallery"})) {
	Register-PSRepository -Name PSGalleryPreview -SourceLocation https://www.Preview.PowerShellGallery.Com/api/v2
	Find-Module DSC* -Repository PSGalleryPreview
	Install-Module DSCTestModule -Repository PSGalleryPreview -Verbose -Forse
}

#A module check and install if needed script
function Load-Module ($m) {
    # If module is not imported or not installed 
    if (Get-Module | Where-Object {$_.Name -eq $m}) {
        # If module is not imported, but available on disk then import
        if (Get-Module -ListAvailable | Where-Object {$_.Name -eq $m}) {
            Import-Module $m -Force    
        }
        else {
            # If module is not imported, not available on disk, but is in online gallery then install and import
            if (Find-Module -Name $m | Where-Object {$_.Name -eq $m}) {
                Install-Module -Name $m -Force 
                Import-Module $m Force
            }
        }
    }
}

Load-Module "MonitorConfig" 
Load-Module "DisplayConfig" 

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

# Getting the right display to set resolution
Get-DisplayConfig | Set-DisplayResolution -DisplayId $disp -Width $vres -Height $hres | Set-DisplayRefreshRate -DisplayId $disp -RefreshRate $rate | Use-DisplayConfig
