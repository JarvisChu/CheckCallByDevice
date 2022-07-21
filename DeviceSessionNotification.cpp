#include "DeviceSessionNotification.h"
#include <Functiondiscoverykeys_devpkey.h>
#include <stdio.h>
#include <string>

#define EXIT_ON_ERROR(hr) { if (FAILED(hr)) { goto Exit; } }
#define SAFE_RELEASE(punk) { if ((punk) != NULL) { (punk)->Release(); (punk) = NULL; } }
namespace Notification {

////////////////////// AudioSessionEventsImpl ////////////////////
AudioSessionEventsImpl::AudioSessionEventsImpl(INotificationCB* pCB) {
	m_cRef = 1;
	m_pCB = pCB;
}

AudioSessionEventsImpl::~AudioSessionEventsImpl() {
	if (m_pCB) m_pCB = nullptr;
}

ULONG STDMETHODCALLTYPE AudioSessionEventsImpl::AddRef() {
	return InterlockedIncrement(&m_cRef);
}

ULONG STDMETHODCALLTYPE AudioSessionEventsImpl::Release() {
	ULONG ulRef = InterlockedDecrement(&m_cRef);
	if (0 == ulRef) {
		delete this;
	}
	return ulRef;
}

HRESULT STDMETHODCALLTYPE AudioSessionEventsImpl::QueryInterface(REFIID riid, VOID **ppvInterface) {
	if (IID_IUnknown == riid) {
		AddRef();
		*ppvInterface = (IUnknown*)this;
	}
	else if (__uuidof(IAudioSessionEvents) == riid) {
		AddRef();
		*ppvInterface = (IAudioSessionEvents*)this;
	}
	else {
		*ppvInterface = NULL;
		return E_NOINTERFACE;
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE AudioSessionEventsImpl::OnChannelVolumeChanged(DWORD ChannelCount, float NewChannelVolumeArray[], DWORD ChangedChannel, LPCGUID EventContext)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE AudioSessionEventsImpl::OnDisplayNameChanged(LPCWSTR NewDisplayName, LPCGUID EventContext)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE AudioSessionEventsImpl::OnGroupingParamChanged(LPCGUID NewGroupingParam, LPCGUID EventContext)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE AudioSessionEventsImpl::OnIconPathChanged(LPCWSTR NewIconPath, LPCGUID EventContext)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE AudioSessionEventsImpl::OnSessionDisconnected(AudioSessionDisconnectReason DisconnectReason)
{
	if (m_pCB) {
		m_pCB->OnSessionDisconnected(DisconnectReason);
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE AudioSessionEventsImpl::OnSimpleVolumeChanged(float NewVolume, BOOL NewMute, LPCGUID EventContext)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE AudioSessionEventsImpl::OnStateChanged(AudioSessionState NewState)
{
	if (m_pCB) {
		m_pCB->OnStateChanged(NewState);
	}
	return S_OK;
}

////////////////////// AudioSessionNotificationImpl ////////////////////
AudioSessionNotificationImpl::AudioSessionNotificationImpl(INotificationCB* pCB)
{
	m_cRef = 1;
	m_pCB = pCB;
}

AudioSessionNotificationImpl::~AudioSessionNotificationImpl()
{
	if (m_pCB) m_pCB = nullptr;
}

ULONG STDMETHODCALLTYPE AudioSessionNotificationImpl::AddRef() {
	return InterlockedIncrement(&m_cRef);
}

ULONG STDMETHODCALLTYPE AudioSessionNotificationImpl::Release() {
	ULONG ulRef = InterlockedDecrement(&m_cRef);
	if (0 == ulRef) {
		delete this;
	}
	return ulRef;
}

HRESULT STDMETHODCALLTYPE AudioSessionNotificationImpl::QueryInterface(REFIID riid, VOID **ppvInterface) {
	if (IID_IUnknown == riid) {
		AddRef();
		*ppvInterface = (IUnknown*)this;
	}
	else if (__uuidof(IAudioSessionNotification) == riid) {
		AddRef();
		*ppvInterface = (IAudioSessionNotification*)this;
	}
	else {
		*ppvInterface = NULL;
		return E_NOINTERFACE;
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE AudioSessionNotificationImpl::OnSessionCreated(IAudioSessionControl *pSessionControl)
{
	if (m_pCB) {
		m_pCB->OnSessionCreated(pSessionControl);
	}
	return S_OK;
}

////////////////////// DeviceSessionNotification ////////////////////
DeviceSessionNotification::DeviceSessionNotification(EDataFlow e) {
	m_dataFlow = e;
	m_pAudioSessionEventsImpl = new AudioSessionEventsImpl(this);
	m_pAudioSessionNotificationImpl = new AudioSessionNotificationImpl(this);
}

DeviceSessionNotification::~DeviceSessionNotification() {
	if (m_pAudioSessionManager2 && m_pAudioSessionNotificationImpl) {
		m_pAudioSessionManager2->UnregisterSessionNotification(m_pAudioSessionNotificationImpl);
	}

	if (m_pAudioSessionEventsImpl) {
		delete m_pAudioSessionEventsImpl;
		m_pAudioSessionEventsImpl = nullptr;
	}

	if (m_pAudioSessionNotificationImpl) {
		delete m_pAudioSessionNotificationImpl;
		m_pAudioSessionNotificationImpl = nullptr;
	}

	SAFE_RELEASE(m_pAudioSessionManager2);
}

HRESULT DeviceSessionNotification::OnSessionCreated(IAudioSessionControl *pSessionControl)
{
	if (!pSessionControl) return S_OK;

	printf("OnSessionCreated\n");

	IAudioSessionControl2* pSessionControl2 = NULL;
	HRESULT hr = pSessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)& pSessionControl2);

	// Exclude system sound session
	hr = pSessionControl2->IsSystemSoundsSession();
	if (S_OK == hr) {
		//wprintf_s(L" this is a system sound.\n");
		SAFE_RELEASE(pSessionControl2);
		return S_OK;
	}

	// Determine which application is using Microphone for recording
	LPWSTR instId = NULL;
	hr = pSessionControl2->GetSessionInstanceIdentifier(&instId);
	if (FAILED(hr)) {
		wprintf_s(L"GetSessionInstanceIdentifier failed .\n");
		SAFE_RELEASE(pSessionControl2);
		return S_OK;
	}

	wprintf_s(L"SessionInstanceIdentifier: %s\n", instId);

	// Check if the application is one we want to monitor
	std::wstring wInstId(instId);
	if (wInstId.find(m_deviceName) == std::wstring::npos) {
		wprintf_s(L"This is NOT the device we want to monitor: %s\n", m_deviceName.c_str());
		SAFE_RELEASE(pSessionControl2);
		return S_OK;
	}

	hr = pSessionControl2->RegisterAudioSessionNotification(this->m_pAudioSessionEventsImpl);

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
		//result = TRUE;
		break;
	case AudioSessionStateExpired:
		wprintf_s(L"Session state: Expired\n", state);
		break;
	}

	return S_OK;
}


HRESULT DeviceSessionNotification::OnSessionDisconnected(AudioSessionDisconnectReason DisconnectReason)
{
	if (DisconnectReason == DisconnectReasonDeviceRemoval) {
		wprintf_s(L"[%s] OnSessionDisconnected, reason:DisconnectReasonDeviceRemoval\n", m_dataFlow == eCapture ? L"Mic" : L"Spk");
	}else if (DisconnectReason == DisconnectReasonServerShutdown) {
		wprintf_s(L"[%s] OnSessionDisconnected, reason:DisconnectReasonServerShutdown\n", m_dataFlow == eCapture ? L"Mic" : L"Spk");
	}else if (DisconnectReason == DisconnectReasonFormatChanged) {
		wprintf_s(L"[%s] OnSessionDisconnected, reason:DisconnectReasonFormatChanged\n", m_dataFlow == eCapture ? L"Mic" : L"Spk");
	}else if (DisconnectReason == DisconnectReasonSessionLogoff) {
		wprintf_s(L"[%s] OnSessionDisconnected, reason:DisconnectReasonSessionLogoff\n", m_dataFlow == eCapture ? L"Mic" : L"Spk");
	}else if (DisconnectReason == DisconnectReasonSessionDisconnected) {
		wprintf_s(L"[%s] OnSessionDisconnected, reason:DisconnectReasonSessionDisconnected\n", m_dataFlow == eCapture ? L"Mic" : L"Spk");
	}else if (DisconnectReason == DisconnectReasonExclusiveModeOverride) {
		wprintf_s(L"[%s] OnSessionDisconnected, reason:DisconnectReasonExclusiveModeOverride\n", m_dataFlow == eCapture ? L"Mic" : L"Spk");
	}

	if (m_pSessionCB) {
		m_pSessionCB->OnSessionDisconnected(m_dataFlow, DisconnectReason);
	}
	
	return S_OK;
}

HRESULT DeviceSessionNotification::OnStateChanged(AudioSessionState NewState)
{
	if (NewState == AudioSessionStateInactive) {
		wprintf_s(L"[%s] OnStateChanged, newState: AudioSessionStateInactive\n", m_dataFlow == eCapture ? L"Mic":L"Spk");
	}else if (NewState == AudioSessionStateActive) {
		wprintf_s(L"[%s] OnStateChanged, newState: AudioSessionStateActive\n", m_dataFlow == eCapture ? L"Mic" : L"Spk");
	}else if (NewState == AudioSessionStateExpired) {
		wprintf_s(L"[%s] OnStateChanged, newState: AudioSessionStateExpired\n", m_dataFlow == eCapture ? L"Mic" : L"Spk");
	}

	if (m_pSessionCB) {
		m_pSessionCB->OnStateChanged(m_dataFlow, NewState);
	}

	return S_OK;
}

HRESULT DeviceSessionNotification::RegisterNotification(const std::wstring& deviceName2Monitor, ISessionCB* pSessionCB)
{
	// 枚举所有设备，从设备列表中找到 麦克风/扬声器设备
	HRESULT hr = S_OK;
	IMMDeviceEnumerator* pEnumerator = NULL;
	//IAudioSessionManager2* pSessionManager = NULL;
	IMMDeviceCollection* pDeviceCollection = NULL;

	hr = CoInitialize(0);
	EXIT_ON_ERROR(hr);

	// Create the device enumerator.
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)& pEnumerator);
	EXIT_ON_ERROR(hr);

	// 枚举所有设备
	hr = pEnumerator->EnumAudioEndpoints(m_dataFlow, DEVICE_STATE_ACTIVE, &pDeviceCollection);
	EXIT_ON_ERROR(hr);

	UINT nDeviceCnt;
	hr = pDeviceCollection->GetCount(&nDeviceCnt);
	EXIT_ON_ERROR(hr);
	for (UINT i = 0; i < nDeviceCnt; i++)
	{
		IMMDevice* pDevice = NULL;
		hr = pDeviceCollection->Item(i, &pDevice);
		EXIT_ON_ERROR(hr);

		IPropertyStore* pProps = NULL;
		hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
		if (FAILED(hr)) { SAFE_RELEASE(pDevice); }
		EXIT_ON_ERROR(hr);

		// 获取设备的 FriendlyName
		PROPVARIANT varName;
		PropVariantInit(&varName);// Initialize container for property value.
		hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);// Get the endpoint's friendly-name property.
		if (FAILED(hr)) { SAFE_RELEASE(pProps); SAFE_RELEASE(pDevice); }
		EXIT_ON_ERROR(hr);

		// 通过 FriendlyName 判断是否是麦克风/扬声器设备
		std::wstring nameStr(varName.pwszVal);
		std::size_t foundEN = std::string::npos;
		std::size_t foundCN = std::string::npos;
		if (m_dataFlow == eCapture) {
			foundEN = nameStr.find(L"Microphone");
			foundCN = nameStr.find(L"麦克风");
		}
		else {
			foundEN = nameStr.find(L"Speaker");
			foundCN = nameStr.find(L"扬声器");
		}

		if (foundEN != std::string::npos || foundCN != std::string::npos) {
			wprintf_s(L"Endpoint friendly name: %s\n", nameStr.c_str());
			// Get the session manager.
			hr = pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, NULL, (void**)&m_pAudioSessionManager2);
			break;
		}

		SAFE_RELEASE(pProps);
		SAFE_RELEASE(pDevice);
	}

	// 如果 m_pAudioSessionManager2 为空，说明没有找到设备，返回错误
	if (!m_pAudioSessionManager2)
	{
		hr = S_FALSE;
		EXIT_ON_ERROR(hr);
	}

	// 注册会话通知
	m_deviceName = deviceName2Monitor;
	m_pSessionCB = pSessionCB;
	hr = m_pAudioSessionManager2->RegisterSessionNotification(m_pAudioSessionNotificationImpl);

	// Register之后必须要调用 IAudioSessionEnumerator::GetCount 才能生效
	IAudioSessionEnumerator* pSessionEnumerator = NULL;
	hr = m_pAudioSessionManager2->GetSessionEnumerator(&pSessionEnumerator); // Get the current list of sessions.
	int cbSessionCount = 0;
	hr = pSessionEnumerator->GetCount(&cbSessionCount);
	wprintf_s(L"Session count: %d\n", cbSessionCount);
	SAFE_RELEASE(pSessionEnumerator);

Exit:
	SAFE_RELEASE(pDeviceCollection);
	//SAFE_RELEASE(pSessionManager);
	SAFE_RELEASE(pEnumerator);

	return hr;
}



}