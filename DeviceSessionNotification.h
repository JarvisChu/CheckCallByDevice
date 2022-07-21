#pragma once

#include <Windows.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <string>

namespace Notification {

	class INotificationCB {
	public:
		virtual HRESULT OnSessionCreated(IAudioSessionControl *pSessionControl) = 0;
		virtual HRESULT OnSessionDisconnected(AudioSessionDisconnectReason DisconnectReason) = 0;
		virtual HRESULT OnStateChanged(AudioSessionState NewState) = 0;
	};

	class ISessionCB {
	public:
		virtual HRESULT OnSessionDisconnected(EDataFlow dataFlow, AudioSessionDisconnectReason DisconnectReason) = 0;
		virtual HRESULT OnStateChanged(EDataFlow dataFlow, AudioSessionState NewState) = 0;
	};

	class AudioSessionEventsImpl : public IAudioSessionEvents {
	public:
		AudioSessionEventsImpl(INotificationCB* pCB);
		~AudioSessionEventsImpl();

		// IUnknown methods -- AddRef, Release, and QueryInterface
		ULONG STDMETHODCALLTYPE AddRef();
		ULONG STDMETHODCALLTYPE Release();
		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface);

		// IAudioSessionEvents methods
		HRESULT STDMETHODCALLTYPE OnChannelVolumeChanged(DWORD ChannelCount, float NewChannelVolumeArray[], DWORD ChangedChannel, LPCGUID EventContext);
		HRESULT STDMETHODCALLTYPE OnDisplayNameChanged(LPCWSTR NewDisplayName, LPCGUID EventContext);
		HRESULT STDMETHODCALLTYPE OnGroupingParamChanged(LPCGUID NewGroupingParam, LPCGUID EventContext);
		HRESULT STDMETHODCALLTYPE OnIconPathChanged(LPCWSTR NewIconPath, LPCGUID EventContext);
		HRESULT STDMETHODCALLTYPE OnSessionDisconnected(AudioSessionDisconnectReason DisconnectReason);
		HRESULT STDMETHODCALLTYPE OnSimpleVolumeChanged(float NewVolume, BOOL NewMute, LPCGUID EventContext);
		HRESULT STDMETHODCALLTYPE OnStateChanged(AudioSessionState NewState);

	protected:
		LONG m_cRef;
		INotificationCB* m_pCB = nullptr;
	};

	class AudioSessionNotificationImpl : public IAudioSessionNotification
	{
	public:
		AudioSessionNotificationImpl(INotificationCB* pCB);
		virtual ~AudioSessionNotificationImpl();

		// IUnknown methods -- AddRef, Release, and QueryInterface
		ULONG STDMETHODCALLTYPE AddRef();
		ULONG STDMETHODCALLTYPE Release();
		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface);

		// IAudioSessionNotification methods 
		HRESULT STDMETHODCALLTYPE OnSessionCreated(IAudioSessionControl *pSessionControl);

	protected:
		LONG m_cRef;
		INotificationCB* m_pCB = nullptr;
	};

	class DeviceSessionNotification: public INotificationCB
	{
	public:
		DeviceSessionNotification(EDataFlow e);
		virtual ~DeviceSessionNotification();

		// Register notification
		HRESULT RegisterNotification(const std::wstring& deviceName2Monitor, ISessionCB* pSessionCB);

		// INotificationCB methods
		HRESULT OnSessionCreated(IAudioSessionControl *pSessionControl);
		HRESULT OnSessionDisconnected(AudioSessionDisconnectReason DisconnectReason);
		HRESULT OnStateChanged(AudioSessionState NewState);

	protected:
		EDataFlow m_dataFlow = eRender;
		std::wstring m_deviceName;
		ISessionCB* m_pSessionCB = nullptr;

		IAudioSessionManager2* m_pAudioSessionManager2 = nullptr;
		IAudioSessionControl2* m_pSessionControl2 = nullptr;
		AudioSessionEventsImpl* m_pAudioSessionEventsImpl = nullptr;
		AudioSessionNotificationImpl* m_pAudioSessionNotificationImpl = nullptr;
	};

	class MicrophoneSessionNotifcation : public DeviceSessionNotification
	{
	public:
		static MicrophoneSessionNotifcation& GetInstance() {
			static MicrophoneSessionNotifcation inst;
			return inst;
		};

	private:
		MicrophoneSessionNotifcation(): DeviceSessionNotification(eCapture) {};
		~MicrophoneSessionNotifcation() {};
	};

	class SpeakerSessionNotifcation : public DeviceSessionNotification
	{
	public:
		static SpeakerSessionNotifcation& GetInstance() {
			static SpeakerSessionNotifcation inst;
			return inst;
		};

	private:
		SpeakerSessionNotifcation() : DeviceSessionNotification(eRender) {};
		~SpeakerSessionNotifcation() {};
	};
}

