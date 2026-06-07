{
  description = "A flake for ESP-IDF development";

  inputs = {
    nixpkgs-esp-dev.url = "github:mirrexagon/nixpkgs-esp-dev";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs-esp-dev, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        esp-shell = nixpkgs-esp-dev.devShells.${system}.default;
      in {
        devShells.default = esp-shell.overrideAttrs (old: {
          nativeBuildInputs = (old.nativeBuildInputs or []) ++ [
            # Add additional packages here
          ];
        });
      });
}
