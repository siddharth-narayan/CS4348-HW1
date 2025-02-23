{
  outputs =
    { nixpkgs, ... }:
    let
      pkgs = import nixpkgs { system = "x86_64-linux"; };
    in
    {
      devShells.x86_64-linux.default = pkgs.mkShell {
        buildInputs = with pkgs; [
          linuxKernel.packages.linux_6_6.perf
          meson
          ninja
        ];
      };
    };
}
