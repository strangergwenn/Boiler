#include "inputparams.h"
#include "boiler/boiler.h"

#include "json/json.h"
#include <fstream>

#include <string>
#include <iostream>

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
	std::string modContentDirectory = modDirectory + "/Saved/StagedBuilds/WindowsNoEditor/" + gameName + "/Plugins/" + modName  + "/Content/Paks/WindowsNoEditor/";
	std::string modPreviewImage = modDirectory + "/Preview.png";

	// Print paths
	std::cout << "Plugin file : " << pluginFileName << std::endl;
	std::cout << "Content dir : " << modContentDirectory << std::endl;
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
			modContentDirectory,
			modPreviewImage);

		tool.shutdown();
	}

	// Exit
	return EXIT_SUCCESS;
}
