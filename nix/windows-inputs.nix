{ pkgs }:
assert pkgs.directx-headers.version == "1.619.1";
pkgs.runCommand "mesa-windows-directx-headers" { } ''
  mkdir -p $out/DirectX-Headers-1.0 $out/share/licenses/directx-headers
  cp -r ${pkgs.directx-headers.src}/. $out/DirectX-Headers-1.0/
  cp ${pkgs.directx-headers.src}/LICENSE $out/share/licenses/directx-headers/
  cp ${
    pkgs.writeText "directx-headers-source.json" (
      builtins.toJSON {
        version = pkgs.directx-headers.version;
        revision = pkgs.directx-headers.src.rev;
        contentHash = pkgs.directx-headers.src.outputHash;
      }
    )
  } $out/share/licenses/directx-headers/source.json
''
