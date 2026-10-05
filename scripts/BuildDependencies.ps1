param(
    [string]$SourceRoot = (Join-Path $PSScriptRoot '..\dependencies'),
    [string]$BuildRoot = (Join-Path $PSScriptRoot '..\build\dependencies'),
    [string]$InstallPrefix = (Join-Path $PSScriptRoot '..\build\commonlib-installed'),
    [string]$Generator = 'Visual Studio 17 2022',
    [switch]$CheckOnly
)
$ErrorActionPreference = 'Stop'
$SourceRoot = [IO.Path]::GetFullPath($SourceRoot)
$BuildRoot = [IO.Path]::GetFullPath($BuildRoot)
$InstallPrefix = [IO.Path]::GetFullPath($InstallPrefix)
$projects = @('fmt', 'spdlog', 'rapidcsv', 'directxmath', 'directxtk', 'CommonLibSSE-NG-10.0.1')
foreach ($project in $projects) {
    if (-not (Test-Path -LiteralPath (Join-Path $SourceRoot "$project\CMakeLists.txt") -PathType Leaf)) {
        throw "Missing corresponding source: $project"
    }
}
$minhook = Join-Path $SourceRoot 'MinHook-hde64-1.3.4'
if (-not (Test-Path -LiteralPath (Join-Path $minhook 'src\hde\hde64.c') -PathType Leaf)) {
    throw 'Missing MinHook-hde64-1.3.4 corresponding source.'
}
$openvr = Join-Path $SourceRoot 'CommonLibSSE-NG-10.0.1\extern\openvr'
foreach ($relative in @('headers\openvr.h', 'lib\win64\openvr_api.lib', 'LICENSE')) {
    if (-not (Test-Path -LiteralPath (Join-Path $openvr $relative) -PathType Leaf)) {
        throw "Missing pinned OpenVR SDK file: $relative"
    }
}
if ($CheckOnly) {
    Write-Output "Source paths verified. Install prefix: $InstallPrefix"
    return
}
function Invoke-CMake {
    param([string[]]$CMakeArguments)
    & cmake @CMakeArguments
    if ($LASTEXITCODE -ne 0) { throw "CMake failed: $($CMakeArguments -join ' ')" }
}
foreach ($project in $projects) {
    $source = Join-Path $SourceRoot $project
    $build = Join-Path $BuildRoot $project
    $options = @(
        '-S', $source, '-B', $build, '-G', $Generator, '-A', 'x64',
        "-DCMAKE_INSTALL_PREFIX=$InstallPrefix", "-DCMAKE_PREFIX_PATH=$InstallPrefix",
        '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL', '-DBUILD_SHARED_LIBS=OFF',
        '-DBUILD_TESTING=OFF', '-DCMAKE_POLICY_VERSION_MINIMUM=3.5'
    )
    switch ($project) {
        'fmt' { $options += @('-DFMT_TEST=OFF', '-DFMT_DOC=OFF', '-DFMT_INSTALL=ON') }
        'spdlog' { $options += @('-DSPDLOG_BUILD_SHARED=OFF', '-DSPDLOG_FMT_EXTERNAL=ON', '-DSPDLOG_INSTALL=ON', '-DSPDLOG_BUILD_TESTS=OFF') }
        'rapidcsv' { $options += '-DRAPIDCSV_BUILD_TESTS=OFF' }
        'directxtk' { $options += @('-DBUILD_TOOLS=OFF', '-DBUILD_XAUDIO_WIN8=OFF', '-DBUILD_XAUDIO_WIN10=OFF') }
        'CommonLibSSE-NG-10.0.1' {
            $options += @(
                '-DBUILD_TESTS=OFF', '-DENABLE_SKYRIM_SE=ON', '-DENABLE_SKYRIM_AE=ON',
                '-DENABLE_SKYRIM_VR=OFF', '-DSKSE_SUPPORT_XBYAK=OFF',
                '-DSKSE_SUPPORT_PATCH_SAFETY=ON', '-DCOMMONLIB_ENABLE_IPO=OFF',
                "-DFETCHCONTENT_SOURCE_DIR_HDE64=$minhook",
                "-DRAPIDCSV_INCLUDE_DIRS=$InstallPrefix/include"
            )
        }
    }
    Invoke-CMake -CMakeArguments $options
    Invoke-CMake -CMakeArguments @('--build', $build, '--config', 'Release')
    Invoke-CMake -CMakeArguments @('--install', $build, '--config', 'Release')
}
# The release source's CommonLib install/export helpers need these two files.
$configDir = Join-Path $InstallPrefix 'lib\cmake\CommonLibSSE'
New-Item -ItemType Directory -Force -Path $configDir | Out-Null
Copy-Item -LiteralPath (Join-Path $SourceRoot 'CommonLibSSE-NG-10.0.1\cmake\CommonLibSSE.cmake') -Destination $configDir -Force
$versionScript = Join-Path $BuildRoot 'WriteCommonLibVersion.cmake'
$versionFile = (Join-Path $configDir 'CommonLibSSEConfigVersion.cmake').Replace('\', '/')
$versionText = "include(CMakePackageConfigHelpers)`nwrite_basic_package_version_file(`"$versionFile`" VERSION 10.0.1 COMPATIBILITY SameMajorVersion)`n"
[IO.File]::WriteAllText($versionScript, $versionText, [Text.UTF8Encoding]::new($false))
Invoke-CMake -CMakeArguments @('-P', $versionScript)
Write-Output "Dependencies installed: $InstallPrefix"
