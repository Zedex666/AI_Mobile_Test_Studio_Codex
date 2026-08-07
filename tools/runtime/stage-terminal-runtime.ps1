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

function Get-ComponentUrls([object]$Component)
{
    $urls = @($Component.url)
    $mirrorsProperty = $Component.PSObject.Properties['mirrors']
    if ($null -ne $mirrorsProperty) {
        $urls += @($mirrorsProperty.Value)
    }
    return @($urls | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
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
    $lastError = $null
    foreach ($url in Get-ComponentUrls $Component) {
        Write-Host "Downloading $($Component.id) $($Component.version) from $url..."
        try {
            $curl = Get-Command curl.exe -ErrorAction SilentlyContinue
            if ($null -ne $curl) {
                & $curl.Source --fail --location --silent --show-error `
                    --retry 3 --retry-delay 2 --connect-timeout 30 --max-time 600 `
                    --output $partialPath $url
                if ($LASTEXITCODE -ne 0) {
                    throw "curl failed with exit code $LASTEXITCODE."
                }
            } else {
                $client = New-Object Net.WebClient
                $client.Headers['User-Agent'] = 'AI-Mobile-Test-Studio-Runtime-Stager'
                try {
                    $client.DownloadFile([Uri]$url, $partialPath)
                } finally {
                    $client.Dispose()
                }
            }

            if (-not (Test-Hash $partialPath $Component.sha256)) {
                throw "SHA-256 verification failed for $($Component.archive)."
            }
            Move-Item -Force -LiteralPath $partialPath -Destination $archivePath
            return $archivePath
        } catch {
            $lastError = $_
            Remove-Item -Force -LiteralPath $partialPath -ErrorAction SilentlyContinue
        }
    }
    throw "Unable to download $($Component.id): $lastError"
}

function Expand-CMakeArchive([string]$Archive, [string]$OutputDirectory)
{
    $cmake = Get-Command cmake -ErrorAction Stop
    Push-Location $OutputDirectory
    try {
        & $cmake.Source -E tar xvf $Archive | Out-Null
        if ($LASTEXITCODE -ne 0) {
            throw "CMake failed to extract $Archive (exit code $LASTEXITCODE)."
        }
    } finally {
        Pop-Location
    }
}

function Copy-DirectoryContents([string]$Source, [string]$Destination)
{
    New-Item -ItemType Directory -Force -Path $Destination | Out-Null
    Copy-Item -Path (Join-Path $Source '*') -Destination $Destination -Recurse -Force
}

function Write-Utf8Json([string]$Path, [object]$Value)
{
    $json = $Value | ConvertTo-Json -Depth 12
    $encoding = New-Object Text.UTF8Encoding($false)
    [IO.File]::WriteAllText($Path, $json + [Environment]::NewLine, $encoding)
}

function ConvertTo-WindowsCommandLineArgument([string]$Argument)
{
    if ($Argument.Length -gt 0 -and $Argument -notmatch '[\s"]') {
        return $Argument
    }

    $escaped = [regex]::Replace($Argument, '(\\*)"', '$1$1\"')
    $escaped = [regex]::Replace($escaped, '(\\+)$', '$1$1')
    return '"' + $escaped + '"'
}

function Invoke-Checked([string]$Program, [string[]]$Arguments, [string]$Description)
{
    $processInfo = New-Object Diagnostics.ProcessStartInfo
    $programExtension = [IO.Path]::GetExtension($Program)
    $formattedArguments = @($Arguments | ForEach-Object {
        ConvertTo-WindowsCommandLineArgument $_
    })
    if ($programExtension -in @('.bat', '.cmd')) {
        $command = (ConvertTo-WindowsCommandLineArgument $Program)
        if ($formattedArguments.Count -gt 0) {
            $command += ' ' + ($formattedArguments -join ' ')
        }
        $processInfo.FileName = $env:ComSpec
        $processInfo.Arguments = '/D /S /C "' + $command + '"'
    } else {
        $processInfo.FileName = $Program
        $processInfo.Arguments = $formattedArguments -join ' '
    }
    $processInfo.UseShellExecute = $false
    $processInfo.CreateNoWindow = $true
    $processInfo.RedirectStandardOutput = $true
    $processInfo.RedirectStandardError = $true

    $process = New-Object Diagnostics.Process
    $process.StartInfo = $processInfo
    try {
        if (-not $process.Start()) {
            throw "$Description failed to start."
        }
        $stdoutTask = $process.StandardOutput.ReadToEndAsync()
        $stderrTask = $process.StandardError.ReadToEndAsync()
        $process.WaitForExit()
        $stdout = $stdoutTask.Result.Trim()
        $stderr = $stderrTask.Result.Trim()
        $output = @($stdout, $stderr) | Where-Object {
            -not [string]::IsNullOrWhiteSpace($_)
        }
        $output = $output -join [Environment]::NewLine
        $exitCode = $process.ExitCode
    } finally {
        $process.Dispose()
    }
    if ($exitCode -ne 0) {
        throw "$Description failed: $output"
    }
    return $output
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
if ($lock.schemaVersion -ne 2 -or $lock.platform -ne 'windows-x64') {
    throw "Unsupported runtime lock schema or platform in $lockPath"
}
$lockDirectory = Split-Path -Parent $lockPath
$npmLockPath = Get-FullPath (Join-Path $lockDirectory $lock.npmRuntime.lockFile)
if (-not (Test-Hash $npmLockPath $lock.npmRuntime.sha256)) {
    throw "Appium package-lock.json does not match runtime-lock.json."
}

$lockHash = (Get-FileHash -LiteralPath $lockPath -Algorithm SHA256).Hash.ToLowerInvariant()
$manifestPath = Join-Path $destinationPath 'manifest.json'
$requiredPaths = @(
    (Join-Path $destinationPath 'opencode\opencode.exe'),
    (Join-Path $destinationPath 'node\node.exe'),
    (Join-Path $destinationPath 'node\npm.cmd'),
    (Join-Path $destinationPath 'node\node_modules\node-pty\package.json'),
    (Join-Path $destinationPath 'conda\conda.exe'),
    (Join-Path $destinationPath 'jdk\bin\java.exe'),
    (Join-Path $destinationPath 'jdk\bin\javac.exe'),
    (Join-Path $destinationPath 'android-sdk\platform-tools\adb.exe'),
    (Join-Path $destinationPath 'android-sdk\cmdline-tools\latest\bin\sdkmanager.bat'),
    (Join-Path $destinationPath 'appium\node_modules\appium\index.js'),
    (Join-Path $destinationPath 'appium\node_modules\appium-uiautomator2-driver\package.json')
)
if (Test-Path -LiteralPath $manifestPath -PathType Leaf) {
    $currentManifest = Get-Content -Raw -Encoding UTF8 $manifestPath | ConvertFrom-Json
    $currentLockProperty = $currentManifest.PSObject.Properties['lockSha256']
    $currentNpmLockProperty = $currentManifest.PSObject.Properties['npmLockSha256']
    $missingPaths = @($requiredPaths | Where-Object {
        -not (Test-Path -LiteralPath $_ -PathType Leaf)
    })
    if ($null -ne $currentLockProperty -and
        $null -ne $currentNpmLockProperty -and
        $currentLockProperty.Value -eq $lockHash -and
        $currentNpmLockProperty.Value -eq $lock.npmRuntime.sha256 -and
        $missingPaths.Count -eq 0) {
        [IO.File]::WriteAllText((Join-Path $destinationPath '.complete'),
                                $lockHash + [Environment]::NewLine,
                                (New-Object Text.UTF8Encoding($false)))
        Write-Host "Portable runtime is current: $destinationPath"
        exit 0
    }
}

New-Item -ItemType Directory -Force -Path $destinationParent, $cachePath | Out-Null
$temporaryPath = Join-Path $destinationParent ('.portable-runtime-' + [guid]::NewGuid().ToString('N'))
Assert-RemovableChildPath $temporaryPath $destinationParent
New-Item -ItemType Directory -Path $temporaryPath | Out-Null

try {
    $opencode = Get-Component $lock 'opencode'
    $node = Get-Component $lock 'node'
    $nodePty = Get-Component $lock 'node-pty'
    $conda = Get-Component $lock 'conda'
    $jdk = Get-Component $lock 'jdk'
    $androidCli = Get-Component $lock 'android-command-line-tools'
    $androidPlatformTools = Get-Component $lock 'android-platform-tools'

    $archives = @{}
    foreach ($component in @($opencode,
                             $node,
                             $nodePty,
                             $conda,
                             $jdk,
                             $androidCli,
                             $androidPlatformTools)) {
        $archives[$component.id] = Get-LockedArchive $component $cachePath
    }

    $extractRoot = Join-Path $temporaryPath '_extract'
    $extractDirectories = @{}
    foreach ($component in @($opencode,
                             $node,
                             $nodePty,
                             $conda,
                             $jdk,
                             $androidCli,
                             $androidPlatformTools)) {
        $extractDirectory = Join-Path $extractRoot $component.id
        New-Item -ItemType Directory -Force -Path $extractDirectory | Out-Null
        Expand-CMakeArchive $archives[$component.id] $extractDirectory
        $extractDirectories[$component.id] = $extractDirectory
    }

    $condaPayloadArchive = Get-ChildItem -Path $extractDirectories['conda'] -File `
        -Filter 'pkg-*.tar.zst' | Select-Object -First 1
    if ($null -eq $condaPayloadArchive) {
        throw 'conda-standalone archive does not contain a package payload.'
    }
    $condaPayloadDirectory = Join-Path $extractRoot 'conda-payload'
    New-Item -ItemType Directory -Force -Path $condaPayloadDirectory | Out-Null
    Expand-CMakeArchive $condaPayloadArchive.FullName $condaPayloadDirectory

    $opencodeExecutable = Get-ChildItem -Path $extractDirectories['opencode'] -Recurse `
        -File -Filter opencode.exe | Select-Object -First 1
    $nodeExecutable = Get-ChildItem -Path $extractDirectories['node'] -Recurse `
        -File -Filter node.exe | Select-Object -First 1
    $nodePtyPackage = Get-ChildItem -Path $extractDirectories['node-pty'] -Recurse `
        -File -Filter package.json | Where-Object { $_.Directory.Name -eq 'package' } | `
        Select-Object -First 1
    $condaExecutable = Get-ChildItem -Path $condaPayloadDirectory -Recurse `
        -File -Filter conda.exe | Where-Object { $_.Directory.Name -eq 'standalone_conda' } | `
        Select-Object -First 1
    $javaExecutable = Get-ChildItem -Path $extractDirectories['jdk'] -Recurse `
        -File -Filter java.exe | Where-Object { $_.Directory.Name -eq 'bin' } | `
        Select-Object -First 1
    $sdkManager = Get-ChildItem -Path $extractDirectories['android-command-line-tools'] `
        -Recurse -File -Filter sdkmanager.bat | Select-Object -First 1
    $adbExecutable = Get-ChildItem -Path $extractDirectories['android-platform-tools'] `
        -Recurse -File -Filter adb.exe | Select-Object -First 1
    if (@($opencodeExecutable,
          $nodeExecutable,
          $nodePtyPackage,
          $condaExecutable,
          $javaExecutable,
          $sdkManager,
          $adbExecutable) -contains $null) {
        throw 'A locked runtime archive does not contain the expected files.'
    }

    $stageRoot = Join-Path $temporaryPath 'stage'
    $stageOpenCode = Join-Path $stageRoot 'opencode'
    $stageNode = Join-Path $stageRoot 'node'
    $stageNodePty = Join-Path $stageNode 'node_modules\node-pty'
    $stageConda = Join-Path $stageRoot 'conda'
    $stageJdk = Join-Path $stageRoot 'jdk'
    $stageAndroidSdk = Join-Path $stageRoot 'android-sdk'
    $stageAndroidCli = Join-Path $stageAndroidSdk 'cmdline-tools\latest'
    $stagePlatformTools = Join-Path $stageAndroidSdk 'platform-tools'
    $stageAppium = Join-Path $stageRoot 'appium'

    Copy-DirectoryContents $nodeExecutable.Directory.FullName $stageNode
    Copy-DirectoryContents $nodePtyPackage.Directory.FullName $stageNodePty
    Copy-DirectoryContents $javaExecutable.Directory.Parent.FullName $stageJdk
    Copy-DirectoryContents $sdkManager.Directory.Parent.FullName $stageAndroidCli
    Copy-DirectoryContents $adbExecutable.Directory.FullName $stagePlatformTools
    New-Item -ItemType Directory -Force -Path $stageOpenCode, $stageConda, $stageAppium | `
        Out-Null
    Copy-Item -LiteralPath $opencodeExecutable.FullName `
        -Destination (Join-Path $stageOpenCode 'opencode.exe')
    Copy-Item -LiteralPath $condaExecutable.FullName `
        -Destination (Join-Path $stageConda 'conda.exe')

    $appiumPackageDirectory = Split-Path -Parent $npmLockPath
    Copy-Item -LiteralPath (Join-Path $appiumPackageDirectory 'package.json') `
        -Destination (Join-Path $stageAppium 'package.json')
    Copy-Item -LiteralPath $npmLockPath `
        -Destination (Join-Path $stageAppium 'package-lock.json')

    $savedEnvironment = @{}
    $temporaryEnvironment = [ordered]@{
        PATH = $stageNode + [IO.Path]::PathSeparator + $env:PATH
        NPM_CONFIG_CACHE = (Join-Path $cachePath 'npm')
        NPM_CONFIG_PREFIX = (Join-Path $temporaryPath 'npm-prefix')
        NPM_CONFIG_USERCONFIG = (Join-Path $temporaryPath 'isolated.npmrc')
        NPM_CONFIG_FETCH_RETRIES = '1'
        NPM_CONFIG_FETCH_RETRY_MINTIMEOUT = '1000'
        NPM_CONFIG_FETCH_RETRY_MAXTIMEOUT = '5000'
        NPM_CONFIG_FETCH_TIMEOUT = '30000'
        NPM_CONFIG_REGISTRY = 'https://registry.npmjs.org/'
        NPM_CONFIG_REPLACE_REGISTRY_HOST = 'always'
        APPIUM_HOME = (Join-Path $temporaryPath 'appium-home')
        JAVA_HOME = $stageJdk
        ANDROID_HOME = $stageAndroidSdk
        ANDROID_SDK_ROOT = $stageAndroidSdk
        CONDA_PKGS_DIRS = (Join-Path $temporaryPath 'conda-pkgs')
        CONDARC = (Join-Path $temporaryPath 'isolated.condarc')
    }
    try {
        foreach ($name in $temporaryEnvironment.Keys) {
            $savedEnvironment[$name] = [Environment]::GetEnvironmentVariable($name, 'Process')
            [Environment]::SetEnvironmentVariable($name,
                                                  $temporaryEnvironment[$name],
                                                  'Process')
        }

        $npmExecutable = Join-Path $stageNode 'npm.cmd'
        $npmInstallOutput = ''
        $npmInstalled = $false
        foreach ($registry in @('https://registry.npmjs.org/',
                                 'https://registry.npmmirror.com/')) {
            $nodeModules = Join-Path $stageAppium 'node_modules'
            if (Test-Path -LiteralPath $nodeModules) {
                Assert-RemovableChildPath $nodeModules $stageAppium
                Remove-Item -Recurse -Force -LiteralPath $nodeModules
            }
            [Environment]::SetEnvironmentVariable('NPM_CONFIG_REGISTRY', $registry, 'Process')
            [Environment]::SetEnvironmentVariable('NPM_CONFIG_REPLACE_REGISTRY_HOST',
                                                   'always',
                                                   'Process')
            $npmInstallOutput = (& $npmExecutable ci --prefix $stageAppium --omit=dev `
                --ignore-scripts --no-audit --no-fund 2>&1 | Out-String).Trim()
            if ($LASTEXITCODE -eq 0) {
                $npmInstalled = $true
                break
            }
        }
        if (-not $npmInstalled) {
            throw "Installing the locked Appium runtime failed: $npmInstallOutput"
        }

        $opencodeVersion = Invoke-Checked (Join-Path $stageOpenCode 'opencode.exe') `
            @('--version') 'OpenCode version check'
        $nodeVersion = Invoke-Checked (Join-Path $stageNode 'node.exe') `
            @('--version') 'Node.js version check'
        $npmVersion = Invoke-Checked $npmExecutable @('--version') 'npm version check'
        if ($npmVersion -ne [string]$lock.bundledComponents.npm.version) {
            throw "npm version check returned '$npmVersion'; " +
                "expected '$($lock.bundledComponents.npm.version)'."
        }
        $condaVersion = Invoke-Checked (Join-Path $stageConda 'conda.exe') `
            @('--version') 'Conda version check'
        $javaVersion = Invoke-Checked (Join-Path $stageJdk 'bin\java.exe') `
            @('-version') 'JDK version check'
        $adbVersion = Invoke-Checked (Join-Path $stagePlatformTools 'adb.exe') `
            @('version') 'Android platform-tools version check'
        $sdkManagerVersion = Invoke-Checked (Join-Path $stageAndroidCli 'bin\sdkmanager.bat') `
            @('--version') 'Android command-line tools version check'
        if ($sdkManagerVersion -notmatch '^\d+(?:\.\d+)*$' -or
            $sdkManagerVersion -ne [string]$androidCli.version) {
            throw "Android command-line tools version check returned '$sdkManagerVersion'; " +
                "expected '$($androidCli.version)'."
        }
        $appiumVersion = Invoke-Checked (Join-Path $stageNode 'node.exe') `
            @((Join-Path $stageAppium 'node_modules\appium\index.js'), '--version') `
            'Appium version check'
    } finally {
        foreach ($name in $temporaryEnvironment.Keys) {
            [Environment]::SetEnvironmentVariable($name, $savedEnvironment[$name], 'Process')
        }
    }

    $manifestComponents = [ordered]@{}
    foreach ($component in $lock.components) {
        $manifestComponents[$component.id] = [ordered]@{
            version = $component.version
            archiveSha256 = $component.sha256
            source = $component.url
            license = $component.license
        }
    }
    $manifestComponents['npm'] = [ordered]@{
        version = $lock.bundledComponents.npm.version
        path = $lock.bundledComponents.npm.path
        parent = $lock.bundledComponents.npm.parent
        license = $lock.bundledComponents.npm.license
    }
    $manifestComponents['appium'] = [ordered]@{
        version = $lock.npmRuntime.components.appium.version
        path = 'appium/node_modules/appium/index.js'
        license = $lock.npmRuntime.components.appium.license
    }
    $manifestComponents['appium-uiautomator2-driver'] = [ordered]@{
        version = $lock.npmRuntime.components.'appium-uiautomator2-driver'.version
        path = 'appium/node_modules/appium-uiautomator2-driver'
        license = $lock.npmRuntime.components.'appium-uiautomator2-driver'.license
    }

    $manifest = [ordered]@{
        schemaVersion = 2
        platform = $lock.platform
        lockSha256 = $lockHash
        npmLockSha256 = $lock.npmRuntime.sha256
        components = $manifestComponents
        checks = [ordered]@{
            opencode = $opencodeVersion
            node = $nodeVersion
            npm = $npmVersion
            conda = $condaVersion
            java = $javaVersion
            adb = $adbVersion
            sdkManager = $sdkManagerVersion
            appium = $appiumVersion
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
    Write-Host "Staged isolated portable runtime at $destinationPath"
} finally {
    if (Test-Path -LiteralPath $temporaryPath) {
        Assert-RemovableChildPath $temporaryPath $destinationParent
        Remove-Item -Recurse -Force -LiteralPath $temporaryPath
    }
}
