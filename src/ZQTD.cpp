// ZQTD.cpp: implementation of the CZQTD class.
//
//////////////////////////////////////////////////////////////////////
#include <iostream>
#include "stdafx.h"
#include "ZQTD.h"

using namespace std;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CZQTD::CZQTD()
{
    
}

CZQTD::~CZQTD()
{

}

void CZQTD::Init(const char* pi_TDAdress, const char* pi_User, const char* pi_pwd, logInfo* pLog)
{
    zq_TDAddress = new char[strlen(pi_TDAdress)];
    strcpy(zq_TDAddress, pi_TDAdress);
    zq_UserID = new char[strlen(pi_User)];
    strcpy(zq_UserID, pi_User);
    zq_Password = new char[strlen(pi_pwd)];
    strcpy(zq_Password, pi_pwd);
	g_pLog = pLog;

    g_Instnum = 0;
	nRequestID = 0;
    bIsgetInst = false;
    bIsgetPosDetail = false;
	// ����һ��CThostFtdcTraderApiʵ��
	m_pTdApi = CZQThostFtdcTraderApi::CreateFtdcTraderApi("");

	// ע��һ�¼������ʵ��
	m_pTdApi->RegisterSpi(this);

    // ���Ĺ�����
	//        TERT_RESTART:�ӱ������տ�ʼ�ش�
	//        TERT_RESUME:���ϴ��յ�������
	//        TERT_QUICK:ֻ���͵�¼�󹫹���������
	m_pTdApi->SubscribePublicTopic(ZQTHOST_TERT_RESUME);//(ZQTHOST_TERT_RESTART);

    // ����˽����
	//        TERT_RESTART:�ӱ������տ�ʼ�ش�
	//        TERT_RESUME:���ϴ��յ�������
	//        TERT_QUICK:ֻ���͵�¼��˽����������
	m_pTdApi->SubscribePrivateTopic(ZQTHOST_TERT_RESTART);
	
	// ���ý����й�ϵͳ����ĵ�ַ������ע������ַ����
	m_pTdApi->RegisterFront(zq_TDAddress);//�˴�tcp���ӷ�ʽ�����������ֱ����IP��

	// ʹ�ͻ��˿�ʼ���̨����������
	m_pTdApi->Init();
}
void CZQTD::GetSysTime()
{
    SYSTEMTIME sys;
    GetLocalTime(&sys); 
    sprintf(systime,"%02d:%02d:%02d.%03d",sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds);
}
void CZQTD::OnFrontConnected()
{
	CZQThostFtdcReqUserLoginField reqUserLogin;
	memset(&reqUserLogin,0,sizeof(reqUserLogin));
	strcpy_s(reqUserLogin.BrokerID,"2011");//�˴�2011��Ҫ��
	strcpy_s(reqUserLogin.UserID, zq_UserID);//�����Լ����ʺ�
	strcpy_s(reqUserLogin.Password, zq_Password);//��������
	int login=m_pTdApi->ReqUserLogin(&reqUserLogin, 1);//��¼
	g_pLog->printLog("���׵�¼�������,���׵�¼���󷵻�ֵ%d\n",login); 
}
void CZQTD::OnFrontDisconnected (int nReason)
{
    g_pLog->printLog("�������ӶϿ���ԭ�����%d\n",nReason);
}

void CZQTD::OnRspUserLogin(CZQThostFtdcRspUserLoginField *pRspUserLogin, CZQThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) 
{
    cerr<<"--------------���׵�¼��Ӧ---------------"<<endl;
    g_pLog->printLog("���׵�¼��Ӧ�����ID:%d,Msg:%s,Date:%s\n",pRspInfo->ErrorID,pRspInfo->ErrorMsg,pRspUserLogin->TradingDay);
    QryInstruments();
    //CZQThostFtdcQryExchangeField pQryExchange;
    //memset(&pQryExchange,0,sizeof(pQryExchange));
    //int ex_rtn=m_pTdApi->ReqQryExchange(&pQryExchange,0);
    //printf("��ѯ��������Ϣ������ϣ�����ֵ%d\n",ex_rtn);
}

