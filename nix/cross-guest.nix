{
  pkgs,
  sources,
  dependencies,
  target,
  configuration,
}:
let
  architecture = if target == "guest-x86" then "x86" else "x64";
  headers = import ./windows-inputs.nix { inherit pkgs; };
  outputs = [
    "src/virtio/vulkan/vulkan_virtio.dll"
    "src/virtio/vulkan/virtio_icd.json"
    "src/gallium/targets/wgl/libgallium_wgl.dll"
    "src/gallium/targets/libgl-gdi/opengl32.dll"
  ];
in
pkgs.runCommand "mesa-helios-msvc-cross-${architecture}-${configuration}"
  {
    nativeBuildInputs = [
      pkgs.meson
      pkgs.ninja
      pkgs.pkg-config
      pkgs.flex
      pkgs.bison
      pkgs.stdenv.cc
      pkgs.llvmPackages_22.lld
      pkgs.llvmPackages_22.llvm
      (pkgs.python3.withPackages (p: [
        p.mako
        p.pyyaml
        p.packaging
      ]))
    ];
    env.LIB = pkgs.lib.concatStringsSep ";" (
      map (path: "${dependencies.msvcSysroot}/${path}/${architecture}") [
        "crt/lib"
        "sdk/lib/ucrt"
        "sdk/lib/um"
      ]
    );
    env.PYTHONDONTWRITEBYTECODE = "1";
  }
  ''
    python3 ${sources.helios}/tools/sync-metadata.py --check
    cp -R ${sources.mesa-helios} source
    chmod -R u+w source
    cp -R ${headers}/DirectX-Headers-1.0 source/subprojects/
    chmod -R u+w source/subprojects/DirectX-Headers-1.0
    cp ${dependencies.protocol}/include/venus-protocol/vn_protocol_driver*.h source/src/virtio/venus-protocol/
    patchShebangs source
    meson setup mesa source --wrap-mode=nodownload \
      --cross-file ${dependencies.msvcCrossFile} \
      --buildtype ${if configuration == "debug" then "debug" else "debugoptimized"} \
      -Db_vscrt=mt '-Dc_args=/FI${sources.helios}/icd/win-build/helios_win_compat.h /FI${./lexer-compat.h} /Z7' \
      '-Dcpp_args=/FI${sources.helios}/icd/win-build/helios_win_compat.h /FI${./lexer-compat.h} /Z7' \
      -Dvulkan-drivers=virtio -Dgallium-drivers=zink -Dplatforms=windows \
      -Dhelios-wdk-include=${sources.helios}/icd/win-build/wdk-include \
      -Dvulkan-manifest-per-architecture=false -Dvideo-codecs= -Dvulkan-layers= \
      -Degl=disabled -Dgbm=disabled -Dglx=disabled -Dopengl=true \
      -Dgles1=disabled -Dgles2=disabled -Dllvm=disabled -Dshader-cache=disabled \
      -Dzlib=disabled -Dzstd=disabled -Dbuild-tests=false -Dperfetto=false \
      -Dxmlconfig=disabled -Dspirv-tools=disabled
    ninja -C mesa -j "$NIX_BUILD_CORES"
    ${pkgs.lib.concatMapStringsSep "\n" (file: ''
      mkdir -p "$out/mesa/$(dirname '${file}')"
      cp 'mesa/${file}' "$out/mesa/${file}"
    '') outputs}
    find mesa -type f -name '*.pdb' | while IFS= read -r symbol; do
      mkdir -p "$out/$(dirname "$symbol")"
      cp "$symbol" "$out/$symbol"
    done
    mkdir -p "$out/protocol-driver" "$out/licenses" "$out/share/winboat"
    cp ${dependencies.protocol}/include/venus-protocol/vn_protocol_driver*.h "$out/protocol-driver/"
    cp -R ${headers}/share/licenses/. "$out/licenses/"
    cp -R ${dependencies.protocol}/share/licenses/. "$out/licenses/"
    cp -R ${dependencies.msvcSysroot}/share/licenses/. "$out/licenses/"
    ${pkgs.lib.concatMapStringsSep "\n"
      (component: ''
        find ${sources.${component}} -type f \( -iname 'LICENSE*' -o -iname 'COPYING*' -o -iname 'NOTICE*' \) \
          | while IFS= read -r notice; do
            relative="''${notice#${sources.${component}}/}"
            mkdir -p "$out/licenses/${component}/$(dirname "$relative")"
            cp "$notice" "$out/licenses/${component}/$relative"
          done
      '')
      [
        "mesa-helios"
        "helios"
      ]
    }
    cp mesa/compile_commands.json "$out/share/winboat/compile_commands.json"
    python3 ${dependencies.msvcInspector} "$out" ${pkgs.llvmPackages_22.llvm}/bin/llvm-readobj \
      --architecture=${architecture} > "$out/images.json"
  ''
