/******************************************************
* Class Name: CFTMD
* Class Functional: CTP�ڻ�������ʵ��
* Author: Ryan Sun
* Date: 2015/08/25
* Description: CTP�ڻ�������ʵ��
*****************************************************/
#if !defined(FTMD__INCLUDED_)
#define FTMD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif

#include <iostream>
#include <stdio.h>
#include <Windows.h>
#include <map>
#include <string>

#include "ThostFtdcMdApi.h"
#include "FTTD.h"
#include "logInfo.h"

using namespace std;


class CFTMD : public CThostFtdcMdSpi  
{
public:
	CFTMD();
	virtual ~CFTMD();
	typedef map<string, CThostFtdcDepthMarketDataField> TYP_QUTOE;
	TYP_QUTOE LastDepth;

	TThostFtdcBrokerIDType qh_BrokerID;
	TThostFtdcAddressType qh_MDAddress;
	TThostFtdcUserIDType qh_UserID;
    TThostFtdcPasswordType qh_Password;

    void Init(const char* pi_BrokerID, const char* pi_MDAdress, const char* pi_User, const char* pi_pwd, CFTTD* pTdHandler, logInfo* pLog);
	void initSubMD();
	void SubscribeMD(char* Instrument);

	///���ͻ����뽻�׺�̨������ͨ������ʱ����δ��¼ǰ�����÷��������á�
	void OnFrontConnected();
	///��¼������Ӧ
	void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	///���ͻ����뽻�׺�̨ͨ�����ӶϿ�ʱ���÷��������á���������������API���Զ��������ӣ��ͻ��˿ɲ�������
	///@param nReason ����ԭ��
	///        0x1001 �����ʧ��
	///        0x1002 ����дʧ��
	///        0x2001 ����������ʱ
	///        0x2002 ��������ʧ��
	///        0x2003 �յ�������
	void OnFrontDisconnected(int nReason);
	///�������֪ͨ
	void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData);


private:
	CThostFtdcMdApi *m_pMdApi;
    CFTTD *g_pTdHandler;
	logInfo *g_pLog;
};
#endif 
