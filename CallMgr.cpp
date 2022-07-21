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
	// ��˷��¼��ص�
	if (dataFlow == eCapture) {

		// ��˷�δ���� �� ����(�������Ѿ��˳�)������Ϊ��ͨ������
		if (NewState == AudioSessionStateInactive || NewState == AudioSessionStateExpired) {
			printf("-- OnClose -- \n");
			if (m_cb) {	
				m_cb->OnClose();
			}
		}

		// ��˷缤��������������ڼ���״̬������Ϊ��ͨ����ʼ
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

	// �������¼��ص�
	else if (dataFlow == eRender) {

		// ��˷缤��������������ڼ���״̬������Ϊ��ͨ����ʼ
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