void CZQTD::PassChange(const char* newpass)
{
    char l_setpass[8];
    memset(&l_setpass,0,sizeof(l_setpass));
    if (strcmp(newpass,"")==0)
    {
        printf("Please Input new password:");
        scanf("%8s",l_setpass);
    }
    else
    {
        strcpy_s(l_setpass, newpass);
    }
    printf("\nThe new passowrd is %s", l_setpass);
    //�޸ĵ�¼����
	CZQThostFtdcUserPasswordUpdateField password;
	memset(&password,0,sizeof(password));
	strcpy_s(password.BrokerID,"2011");
	strcpy_s(password.UserID, zq_UserID);
	strcpy_s(password.OldPassword, zq_Password);
	strcpy_s(password.NewPassword, l_setpass);
	int ex_rtn=m_pTdApi->ReqUserPasswordUpdate(&password,1);
    g_pLog->printLog("��������������ϣ�����ֵ%d\n",ex_rtn);
}

void CZQTD::CancelAll()
{

}
void CZQTD::QryInstruments()
{
    //��ʼ����Լ�б�
    g_Instnum = 0;
    memset(&g_pInstinfo, 0, sizeof(g_pInstinfo));
    //��ú�Լ�б�
	CZQThostFtdcQryInstrumentField qryField;
    memset(&qryField, 0, sizeof(qryField));
    int resCode = m_pTdApi-> ReqQryInstrument(&qryField, 0);
	g_pLog->printLog("��Լ�б���������ϣ�����ֵ��%d\n",resCode);	
}

