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
    in {
      formatter = pkgs.alejandra;
      devShell = pebble.pebbleEnv.${system} {};
    });
}
