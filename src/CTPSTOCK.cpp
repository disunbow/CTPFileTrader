#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <locale.h>
#include <assert.h>
#include <tchar.h>
#include <conio.h>
#include <time.h>
#include <Windows.h>

#include "stdafx.h"
#include "ZQMD.h"
#include "ZQTD.h"
#include "config.h"
#include "PathHelper.h"
#include "FileSystemWatcher.h"
#include "logInfo.h"

#define CONST_LINEBUF_SIZE 512

char *zq_MDAddress;
char *zq_TDAddress;
char *zq_UserID;
char *zq_Password;
char *zq_OrderPath;
char *zq_ArchivePath;
char *zq_LogPath;

CZQTD *pTdHandler=new CZQTD();
CZQMD *pMdHandler=new CZQMD();
logInfo* g_pLog = new logInfo();

void printtime()
{
    time_t t = time( 0 );   
    char tmpBuf[255];   
    strftime(tmpBuf, 255, "%Y%m%d%H%M%S", localtime(&t)); //format date and time. 
    g_pLog->printLog("%s|",tmpBuf); 
}

int chkMoveFile(LPCWSTR pi_ordrfile,char* po_ordrfile)
{
    time_t nowtime;  
    struct tm *local;  
     
    nowtime = time(NULL); //��ȡ����ʱ��  
    local=localtime(&nowtime);  //��ȡ��ǰϵͳʱ��  
  
    char filepre_tm[18];  
    strftime(filepre_tm,18,"%Y%m%d_%H%M%S",local);  
    
    int err_cd = 0;
    int len= WideCharToMultiByte(CP_ACP,0,pi_ordrfile,wcslen(pi_ordrfile),NULL,0,NULL,NULL);  
    char* filename = new char[len+1];  
    WideCharToMultiByte(CP_ACP,0,pi_ordrfile,wcslen(pi_ordrfile),filename,len,NULL,NULL);  
    filename[len]='\0';
    
    char* srcFile = new char[strlen(zq_OrderPath)+strlen(filename)+1];
    char* trgFile = new char[strlen(zq_ArchivePath)+strlen(filename)+1];
    sprintf(srcFile,"%s\\%s", zq_OrderPath, filename);
    sprintf(trgFile,"%s\\%s_%s", zq_ArchivePath, filepre_tm, filename);

    if(MoveFileEx(srcFile, trgFile,MOVEFILE_REPLACE_EXISTING))
    {
        strcpy(po_ordrfile, trgFile);
    }
    else
    {
        strcpy(po_ordrfile, "");
        err_cd = 1;
    }
    return err_cd;
}

void get_config(char **envp)
{
    std::string cfgfile = GetConfigDir() + GetBasicFileName() + ".ini";
    char *zq_cfgfile = new char[strlen(cfgfile.c_str())];
    strcpy(zq_cfgfile, cfgfile.c_str());
    printf("%s\n", zq_cfgfile);
    Config config(cfgfile, envp);
    zq_MDAddress = new char[strlen(config.pString("zq_MDAddress").c_str())];
    zq_TDAddress = new char[strlen(config.pString("zq_TDAddress").c_str())];
    zq_UserID = new char[strlen(config.pString("zq_UserID").c_str())];
    zq_Password = new char[strlen(config.pString("zq_Password").c_str())];
    zq_OrderPath = new char[strlen(config.pString("zq_OrderPath").c_str())];
    zq_ArchivePath = new char[strlen(config.pString("zq_OrderArchive").c_str())];
    zq_LogPath = new char[strlen(config.pString("zq_OrderLog").c_str())];
    strcpy(zq_MDAddress, config.pString("zq_MDAddress").c_str());
    strcpy(zq_TDAddress, config.pString("zq_TDAddress").c_str());
    strcpy(zq_UserID, config.pString("zq_UserID").c_str());
    strcpy(zq_Password, config.pString("zq_Password").c_str());
    strcpy(zq_OrderPath, config.pString("zq_OrderPath").c_str());
    strcpy(zq_ArchivePath, config.pString("zq_OrderArchive").c_str());
    strcpy(zq_LogPath, config.pString("zq_OrderLog").c_str());
    printf ("zq_MDAddress=%s\n", zq_MDAddress);
    printf ("zq_TDAddress=%s\n", zq_TDAddress);
    printf ("zq_UserID=%s\n", zq_UserID);
    printf ("zq_Password=%s\n", zq_Password);
    printf ("zq_OrderPath=%s\n", zq_OrderPath);
    printf ("zq_ArchivePath=%s\n", zq_ArchivePath);
    printf ("zq_LogPath=%s\n", zq_LogPath);
}

