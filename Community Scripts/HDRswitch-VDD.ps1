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
import-module -name $dpth -InformationAction:Ignore -Verbose:$false -WarningAction:SilentlyContinue 

# Import the other localmodule
$mpth = Get-Module -ListAvailable $moduleName2 | select path | Select-Object -ExpandProperty path
import-module -name $mpth -InformationAction:Ignore -Verbose:$false -WarningAction:SilentlyContinue 
 
$disp = Get-Monitor | Select-String -Pattern "LNX0000" | Select-Object LineNumber | Select-Object -ExpandProperty LineNumber
$bpcc = Get-DisplayColorInfo -DisplayId $disp | Select-Object BitsPerColorChannel | Select-Object -ExpandProperty BitsPerColorChannel
 
if ( $bpcc -is [int] ) {
    if ( $bpcc -gt 8 ) {
        Disable-DisplayAdvancedColor -DisplayId $disp
	    break
	}
	if ( $bpcc -lt 10 ) {
        Enable-DisplayAdvancedColor -DisplayId $disp
	    break
	}
}
else { write-error "Something went wrong in identifying the Colormode of the display." ; break }