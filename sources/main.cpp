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

	// Build paths
	std::string ModsDirectory = "Mods";
	std::string modDirectory = ModsDirectory + "/" + modName;
	std::string pluginFileName = modDirectory + "/" + modName + ".uplugin";
	std::string modPreviewImage = modDirectory + "/Preview.png";
	std::string modContentDirectoryWindows = modDirectory + "/Saved/StagedBuilds/WindowsNoEditor/" + gameName + "/Mods/" + modName + "/Content/Paks/WindowsNoEditor/";
	std::string modContentDirectoryLinux = modDirectory + "/Saved/StagedBuilds/LinuxNoEditor/" + gameName + "/Mods/" + modName + "/Content/Paks/LinuxNoEditor/";
	std::string modContentDirectoryMac = modDirectory + "/Saved/StagedBuilds/MacNoEditor/" + gameName + "/Mods/" + modName + "/Content/Paks/MacNoEditor/";
	std::string modContentBuildDirectory = modDirectory + "/Build";

	// Copy Windows & Linux folders to a unified folder
	try
	{
		// Create build folder
		if (std::filesystem::exists(modContentBuildDirectory))
		{
			std::filesystem::remove_all(modContentBuildDirectory);
		}
		std::filesystem::create_directory(modContentBuildDirectory);

		// Copy Windows
		std::filesystem::copy(modContentDirectoryWindows, modContentBuildDirectory + "/Windows");

		// Copy Linux
		if (std::filesystem::exists(modContentDirectoryLinux))
		{
			std::filesystem::copy(modContentDirectoryLinux, modContentBuildDirectory + "/Linux");
		}

		// Copy Mac
		if (std::filesystem::exists(modContentDirectoryMac))
		{
			std::filesystem::copy(modContentDirectoryMac, modContentBuildDirectory + "/Mac");
		}
	}
	catch (const std::filesystem::filesystem_error & ex)
	{
		std::cout << ex.what() << std::endl;
		std::cout << "Required mod files could not be found !" << std::endl;
		std::cout << " - Make sure " << pluginFileName << " exists." << std::endl;
		std::cout << " - Make sure " << modPreviewImage << " exists." << std::endl;
		std::cout << " - Make sure " << modContentDirectoryWindows << " exists." << std::endl;

		return EXIT_SUCCESS;
	}

	// Print paths
	std::cout << "Plugin file : " << pluginFileName << std::endl;
	std::cout << "Content dir : " << modContentBuildDirectory << std::endl;
	std::cout << "Preview dir : " << modPreviewImage << std::endl;

	// Read mod file
	std::ifstream modFile(pluginFileName);
	std::stringstream modFileContent;
	modFileContent << modFile.rdbuf();

	// Check result
	if (modFileContent.str().length() == 0)
	{
		std::cout << "Plugin file " << pluginFileName << " could not be found." << std::endl;
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
			modContentBuildDirectory,
			modPreviewImage);

		tool.shutdown();
	}

	// Exit
	return EXIT_SUCCESS;
}
