# Builds the native viewer, runs regression tests, and creates a portable ZIP.
# No installs or changes to the system PATH are performed.
[CmdletBinding()]
param(
    [ValidateSet('Release', 'Debug', 'RelWithDebInfo')]
    [string]$Configuration = 'Release',
    [string]$BuildDirectory = 'build',
    [switch]$SkipTests,
    [switch]$SkipPackage
)
$ErrorActionPreference = 'Stop'
$sourceDirectory = Split-Path -Parent $PSScriptRoot
if (-not [IO.Path]::IsPathRooted($BuildDirectory)) {
    $BuildDirectory = Join-Path $sourceDirectory $BuildDirectory
}
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$visualStudioPath = $null
$visualStudioVersion = $null
if (Test-Path -LiteralPath $vswhere) {
    $visualStudioPath = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    $visualStudioVersion = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion
}
$cmakeCommand = Get-Command cmake.exe -ErrorAction SilentlyContinue
if ($cmakeCommand) {
    $cmake = $cmakeCommand.Source
} elseif ($visualStudioPath) {
    $cmake = Join-Path $visualStudioPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
} else {
    throw 'CMake was not found. Use Visual Studio 2019/2022 with Desktop development with C++ and C++ CMake tools, or add CMake 3.20+ to PATH.'
}
if (-not (Test-Path -LiteralPath $cmake)) {
    throw "CMake was not found at $cmake. Enable C++ CMake tools in your Visual Studio installation or add CMake 3.20+ to PATH."
}
$cmakeDirectory = Split-Path -Parent $cmake
$ctest = Join-Path $cmakeDirectory 'ctest.exe'
$cpack = Join-Path $cmakeDirectory 'cpack.exe'

function Invoke-Checked([string]$Executable, [string[]]$Arguments) {
    & $Executable @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Executable failed with exit code $LASTEXITCODE." }
}

$configureArguments = @('-S', $sourceDirectory, '-B', $BuildDirectory, '-DBUILD_TESTING=ON')
# Preserve the generator of an existing build. For new folders prefer the
# installed Visual Studio generator; no developer shell is required.
if (-not (Test-Path -LiteralPath (Join-Path $BuildDirectory 'CMakeCache.txt'))) {
    if ($visualStudioVersion -like '17.*') {
        $configureArguments += @('-G', 'Visual Studio 17 2022', '-A', 'x64')
    } elseif ($visualStudioVersion -like '16.*') {
        $configureArguments += @('-G', 'Visual Studio 16 2019', '-A', 'x64')
    }
}
Invoke-Checked $cmake $configureArguments
Invoke-Checked $cmake @('--build', $BuildDirectory, '--config', $Configuration, '--parallel')
if (-not $SkipTests) {
    Invoke-Checked $ctest @('--test-dir', $BuildDirectory, '-C', $Configuration, '--output-on-failure')
}
if (-not $SkipPackage) {
    Invoke-Checked $cpack @('--config', (Join-Path $BuildDirectory 'CPackConfig.cmake'), '-C', $Configuration, '-B', (Join-Path $BuildDirectory 'packages'))
}
Write-Host "Viewer: $(Join-Path $BuildDirectory "bin\$Configuration\Rasterizer.exe")"
if (-not $SkipPackage) { Write-Host "Packages: $(Join-Path $BuildDirectory 'packages')" }
