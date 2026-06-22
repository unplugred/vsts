# activated w "nix develop" or w direnv
{
	inputs = {
		nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
		flake-utils.url = "github:numtide/flake-utils";
	};

	outputs = { self, nixpkgs, flake-utils, ... }:
	flake-utils.lib.eachDefaultSystem (system:
	let
		pkgs = nixpkgs.legacyPackages.${system};
	in {
		devShells.default = pkgs.mkShell {
			# packages
			buildInputs = with pkgs; [
				git
				(python311.withPackages (ps: with ps; [ # python packages
				]))
				cmake
				pkg-config
				#gnumake
				#gcc
				gdb
				alsa-lib      #libasound2-dev
				#jack2         #libjack-jackd2-dev
				#ladspa-sdk    #ladspa-sdk
				#curl          #libcurl4-openssl-dev
				#fontconfig    #libfontconfig1-dev
				freetype      #libfreetype-dev
				libx11        #libx11-dev
				libxcomposite #libxcomposite-dev
				libxcursor    #libxcursor-dev
				libxext       #libxext-dex
				libxinerama   #libxinerama-dev
				libxrandr     #libxrandr-dev
				#libxrender    #libxrender-dev
				#webkitgtk_4_1 #libwebkit2gtk-4.1-dev
				libGL         #libglu1-mesa-dev
				#mesa          #mesa-common-dev
			];

			# environment variables
			PROJECT_NAME = "vsts";
		};
	});
}
