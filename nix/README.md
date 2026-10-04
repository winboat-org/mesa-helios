# mesa-helios build interface

`default.nix` accepts schemaVersion 1, `pkgs`, explicit `sources`,
`dependencies`, `target`, `configuration` (release/debug) and `toolchain`.
It returns a derivation for native/cross outputs or a devbox dispatch record
for ABI constrained Windows targets. Dependencies are immutable Nix output
paths. Sources are exported snapshots, including selected gitlink contents.
Only locked toolchains are accepted (`toolchain = {}`); nonempty overrides
are refused. No recipe downloads dependencies during compilation.

The committed devenv inputs/lock match the environment workspace. From this
checkout run `devenv shell -- wb-component-build /path/to/spec.json`. The JSON
spec supplies system, schemaVersion, sources (`id: {path: ..., narHash: "sha256-..."}`), dependencies
(`id: /nix/store/...`), target and configuration explicitly. Nix therefore never
assumes the location of another checkout. The environment's `wb build` prepares
these snapshots and records full source/toolchain/artifact provenance.

Guest dispatch records require Stage 4's local disk mirror and durable elevated
backend. Build and runtime verification are recorded separately; MinGW outputs
cannot substitute for an MSVC static engine. Licenses and debug symbols must accompany
exported artifacts.

Windows dispatch uses `Build-Guest.ps1` with native clang-cl, the matched
SDK/WDK and static CRT for x64 or x86. `windows-inputs.nix` supplies the exact
DirectX-Headers source selected by Mesa's wrap from locked Nixpkgs; downloads
remain disabled in Meson. The operation creates its own build source copy,
regenerates protocol driver headers from the explicit paired Venus source,
and returns Vulkan, WGL and OpenGL images with generated headers and symbols.
Shared WinFlex temporary isolation and Python utilities come from the environment
workspace. Native build and runtime acceptance are tracked there separately.
