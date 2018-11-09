# Boiler

Boiler is a C++ tool to upload Unreal Engine 4 (or other engines) mods to the Steam Workshop.

## How to build

Dependencies

 * Visual Studio 2017
 * CMake 3.12
 * Steam SDK (to install into 'external/sdk' such as 'external/sdk/public' is a valid path)

Windows build

 * Open a terminal in the root folder
 * 'Run Configure.bat'
 * Open the 'Boiler.sln' solution in build/Win64
 * Switch build to 'Release'
 * Start the build, the output will be named 'Boiler.exe' in the Release folder

## How to use

Place Boiler.exe, steam_api64.dll, steam_appid.txt at the root of your Unreal Engine editor project, next to the "Plugins" directory where mods are installed. The AppID must match the game.

Run 'Boiler --game <game name> --mod <mod name>' and wait for the command to end. The game name would be the UE4 project name.

After the command has finished, visit your Steam Workshop page for the game you uploaded a mod for and complete the setup.