void CZQTD::OnRspQryInstrument(CZQThostFtdcInstrumentField *pInstrument, CZQThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    prtErr("OnRspQryInstrument", pRspInfo);
	if (
		(pInstrument!=NULL) 
     && ((((pInstrument->ProductClass == '6') ||
         (pInstrument->ProductClass == '8') ||
         (pInstrument->ProductClass == '9'))
     && ((strcmp(pInstrument->ProductID, "SHA") == 0) ||
		 (strcmp(pInstrument->ProductID, "SZA") == 0) ||
		 (strcmp(pInstrument->ProductID, "SHCYB") == 0) ||
		 (strcmp(pInstrument->ProductID, "SZCYB") == 0) ||
         (strcmp(pInstrument->ProductID, "SHETF") == 0) ||
         (strcmp(pInstrument->ProductID, "SZETF") == 0) ||
         (strcmp(pInstrument->ProductID, "SHBONDS") == 0) ||
         (strcmp(pInstrument->ProductID, "SZBONDS") == 0)))
         || (strcmp(pInstrument->InstrumentID, "204001") == 0)
         || (strcmp(pInstrument->InstrumentID, "131810") == 0)
         || (strcmp(pInstrument->InstrumentID, "000001") == 0)
         || (strcmp(pInstrument->InstrumentID, "159901") == 0)
         || (strcmp(pInstrument->InstrumentID, "510050") == 0)
         || (strcmp(pInstrument->InstrumentID, "510051") == 0)
         || (strcmp(pInstrument->InstrumentID, "502048") == 0)
        )
	   )
	{
    	g_Instnum++;
	    g_pInstinfo[g_Instnum-1] = *pInstrument;
        if (strcmp(pInstrument->InstrumentID, "510051") == 0) g_pLog->printLog("Instrument:%s;Name:%s;TypeID:%s;Type:%c;Max:%d;Status:%c;PriceTick=%.5f;VolumeMultiple=%d\n",g_pInstinfo[g_Instnum-1].InstrumentID,g_pInstinfo[g_Instnum-1].InstrumentName,g_pInstinfo[g_Instnum-1].ProductID,g_pInstinfo[g_Instnum-1].ProductClass,g_pInstinfo[g_Instnum-1].MaxMarketOrderVolume,g_pInstinfo[g_Instnum-1].InstrumentStatusFlag,g_pInstinfo[g_Instnum-1].PriceTick,g_pInstinfo[g_Instnum-1].VolumeMultiple);
	}
	if(bIsLast)
	{
		g_pLog->printLog("ȡ�û���A�ɹ�Ʊ�б�����:%d\n", g_Instnum);
		bIsgetInst = true;
	}
    return;
}
void CZQTD::OnRspQryInvestorPosition(CZQThostFtdcInvestorPositionField *pInvestorPosition, CZQThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    prtErr("OnRspQryInvestorPosition", pRspInfo);
    if (pInvestorPosition != NULL)
    {
        g_pLog->printLog("��Լ����:%s", pInvestorPosition->InstrumentID);
        g_pLog->printLog("�ֲֶ�շ���:%c", pInvestorPosition->PosiDirection);
        g_pLog->printLog("Ͷ���ױ���־:%c", pInvestorPosition->HedgeFlag);
        g_pLog->printLog("�ֲ�����:%c", pInvestorPosition->PositionDate);
        g_pLog->printLog("���ճֲ�:%d", pInvestorPosition->YdPosition);
        g_pLog->printLog("���ճֲ�:%d", pInvestorPosition->Position);
        g_pLog->printLog("��ͷ����:%d", pInvestorPosition->LongFrozen);
        g_pLog->printLog("��ͷ����:%d", pInvestorPosition->ShortFrozen);
        g_pLog->printLog("������:%d", pInvestorPosition->OpenVolume);
        g_pLog->printLog("ƽ����:%d", pInvestorPosition->CloseVolume);
        g_pLog->printLog("���ֽ��:%.2f", pInvestorPosition->OpenAmount);
        g_pLog->printLog("ƽ�ֽ��:%.2f", pInvestorPosition->CloseAmount);
        g_pLog->printLog("�ֲֳɱ�:%.3f", pInvestorPosition->PositionCost);
        g_pLog->printLog("������:%.2f", pInvestorPosition->Commission);
        g_pLog->printLog("ƽ��ӯ��:%.2f", pInvestorPosition->CloseProfit);
        g_pLog->printLog("�ֲ�ӯ��:%.2f", pInvestorPosition->PositionProfit);
        g_pLog->printLog("������:%s", pInvestorPosition->TradingDay);
        g_pLog->printLog("���ֳɱ�:%.2f", pInvestorPosition->OpenCost);
        g_pLog->printLog("���ճֲ�:%d", pInvestorPosition->TodayPosition);
        g_pLog->printLog("������:%.3f", pInvestorPosition->TransferFee);
        g_pLog->printLog("ӡ��˰:%.3f", pInvestorPosition->StampTax);
        g_pLog->printLog("֤ȯ��ֵ:%.2f", pInvestorPosition->StockValue);
        g_pLog->printLog("����������:%s\n", pInvestorPosition->ExchangeID);
    }
    if(bIsLast)
	{
		bIsgetInst = true;
	}
    return;
}
void CZQTD::OnRspQryInvestorPositionDetail(CZQThostFtdcInvestorPositionDetailField *pInvestorPositionDetail, CZQThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if (pInvestorPositionDetail != NULL)
    {
        g_pLog->printLog("��Լ����:%s;", pInvestorPositionDetail->InstrumentID);
        g_pLog->printLog("����:%c;", pInvestorPositionDetail->Direction);
        g_pLog->printLog("��������:%s;",pInvestorPositionDetail->OpenDate);
        g_pLog->printLog("�ɽ����:%s;",pInvestorPositionDetail->TradeID);
        g_pLog->printLog("����:%d;",pInvestorPositionDetail->Volume);
        g_pLog->printLog("���ּ�:%f;",pInvestorPositionDetail->OpenPrice);
        g_pLog->printLog("������:%s;",pInvestorPositionDetail->TradingDay);
        g_pLog->printLog("�ɽ�����:%c;",pInvestorPositionDetail->TradeType);
        g_pLog->printLog("����������:%s;",pInvestorPositionDetail->ExchangeID);
        g_pLog->printLog("Ͷ���߱�֤��:%f;",pInvestorPositionDetail->Margin);
        g_pLog->printLog("��������֤��:%f;",pInvestorPositionDetail->ExchMargin);
        g_pLog->printLog("������:%f;",pInvestorPositionDetail->LastSettlementPrice);
        g_pLog->printLog("�����:%f;",pInvestorPositionDetail->SettlementPrice);
        g_pLog->printLog("ƽ����:%d;",pInvestorPositionDetail->CloseVolume);
        g_pLog->printLog("ƽ�ֽ��:%f;",pInvestorPositionDetail->CloseAmount);
        g_pLog->printLog("������:%f;",pInvestorPositionDetail->TransferFee);
        g_pLog->printLog("ӡ��˰:%f;",pInvestorPositionDetail->StampTax);
        g_pLog->printLog("������:%f;\n",pInvestorPositionDetail->Commission);
    }
    else
    {
        g_pLog->printLog("�ֲ���ϸ��ѯδ�ܷ������ݡ�\n");
    }
	if(bIsLast)
	{
		bIsgetInst = true;
	}
    return;
}
void CZQTD::OnRspQryExchange(CZQThostFtdcExchangeField *pExchange, CZQThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if (pExchange != NULL)
    {
        g_pLog->printLog("����������:%s;", pExchange->ExchangeID);
        g_pLog->printLog("����������:%s;", pExchange->ExchangeName);
        g_pLog->printLog("����������:%c;\n",pExchange->ExchangeProperty);
    }
    else
    {
        g_pLog->printLog("��������ѯδ�ܷ������ݡ�\n");
    }
    return;
}

