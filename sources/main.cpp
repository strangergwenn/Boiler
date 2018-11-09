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
	std::string pluginsDirectory = "Plugins";
	std::string modDirectory = pluginsDirectory + "/" + modName;
	std::string pluginFileName = modDirectory + "/" + modName + ".uplugin";
	std::string modPreviewImage = modDirectory + "/Preview.png";
	std::string modContentDirectoryWindows = modDirectory + "/Saved/StagedBuilds/WindowsNoEditor/" + gameName + "/Plugins/" + modName + "/Content/Paks/WindowsNoEditor/";
	std::string modContentDirectoryLinux = modDirectory + "/Saved/StagedBuilds/LinuxNoEditor/" + gameName + "/Plugins/" + modName + "/Content/Paks/LinuxNoEditor/";

	// Copy Windows & Linux folders to a unified folder
	std::string modContentBuildDirectory = modDirectory + "/Build";
	if (std::filesystem::exists(modContentBuildDirectory))
	{
		std::filesystem::remove_all(modContentBuildDirectory);
	}
	std::filesystem::create_directory(modContentBuildDirectory);
	std::filesystem::copy(modContentDirectoryWindows, modContentBuildDirectory + "/Windows");
	std::filesystem::copy(modContentDirectoryLinux, modContentBuildDirectory + "/Linux");

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
		std::cout << "Plugin file " << pluginFileName.c_str() << " could not be found." << std::endl;
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
