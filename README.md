# NovelCHIP-8
A CHIP-8 emulator powered by [NovelRT](https://github.com/novelrt/NovelRT)!

This was really created for two purposes:
1. Seeing how an emulator works, and
2. Seeing if I can create something using NovelRT (which I happen to be a developer for).
_(Making emulators in game engines is usually overkill, but i digress...)_

I'm more or less proud of the second one, and although I wound up using a _lot_ of reference material for #1, it was definitely an experience nonetheless!

## Running

Chip8.exe [insert path to CHIP-8 rom here]

Example:
`chip8.exe C:\roms\PONG`
_(Your CHIP-8 rom may or may not have a file extension on it.)_


## Build Requirements _(for Windows)_

- CMake (at least version 3.13 or higher)
- Git for Windows
- Visual Studio 2017/2019
- vcpkg (found [here](https://github.com/microsoft/vcpkg))

## Building

1. Clone the repository
2. Use the following command for vcpkg to pull all dependencies required by NovelRT: 
`vcpkg install freetype glad glfw3 glm gtest libsndfile lua nethost openal-soft spdlog --triplet x64-windows `
3. Once the above command is performed, use the following command to tell `vcpkg` that we'd like to use it's toolchain: `vcpkg integrate install`
Please notate the path that the command gives you in regards to the toolchain file that it created.
4. Navigate back to the repository's folder, and create a build folder (no in-source building please!). Then, navigate inside the folder you just created.
5. Use the following to tell CMake to generate the solution files with the following:
`cmake ../. -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg-toolchain-file] `
_(Note: You may have to specify `-G "Visual Studio 15 2017" -A x64` or `-G "Visual Studio 16 2019" -A x64` respectively if it does not automatically specify Visual Studio as the generator.)_
6. That's it! If done successfully, CMake should have generated the solution, and you can build normally within Visual Studio.


