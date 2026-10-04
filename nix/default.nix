{
  pkgs,
  sources,
  dependencies,
  target ? "host",
  configuration ? "release",
  toolchain ? { },
  schemaVersion ? 1,
}:
assert toolchain == { };
assert schemaVersion == 1;
if (target == "guest-x64" || target == "guest-x86") && dependencies ? msvcCrossFile then
  import ./cross-guest.nix {
    inherit
      pkgs
      sources
      dependencies
      target
      configuration
      ;
  }
else if target == "guest-x64" || target == "guest-x86" then
  {
    backend = "devbox";
    purpose = "build";
    architecture = if target == "guest-x86" then "x86" else "x64";
    crt = "mt";
    commands = [
      [
        "powershell.exe"
        "-NoProfile"
        "-ExecutionPolicy"
        "Bypass"
        "-File"
        "@sourceDirectory@/nix/Build-Guest.ps1"
        "-Specification"
        "@specification@"
      ]
    ];
    outputs = [
      "mesa/src/virtio/vulkan/vulkan_virtio.dll"
      "mesa/src/virtio/vulkan/virtio_icd.json"
      "mesa/src/gallium/targets/wgl/libgallium_wgl.dll"
      "mesa/src/gallium/targets/libgl-gdi/opengl32.dll"
    ];
    preserveDirectories = [ "protocol-driver" ];
    protocol = dependencies.protocol;
    requirements = [
      "LLVM-22.1.8-clang-cl-static-CRT"
      "Nix-pinned-Mako-PyYAML-Packaging"
      "WinFlexBison-2.5.25"
      "SDK-10.0.26100.0"
      "fixed-Meson-wrap-closure"
    ];
  }
else
  assert target == "host";
  pkgs.stdenv.mkDerivation {
    pname = "mesa-helios";
    version = pkgs.lib.strings.trim (builtins.readFile (sources.mesa-helios + "/VERSION"));
    src = sources.mesa-helios;
    nativeBuildInputs = pkgs.mesa.nativeBuildInputs;
    buildInputs = pkgs.mesa.buildInputs ++ [ pkgs.gtest ];
    postPatch = ''
      patchShebangs .
      cp ${dependencies.protocol}/include/venus-protocol/vn_protocol_driver*.h src/virtio/venus-protocol/
    '';
    mesonBuildType = if configuration == "debug" then "debug" else "debugoptimized";
    mesonWrapMode = "nodownload";
    mesonFlags = [
      "-Dauto_features=auto"
      "-Dgallium-drivers=zink,softpipe"
      "-Dvulkan-drivers=virtio"
      "-Dvulkan-layers=[]"
      "-Dgallium-rusticl=false"
      "-Dllvm=disabled"
      "-Dplatforms=x11,wayland"
      "-Dglvnd=enabled"
      "-Dvideo-codecs=[]"
      "-Dtools=[]"
      "-Dbuild-tests=true"
    ];
    separateDebugInfo = true;
    doCheck = true;
    postInstall = ''
      mkdir -p $out/share/licenses/mesa-helios
      cp -r ../licenses/. $out/share/licenses/mesa-helios/
    '';
  }
