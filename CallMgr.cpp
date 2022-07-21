#include "CallMgr.h"
#include "DeviceSessionNotification.h"



CallMgr::CallMgr()
{
}


CallMgr::~CallMgr()
{
}

void CallMgr::SetCallEventListener(std::wstring micAppName, std::wstring speakerAppName, ICallEvent* cb)
{
	m_cb = cb;
	Notification::MicrophoneSessionNotifcation::GetInstance().RegisterNotification(micAppName, this);
	Notification::SpeakerSessionNotifcation::GetInstance().RegisterNotification(speakerAppName, this);
}

HRESULT CallMgr::OnSessionDisconnected(EDataFlow dataFlow, AudioSessionDisconnectReason DisconnectReason)
{
	return S_OK;
}

HRESULT CallMgr::OnStateChanged(EDataFlow dataFlow, AudioSessionState NewState)
{
	// 麦克风事件回调
	if (dataFlow == eCapture) {

		// 麦克风未激活 或 过期(即程序已经退出)，则认为是通话结束
		if (NewState == AudioSessionStateInactive || NewState == AudioSessionStateExpired) {
			printf("-- OnClose -- \n");
			if (m_cb) {	
				m_cb->OnClose();
			}
		}

		// 麦克风激活和扬声器都出于激活状态，则认为是通话开始
		else if (m_lastSpeakerState == AudioSessionStateActive && NewState == AudioSessionStateActive) {
			printf("-- OnCall -- \n");
			printf("-- OnTalkBegin -- \n");
			if (m_cb) {
				m_cb->OnCall();	
				m_cb->OnTalkBegin();
			}
		}

		m_lastMicState = NewState;
	}

	// 扬声器事件回调
	else if (dataFlow == eRender) {

		// 麦克风激活和扬声器都出于激活状态，则认为是通话开始
		if(m_lastMicState == AudioSessionStateActive && NewState == AudioSessionStateActive) {
			printf("-- OnCall -- \n");
			printf("-- OnTalkBegin -- \n");
			if (m_cb) {
				m_cb->OnCall();	
				m_cb->OnTalkBegin();
			}
		}

		m_lastSpeakerState = NewState;
	}


	return S_OK;
}
