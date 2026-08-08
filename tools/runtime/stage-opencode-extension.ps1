[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Source,

    [Parameter(Mandatory = $true)]
    [string]$Destination,

    [Parameter(Mandatory = $true)]
    [string]$NpmExecutable,

    [string]$CacheDirectory = ''
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

function Get-FullPath([string]$Path)
{
    return [IO.Path]::GetFullPath($Path)
}

function Assert-ChildPath([string]$Child, [string]$Parent)
{
    $childPath = Get-FullPath $Child
    $parentPath = (Get-FullPath $Parent).TrimEnd([IO.Path]::DirectorySeparatorChar,
                                                 [IO.Path]::AltDirectorySeparatorChar)
    $prefix = $parentPath + [IO.Path]::DirectorySeparatorChar
    if (-not $childPath.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to replace path outside the extension staging directory: $childPath"
    }
}

function Get-SourceFingerprint([string]$SourcePath)
{
    $files = Get-ChildItem -LiteralPath $SourcePath -Recurse -File |
        Where-Object { $_.FullName -notmatch '[\\/]node_modules[\\/]' } |
        Sort-Object FullName
    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        foreach ($file in $files) {
            $relative = $file.FullName.Substring($SourcePath.Length).TrimStart('\', '/')
            $nameBytes = [Text.Encoding]::UTF8.GetBytes($relative.Replace('\', '/'))
            [void]$sha.TransformBlock($nameBytes, 0, $nameBytes.Length, $nameBytes, 0)
            $content = [IO.File]::ReadAllBytes($file.FullName)
            [void]$sha.TransformBlock($content, 0, $content.Length, $content, 0)
        }
        [void]$sha.TransformFinalBlock([byte[]]::new(0), 0, 0)
        return ([BitConverter]::ToString($sha.Hash)).Replace('-', '').ToLowerInvariant()
    } finally {
        $sha.Dispose()
    }
}

$sourcePath = Get-FullPath $Source
$destinationPath = Get-FullPath $Destination
$destinationParent = Split-Path -Parent $destinationPath
$npmPath = Get-FullPath $NpmExecutable
if ([string]::IsNullOrWhiteSpace($CacheDirectory)) {
    $CacheDirectory = Join-Path $destinationParent 'opencode-extension-npm-cache'
}
$cachePath = Get-FullPath $CacheDirectory

foreach ($required in @(
    (Join-Path $sourcePath 'package.json'),
    (Join-Path $sourcePath 'package-lock.json'),
    (Join-Path $sourcePath 'plugins\ai-mobile-test-studio.ts'),
    $npmPath
)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Required OpenCode extension input does not exist: $required"
    }
}

$fingerprint = Get-SourceFingerprint $sourcePath
$stampPath = Join-Path $destinationPath '.complete'
$expectedModule = Join-Path $destinationPath 'node_modules\@opencode-ai\plugin\package.json'
if ((Test-Path -LiteralPath $stampPath -PathType Leaf) -and
    (Test-Path -LiteralPath $expectedModule -PathType Leaf) -and
    ((Get-Content -Raw -LiteralPath $stampPath).Trim() -eq $fingerprint)) {
    Write-Host "OpenCode extension is current: $destinationPath"
    exit 0
}

New-Item -ItemType Directory -Force -Path $destinationParent, $cachePath | Out-Null
$temporaryPath = Join-Path $destinationParent ('.opencode-extension-' + [guid]::NewGuid().ToString('N'))
Assert-ChildPath $temporaryPath $destinationParent
New-Item -ItemType Directory -Path $temporaryPath | Out-Null

try {
    Copy-Item -Path (Join-Path $sourcePath '*') -Destination $temporaryPath -Recurse -Force
    $savedCache = [Environment]::GetEnvironmentVariable('NPM_CONFIG_CACHE', 'Process')
    try {
        [Environment]::SetEnvironmentVariable('NPM_CONFIG_CACHE', $cachePath, 'Process')
        $savedErrorActionPreference = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        $output = (& $npmPath ci --prefix $temporaryPath --omit=dev --ignore-scripts `
            --no-audit --no-fund --loglevel=error 2>&1 | Out-String).Trim()
        $npmExitCode = $LASTEXITCODE
        $ErrorActionPreference = $savedErrorActionPreference
        if ($npmExitCode -ne 0) {
            throw "Installing locked OpenCode extension dependencies failed: $output"
        }
    } finally {
        [Environment]::SetEnvironmentVariable('NPM_CONFIG_CACHE', $savedCache, 'Process')
    }

    [IO.File]::WriteAllText((Join-Path $temporaryPath '.complete'),
                            $fingerprint + [Environment]::NewLine,
                            (New-Object Text.UTF8Encoding($false)))
    if (Test-Path -LiteralPath $destinationPath) {
        Assert-ChildPath $destinationPath $destinationParent
        Remove-Item -Recurse -Force -LiteralPath $destinationPath
    }
    Move-Item -LiteralPath $temporaryPath -Destination $destinationPath
    Write-Host "Staged OpenCode extension at $destinationPath"
} finally {
    if (Test-Path -LiteralPath $temporaryPath) {
        Assert-ChildPath $temporaryPath $destinationParent
        Remove-Item -Recurse -Force -LiteralPath $temporaryPath
    }
}
