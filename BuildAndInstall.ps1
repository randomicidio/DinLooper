$ErrorActionPreference = 'Stop'

$reaper = Get-Process -Name 'reaper' -ErrorAction SilentlyContinue

if ($null -ne $reaper)
{
    throw 'Close REAPER before building and installing DinLooper.'
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} `
    'Microsoft Visual Studio\Installer\vswhere.exe'

if (-not (Test-Path -LiteralPath $vswhere))
{
    throw 'Visual Studio Installer could not be found.'
}

$msbuild = & $vswhere `
    -latest `
    -products '*' `
    -requires Microsoft.Component.MSBuild `
    -find 'MSBuild\**\Bin\MSBuild.exe' |
    Select-Object -First 1

if ([string]::IsNullOrWhiteSpace($msbuild))
{
    throw 'MSBuild could not be found.'
}

$solution = Join-Path $PSScriptRoot `
    'Builds\VisualStudio2022\DinLooper.sln'

if (-not (Test-Path -LiteralPath $solution))
{
    throw 'DinLooper.sln could not be found. Regenerate it with Projucer.'
}

& $msbuild `
    $solution `
    '/t:Build' `
    '/p:Configuration=Debug' `
    '/p:Platform=x64' `
    '/m' `
    '/nologo'

if ($LASTEXITCODE -ne 0)
{
    throw "DinLooper build failed with exit code $LASTEXITCODE."
}

Write-Host ''
Write-Host 'DinLooper was built and installed in C:\VST\DinLooper.vst3.'
