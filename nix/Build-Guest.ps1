param([Parameter(Mandatory)][string]$Specification)
. (Join-Path $env:WINBOAT_CONTROL_ROOT 'Control.ps1')
$spec = Read-ControlJson $Specification
$mesa = Join-Path $spec.sourceRoot $spec.sources.'mesa-helios'.relativePath
$helios = Join-Path $spec.sourceRoot $spec.sources.helios.relativePath
$protocol = Join-Path $spec.sourceRoot $spec.sources.'venus-protocol'.relativePath
$utilities = @($spec.prerequisites | Where-Object kind -eq 'utilities')
$headersInput = @($spec.prerequisites | Where-Object kind -eq 'mesa')
if ($utilities.Count -ne 1 -or $headersInput.Count -ne 1) { throw 'Verified Windows utilities and DirectX headers closures are required' }
$env:PYTHONPATH = Join-Path $utilities[0].root 'python'
$env:PATH = (Join-Path $utilities[0].root 'bin') + ';' + $env:PATH
& python.exe -c 'import mako, yaml, packaging; print(mako.__version__, yaml.__version__, packaging.__version__)'
if ($LASTEXITCODE) { exit $LASTEXITCODE }
& python.exe (Join-Path $helios 'tools\sync-metadata.py') --check
if ($LASTEXITCODE) { exit $LASTEXITCODE }
# Patch only this operation's build copy; retain the verified source mirror and
# every generated driver header in the returned artifact table.
$source = Join-Path $spec.buildRoot 'mesa-source'
Copy-Item -LiteralPath $mesa -Destination $source -Recurse
# The wrap's exact locked source is present locally, so Meson can resolve its
# dependency with downloads disabled and build the matching GUID library.
Copy-Item -LiteralPath (Join-Path $headersInput[0].root 'DirectX-Headers-1.0') -Destination (Join-Path $source 'subprojects') -Recurse
$headers = Join-Path $spec.buildRoot 'protocol-driver'
New-Item -ItemType Directory -Path $headers | Out-Null
$banner = Join-Path $headers 'banner'
$revision = $spec.sources.'venus-protocol'.revision.Substring(0,8)
[IO.File]::WriteAllText($banner,(Get-Content -Raw (Join-Path $protocol 'templates\banner.in')).Replace('@VCS_TAG@',$revision))
Push-Location $protocol
try {
    & python.exe 'vn_protocol.py' --outdir $headers --banner $banner
    if ($LASTEXITCODE) { exit $LASTEXITCODE }
} finally { Pop-Location }
Copy-Item (Join-Path $headers '*.h') (Join-Path $source 'src\virtio\venus-protocol') -Force
$nativeName = if ($spec.architecture -eq 'x86') {'clang-cl-x86-native.ini'} else {'clang-cl-native.ini'}
$native = Join-Path $helios "ci\windows\$nativeName"
$build = Join-Path $spec.buildRoot 'mesa'
$compat = Join-Path $helios 'icd\win-build\helios_win_compat.h'
$wdk = Join-Path $helios 'icd\win-build\wdk-include'
$type = if ($spec.configuration -eq 'debug') {'debug'} else {'debugoptimized'}
& meson.exe setup $build $source --native-file $native --wrap-mode=nodownload --buildtype=$type -Db_vscrt=mt `
    "-Dc_args=/FI$compat /Z7" "-Dcpp_args=/FI$compat /D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH /Z7" `
    -Dvulkan-drivers=virtio -Dgallium-drivers=zink -Dplatforms=windows "-Dhelios-wdk-include=$wdk" `
    -Dvulkan-manifest-per-architecture=false -Dvideo-codecs= -Dvulkan-layers= -Degl=disabled -Dgbm=disabled `
    -Dglx=disabled -Dopengl=true -Dgles1=disabled -Dgles2=disabled -Dllvm=disabled -Dshader-cache=disabled `
    -Dzlib=disabled -Dzstd=disabled -Dbuild-tests=false -Dperfetto=false -Dxmlconfig=disabled -Dspirv-tools=disabled
if ($LASTEXITCODE) { exit $LASTEXITCODE }
& meson.exe compile -C $build -j 2
if ($LASTEXITCODE) { exit $LASTEXITCODE }
