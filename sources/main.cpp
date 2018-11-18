#include "inputparams.h"
#include "boiler/boiler.h"

#include "json/json.h"

#include <filesystem>
#include <string>
#include <iostream>
#include <fstream>

#ifdef _WIN32
#  include <windows.h>
#endif


/*----------------------------------------------------
	Modding support tools
----------------------------------------------------*/

void uploadMod(Boiler* tool, const std::string& gameName, const std::string& modName)
{
	// Mod paths
	std::string modsDirectory = "Mods";
	std::string modDirectory = modsDirectory + "/" + modName;
	std::string modFileName = modDirectory + "/" + modName + ".uplugin";
	std::string modPreviewImage = modDirectory + "/Preview.png";

	// Cooked pak file paths
	std::string modContentDirectoryWindows = modDirectory + "/Saved/StagedBuilds/WindowsNoEditor/" + gameName + "/Mods/" + modName + "/Content/Paks/WindowsNoEditor/";
	std::string modContentDirectoryLinux = modDirectory + "/Saved/StagedBuilds/LinuxNoEditor/" + gameName + "/Mods/" + modName + "/Content/Paks/LinuxNoEditor/";
	std::string modContentDirectoryMac = modDirectory + "/Saved/StagedBuilds/MacNoEditor/" + gameName + "/Mods/" + modName + "/Content/Paks/MacNoEditor/";

	// Destination paths
	std::string modBuildRootDirectory = modDirectory + "/Build";
	std::string modBuildDirectory = modBuildRootDirectory + "/" + modName;
	std::string modBuildContentDirectory = modBuildDirectory + "/Content";
	std::string modBuildPakDirectory = modBuildContentDirectory + "/Paks";

	try
	{
		// Create build folder
		if (std::filesystem::exists(modBuildDirectory))
		{
			std::filesystem::remove_all(modBuildDirectory);
		}
		std::filesystem::create_directory(modBuildRootDirectory);
		std::filesystem::create_directory(modBuildDirectory);
		std::filesystem::create_directory(modBuildContentDirectory);
		std::filesystem::create_directory(modBuildPakDirectory);

		// Copy required files
		std::filesystem::copy(modFileName, modBuildDirectory);
		std::filesystem::copy(modContentDirectoryWindows, modBuildPakDirectory + "/WindowsNoEditor");

		// Copy Linux
		if (std::filesystem::exists(modContentDirectoryLinux))
		{
			std::filesystem::copy(modContentDirectoryLinux, modBuildPakDirectory + "/LinuxNoEditor");
		}

		// Copy Mac
		if (std::filesystem::exists(modContentDirectoryMac))
		{
			std::filesystem::copy(modContentDirectoryMac, modBuildPakDirectory + "/MacNoEditor");
		}
	}
	catch (const std::filesystem::filesystem_error & ex)
	{
		std::cout << ex.what() << std::endl;
		std::cout << "Required mod files could not be found !" << std::endl;
		std::cout << " - Make sure " << modFileName << " exists." << std::endl;
		std::cout << " - Make sure " << modPreviewImage << " exists." << std::endl;
		std::cout << " - Make sure " << modContentDirectoryWindows << " exists." << std::endl;

		return;
	}

	// Print paths
	std::cout << "Plugin file : " << modFileName << std::endl;
	std::cout << "Content dir : " << modBuildRootDirectory << std::endl;
	std::cout << "Preview dir : " << modPreviewImage << std::endl;

	// Read mod file
	std::ifstream modFile(modFileName);
	std::stringstream modFileContent;
	modFileContent << modFile.rdbuf();

	// Check result
	if (modFileContent.str().length() == 0)
	{
		std::cout << "Plugin file " << modFileName << " could not be found." << std::endl;
		return;
	}

	// Parse mod file
	Json::Reader reader;
	Json::Value plugin;
	reader.parse(modFileContent.str(), plugin);

	// Run Boiler upload tool
	tool->uploadMod(
		plugin["FriendlyName"].asString(),
		plugin["Description"].asString(),
		modBuildRootDirectory,
		modPreviewImage);
}