void CZQTD::QueryPositionDetail()
{
   CZQThostFtdcQryInvestorPositionDetailField qryPosDetail;
   memset(&qryPosDetail, 0, sizeof(qryPosDetail));
   bIsgetPosDetail = false;
   strcpy_s(qryPosDetail.BrokerID,"2011");
   strcpy_s(qryPosDetail.ExchangeID,"");
   strcpy_s(qryPosDetail.InstrumentID,"");
   strcpy_s(qryPosDetail.InvestorID,zq_UserID);
   int resCode = m_pTdApi-> ReqQryInvestorPositionDetail(&qryPosDetail, 0);
   g_pLog->printLog("�ֲ���ϸ��ѯ������ϣ�����ֵ��%d\n",resCode);	
   return;
}
void CZQTD::QueryPosition()
{
   CZQThostFtdcQryInvestorPositionField qryPos;
   memset(&qryPos, 0, sizeof(qryPos));
   strcpy_s(qryPos.BrokerID,"2011");
   strcpy_s(qryPos.InvestorID, zq_UserID);
   strcpy_s(qryPos.InstrumentID,"");
   int resCode = m_pTdApi-> ReqQryInvestorPosition(&qryPos, 0);
   g_pLog->printLog("�ֲֲ�ѯ������ϣ�����ֵ��%d\n",resCode);	
   return;
}
void CZQTD::QueryOrders()
{
   CZQThostFtdcQryOrderField qryOrder;
   memset(&qryOrder,0,sizeof(qryOrder));
   strcpy_s(qryOrder.BrokerID,"2011");
   strcpy_s(qryOrder.InvestorID, zq_UserID);
   strcpy_s(qryOrder.ExchangeID, "");
   strcpy_s(qryOrder.InstrumentID,"");
   strcpy_s(qryOrder.OrderSysID,"");
   strcpy_s(qryOrder.InsertTimeStart,"");
   strcpy_s(qryOrder.InsertTimeEnd,"");
   //strcpy_s(qryOrder.ExchangeID, "SSE");
   //strcpy_s(qryOrder.InstrumentID,"600016");
   //strcpy_s(qryOrder.OrderSysID,"N537");
   //strcpy_s(qryOrder.InsertTimeStart,"09:00:00");
   //strcpy_s(qryOrder.InsertTimeEnd,"15:00:00");

   int resCode = m_pTdApi->ReqQryOrder(&qryOrder, 0);
   g_pLog->printLog("ί�в�ѯ:ExchangeID:%s;InstrumentID:%s;OrderSysID:%s;InsertTimeStart:%s;InsertTimeEnd:%s\n",
          qryOrder.ExchangeID,qryOrder.InstrumentID,qryOrder.OrderSysID,qryOrder.InsertTimeStart,qryOrder.InsertTimeEnd);	
   g_pLog->printLog("ί�в�ѯ������ϣ�����ֵ��%d\n",resCode);	
   return;
}
void CZQTD::QueryTrades()
{
   CZQThostFtdcQryTradeField qryTrade;
   memset(&qryTrade,0,sizeof(qryTrade));
   strcpy_s(qryTrade.BrokerID,"2011");
   strcpy_s(qryTrade.InvestorID, zq_UserID);
   strcpy_s(qryTrade.InstrumentID,"");
   strcpy_s(qryTrade.ExchangeID, "");
   strcpy_s(qryTrade.TradeID,"");
   strcpy_s(qryTrade.TradeTimeStart,"");
   strcpy_s(qryTrade.TradeTimeEnd,"");
   //strcpy_s(qryOrder.ExchangeID, "SSE");
   //strcpy_s(qryOrder.InstrumentID,"600016");
   //strcpy_s(qryOrder.OrderSysID,"N537");
   //strcpy_s(qryOrder.InsertTimeStart,"09:00:00");
   //strcpy_s(qryOrder.InsertTimeEnd,"15:00:00");

   int resCode = m_pTdApi->ReqQryTrade(&qryTrade, 0);
   g_pLog->printLog("�ɽ���ѯ:ExchangeID:%s;InstrumentID:%s;TradeID:%s;TradeTimeStart:%s;TradeTimeEnd:%s\n",
          qryTrade.ExchangeID,qryTrade.InstrumentID,qryTrade.TradeID,qryTrade.TradeTimeStart,qryTrade.TradeTimeEnd);	
   g_pLog->printLog("ί�в�ѯ������ϣ�����ֵ��%d\n",resCode);	
   return;
}
void CZQTD::QueryMD()
{
   CZQThostFtdcQryDepthMarketDataField qryDepthMarketData;
   memset(&qryDepthMarketData,0,sizeof(qryDepthMarketData));
   strcpy_s(qryDepthMarketData.InstrumentID ,"002734");
   int resCode = m_pTdApi->ReqQryDepthMarketData(&qryDepthMarketData, 0);
   g_pLog->printLog("�����ѯ:InstrumentID:%s\n",
          qryDepthMarketData.InstrumentID);	
   g_pLog->printLog("ί�в�ѯ������ϣ�����ֵ��%d\n",resCode);	
   return;
}
void CZQTD::QueryAcct()
{
   CZQThostFtdcQryTradingAccountField qryTradingAccount;
   memset(&qryTradingAccount,0,sizeof(qryTradingAccount));
   strcpy_s(qryTradingAccount.BrokerID, "2011");
   strcpy_s(qryTradingAccount.InvestorID, zq_UserID);
   int resCode = m_pTdApi->ReqQryTradingAccount(&qryTradingAccount, 0);
   g_pLog->printLog("�˻���ѯ:InvestorID:%s\n",
          qryTradingAccount.InvestorID);	
   g_pLog->printLog("ί�в�ѯ������ϣ�����ֵ��%d\n",resCode);	
   return;
}
void CZQTD::prtErr(const char* pFuncID, CZQThostFtdcRspInfoField *pRspInfo)
{
    if (pRspInfo==NULL)
    {
        return; //g_pLog->printLog("%s: pRspInfo = NULL\n", pFuncID);
    }
    else
    {
        g_pLog->printLog("%s:���ش���:%d,Msg:%s\n", pFuncID, pRspInfo->ErrorID,pRspInfo->ErrorMsg);
    }
    return;
}
void CZQTD::PlaceOrder(const char* pi_Instrument, 
                       const char* pi_ExchangeID,
                       const char* pi_BuyOrSell,
                       const char* pi_Price,
                       int pi_Position)
{
	CZQThostFtdcInputOrderField pInputOrder;
    memset(&pInputOrder,0,sizeof(pInputOrder));
    CZQThostFtdcInputOrderField * pIptOrdFld=&pInputOrder;

    strcpy_s(pIptOrdFld->BrokerID,"2011");    //�����̴���
	strcpy_s(pIptOrdFld->InvestorID, zq_UserID); //Ͷ���ߴ���
	//strcpy_s(pIptOrdFld->UserID, zq_UserID); //Ͷ���ߴ���
	strcpy_s(pIptOrdFld->InstrumentID, pi_Instrument); //��Լ����
    strcpy_s(pIptOrdFld->ExchangeID, pi_ExchangeID); //���������� //SSE�Ͻ��� SZE���
    pIptOrdFld->OrderPriceType=THOST_FTDC_OPT_LimitPrice; //�����۸����� //����ע�⣬SSE����SZE��֧��AnyPrice
    if (strcmp(pi_BuyOrSell, "B")==0||
        strcmp(pi_BuyOrSell, "BUY")==0)
    {
        pIptOrdFld->Direction=THOST_FTDC_D_Buy; //��������
    }
    else if (strcmp(pi_BuyOrSell, "S")==0||
        strcmp(pi_BuyOrSell, "SELL")==0)
    {
        pIptOrdFld->Direction=THOST_FTDC_D_Sell; //��������
    }
    else if (strcmp(pi_BuyOrSell, "P")==0||
        strcmp(pi_BuyOrSell, "ETFPUR")==0)
    {
        pIptOrdFld->Direction=THOST_FTDC_D_ETFPur; //��������
    }
    else if (strcmp(pi_BuyOrSell, "R")==0||
        strcmp(pi_BuyOrSell, "ETFRED")==0)
    {
        pIptOrdFld->Direction=THOST_FTDC_D_ETFRed; //��������
    }

    pIptOrdFld->VolumeTotalOriginal=pi_Position; //����
    pIptOrdFld->TimeCondition = THOST_FTDC_TC_GFD;  //��Ч������ //������Ч
    pIptOrdFld->VolumeCondition=THOST_FTDC_VC_AV; //�ɽ�������
    pIptOrdFld->ContingentCondition = THOST_FTDC_CC_Immediately; //��������
    pIptOrdFld->ForceCloseReason = THOST_FTDC_FCC_NotForceClose;  //ǿƽԭ��
    strcpy_s(pIptOrdFld->LimitPrice, pi_Price);
    //���һ�ο���̨���������󣬶��ʹ��ReqOrderInsert�������˴���nRequestIDҪд�ɵ��������ģ���nRequestID++����ʼΪ1
	nRequestID++;

    SYSTEMTIME sys;
    GetLocalTime(&sys); 
    sprintf(systime,"%02d%02d%02d%03d",sys.wHour,sys.wMinute, sys.wSecond, sys.wMilliseconds);
    
    //pIptOrdFld->RequestID = nRequestID;
    sprintf(pIptOrdFld->OrderRef,"%s%03d", systime, nRequestID);

    int resCode=m_pTdApi->ReqOrderInsert(pIptOrdFld, nRequestID);
	GetSysTime();
	g_pLog->printLog("����-%s:��Լ:%s;������:%s;����:%c;�۸�:%s;����:%d;������ʼ����ֵ:%d;OrderRef:%s;RequestID:%d\n",
           systime,
           pIptOrdFld->InstrumentID,
           pIptOrdFld->ExchangeID,
           pIptOrdFld->Direction,
           pIptOrdFld->LimitPrice,
           pIptOrdFld->VolumeTotalOriginal,
           resCode,
		   pIptOrdFld->OrderRef,
           pIptOrdFld->RequestID);	
}

