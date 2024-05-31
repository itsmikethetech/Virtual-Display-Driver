# Define the repository name and URL
$repoName = "PSGallery"
$repoUrl = "https://www.powershellgallery.com/api/v2"

# Check if the repository is registered
if (-not (Get-PSRepository -Name $repoName)) {
    # Register the repository 
	Register-PSRepository -Name $repoName -SourceLocation $repoUrl -Confirm:$false -InformationAction:Ignore -WarningAction:SilentlyContinue
}
#Trust the repo, either its allready installed or newly added
Set-PSRepository -Name $repoName -InstallationPolicy Trusted -InformationAction:Ignore -WarningAction:SilentlyContinue

# Define the module name and version
$moduleName1 = "DisplayConfig"
$version1 = "1.1.1"

# Define the module name and version
$moduleName2 = "MonitorConfig"
$version2 = "1.0.3"

# Check if the module1 is installed
if (-not (Get-InstalledModule -Name $moduleName1 -MinimumVersion $version1)) {
    # Save the module
    Save-Module -Name $moduleName1 -Repository $repoName -RequiredVersion $version1 -Confirm:$false -InformationAction:Ignore -WarningAction:SilentlyContinue
}

# Check if the module2 is installed
if (-not (Get-InstalledModule -Name $moduleName2 -MinimumVersion $version2)) {
    # Save the module
    Save-Module -Name $moduleName2 -Repository $repoName -RequiredVersion $version2 -Confirm:$false -InformationAction:Ignore -WarningAction:SilentlyContinue
}