CZQThostFtdcInstrumentField getInstInfo(const char* InstrumentID)
{
    int  i = 0, instnum = pTdHandler->g_Instnum;
    CZQThostFtdcInstrumentField Inst;
    memset(&Inst, 0, sizeof(CZQThostFtdcInstrumentField));
    while (i<instnum)
    {
        if (strcmp(pTdHandler->g_pInstinfo[i].InstrumentID, InstrumentID)==0)
        {
            Inst = pTdHandler->g_pInstinfo[i];
            break;
        }
        i++;
    }
    return Inst;
}

double getOrderPrice(const char* InstrumentID, char* pi_BOS, int pi_Pricelvl = 5)
{
    CZQThostFtdcDepthMarketDataField LastMD;
    memset(&LastMD, 0, sizeof(CZQThostFtdcDepthMarketDataField));

    if ((pMdHandler->LastDepth.find(InstrumentID) != pMdHandler->LastDepth.end()) &&
        strcmp(InstrumentID,"") != 0)
    {
        LastMD = pMdHandler->LastDepth[InstrumentID];
        g_pLog->printLog("��Լ:%s;����:%.2lf;����:%.2lf;��:%.2lf;���:%.2lf;���:%.2lf;��ͣ:%.2lf;��ͣ:%.2lf;\n",
            InstrumentID, LastMD.LastPrice, LastMD.PreClosePrice,LastMD.OpenPrice,LastMD.HighestPrice,LastMD.LowestPrice,
            LastMD.UpperLimitPrice,LastMD.LowerLimitPrice);
        if (strcmp(pi_BOS, "B")==0||
            strcmp(pi_BOS, "BUY")==0)
        {
            if((LastMD.AskPrice5!=0)&&(pi_Pricelvl>=5)) 
            {
                return LastMD.AskPrice5;
            }
            else if((LastMD.AskPrice4!=0)&&(pi_Pricelvl>=4)) 
            {
                return LastMD.AskPrice4;
            }
            else if((LastMD.AskPrice3!=0)&&(pi_Pricelvl>=3)) 
            {
                return LastMD.AskPrice3;
            }
            else if((LastMD.AskPrice2!=0)&&(pi_Pricelvl>=2)) 
            {
                return LastMD.AskPrice2;
            }
            else if((LastMD.AskPrice1!=0)&&(pi_Pricelvl>=1)) 
            {
                return LastMD.AskPrice1;
            }
            else if((LastMD.BidPrice5!=0)&&(pi_Pricelvl<=-5)) 
            {
                return LastMD.BidPrice5;
            }
            else if((LastMD.BidPrice4!=0)&&(pi_Pricelvl<=-4)) 
            {
                return LastMD.BidPrice4;
            }
            else if((LastMD.BidPrice3!=0)&&(pi_Pricelvl<=-3)) 
            {
                return LastMD.BidPrice3;
            }
            else if((LastMD.BidPrice2!=0)&&(pi_Pricelvl<=-2)) 
            {
                return LastMD.BidPrice2;
            }
            else if((LastMD.BidPrice1!=0)&&(pi_Pricelvl<=-1)) 
            {
                return LastMD.BidPrice1;
            }
            else
            {
                return LastMD.LastPrice;
            }
        }
        else if (strcmp(pi_BOS, "S")==0||
                 strcmp(pi_BOS, "SELL")==0)
        {
            if((LastMD.BidPrice5!=0)&&(pi_Pricelvl>=5)) 
            {
                return LastMD.BidPrice5;
            }
            else if((LastMD.BidPrice4!=0)&&(pi_Pricelvl>=4)) 
            {
                return LastMD.BidPrice4;
            }
            else if((LastMD.BidPrice3!=0)&&(pi_Pricelvl>=3)) 
            {
                return LastMD.BidPrice3;
            }
            else if((LastMD.BidPrice2!=0)&&(pi_Pricelvl>=2)) 
            {
                return LastMD.BidPrice2;
            }
            else if((LastMD.BidPrice1!=0)&&(pi_Pricelvl>=1)) 
            {
                return LastMD.BidPrice1;
            }
            else if((LastMD.AskPrice5!=0)&&(pi_Pricelvl<=-5)) 
            {
                return LastMD.AskPrice5;
            }
            else if((LastMD.AskPrice4!=0)&&(pi_Pricelvl<=-4)) 
            {
                return LastMD.AskPrice4;
            }
            else if((LastMD.AskPrice3!=0)&&(pi_Pricelvl<=-3)) 
            {
                return LastMD.AskPrice3;
            }
            else if((LastMD.AskPrice2!=0)&&(pi_Pricelvl<=-2)) 
            {
                return LastMD.AskPrice2;
            }
            else if((LastMD.AskPrice1!=0)&&(pi_Pricelvl<=-1)) 
            {
                return LastMD.AskPrice1;
            }
            else
            {
                return LastMD.LastPrice;
            }
        }
        else if (strcmp(pi_BOS, "P")==0||
                 strcmp(pi_BOS, "ETFPur")==0)
        {
            return 1.0;
        }
        else if (strcmp(pi_BOS, "R")==0||
                 strcmp(pi_BOS, "ETFRed")==0)
        {
            return 1.0;
        }
    }
    else
    {
        return 0;
    }
}

