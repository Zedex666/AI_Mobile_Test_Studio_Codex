[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Destination,

    [string]$LockFile = '',

    [string]$CacheDirectory = ''
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
[Net.ServicePointManager]::SecurityProtocol =
    [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12

if ([string]::IsNullOrWhiteSpace($LockFile)) {
    $LockFile = Join-Path $PSScriptRoot 'runtime-lock.json'
}
if ([string]::IsNullOrWhiteSpace($CacheDirectory)) {
    $CacheDirectory = Join-Path $PSScriptRoot '.cache'
}

function Get-FullPath([string]$Path)
{
    return [IO.Path]::GetFullPath($Path)
}

function Assert-RemovableChildPath([string]$Child, [string]$Parent)
{
    $childPath = Get-FullPath $Child
    $parentPath = (Get-FullPath $Parent).TrimEnd([IO.Path]::DirectorySeparatorChar,
                                                 [IO.Path]::AltDirectorySeparatorChar)
    $prefix = $parentPath + [IO.Path]::DirectorySeparatorChar
    if (-not $childPath.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to replace path outside the runtime staging directory: $childPath"
    }
}

function Get-Component([object]$Lock, [string]$Id)
{
    $component = $Lock.components | Where-Object { $_.id -eq $Id } | Select-Object -First 1
    if ($null -eq $component) {
        throw "Runtime lock is missing component '$Id'."
    }
    return $component
}

function Test-Hash([string]$Path, [string]$ExpectedHash)
{
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return $false
    }
    $actualHash = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
    return $actualHash.Equals($ExpectedHash, [StringComparison]::OrdinalIgnoreCase)
}

function Get-LockedArchive([object]$Component, [string]$CacheRoot)
{
    $archivePath = Join-Path $CacheRoot $Component.archive
    if (Test-Hash $archivePath $Component.sha256) {
        return $archivePath
    }
    if (Test-Path -LiteralPath $archivePath) {
        Remove-Item -Force -LiteralPath $archivePath
    }

    $partialPath = "$archivePath.partial-$PID"
    Write-Host "Downloading $($Component.id) $($Component.version)..."
    try {
        $curl = Get-Command curl.exe -ErrorAction SilentlyContinue
        if ($null -ne $curl) {
            & $curl.Source --fail --location --silent --show-error `
                --retry 4 --retry-delay 2 --connect-timeout 30 `
                --output $partialPath $Component.url
            if ($LASTEXITCODE -ne 0) {
                throw "curl failed to download $($Component.url) (exit code $LASTEXITCODE)."
            }
        } else {
            $client = New-Object Net.WebClient
            $client.Headers['User-Agent'] = 'AI-Mobile-Test-Studio-Runtime-Stager'
            try {
                $client.DownloadFile([Uri]$Component.url, $partialPath)
            } finally {
                $client.Dispose()
            }
        }
    } catch {
        Remove-Item -Force -LiteralPath $partialPath -ErrorAction SilentlyContinue
        throw
    }

    if (-not (Test-Hash $partialPath $Component.sha256)) {
        Remove-Item -Force -LiteralPath $partialPath -ErrorAction SilentlyContinue
        throw "SHA-256 verification failed for $($Component.archive)."
    }
    Move-Item -Force -LiteralPath $partialPath -Destination $archivePath
    return $archivePath
}

function Expand-TarGzip([string]$Archive, [string]$OutputDirectory)
{
    $cmake = Get-Command cmake -ErrorAction Stop
    Push-Location $OutputDirectory
    try {
        & $cmake.Source -E tar xzf $Archive
        if ($LASTEXITCODE -ne 0) {
            throw "CMake failed to extract $Archive (exit code $LASTEXITCODE)."
        }
    } finally {
        Pop-Location
    }
}

function Expand-LockedArchive([string]$Archive, [string]$OutputDirectory)
{
    if ($Archive.EndsWith('.zip', [StringComparison]::OrdinalIgnoreCase)) {
        Expand-Archive -LiteralPath $Archive -DestinationPath $OutputDirectory
        return
    }
    if ($Archive.EndsWith('.tgz', [StringComparison]::OrdinalIgnoreCase) -or
        $Archive.EndsWith('.tar.gz', [StringComparison]::OrdinalIgnoreCase)) {
        Expand-TarGzip $Archive $OutputDirectory
        return
    }
    throw "Unsupported runtime archive format: $Archive"
}

function Write-Utf8Json([string]$Path, [object]$Value)
{
    $json = $Value | ConvertTo-Json -Depth 8
    $encoding = New-Object Text.UTF8Encoding($false)
    [IO.File]::WriteAllText($Path, $json + [Environment]::NewLine, $encoding)
}

$lockPath = Get-FullPath $LockFile
$destinationPath = Get-FullPath $Destination
$destinationParent = Split-Path -Parent $destinationPath
$cachePath = Get-FullPath $CacheDirectory
if (-not (Test-Path -LiteralPath $lockPath -PathType Leaf)) {
    throw "Runtime lock does not exist: $lockPath"
}
if ([string]::IsNullOrWhiteSpace($destinationParent)) {
    throw "Runtime destination must have a parent directory: $destinationPath"
}

$lock = Get-Content -Raw -Encoding UTF8 $lockPath | ConvertFrom-Json
if ($lock.schemaVersion -ne 1 -or $lock.platform -ne 'windows-x64') {
    throw "Unsupported runtime lock schema or platform in $lockPath"
}
$lockHash = (Get-FileHash -LiteralPath $lockPath -Algorithm SHA256).Hash.ToLowerInvariant()
$manifestPath = Join-Path $destinationPath 'manifest.json'
$requiredPaths = @(
    (Join-Path $destinationPath 'opencode\opencode.exe'),
    (Join-Path $destinationPath 'node\node.exe'),
    (Join-Path $destinationPath 'node\node_modules\node-pty\package.json')
)
if (Test-Path -LiteralPath $manifestPath -PathType Leaf) {
    $currentManifest = Get-Content -Raw -Encoding UTF8 $manifestPath | ConvertFrom-Json
    $currentLockProperty = $currentManifest.PSObject.Properties['lockSha256']
    $missingPaths = @($requiredPaths | Where-Object {
        -not (Test-Path -LiteralPath $_ -PathType Leaf)
    })
    $allFilesExist = $missingPaths.Count -eq 0
    if ($null -ne $currentLockProperty -and
        $currentLockProperty.Value -eq $lockHash -and $allFilesExist) {
        [IO.File]::WriteAllText((Join-Path $destinationPath '.complete'),
                                $lockHash + [Environment]::NewLine,
                                (New-Object Text.UTF8Encoding($false)))
        Write-Host "Terminal runtime is current: $destinationPath"
        exit 0
    }
}

New-Item -ItemType Directory -Force -Path $destinationParent, $cachePath | Out-Null
$temporaryPath = Join-Path $destinationParent ('.terminal-runtime-' + [guid]::NewGuid().ToString('N'))
Assert-RemovableChildPath $temporaryPath $destinationParent
New-Item -ItemType Directory -Path $temporaryPath | Out-Null

try {
    $opencode = Get-Component $lock 'opencode'
    $node = Get-Component $lock 'node'
    $nodePty = Get-Component $lock 'node-pty'
    $opencodeArchive = Get-LockedArchive $opencode $cachePath
    $nodeArchive = Get-LockedArchive $node $cachePath
    $nodePtyArchive = Get-LockedArchive $nodePty $cachePath

    $extractRoot = Join-Path $temporaryPath '_extract'
    $opencodeExtract = Join-Path $extractRoot 'opencode'
    $nodeExtract = Join-Path $extractRoot 'node'
    $nodePtyExtract = Join-Path $extractRoot 'node-pty'
    New-Item -ItemType Directory -Force -Path $opencodeExtract, $nodeExtract, $nodePtyExtract | Out-Null
    Expand-LockedArchive $opencodeArchive $opencodeExtract
    Expand-LockedArchive $nodeArchive $nodeExtract
    Expand-LockedArchive $nodePtyArchive $nodePtyExtract

    $opencodeExecutable = Get-ChildItem -Path $opencodeExtract -Recurse -File -Filter opencode.exe |
        Select-Object -First 1
    $nodeExecutable = Get-ChildItem -Path $nodeExtract -Recurse -File -Filter node.exe |
        Select-Object -First 1
    $nodePtyPackage = Get-ChildItem -Path $nodePtyExtract -Recurse -File -Filter package.json |
        Where-Object { $_.Directory.Name -eq 'package' } |
        Select-Object -First 1
    if ($null -eq $opencodeExecutable -or $null -eq $nodeExecutable -or $null -eq $nodePtyPackage) {
        throw 'A locked runtime archive does not contain the expected files.'
    }

    $stageRoot = Join-Path $temporaryPath 'stage'
    $stageOpenCode = Join-Path $stageRoot 'opencode'
    $stageNode = Join-Path $stageRoot 'node'
    $stageNodePty = Join-Path $stageNode 'node_modules\node-pty'
    New-Item -ItemType Directory -Force -Path $stageOpenCode, $stageNode, $stageNodePty | Out-Null
    Copy-Item -LiteralPath $opencodeExecutable.FullName -Destination (Join-Path $stageOpenCode 'opencode.exe')
    Copy-Item -LiteralPath $nodeExecutable.FullName -Destination (Join-Path $stageNode 'node.exe')
    Copy-Item -Path (Join-Path $nodePtyPackage.Directory.FullName '*') -Destination $stageNodePty -Recurse

    $nodeLicense = Get-ChildItem -Path $nodeExecutable.Directory.FullName -File -Filter LICENSE |
        Select-Object -First 1
    if ($null -ne $nodeLicense) {
        Copy-Item -LiteralPath $nodeLicense.FullName -Destination (Join-Path $stageNode 'LICENSE')
    }

    $opencodeVersion = (& (Join-Path $stageOpenCode 'opencode.exe') --version 2>&1 | Out-String).Trim()
    if ($LASTEXITCODE -ne 0) {
        throw "Staged OpenCode failed its version check: $opencodeVersion"
    }
    $nodeVersion = (& (Join-Path $stageNode 'node.exe') --version 2>&1 | Out-String).Trim()
    if ($LASTEXITCODE -ne 0) {
        throw "Staged Node.js failed its version check: $nodeVersion"
    }

    $manifest = [ordered]@{
        schemaVersion = 1
        platform = $lock.platform
        lockSha256 = $lockHash
        components = [ordered]@{
            opencode = [ordered]@{
                version = $opencode.version
                path = 'opencode/opencode.exe'
                archiveSha256 = $opencode.sha256
                source = $opencode.url
                license = $opencode.license
            }
            node = [ordered]@{
                version = $node.version
                path = 'node/node.exe'
                archiveSha256 = $node.sha256
                source = $node.url
                license = $node.license
            }
            'node-pty' = [ordered]@{
                version = $nodePty.version
                path = 'node/node_modules/node-pty'
                archiveSha256 = $nodePty.sha256
                source = $nodePty.url
                license = $nodePty.license
            }
        }
    }
    Write-Utf8Json (Join-Path $stageRoot 'manifest.json') $manifest
    [IO.File]::WriteAllText((Join-Path $stageRoot '.complete'),
                            $lockHash + [Environment]::NewLine,
                            (New-Object Text.UTF8Encoding($false)))

    if (Test-Path -LiteralPath $destinationPath) {
        Assert-RemovableChildPath $destinationPath $destinationParent
        Remove-Item -Recurse -Force -LiteralPath $destinationPath
    }
    Move-Item -LiteralPath $stageRoot -Destination $destinationPath
    Write-Host "Staged OpenCode $opencodeVersion and Node.js $nodeVersion at $destinationPath"
} finally {
    if (Test-Path -LiteralPath $temporaryPath) {
        Assert-RemovableChildPath $temporaryPath $destinationParent
        Remove-Item -Recurse -Force -LiteralPath $temporaryPath
    }
}
