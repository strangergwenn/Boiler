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
	std::string modName;
	params.getOption("--name", "Mod name", modName);

	// Check result
	if (modName.length() == 0)
	{
		std::cout << "Usage : Boiler --name <mod name>" << std::endl;
		return EXIT_SUCCESS;
	}

	// Build paths
	std::string pluginsDirectory = "Plugins";
	std::string modDirectory = pluginsDirectory + "/" + modName;
	std::string pluginFileName = modDirectory + "/" + modName + ".uplugin";
	std::string modContentDirectory = modDirectory + "/content";
	std::string modPreviewImage = modDirectory + "/preview.png";

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
