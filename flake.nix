{
  inputs = {
    flake-utils.url = "github:numtide/flake-utils";
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    pebble.url = "github:pebble-dev/pebble.nix";
  };

  outputs = {
    flake-utils,
    nixpkgs,
    pebble,
    ...
  }:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = nixpkgs.legacyPackages.${system};
      toolchain = pebble.packages.${system}.pebble-toolchain-bin;
    in {
      formatter = pkgs.alejandra;
      devShell =
        (pebble.pebbleEnv.${system} {
          packages = with pkgs; [clang-tools lldb];
        }).overrideAttrs (old: {
          # fix clangd/clang-tidy import errors
          env = old.env // {CPATH = "${toolchain}/arm-none-eabi/include";};
        });
    });
}
