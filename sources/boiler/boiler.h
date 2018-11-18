#include <cstdint>
#include <string>
#include <vector>
#include <atomic>

#include "steam/steam_api.h"
#include "steam/isteamugc.h"

class Boiler
{

public:

	/*----------------------------------------------------
		Public API
	----------------------------------------------------*/

	Boiler()
		: m_appId(0)
	{}

	/**
	 * @brief Initialize the Steamworks SDK
	 * @return true if successful
	 */
	bool initialize();

	/** @brief Shutdown the Steamworks SDK */
	void shutdown();

	/**
	 * @brief Find all currently installed mods
	 * @return List of mod folders
	*/
	std::vector<std::pair<std::string, uint32>> discoverMods();

	/**
	* @brief Update mod contents
	* @param modName           Name of the mod file to load
	* @param modDescription    Description to send to Steamworks
	* @param modPath           Folder to use as mod content for uploading
	* @param modPreviewPicture Preview image as a PNG file
	*/
	void uploadMod(const std::string& modName, const std::string& modDescription, const std::string& modContentPath, const std::string& modPreviewPicture);


private:

	/*----------------------------------------------------
		Internal
	----------------------------------------------------*/

	/** @brief Get up to 50 published mods from this Steam user */
	void queryUGCList(bool clearPreviousResults = true);

	/**Create a new mod file */
	void createMod();

	/** @brief Upload a submitted mod */
	void updateMod(PublishedFileId_t publishedFileId);

	/** Wait for a query to end */
	void wait();


private:

	/*----------------------------------------------------
		Steam callbacks
	----------------------------------------------------*/

	/** @brief List of UGC is ready */
	void onUGCQueryComplete(SteamUGCQueryCompleted_t* result, bool failure);

	/** @brief Callback on mod created */
	void onCreated(CreateItemResult_t* result, bool failure);

	/** @brief Callback on mod submitted */
	void onSubmitted(SubmitItemUpdateResult_t* result, bool failure);

	/** @brief Check the callback results are correct */
	bool checkResult(EResult result, bool agreementRequired, bool failure);


private:

	/*----------------------------------------------------
		Data
	----------------------------------------------------*/

	// Runtime data
	AppId_t                                       m_appId;
	std::atomic<bool>                             m_running;
	uint32_t                                      m_currentUGCListIndex;
	std::vector<SteamUGCDetails_t>                m_currentUGCList;
	PublishedFileId_t                             m_currentModId;

	// Mod settings
	std::string                                   m_modContentPath;
	std::string                                   m_modName;
	std::string                                   m_modDescription;
	std::string                                   m_modPreviewPicture;

	// Callbacks
	CCallResult<Boiler, SteamUGCQueryCompleted_t> m_ugcQueryHandler;
	CCallResult<Boiler, CreateItemResult_t>       m_createResultHandler;
	CCallResult<Boiler, SubmitItemUpdateResult_t> m_submitResultHandler;

};
