{
  description = "C++ development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
      in
      {
        devShells.default = pkgs.mkShell {
          nativeBuildInputs = with pkgs; [
            clang
            binutils
            clang-tools
            vulkan-headers
            vulkan-validation-layers
            wayland-scanner
            openssl
            cmake
            pkg-config
            ninja
            gdb
            rustc
            cargo
          ];

          buildInputs = with pkgs; [
            vulkan-loader
            shaderc
            bullet
            freetype
            openal
            boost
            wayland
            libxkbcommon
            libX11
            libXrandr
            libXinerama
            libXcursor
            libXi
            gtk3
            sysprof
          ];
        };
      }
    );
}