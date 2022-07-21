#pragma once
#include <string>
#include "DeviceSessionNotification.h"

class ICallEvent {
public:
	virtual void OnCall() = 0;
	virtual void OnTalkBegin() = 0;
	virtual void OnClose() = 0;
};

class CallMgr: public Notification::ISessionCB
{
public:
	static CallMgr& GetInstance() {
		static CallMgr inst;
		return inst;
	};

	void SetCallEventListener(std::wstring micAppName, std::wstring speakerAppName, ICallEvent* cb);

	// Notification::ISessionCB
	HRESULT OnSessionDisconnected(EDataFlow dataFlow, AudioSessionDisconnectReason DisconnectReason);
	HRESULT OnStateChanged(EDataFlow dataFlow, AudioSessionState NewState);

private:
	CallMgr();
	~CallMgr();
private:
	ICallEvent* m_cb = nullptr;
	AudioSessionState m_lastMicState = AudioSessionStateInactive;
	AudioSessionState m_lastSpeakerState = AudioSessionStateInactive;

private:

};

