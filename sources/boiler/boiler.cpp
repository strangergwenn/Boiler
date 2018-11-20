#include "boiler.h"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>


/*----------------------------------------------------
	Public methods
----------------------------------------------------*/

bool Boiler::initialize()
{
	if (SteamAPI_Init())
	{
		std::ifstream appIdFile("steam_appid.txt");

		appIdFile >> m_appId;

		if (m_appId != 0)
		{
			std::cout << "Initialized for AppId " << m_appId << std::endl;
			return true;
		}
		else
		{
			std::cout << "Failed to extract Steam AppId, is steam_appid.txt present ?" << std::endl;
		}
	}
	else
	{
		std::cout << "Failed to initialize Steam." << std::endl;
	}

	return false;
}

void Boiler::shutdown()
{
	SteamAPI_Shutdown();
}

std::vector<ModInfo> Boiler::discoverMods()
{
	std::vector<ModInfo> results;

	// Build UGC query
	queryUGCList(false, true);
	wait();
	std::cout << "Mod list loaded" << std::endl;

	// Search UGC list for installed mods
	for (SteamUGCDetails_t details : m_currentUGCList)
	{
		uint32_t modState = SteamUGC()->GetItemState(details.m_nPublishedFileId);

		if (modState & k_EItemStateInstalled)
		{
			uint64 sizeOnDisk;
			char folder[4096];
			uint32 timestamp;

			if (SteamUGC()->GetItemInstallInfo(details.m_nPublishedFileId, &sizeOnDisk, folder, 4096, &timestamp))
			{
				ModInfo info;
				info.path = folder;
				info.timestamp = timestamp;
				info.identifier = details.m_nPublishedFileId;
				results.push_back(info);

				std::cout << "Found installed mod " << details.m_rgchTitle << " at " << folder << std::endl;
			}
		}
	}

	return results;
}

void Boiler::uploadMod(const std::string& modName, const std::string& modDescription, const std::string& modContentPath, const std::string& modPreviewPicture)
{
	// Copy parameters
	m_modContentPath = modContentPath;
	m_modName = modName;
	m_modDescription = modDescription;
	m_modPreviewPicture = modPreviewPicture;

	// Build UGC query
	queryUGCList(true, true);
	wait();
	std::cout << "Mod list loaded" << std::endl;

	// Look at the UGC results to search for that mod
	bool foundMod = false;
	for (SteamUGCDetails_t details : m_currentUGCList)
	{
		if (std::string(details.m_rgchTitle) == m_modName)
		{
			foundMod = true;
			m_currentModId = details.m_nPublishedFileId;
			std::cout << "Found existing mod with ID " << details.m_nPublishedFileId << std::endl;
		}
	}

	// Create mod if not found
	if (!foundMod)
	{
		std::cout << "Creating new mod" << std::endl;

		createMod();
		wait();
		std::cout << "Mod created" << std::endl;
	}

	// Update mod
	updateMod(m_currentModId);
	wait();
	std::cout << "Upload done" << std::endl;
}


/*----------------------------------------------------
	Internal
----------------------------------------------------*/

void Boiler::queryUGCList(bool published, bool clearPreviousResults)
{
	m_running = true;
	m_onlyPublishedUGC = published;

	// Update the UGC list state
	if (clearPreviousResults)
	{
		m_currentUGCList.clear();
		m_currentUGCListIndex = 0;
	}
	m_currentUGCListIndex++;

	// Build parameters
	EUserUGCList modType = published ? k_EUserUGCList_Published : k_EUserUGCList_Subscribed;
	EUGCMatchingUGCType matchingType = k_EUGCMatchingUGCType_Items;
	AccountID_t accountId = SteamUser()->GetSteamID().GetAccountID();

	// Start request
	UGCQueryHandle_t handle = SteamUGC()->CreateQueryUserUGCRequest(accountId, modType, matchingType, k_EUserUGCListSortOrder_CreationOrderDesc, m_appId, m_appId, m_currentUGCListIndex);
	SteamAPICall_t res = SteamUGC()->SendQueryUGCRequest(handle);
	m_ugcQueryHandler.Set(res, this, &Boiler::onUGCQueryComplete);
}

void Boiler::createMod()
{
	m_running = true;

	// Submit creation
	SteamAPICall_t res = SteamUGC()->CreateItem(m_appId, EWorkshopFileType::k_EWorkshopFileTypeCommunity);
	m_createResultHandler.Set(res, this, &Boiler::onCreated);
}

