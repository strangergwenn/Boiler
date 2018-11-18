#include "inputparams.h"
#include "boiler/boiler.h"

#include "json/json.h"

#include <filesystem>
#include <string>
#include <iostream>
#include <fstream>


int main(int argc, char** argv)
{

	// Get parameters
	InputParams params(argc, argv);
	std::string gameName, modName;
	params.getOption("--game", "Mod name", gameName);
	params.getOption("--mod", "Mod name", modName);

	// Check result
	if (gameName.length() == 0 || modName.length() == 0)
	{
		std::cout << "Usage : Boiler --game <name> --mod <name>" << std::endl;
		return EXIT_SUCCESS;
	}

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
	std::string modBuildDirectory = modDirectory + "/Build";
	std::string modBuildContentDirectory = modBuildDirectory + "/Content";
	std::string modBuildPakDirectory = modBuildContentDirectory + "/Paks";

	try
	{
		// Create build folder
		if (std::filesystem::exists(modBuildDirectory))
		{
			std::filesystem::remove_all(modBuildDirectory);
		}
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

		return EXIT_SUCCESS;
	}

	// Print paths
	std::cout << "Plugin file : " << modFileName << std::endl;
	std::cout << "Content dir : " << modBuildDirectory << std::endl;
	std::cout << "Preview dir : " << modPreviewImage << std::endl;

	// Read mod file
	std::ifstream modFile(modFileName);
	std::stringstream modFileContent;
	modFileContent << modFile.rdbuf();

	// Check result
	if (modFileContent.str().length() == 0)
	{
		std::cout << "Plugin file " << modFileName << " could not be found." << std::endl;
		return EXIT_SUCCESS;
	}

	// Parse mod file
	Json::Reader reader;
	Json::Value plugin;
	reader.parse(modFileContent.str(), plugin);

	// Run Boiler upload tool
	Boiler tool;
	if (tool.initialize())
	{
		tool.upload(
			plugin["FriendlyName"].asString(),
			plugin["Description"].asString(),
			modBuildDirectory,
			modPreviewImage);

		tool.shutdown();
	}

	// Exit
	return EXIT_SUCCESS;
}