void installMod(const std::string& modName, const std::string& sourcePath, const std::string& destinationPath, uint32_t modTimestamp)
{
	// Setup paths
	std::string sourceDir = sourcePath + "/" + modName;
	std::string destinationDir = destinationPath + "/" + modName;
	std::string timestampFile = destinationDir + "/time";

	try
	{
		// It's an update
		if (std::filesystem::exists(timestampFile))
		{
			uint32_t previousTimestamp;

			std::ifstream timeFile;
			timeFile.open(timestampFile);
			timeFile >> previousTimestamp;
			timeFile.close();

			if (modTimestamp > previousTimestamp)
			{
				std::cout << "Updating " << modName << std::endl;

				if (std::filesystem::exists(destinationDir))
				{
					std::filesystem::remove_all(destinationDir);
				}
				std::filesystem::copy(sourceDir, destinationDir, std::filesystem::copy_options::recursive);
			}
			else
			{
				std::cout << "Ignoring up-to-date mod "<< modName << std::endl;
			}
		}

		// Fresh install
		else
		{
			std::cout << "Installing " << modName << std::endl;

			if (std::filesystem::exists(destinationDir))
			{
				std::filesystem::remove_all(destinationDir);
			}
			std::filesystem::copy(sourceDir, destinationDir, std::filesystem::copy_options::recursive);

			std::ofstream timeFile;
			timeFile.open(timestampFile);
			timeFile << modTimestamp;
			timeFile.close();
		}
	}
	catch (const std::filesystem::filesystem_error & ex)
	{
		std::cout << ex.what() << std::endl;
	}
}

void installMods(Boiler* tool, const std::string& gameName)
{
	std::string modsDirectory = gameName + "/Mods";
	std::filesystem::create_directory(modsDirectory);

	// Assume a "ModName" folder in the source mod directory, copy it as-is in /Mods/
	for (std::pair<std::string, uint32> mod : tool->discoverMods())
	{
		for (auto& p : std::filesystem::directory_iterator(mod.first))
		{
			if (p.is_directory())
			{
				installMod(p.path().filename().string(), mod.first, modsDirectory, mod.second);
			}
		}
	}
}

std::string getExecutableExtension()
{
#ifdef _WIN32
	return ".exe";
#else
	return ".sh";
#endif
}

std::string detectUnrealGame()
{
	std::vector<std::string> folders;
	std::vector<std::string> executables;

	// Look for executables and folders
	bool foundEngine = false;
	for (auto& p : std::filesystem::directory_iterator("./"))
	{
		std::string fileName = p.path().filename().string();
		if (p.is_regular_file()
			&& fileName.find("Launcher") == std::string::npos
			&& fileName.find(getExecutableExtension()) != std::string::npos)
		{
			executables.push_back(fileName);
		}
		else if (p.is_directory())
		{
			if (fileName == "Engine")
			{
				foundEngine = true;
			}
			else
			{
				folders.push_back(fileName);
			}
		}
	}

	// A UE4 game called GameName features GameName.exe, an Engine folder and a GameName folder
	if (foundEngine)
	{
		for (std::string executable : executables)
		{
			for (std::string folder : folders)
			{
				if (executable.rfind(folder, 0) == 0)
				{
					std::cout << "Found game executable " << executable << std::endl;
					return folder;
				}
			}
		}
	}

	return std::string();
}

void launchGame(const std::string& gameName, InputParams& params)
{
	std::string commandLine = std::filesystem::current_path().string() + "/" + gameName + getExecutableExtension() + " " + params.getRaw();
	std::cout << commandLine << std::endl;

#ifdef _WIN32
	WinExec(commandLine.c_str(), SW_HIDE);
#else
	system(commandLine.c_str());
#endif
}


/*----------------------------------------------------
	Main
----------------------------------------------------*/

int main(int argc, char** argv)
{
	// Get parameters
	InputParams params(argc, argv);
	std::string gameName, modName;
	params.getOption("--game", "Mod name", gameName);
	params.getOption("--mod", "Mod name", modName);

	// Help parameter
	if (params.isSet("--help") || params.isSet("-h"))
	{
		std::cout << "Boiler is a tool for uploading mods to the Steam workshop, developed by Deimos Games for Helium Rain." << std::endl;
		std::cout << "To create or update a mod, please run Boiler --game <name> --mod <name>." << std::endl;
		std::cout << "This tool also installs mods and launches the packaged game when run without these parameters." << std::endl;
		return EXIT_SUCCESS;
	}

	// Main tool run
	Boiler tool;
	if (tool.initialize())
	{
		// Upload mod
		if (gameName.length() && modName.length())
		{
			uploadMod(&tool, gameName, modName);
		}

		// Process local mods and launch the game
		else
		{
			// Get local UE4 game
			gameName = detectUnrealGame();
			if (!gameName.length())
			{
				std::cout << "Couldn't detect game executable. Run the tool with --help." << std::endl;
				return EXIT_SUCCESS;
			}

			// Install mods
			installMods(&tool, gameName);

			// Run game
			launchGame(gameName, params);
		}

		tool.shutdown();
	}

	// Exit
	return EXIT_SUCCESS;
}

#ifdef _WIN32
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	main(__argc, __argv);
}
#endif