void Boiler::updateMod(PublishedFileId_t publishedFileId)
{
	m_running = true;

	// Set item properties
	UGCUpdateHandle_t handle = SteamUGC()->StartItemUpdate(m_appId, publishedFileId);
	if (!SteamUGC()->SetItemUpdateLanguage(handle, "english"))
	{
		std::cout << "Invalid language." << std::endl;
	}
	if (!SteamUGC()->SetItemTitle(handle, m_modName.c_str()))
	{
		std::cout << "Invalid mod name." << std::endl;
	}
	if (!SteamUGC()->SetItemDescription(handle, m_modDescription.c_str()))
	{
		std::cout << "Invalid mod description." << std::endl;
	}
	if (!SteamUGC()->SetItemContent(handle, std::filesystem::absolute(m_modContentPath).string().c_str()))
	{
		std::cout << "Invalid mod content folder." << std::endl;
	}
	if (!SteamUGC()->SetItemPreview(handle, std::filesystem::absolute(m_modPreviewPicture).string().c_str()))
	{
		std::cout << "Invalid mod preview picture." << std::endl;
	}

	// Submit update
	SteamAPICall_t res = SteamUGC()->SubmitItemUpdate(handle, "Boiler update");
	m_submitResultHandler.Set(res, this, &Boiler::onSubmitted);
}

void Boiler::wait()
{
	while (m_running)
	{
		SteamAPI_RunCallbacks();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}


/*----------------------------------------------------
	Steam callbacks
----------------------------------------------------*/

void Boiler::onUGCQueryComplete(SteamUGCQueryCompleted_t* result, bool failure)
{
	if (checkResult(result->m_eResult, false, failure))
	{
		// Analyze details to look for a matching mod file to update
		SteamUGCDetails_t details;
		for (uint32_t i = 0; i < result->m_unNumResultsReturned; i++)
		{
			SteamUGC()->GetQueryUGCResult(result->m_handle, i, &details);
			m_currentUGCList.push_back(details);
		}

		std::cout << "Got " << m_currentUGCList.size() << " published mods out of " << result->m_unTotalMatchingResults << std::endl;

		// If not found and more results left, request the next page
		if (m_currentUGCList.size() < result->m_unTotalMatchingResults)
		{
			queryUGCList(m_onlyPublishedUGC, false);
		}
		else
		{
			m_running = false;
		}
	}
	else
	{
		m_running = false;
	}
}

void Boiler::onCreated(CreateItemResult_t* result, bool failure)
{
	if (checkResult(result->m_eResult, result->m_bUserNeedsToAcceptWorkshopLegalAgreement, failure))
	{
		std::cout << "Successfully created mod with ID " << result->m_nPublishedFileId << std::endl;
		m_currentModId = result->m_nPublishedFileId;
	}

	m_running = false;
}

void Boiler::onSubmitted(SubmitItemUpdateResult_t* result, bool failure)
{
	if (checkResult(result->m_eResult, result->m_bUserNeedsToAcceptWorkshopLegalAgreement, failure))
	{
		std::cout << "Successfully uploaded mod" << std::endl;
	}

	m_running = false;
}

bool Boiler::checkResult(EResult result, bool agreementRequired, bool failure)
{
	if (agreementRequired)
	{
		std::cout << "In order to use this tool, you must agree to the Steam Workshop terms of service as outlined at http://steamcommunity.com/sharedfiles/workshoplegalagreement." << std::endl;
	}
	else if (failure)
	{
		std::cout << "Callback failed." << std::endl;
	}
	else if (result == k_EResultOK)
	{
		return true;
	}
	else
	{
		switch (result)
		{
		case k_EResultInsufficientPrivilege:
			std::cout << "Invalid permissions for uploading." << std::endl;
			break;

		case k_EResultTimeout:
			std::cout << "Network timeout." << std::endl;
			break;

		case k_EResultAccessDenied:
			std::cout << "The user doesn't own a license for the provided app ID." << std::endl;
			break;

		case k_EResultNotLoggedOn:
			std::cout << "The user is not logged on to Steam." << std::endl;
			break;

		case k_EResultFileNotFound:
			std::cout << "Failed to get the workshop info for the item, or failed to read the preview file, or the provided content folder is not valid." << std::endl;
			break;

		case k_EResultInvalidParam:
			std::cout << "Either the provided app ID is invalid or doesn't match the consumer app ID of the item, or ISteamUGC was not enabled for this application, or the preview file is smaller than 16 bytes." << std::endl;
			break;

		case k_EResultAccountLimitExceeded:
			std::cout << "The preview image is too large, it must be less than 1 megabyte; or there is not enough space available on the user's Steam Cloud." << std::endl;
			break;

		default:
			std::cout << "An error occurred (" << result << ")" << std::endl;
			break;
		}
	}

	return false;
}
