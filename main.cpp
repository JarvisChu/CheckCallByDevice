#include <stdio.h>
#include <Windows.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <string>

#include "CallMgr.h"

#pragma comment(lib, "winmm.lib")

#define SAFE_RELEASE(punk) { if ((punk) != NULL) { (punk)->Release(); (punk) = NULL; } }

BOOL IsMicrophoneRecording()
{
	// #1 Get the audio endpoint associated with the microphone device
	HRESULT hr = S_OK;
	IMMDeviceEnumerator* pEnumerator = NULL;
	IAudioSessionManager2* pSessionManager = NULL;
	BOOL result = FALSE;

	CoInitialize(0);

	// Create the device enumerator.
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator),NULL, CLSCTX_ALL,__uuidof(IMMDeviceEnumerator),(void**)& pEnumerator);

	IMMDeviceCollection* dCol = NULL;
	hr = pEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &dCol);
	UINT dCount;
	hr = dCol->GetCount(&dCount);
	for (UINT i = 0; i < dCount; i++)
	{
		IMMDevice* pCaptureDevice = NULL;
		hr = dCol->Item(i, &pCaptureDevice);

		IPropertyStore* pProps = NULL;
		hr = pCaptureDevice->OpenPropertyStore(STGM_READ, &pProps);

		PROPVARIANT varName;
		// Initialize container for property value.
		PropVariantInit(&varName);

		// Get the endpoint's friendly-name property.
		hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);

		std::wstring nameStr(varName.pwszVal);

		// #2 Determine whether it is the microphone device you are focusing on
		std::size_t foundEN = nameStr.find(L"Microphone");
		std::size_t foundCN = nameStr.find(L"麦克风");
		if (foundEN != std::string::npos || foundCN != std::string::npos)
		{
			// Print endpoint friendly name.
			//printf("Endpoint friendly name: \"%S\"\n", varName.pwszVal);
			wprintf_s(L"Endpoint friendly name: %s\n", nameStr.c_str());

			// Get the session manager.
			hr = pCaptureDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL,NULL, (void**)& pSessionManager);
			break;
		}
	}

	// Get session state
	if (!pSessionManager)
	{
		return (result = FALSE);
	}

	int cbSessionCount = 0;
	LPWSTR pswSession = NULL;

	IAudioSessionEnumerator* pSessionList = NULL;
	IAudioSessionControl* pSessionControl = NULL;
	IAudioSessionControl2* pSessionControl2 = NULL;

	// Get the current list of sessions.
	hr = pSessionManager->GetSessionEnumerator(&pSessionList);

	// Get the session count.
	hr = pSessionList->GetCount(&cbSessionCount);
	//wprintf_s(L"Session count: %d\n", cbSessionCount);

	for (int index = 0; index < cbSessionCount; index++)
	{
		CoTaskMemFree(pswSession);
		SAFE_RELEASE(pSessionControl);

		// Get the <n>th session.
		hr = pSessionList->GetSession(index, &pSessionControl);

		hr = pSessionControl->QueryInterface(
			__uuidof(IAudioSessionControl2), (void**)& pSessionControl2);

		// Exclude system sound session
		hr = pSessionControl2->IsSystemSoundsSession();
		if (S_OK == hr)
		{
			//wprintf_s(L" this is a system sound.\n");
			continue;
		}

		// Optional. Determine which application is using Microphone for recording
		LPWSTR instId = NULL;
		hr = pSessionControl2->GetSessionInstanceIdentifier(&instId);
		if (S_OK == hr)
		{
			wprintf_s(L"SessionInstanceIdentifier: %s\n", instId);
		}

		AudioSessionState state;
		hr = pSessionControl->GetState(&state);
		switch (state)
		{
		case AudioSessionStateInactive:
			wprintf_s(L"Session state: Inactive\n", state);
			break;
		case AudioSessionStateActive:
			// #3 Active state indicates it is recording, otherwise is not recording.
			wprintf_s(L"Session state: Active\n", state);
			result = TRUE;
			break;
		case AudioSessionStateExpired:
			wprintf_s(L"Session state: Expired\n", state);
			break;
		}
	}

done:
	// Clean up.
	SAFE_RELEASE(pSessionControl);
	SAFE_RELEASE(pSessionList);
	SAFE_RELEASE(pSessionControl2);
	SAFE_RELEASE(pSessionManager);
	SAFE_RELEASE(pEnumerator);

	return result;
}

int main()
{
	setlocale(LC_ALL, ""); // wprintf

	//IsMicrophoneRecording();

	CallMgr::GetInstance().SetCallEventListener(L"SoundRec.exe", L"cloudmusic.exe", nullptr);

	getchar();

	return 0;
}