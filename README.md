### Network Royale - BCIT COMP3980 Final Project Submission

This branch is the submission for Comp3980 Final project with minimal functionality.

Project Requirements:
- Peer-to-peer connection using UDP
- Display using ncurses library
- Player movement sent as a UDP packet
- Synchronized movement updated on both player's display
- Customized protocol for exchanging movement updates
- Read keyboard input
- Read controller input using SDL2 (Bonus)
- Random movement after a set period of inactivity
- Game elements (Bonus)

## Contributors

Kevin Nguyen (Banunu) [https://github.com/kvnbanunu]
Evin Gonzales [https://github.com/evin-gg]

## Prerequisites

This project uses a build system written by D'arcy Smith. To compile using the build system you need the template structure from [https://github.com/programming101dev/template-c]

Tested Platforms:
- Arch Linux 2024.12.01
- Manjaro 24.2
- Ubuntu 2024.04.1
- MacOS 14.2 (clang only)
- FreeBSD 14.0-RELEASE-p4

Dependencies:
- gcc or clang (Makefile specifies gcc)
- ncurses
- SDL2 (Optional)
- make

## Installation

Clone this repository:
```sh
git clone --single-branch -branch comp3980 https://github.com/kvnbanunu/networkroyale
```

Build with make:

```sh
make build
```

Build with D'arcy's system:

1. Link your .flags directory
   ```sh
   ./link-flags.sh <path>/.flags
   ```
2. Change compiler to gcc or clang
   ```sh
   ./change-compiler.sh -c <gcc or clang>
   ```
3. Generate cmakelist
   ```sh
   ./generate-cmakelists.sh
   ```
4. Build
   ```sh
   ./build.sh
   ```

## Usage

1. Change to build directory
   ```sh
   cd build/
   ```
2. Run server
   ```sh
   ./server
   ```
3. Open another terminal on the same computer or another computer on the same network
4. Run client
   ```sh
   ./client <ip address> <port>
   ```
