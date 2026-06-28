{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
    nativeBuildInputs = with pkgs; [
        clang
        binutils
        clang-tools
        vulkan-headers
        vulkan-validation-layers
        wayland-scanner
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
        openal-soft

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
}