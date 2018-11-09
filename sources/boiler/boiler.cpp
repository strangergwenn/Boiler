#include "boiler.h"

#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>

#include <windows.h>


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

void Boiler::upload(std::string& modName, std::string& modDescription, std::string& modContentPath, std::string& modPreviewPicture)
{
	// Copy parameters
	m_running = true;
	m_currentUGCListIndex = 0;
	m_modContentPath = modContentPath;
	m_modName = modName;
	m_modDescription = modDescription;
	m_modPreviewPicture = modPreviewPicture;

	// Build UGC query
	m_currentUGCListCount = 0;
	queryUGCList();

	// Wait for end of execution
	while (m_running)
	{
		SteamAPI_RunCallbacks();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}


/*----------------------------------------------------
	Internal
----------------------------------------------------*/

void Boiler::queryUGCList()
{
	m_currentUGCListIndex++;

	// Build parameters
	EUserUGCList modType = k_EUserUGCList_Published;
	EUGCMatchingUGCType matchingType = k_EUGCMatchingUGCType_Items;
	AccountID_t accountId = SteamUser()->GetSteamID().GetAccountID();

	// Start request
	UGCQueryHandle_t handle = SteamUGC()->CreateQueryUserUGCRequest(accountId, modType, matchingType, k_EUserUGCListSortOrder_CreationOrderDesc, m_appId, m_appId, m_currentUGCListIndex);
	SteamAPICall_t res = SteamUGC()->SendQueryUGCRequest(handle);
	m_ugcQueryHandler.Set(res, this, &Boiler::onUGCQueryComplete);
}

void Boiler::uploadMod(PublishedFileId_t publishedFileId)
{
	TCHAR modContentFullPath[MAX_PATH];
	TCHAR modPreviewFullPath[MAX_PATH];
	GetFullPathName(m_modContentPath.c_str(), MAX_PATH, modContentFullPath, NULL);
	GetFullPathName(m_modPreviewPicture.c_str(), MAX_PATH, modPreviewFullPath, NULL);

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
	if (!SteamUGC()->SetItemContent(handle, modContentFullPath))
	{
		std::cout << "Invalid mod content folder." << std::endl;
	}
	if (!SteamUGC()->SetItemPreview(handle, modPreviewFullPath))
	{
		std::cout << "Invalid mod preview picture." << std::endl;
	}

	// Submit update
	SteamAPICall_t res = SteamUGC()->SubmitItemUpdate(handle, "Boiler update");
	m_submitResultHandler.Set(res, this, &Boiler::onSubmitted);
}


/*----------------------------------------------------
	Steam callbacks
----------------------------------------------------*/

void Boiler::onUGCQueryComplete(SteamUGCQueryCompleted_t* result, bool failure)
{
	if (checkResult(result->m_eResult, false, failure))
	{
		m_currentUGCListCount += result->m_unNumResultsReturned;
		std::cout << "Got " << m_currentUGCListCount << " published mods out of " << result->m_unTotalMatchingResults << std::endl;

		// Analyze details to look for a matching mod file to update
		SteamUGCDetails_t details;
		for (uint32_t i = 0; i < result->m_unNumResultsReturned; i++)
		{
			SteamUGC()->GetQueryUGCResult(result->m_handle, i, &details);

			if (std::string(details.m_rgchTitle) == m_modName)
			{
				std::cout << "Found existing mod with ID " << details.m_nPublishedFileId << std::endl;
				uploadMod(details.m_nPublishedFileId);
				return;
			}
		}

		// If not found and more results left, request the next page
		if (m_currentUGCListCount < result->m_unTotalMatchingResults)
		{
			queryUGCList();
		}

		// If we're done, create the file
		else
		{
			SteamAPICall_t res = SteamUGC()->CreateItem(m_appId, EWorkshopFileType::k_EWorkshopFileTypeCommunity);
			m_createResultHandler.Set(res, this, &Boiler::onCreated);
		}
	}
}

void Boiler::onCreated(CreateItemResult_t* result, bool failure)
{
	if (checkResult(result->m_eResult, result->m_bUserNeedsToAcceptWorkshopLegalAgreement, failure))
	{
		std::cout << "Successfully created mod with ID " << result->m_nPublishedFileId << std::endl;

		uploadMod(result->m_nPublishedFileId);
	}
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
