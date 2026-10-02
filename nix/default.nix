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
if target == "guest-x64" || target == "guest-x86" then
  {
    backend = "devbox";
    purpose = "build";
    architecture = if target == "guest-x86" then "x86" else "x64";
    nativeFile = ./mingw-native.ini;
    commands = [
      [
        "meson"
        "setup"
        "@buildDirectory@"
        "@sourceDirectory@"
        "--wrap-mode=nodownload"
        "--buildtype"
        (if configuration == "debug" then "debug" else "release")
        "--native-file"
        "@nativeFile@"
        "-Dvulkan-drivers=virtio"
        "-Dgallium-drivers=zink"
        "-Dplatforms=windows"
      ]
      [
        "ninja"
        "-C"
        "@buildDirectory@"
      ]
    ];
    protocol = dependencies.protocol;
    requirements = [
      "MinGW-w64-x64-or-x86-static-runtime"
      "fixed-shader-tools"
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