void CZQTD::OnRspOrderInsert(CZQThostFtdcInputOrderField *pInputOrder, CZQThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
     prtErr("OnRspOrderInsert", pRspInfo);
     GetSysTime();
     g_pLog->printLog("OnRspOrderInsert-%s:��Լ:%s;��������:%s;Ref:%s;RequestID:%d\n",systime,pInputOrder->InstrumentID,pRspInfo->ErrorMsg,pInputOrder->OrderRef,pInputOrder->RequestID);
     return;
}
void CZQTD::OnRspQryOrder(CZQThostFtdcOrderField *pOrder, CZQThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
 {
    prtErr("OnRspQryOrder", pRspInfo);
    if(pOrder!=NULL)
    {
		GetSysTime();
		g_pLog->printLog("OnRspQryOrder-%s:��Լ:%s;����:%c;����״̬:%s;RequestID:%d;OrderRef:%s;LocalOrderID:%s\n", systime, pOrder->InstrumentID, pOrder->Direction, pOrder->StatusMsg, pOrder->RequestID, pOrder->OrderRef, pOrder->OrderLocalID);
        g_pLog->printLog("OnRspQryOrder:BrokerID:%s;InvestorID:%s;ExchangeID:%s;SessionID:%d;OrderSysID:%s;InsertTime:%s\n",pOrder->BrokerID,pOrder->InvestorID,pOrder->ExchangeID,pOrder->SessionID,pOrder->OrderSysID,pOrder->InsertTime);
    }
	return;
}
void CZQTD::OnRspQryTrade(CZQThostFtdcTradeField *pTrade, CZQThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
 {
    prtErr("OnRspQryTrade", pRspInfo);
    if(pTrade!=NULL)
    {
		GetSysTime();
		g_pLog->printLog("OnRspQryTrade-%s:��Լ:%s;�ɽ���:%s;�ɽ�ʱ��:%s\n", systime, pTrade->InstrumentID, pTrade->Price, pTrade->TradeTime);
    }
	return;
}
void CZQTD::OnRspQryDepthMarketData(CZQThostFtdcDepthMarketDataField *pDepthMarketData, CZQThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
 {
    prtErr("OnRspQryDepthMarketData", pRspInfo);
    if(pDepthMarketData!=NULL)
    {
        g_pLog->printLog("OnRspQryDepthMarketData:��Լ:%s;���̼�:%f\n",pDepthMarketData->InstrumentID,pDepthMarketData->OpenPrice);
    }
	return;
}
void CZQTD::OnRspQryTradingAccount(CZQThostFtdcTradingAccountField *pTradingAccount, CZQThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    prtErr("OnRspQryTradingAccount", pRspInfo);
    if(pTradingAccount!=NULL)
    {
        g_pLog->printLog("OnRspQryTradingAccount:StockValue=%.2f;Available=%.2f;WithdrawQuota=%.2f\n",pTradingAccount->StockValue,pTradingAccount->Available,pTradingAccount->WithdrawQuota);
    }
    return;
}
void CZQTD::OnRspQryTradingCode(CZQThostFtdcTradingCodeField *pTradingCode, CZQThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	g_pLog->printLog("InvestorID:%d   ClientID:%d\n",pTradingCode->InvestorID,pTradingCode->ClientID);
	return;
}
void CZQTD::OnRspQryInstrumentCommissionRate(CZQThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, CZQThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	return;
}
//ErrRSP&Rtn/////////////////////////////////////////////////////////////////////
void CZQTD::OnRspError(CZQThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	return;
}
void CZQTD::OnErrRtnOrderInsert(CZQThostFtdcInputOrderField *pInputOrder, CZQThostFtdcRspInfoField *pRspInfo)
{
	return;
}
void CZQTD::OnErrRtnOrderAction(CZQThostFtdcOrderActionField *pOrderAction, CZQThostFtdcRspInfoField *pRspInfo)
{
	return;
}
//Rtn/////////////////////////////////////////////////////////////////////
void CZQTD::OnRtnOrder(CZQThostFtdcOrderField *pOrder)
{
    if(pOrder!=NULL)
    {
        GetSysTime();
        g_pLog->printLog("OnRtnOrder-%s:��Լ:%s;����:%c;����״̬:%s;OrderStatus:%c;RequestID:%d;OrderRef:%s;LocalOrderID:%s\n",systime,pOrder->InstrumentID,pOrder->Direction,pOrder->StatusMsg,pOrder->OrderStatus,pOrder->RequestID,pOrder->OrderRef,pOrder->OrderLocalID);
        g_pLog->printLog("OnRtnOrder-%s:BrokerID:%s;InvestorID:%s;ExchangeID:%s;SessionID:%d;OrderSysID:%s;InsertTime:%s\n",systime,pOrder->BrokerID,pOrder->InvestorID,pOrder->ExchangeID,pOrder->SessionID,pOrder->OrderSysID,pOrder->InsertTime);
    }
	return;
}
void CZQTD::OnRtnTrade(CZQThostFtdcTradeField *pTrade)
{
	if (pTrade != NULL)
	{
		GetSysTime();
		//g_pLog->printLog("OnRtnTrade-%s:��Լ:%s;�۸�:%s;OrderLocalID:%s\n",systime,pTrade->InstrumentID,pTrade->Price,pTrade->OrderLocalID);
		g_pLog->printLog("OnRtnTrade-%s:", systime);
		g_pLog->printLog("���͹�˾����:%s;", pTrade->BrokerID);
		g_pLog->printLog("Ͷ���ߴ���:%s;", pTrade->InvestorID);
		g_pLog->printLog("��Լ����:%s;", pTrade->InstrumentID);
		g_pLog->printLog("��������:%s;", pTrade->OrderRef);
		g_pLog->printLog("�û�����:%s;", pTrade->UserID);
		g_pLog->printLog("����������:%s;", pTrade->ExchangeID);
		g_pLog->printLog("�ɽ����:%s;", pTrade->TradeID);
		g_pLog->printLog("��������:%c;", pTrade->Direction);
		g_pLog->printLog("�������:%s;", pTrade->OrderSysID);
		g_pLog->printLog("��Ա����:%s;", pTrade->ParticipantID);
		g_pLog->printLog("�ͻ�����:%s;", pTrade->ClientID);
		g_pLog->printLog("���׽�ɫ:%c;", pTrade->TradingRole);
		g_pLog->printLog("��Լ�ڽ������Ĵ���:%s;", pTrade->ExchangeInstID);
		g_pLog->printLog("��ƽ��־:%c;", pTrade->OffsetFlag);
		g_pLog->printLog("Ͷ���ױ���־:%c;", pTrade->HedgeFlag);
		g_pLog->printLog("�۸�:%s;", pTrade->Price);
		g_pLog->printLog("����:%d;", pTrade->Volume);
		g_pLog->printLog("�ɽ�ʱ��:%s;", pTrade->TradeDate);
		g_pLog->printLog("�ɽ�ʱ��:%s;", pTrade->TradeTime);
		g_pLog->printLog("�ɽ�����:%c;", pTrade->TradeType);
		g_pLog->printLog("�ɽ�����Դ:%c;", pTrade->PriceSource);
		g_pLog->printLog("����������Ա����:%s;", pTrade->TraderID);
		g_pLog->printLog("���ر������:%s;", pTrade->OrderLocalID);
		g_pLog->printLog("�����Ա���:%s;", pTrade->ClearingPartID);
		g_pLog->printLog("ҵ��Ԫ:%s;", pTrade->BusinessUnit);
		g_pLog->printLog("���:%d;", pTrade->SequenceNo);
		g_pLog->printLog("������:%s;", pTrade->TradingDay);
		g_pLog->printLog("������:%d;", pTrade->SettlementID);
		g_pLog->printLog("���͹�˾�������:%d;\n", pTrade->BrokerOrderSeq);
	}
	return;
}
void CZQTD::OnRtnInstrumentStatus(CZQThostFtdcInstrumentStatusField *pInstrumentStatus)
{
    g_pLog->printLog("OnRtnInstrumentStatus:%s,%c",pInstrumentStatus->InstrumentID,pInstrumentStatus->InstrumentStatus);
    return;
}
/////////////////////////////////////////////////////////////////////////////////////
double CZQTD::getPrice(TZQThostFtdcStockPriceType price)
{
	return sizeof(price) / sizeof(price[0]);
}