void readfile(const char* pi_ordrfile)
{
    ifstream infile;
    infile.open(pi_ordrfile, ifstream::in);
    string Inststr, BoSstr;
    char linebuf[CONST_LINEBUF_SIZE]={0};
    char InstrumentID[6];
    char BuyOrSell[3];
    TZQThostFtdcExchangeIDType ExID;
    TZQThostFtdcStockPriceType OrdrPrice;
    double PriceBuf;
    int Pos, Pricelvl;
    while (infile.getline(linebuf,sizeof(linebuf)))
    {
        std::stringstream words(linebuf);
        memset(ExID, 0, sizeof(ExID));
        memset(OrdrPrice, 0, sizeof(OrdrPrice));
        PriceBuf = 0;
        Inststr.clear();
        BoSstr.clear();
        Pos = 0;
        Pricelvl = 5;
        words>>Inststr;
        words>>BoSstr;
        words>>Pos;
        words>>Pricelvl;
        strcpy(InstrumentID, Inststr.c_str());
        strcpy(BuyOrSell, BoSstr.c_str());
        if ((strcmp(InstrumentID, "") != 0) &&
            (strcmp(BuyOrSell, "") != 0))
        {
            strcpy(ExID,getInstInfo(InstrumentID).ExchangeID);
            PriceBuf = getOrderPrice(InstrumentID, BuyOrSell, Pricelvl);
            printtime();g_pLog->printLog("ȡ�ú�Լ�۸�%f\n", PriceBuf);
            sprintf(OrdrPrice,"%.2lf",PriceBuf);
            printtime();g_pLog->printLog("��ȡ:������=%s;��Լ=%s;����=%s;�۸�=%s;����=%d;\n",ExID,InstrumentID,BuyOrSell,OrdrPrice,Pos);
            pTdHandler->PlaceOrder(InstrumentID, 
                                   getInstInfo(InstrumentID).ExchangeID,
                                   BuyOrSell,
                                   OrdrPrice,
                                   Pos);
        }
    }
/*    
    while (!infile.eof()) 
    {
        memset(ExID, 0, sizeof(ExID));
        memset(OrdrPrice, 0, sizeof(OrdrPrice));
        PriceBuf = 0;
        Inststr.clear();
        BoSstr.clear();
        Pos = 0;
        Pricelvl = 1;
        infile>>Inststr>>BoSstr>>Pos>>Pricelvl;
        strcpy(InstrumentID, Inststr.c_str());
        strcpy(BuyOrSell, BoSstr.c_str());
        if ((strcmp(InstrumentID, "") != 0) &&
            (strcmp(BuyOrSell, "") != 0))
        {
            strcpy(ExID,getInstInfo(InstrumentID).ExchangeID);
            PriceBuf = getOrderPrice(InstrumentID, BuyOrSell, Pricelvl);
            printtime();printf("ȡ�ú�Լ�۸�%f\n", PriceBuf);
            sprintf(OrdrPrice,"%.2lf",PriceBuf);
            printtime();printf("��ȡ:������=%s;��Լ=%s;����=%s;�۸�=%s;����=%d;\n",ExID,InstrumentID,BuyOrSell,OrdrPrice,Pos);
            pTdHandler->PlaceOrder(InstrumentID, 
                                   getInstInfo(InstrumentID).ExchangeID,
                                   BuyOrSell,
                                   OrdrPrice,
                                   Pos);
        }
    }
*/
}

