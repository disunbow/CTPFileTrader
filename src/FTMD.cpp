#include "stdafx.h"
#include "FTMD.h"
#include <iostream>
CFTMD::CFTMD()
{

}

CFTMD::~CFTMD()
{
	
}

void CFTMD::Init(const char* pi_BrokerID, const char* pi_MDAdress, const char* pi_User, const char* pi_pwd, CFTTD* pTdHandler, logInfo* pLog)
{
	// ����һ��CThostFtdcMdApiʵ��
    memset(qh_BrokerID, 0, sizeof(qh_BrokerID));
    memset(qh_MDAddress, 0, sizeof(qh_MDAddress));
    memset(qh_UserID, 0, sizeof(qh_UserID));
    memset(qh_Password, 0, sizeof(qh_Password));
    strcpy(qh_BrokerID, pi_BrokerID);
    strcpy(qh_MDAddress, pi_MDAdress);
    strcpy(qh_UserID, pi_User);
    strcpy(qh_Password, pi_pwd);
    g_pTdHandler = pTdHandler;
	g_pLog = pLog;

	m_pMdApi=CThostFtdcMdApi::CreateFtdcMdApi("");
	m_pMdApi->RegisterSpi(this);
	m_pMdApi->RegisterFront(qh_MDAddress);
	m_pMdApi->Init();
}

void CFTMD::OnFrontConnected()
{
	CThostFtdcReqUserLoginField reqUserLogin;
	memset(&reqUserLogin,0,sizeof(reqUserLogin));
	strcpy_s(reqUserLogin.BrokerID, qh_BrokerID);
	strcpy_s(reqUserLogin.UserID, qh_UserID);
	strcpy_s(reqUserLogin.Password, qh_Password);
	g_pLog->printLog("�����½����...\n");
	m_pMdApi->ReqUserLogin(&reqUserLogin,1);
}
void CFTMD::OnFrontDisconnected(int nReason)
{
	g_pLog->printLog("�������ӶϿ�!\n");
}
void CFTMD::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) 
{
	g_pLog->printLog("�����½�ɹ�!\n");
    initSubMD();
}
void CFTMD::SubscribeMD(char* Instrument)
{
	//��������
	char** Instruments;
    Instruments=new char*[0];
    Instruments[0]= new char[strlen(Instrument)];
    strcpy(Instruments[0], Instrument);
	CThostFtdcDepthMarketDataField tmpMD;
	memset(&tmpMD, 0, sizeof(tmpMD));
	LastDepth[Instrument] = tmpMD;
	int rtn_cd = m_pMdApi->SubscribeMarketData (Instruments, 1);
	g_pLog->printLog("����%s����, ���ش���:%d\n", Instrument, rtn_cd);
}
void CFTMD::initSubMD()
{
        //�ȴ��������ж��ĺ�Լ�б�ˢ�����
	while (!g_pTdHandler->bIsgetInst)
	{
	    Sleep(100);
	}
    //���ݺ�Լ�б�������
	for (int i=0; i<g_pTdHandler->g_Instnum; i++)
	{
		SubscribeMD(g_pTdHandler->g_pInstinfo[i].InstrumentID);
	}
    //�����¼����ȴ�3�����鷴��
    g_pLog->printLog("�ȴ����鷴��...\n");
    HANDLE g_hEvent = CreateEvent(NULL, true, false, NULL);	
	//WaitForSingleObject(g_hEvent, INFINITE);
    WaitForSingleObject(g_hEvent, 1000);
}

void CFTMD::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData) 
{
	CThostFtdcDepthMarketDataField *pMD;
	LastDepth[pDepthMarketData->InstrumentID] = *pDepthMarketData;
	pMD = &LastDepth[pDepthMarketData->InstrumentID];

	pMD->LastPrice = (pMD->LastPrice > 10000000.0) ? 0 : pMD->LastPrice;                          ///���¼�
	pMD->OpenPrice = (pMD->OpenPrice > 10000000.0) ? pMD->LastPrice : pMD->OpenPrice;             ///����
	pMD->HighestPrice = (pMD->HighestPrice > 10000000.0) ? pMD->LastPrice : pMD->HighestPrice;    ///��߼�
	pMD->LowestPrice = (pMD->LowestPrice > 10000000.0) ? pMD->LastPrice : pMD->LowestPrice;       ///��ͼ�
	pMD->BidPrice1 = (pMD->BidPrice1 > 10000000.0) ? pMD->LastPrice : pMD->BidPrice1;             ///�����һ
	pMD->AskPrice1 = (pMD->AskPrice1 > 10000000.0) ? pMD->LastPrice : pMD->AskPrice1;             ///������һ
	pMD->AveragePrice = (pMD->AveragePrice > 10000000.0) ? pMD->LastPrice : pMD->AveragePrice;    ///���վ���
	/*
	if (pDepthMarketData->LastPrice == 0)
	{
		g_pLog->printLog("MarketData:%s,%s,%s,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d\n", pDepthMarketData->InstrumentID,
			pDepthMarketData->UpdateTime,
			pDepthMarketData->TradingDay,
			pDepthMarketData->PreClosePrice,
			pDepthMarketData->OpenPrice,
			pDepthMarketData->LastPrice,
			pDepthMarketData->UpperLimitPrice,
			pDepthMarketData->LowerLimitPrice,
			pDepthMarketData->AskPrice1,
			pDepthMarketData->BidPrice1,
			pDepthMarketData->Volume);
	}
	*/
}
