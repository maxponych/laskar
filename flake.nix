{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    rust-overlay.url = "github:oxalica/rust-overlay";
  };
  outputs = { self, nixpkgs, rust-overlay }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs {
        inherit system;
        overlays = [ rust-overlay.overlays.default ];
      };
    in {
      devShells.${system}.default = pkgs.mkShell {
        buildInputs = [
          (pkgs.rust-bin.stable.latest.default.override {
            targets = [ "x86_64-unknown-uefi" ];
          })
          
          pkgs.nasm
          
          pkgs.gcc
          pkgs.binutils

          pkgs.bear
          
          pkgs.dosfstools
          pkgs.mtools
          
          pkgs.coreutils
          
          pkgs.qemu
          
          pkgs.OVMF
        ];
        
        shellHook = ''
          echo "OS Development Environment"
          echo "OVMF firmware location: ${pkgs.OVMF.fd}/FV/OVMF_CODE.fd"
          export OVMF_CODE="${pkgs.OVMF.fd}/FV/OVMF_CODE.fd"
        '';
      };
    };
}