void __stdcall MyDeal( FileSystemWatcher::ACTION act, LPCWSTR filename, LPVOID lParam )
{
    char* ordrfile = new char[500];
    int rtnMoveFile = 0;
    static FileSystemWatcher::ACTION pre = FileSystemWatcher::ACTION_ERRSTOP;
    switch( act )
    {
        case FileSystemWatcher::ACTION_ADDED:
            printtime();wprintf_s(L"����:%s\n", filename);
            rtnMoveFile = chkMoveFile(filename, ordrfile);
            if (rtnMoveFile == 0)
            {
                printtime();g_pLog->printLog("�ļ��ƶ���Archive�ɹ���\n");
                printtime();g_pLog->printLog("��ȡ�ļ�:%s\n", ordrfile);
                readfile(ordrfile);
            }
            else
            {
                printtime();g_pLog->printLog("�ļ��ƶ���Archiveʧ�ܣ�\n");
            }
            break;
        case FileSystemWatcher::ACTION_REMOVED:
            printtime();wprintf_s(L"ɾ��:%s\n", filename);
            break;
        case FileSystemWatcher::ACTION_MODIFIED:
            printtime();wprintf_s(L"�޸�:%s\n", filename);
            rtnMoveFile = chkMoveFile(filename, ordrfile);
            if (rtnMoveFile == 0)
            {
                printtime();g_pLog->printLog("�ļ��ƶ���Archive�ɹ���\n");
                printtime();g_pLog->printLog("��ȡ�ļ�:%s\n", ordrfile);
                readfile(ordrfile);
            }
            else
            {
                printtime();g_pLog->printLog("�ļ��ƶ���Archiveʧ�ܣ�\n");
            }
            break;
        case FileSystemWatcher::ACTION_RENAMED_OLD:
            printtime();wprintf_s(L"����(ԭ����):%s\n", filename);
            break;
        case FileSystemWatcher::ACTION_RENAMED_NEW:
            assert( pre == FileSystemWatcher::ACTION_RENAMED_OLD );
            printtime();wprintf_s(L"����(������):%s\n", filename);
            break;
        case FileSystemWatcher::ACTION_ERRSTOP:
        default:
            printtime();wprintf_s(L"--����--%s\n", filename);
            break;
    }
    pre = act;
}

int main(int argc, char* argv[], char *envp[])
{
    //ȡ��INI�ļ�������Ϣ
    get_config(envp);

	//log��ʵ��
	g_pLog->SetLogPath(zq_LogPath);
	
	//��ʼ������ͽ�����
	pTdHandler->Init(zq_TDAddress, zq_UserID, zq_Password, g_pLog);
	pMdHandler->Init(zq_MDAddress, zq_UserID, zq_Password, pTdHandler, g_pLog);
    
    //����ɨ���ļ������ƺ͹�����
    LPCTSTR sDir= TEXT(zq_OrderPath);
    DWORD dwNotifyFilter = FileSystemWatcher::FILTER_FILE_NAME|
                           FileSystemWatcher::FILTER_DIR_NAME|
                           FileSystemWatcher::FILTER_LAST_WRITE_NAME|
                           FileSystemWatcher::FILTER_SIZE_NAME;

    //�����ļ��м���ಢ��ʼ��
    FileSystemWatcher fsw;
    bool r = fsw.Run( sDir, true, dwNotifyFilter, &MyDeal, 0 );
    if( !r ) return -1;
    _tsetlocale( LC_CTYPE, TEXT("chs") );
    _tprintf_s(TEXT("�ɹ�����ļ���:%s\n"),sDir); 
    _tprintf_s(TEXT("��<p>��ѯ�ֲ֣���<q>�˳�����\n"));
    char pressKey = '\0';
    while(pressKey!='q') 
    {
        pressKey = _getch();
        if (pressKey == 'p')
        {
            pTdHandler->QueryPosition();
        }
        else if (pressKey == 'd')
        {
            pTdHandler->QueryPositionDetail();
        }
        else if (pressKey == 'o')
        {
            pTdHandler->QueryOrders();
        }
        else if (pressKey == 't')
        {
            pTdHandler->QueryTrades();
        }
        else if (pressKey == 'm')
        {
            pTdHandler->QueryMD();
        }
        else if (pressKey == 'a')
        {
            pTdHandler->QueryAcct();
        }
        else if (pressKey == 's')
        {
            pTdHandler->PassChange("");
        }
		else if (pressKey == '1')
		{
			g_pLog->setLogType(1);
		}
		else if (pressKey == '2')
		{
			g_pLog->setLogType(2);
		}
		else if (pressKey == '3')
		{
			g_pLog->setLogType(3);
		}
		else if (pressKey == '0')
		{
			g_pLog->setLogType(0);
		}
    }
    fsw.Close(1000);
	return 0;
}
