
// DemoDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "Demo.h"
#include "DemoDlg.h"
#include "afxdialogex.h"
#include<winsock2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#pragma warning(disable:4996) 






// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CDemoDlg 对话框
// 状态接收线程 
UINT __cdecl  StatusThread(LPVOID pArgument)
{
	CDemoDlg* pWnd = (CDemoDlg*)pArgument; 

	SOCKET sock_status = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);   // UDP 通讯 
	if (sock_status == INVALID_SOCKET)
	{
		WSACleanup();
		return 2;
	}

	int defRcvBufSize, defSndBufSize; 
	int optlen = sizeof(defRcvBufSize);
	if (getsockopt(sock_status, SOL_SOCKET, SO_RCVBUF, (char *) & defRcvBufSize, &optlen) < 0)  // 读缓冲区 
	{
		
	}

	if (getsockopt(sock_status, SOL_SOCKET, SO_SNDBUF, (char*)&defSndBufSize, &optlen) < 0)   // 写缓冲区 
	{

	}



	SOCKADDR_IN   servAddr;    // 服务器地址 
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons((short)6001);    // 6001 状态端口 
	servAddr.sin_addr.s_addr = inet_addr("192.168.0.30");

	// 先发送打开状态命令，这样控制端才知道客户端被分配的端口 
	while (pWnd->m_id == 0)  // 还没有登录 
	{
		Sleep(1000); 
	}

	struct TagDataChannel cmd_status;
	ZeroMemory(&cmd_status,sizeof(cmd_status));
	cmd_status.cmd_flag = DATACHANNEL;   // 打开数据通道 
	cmd_status.cmd_type = CMD_TO_SERVER; 
	cmd_status.nID = pWnd->m_id;    // 登录命令获取的ID 
	cmd_status.cmd_control = 1;   // 1 表示打开 

	pWnd->SetCheckBit((unsigned char*)&cmd_status, sizeof(cmd_status));

	int nRet = 0;
	for (int i = 0; i < 3; i++)  // 发3次，有可能会返回 SOCKET_ERROR 
	{
		nRet = sendto(sock_status, (char*)&cmd_status, sizeof(cmd_status), 0, (SOCKADDR*)&servAddr, sizeof(servAddr));  // 发送命令打开通道 
		if (SOCKET_ERROR == nRet)
		{
			TRACE1("sendto error ,error code is %d \n", WSAGetLastError());
			Sleep(1);
			
		}
		else
			break; 
	}
	

	ZeroMemory(&cmd_status, sizeof(cmd_status));

	recvfrom(sock_status, (char*)&cmd_status, sizeof(cmd_status), 0, NULL, NULL);  // 设备返回打开是否成功
	 
	if (cmd_status.cmd_flag == DATACHANNEL) // 打开成功
	{
		CString sChlIndex, sUserID, sBarcode, sTimeBegin ,sTimeEnd, sFlowID, sStatus, sError; 
		CString sTime; 
		struct TagTestResult cmd_TestResult;   //  
		struct TagSampleData cmd_SampleData;   // 显示数据 
		char RevBuf[1024] = { 0 };   // 接收缓冲区 
		char RevBuf_1[40] = { 0 };   // 接收缓冲区 
		
		int nRevLen = 0, len1 = 0;
		int nRetError; 
		// while (recvfrom(sock_status, (char*)&cmd_TestResult, sizeof(cmd_TestResult), 0, NULL, NULL))   //一直读
		while ( 1)   //  先读取数据头  cmd_flag + cmd_type; 2个字节 
		{
			nRevLen = recvfrom(sock_status, RevBuf, sizeof(RevBuf), 0, NULL, NULL);
			if (nRevLen == SOCKET_ERROR)
			{
				Sleep(1);
				nRetError = WSAGetLastError();  // 
				// WSAEMSGSIZE = 10040   消息太长。
				// 	在数据报套接字上发送的消息大于内部消息缓冲区或其他一些网络限制，或者用于接收数据报的缓冲区小于数据报本身。
				// 每次只读取部分缓冲区内容，报错10040 
				// 例如：报文有40个字节，只读2个字节，则 返回 WSAEMSGSIZE 错误
				continue; 
			}
			if (RevBuf[1] != CMD_TO_CLIENT)  // 不是发给客户端的数据，直接丢弃
				continue; 
			if (RevBuf[0] != TESTRESULT && RevBuf[0] != CHLDATALIST)  // 不是这两个命令 
				continue; 
			
			if (!pWnd->CheckBit((unsigned char*)RevBuf, nRevLen))  // 检查校验位 
				continue; 

			
			switch (RevBuf[0])
			{
			case TESTRESULT :   // 测试结果 ，如果没有收到，主机会一直重发 
				
				memcpy(&cmd_TestResult, RevBuf,sizeof(RevBuf));
				cmd_TestResult.cmd_type = CMD_TO_SERVER;  // 给服务器返回一个相同的应答 
				nRet = sendto(sock_status, (char*)&cmd_TestResult, sizeof(cmd_TestResult), 0, (SOCKADDR*)&servAddr, sizeof(servAddr));  // 发送收到应答命令 
				if (SOCKET_ERROR == nRet)
				{
					TRACE1("sendto error ,error code is %d \n", WSAGetLastError());
					nRet = sendto(sock_status, (char*)&cmd_TestResult, sizeof(cmd_TestResult), 0, (SOCKADDR*)&servAddr, sizeof(servAddr));  // 发送收到应答命令 
				}
				if (nRet > 0)  // 如果 < 0 ,可能是缓冲区里的一些错误数据，应该抛弃，不算接收TESTRESULT 消息 
				{
					TRACE("receive a testresult msg \n");
					CTime tmFileName = CTime::GetCurrentTime();
					sTime.Format(_T("%d-%d-%d %d:%d:%d"), tmFileName.GetYear(), tmFileName.GetMonth(), tmFileName.GetDay(), tmFileName.GetHour(), tmFileName.GetMinute(), tmFileName.GetSecond()); 
					sUserID.Format(_T("%d"), cmd_TestResult.nID); 
					sChlIndex.Format(_T("%d-%d-%d"), cmd_TestResult.cReg, cmd_TestResult.cBox, cmd_TestResult.cChl); 
					sBarcode = cmd_TestResult.szBarcode;  // ?? 
					sTimeBegin = cmd_TestResult.szBeginTime;   // 流程开始时间  
					sTimeEnd = cmd_TestResult.szEndTime;       // 流程结束时间 
					sFlowID.Format(_T("%d"), cmd_TestResult.nProcedureNO); 
					if (cmd_TestResult.status == STATUS_OK)
						sStatus = _T("OK");
					else
						sStatus = _T("NG"); 

					sError.Format(_T("0x%X"), cmd_TestResult.nErrCode); 
					pWnd->m_ctlList.AddString(sTime + _T(" | ") + sUserID + _T(" | ") + sChlIndex + _T(" | ") + sBarcode + _T(" | ")+ sTimeBegin + _T(" | ") + sTimeEnd + _T(" | ")+ sFlowID + _T(" | ") + sStatus + _T(" | ") + sError);

				}
				 break; 
			case CHLDATALIST :   // 通道显示值  
				// 具体通道的显示值，不需要应答  
				memcpy(&cmd_SampleData, RevBuf, sizeof(cmd_SampleData));
						 
				pWnd->ShowChlValue((char *) & cmd_SampleData, sizeof(cmd_SampleData));  // 主界面显示 

				break; 
			default:
				break;
			}
		 }

	}

	return 0; 
}


CDemoDlg::CDemoDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DEMO_DIALOG, pParent)
	, m_nReg(0)
	, m_nBox(0)
	, m_nChl(0)
	, m_nFlowID(0)
	, m_nStartVote(0)
	, m_nEndVote(0)
	, m_nStartCurrent(0)
	, m_nEndCurrent(0)
	, m_nCap(0)
	, m_nEnergy(0)
	, m_nStepTime(0)
	, m_nMode(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDemoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT2, m_nReg);
	DDX_Text(pDX, IDC_EDIT3, m_nBox);
	DDX_Text(pDX, IDC_EDIT4, m_nChl);
	DDX_Control(pDX, IDC_LIST1, m_ctlList);
	DDX_Text(pDX, IDC_EDIT5, m_nFlowID);
	DDX_Control(pDX, IDC_IPADDRESS1, m_ctlIPEdit);
	DDX_Control(pDX, IDC_STATIC_CH1, m_ctlChl1);
	DDX_Control(pDX, IDC_STATIC_CH2, m_ctlChl2);
	DDX_Control(pDX, IDC_STATIC_CH3, m_ctlChl3);
	DDX_Control(pDX, IDC_STATIC_CH4, m_ctlChl4);
	DDX_Control(pDX, IDC_STATIC_CH5, m_ctlChl5);
	DDX_Control(pDX, IDC_STATIC_CH6, m_ctlChl6);
	DDX_Control(pDX, IDC_STATIC_CH7, m_ctlChl7);
	DDX_Control(pDX, IDC_STATIC_CH8, m_ctlChl8);
	DDX_Text(pDX, IDC_EDIT_START_VOTE, m_nStartVote);
	DDX_Text(pDX, IDC_EDIT_END_VOTE, m_nEndVote);
	DDX_Text(pDX, IDC_EDIT_START_CURRENT, m_nStartCurrent);
	DDX_Text(pDX, IDC_EDIT_END_CURRENT, m_nEndCurrent);
	DDX_Text(pDX, IDC_EDIT_CAP, m_nCap);
	DDX_Text(pDX, IDC_EDIT_ENERGY, m_nEnergy);
	DDX_Text(pDX, IDC_EDIT_ENERGY2, m_nStepTime);
	DDX_Text(pDX, IDC_EDIT_ENERGY3, m_nMode);
}

BEGIN_MESSAGE_MAP(CDemoDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CDemoDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CDemoDlg::OnBnClickedButton2)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON3, &CDemoDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CDemoDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON5, &CDemoDlg::OnBnClickedButton5)
	ON_BN_CLICKED(IDC_BUTTON6, &CDemoDlg::OnBnClickedButton6)
	ON_BN_CLICKED(IDC_BUTTON7, &CDemoDlg::OnBnClickedButton7)
END_MESSAGE_MAP()


// CDemoDlg 消息处理程序

BOOL CDemoDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	m_id = 0; 

	m_nReg = 1; 
	m_nBox = 11; 
	m_nChl = 1; 

	m_nFlowID = 2; 
	m_unServerIP = 0;  
	UpdateData(FALSE); 

	WORD wVersionRequested;
	WSADATA wsaData;
	

	wVersionRequested = MAKEWORD(2, 2);
	if (WSAStartup(wVersionRequested, &wsaData) != 0)
	{
		//初始化套接字环境失败
		return 1;
	}


	m_sock_control = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);   // UDP 通讯 
	if (m_sock_control == INVALID_SOCKET)
	{
		WSACleanup();
		return 2;
	}

	// SOCKADDR_IN   servAddr;    // 服务器地址 
	m_servAddr.sin_family = AF_INET;
	m_servAddr.sin_port = htons((short)6000);    // 6000 端口 
	m_servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // 默认 

	m_ctlIPEdit.SetAddress(127,0,0,1); 

	AfxBeginThread(StatusThread, this); 




	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CDemoDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CDemoDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CDemoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


// 登录主机 
void CDemoDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
#pragma pack(1)
	struct TagLogin
	{
		BYTE cmd_flag;
		BYTE cmd_type;
		unsigned char szName[50];   // 登录名 
		unsigned char szPass[50];   // 登录密码
		int  nID;  // 返回用户ID
		BYTE status;   // 状态 
		BYTE check;   // 校验和 

	};

	struct TagQuestBox
	{
		unsigned char  cmd_flag;
		unsigned char  cmd_type;
		int      nID;  // 返回用户ID
		int      nBoxCount;  // 在线箱数量
		BYTE	   status;   // 状态 
		BYTE	   check;   // 校验和  
	};

	 
	struct TagOnlineBoxConfig
	{
		unsigned char  cmd_flag;
		unsigned char  cmd_type;
		int      nID;  // 返回用户ID
		int      nBoxID;  // 在线箱ID 从0 开始 
		int      nChlCount;  // 通道数
		int      nVote;      // 电压量程
		int      vCurrent;    //  电流量程
		
		unsigned char      nReg;     // 柜号
		unsigned char      nBox;     // 箱号 

		BYTE	   status;   // 状态 
		BYTE	   check;   // 校验和  
	};

	

	struct TagQuestChlStatus
	{
		unsigned char  cmd_flag;
		unsigned char  cmd_type;
		int      nID;  // 返回用户ID

		unsigned char      nReg;     // 柜号
		unsigned char      nBox;     // 箱号 
		unsigned char      nChl;     // 通道号 
		unsigned char      Chl_status;  // 通道状态 

		BYTE	   status;   // 状态 
		BYTE	   check;   // 校验和  
	};



#pragma pack()

	struct TagLogin cmd_login;
	ZeroMemory(&cmd_login, sizeof(cmd_login));

	cmd_login.cmd_flag = LOGIN_CMD;
	cmd_login.cmd_type = CMD_TO_SERVER;
	strcpy_s((char*)cmd_login.szName, sizeof(cmd_login.szName), "user");
	strcpy_s((char*)cmd_login.szPass, sizeof(cmd_login.szPass), "ydn123456");
	cmd_login.status = STATUS_OK;
	SetCheckBit((unsigned char *)&cmd_login,sizeof(cmd_login));

	for (int i = 0; i < 3; i++)
	{
		int nRet = sendto(m_sock_control, (char*)&cmd_login, sizeof(cmd_login), 0, (SOCKADDR*)&m_servAddr, sizeof(m_servAddr));
		if (SOCKET_ERROR == nRet)
		{
			TRACE1("sendto error ,error code is %d \n", WSAGetLastError());
			Sleep(1); 
		}
		else
			break; 

	}
	

	ZeroMemory(&cmd_login, sizeof(cmd_login));

	recvfrom(m_sock_control, (char*)&cmd_login, sizeof(cmd_login), 0, NULL, NULL);
	m_id = cmd_login.nID;  // 用户ID 

	TagQuestBox box_data; 
	ZeroMemory(&box_data, sizeof(box_data));
	box_data.cmd_flag = QUESTONLINEBOX_CMD;
	box_data.cmd_type = CMD_TO_SERVER;
	box_data.nID = m_id; 
	SetCheckBit((unsigned char*)&box_data, sizeof(box_data)); 

	for (int i = 0; i < 3; i++)
	{
		int nRet = sendto(m_sock_control, (char*)&box_data, sizeof(box_data), 0, (SOCKADDR*)&m_servAddr, sizeof(m_servAddr));
		if (SOCKET_ERROR == nRet)
		{
			TRACE1("sendto error ,error code is %d \n", WSAGetLastError());
			Sleep(1);
		}
		else
			break;

	}

	ZeroMemory(&box_data, sizeof(box_data));

	recvfrom(m_sock_control, (char*)&box_data, sizeof(box_data), 0, NULL, NULL);

	int nBoxCount = box_data.nBoxCount; // box 数量 
	
	CString sBoxConfigInfo; 

	for (int i = 0; i < nBoxCount; i++)
	{
		// 分别获取每个箱的量程
		TagOnlineBoxConfig boxCongig; 
		ZeroMemory(&boxCongig, sizeof(boxCongig));

		boxCongig.cmd_flag = QUESTBOXCONFIG; 
		boxCongig.cmd_type = CMD_TO_SERVER; 
		boxCongig.nID = m_id;
		boxCongig.nBoxID = i; 
		SetCheckBit((unsigned char*)&boxCongig, sizeof(boxCongig));
		// 发送查询BOX 配置命令  
		for (int i = 0; i < 3; i++)
		{   
			int nRet = sendto(m_sock_control, (char*)&boxCongig, sizeof(boxCongig), 0, (SOCKADDR*)&m_servAddr, sizeof(m_servAddr));
			if (SOCKET_ERROR == nRet)
			{
				TRACE1("sendto error ,error code is %d \n", WSAGetLastError());
				Sleep(1);
			}
			else
				break;

		}

		ZeroMemory(&boxCongig, sizeof(boxCongig));

		recvfrom(m_sock_control, (char*)&boxCongig, sizeof(boxCongig), 0, NULL, NULL);   // 获取每个箱的配置信息 
		CString sTmp; 
		sTmp.Format(_T("m_id :%d boxID: %d ,柜号: %d  箱号: %d 通道数: %d 电压量程: %d mV 电流量程: %d mA \n"), m_id,i, boxCongig.nReg, boxCongig.nBox, boxCongig.nChlCount, boxCongig.nVote, boxCongig.vCurrent);
		sBoxConfigInfo += sTmp; 

		TagQuestChlStatus  chlStatus;   // 通道状态 
		ZeroMemory(&chlStatus, sizeof(chlStatus));

		chlStatus.cmd_flag = QUESTCHLSTATUS;
		chlStatus.cmd_type = CMD_TO_SERVER;
		chlStatus.nID = m_id;
		chlStatus.nReg = boxCongig.nReg; 
		chlStatus.nBox = boxCongig.nBox; 
		chlStatus.nChl = 1;    // 通道号 1-8  
		SetCheckBit((unsigned char*)&chlStatus, sizeof(chlStatus));

		// 发送通道状态查询  
		for (int i = 0; i < 3; i++)
		{
			int nRet = sendto(m_sock_control, (char*)&chlStatus, sizeof(chlStatus), 0, (SOCKADDR*)&m_servAddr, sizeof(m_servAddr));
			if (SOCKET_ERROR == nRet)
			{
				TRACE1("sendto error ,error code is %d \n", WSAGetLastError());
				Sleep(1);
			}
			else
				break;

		}

		ZeroMemory(&chlStatus, sizeof(chlStatus));
		recvfrom(m_sock_control, (char*)&chlStatus, sizeof(chlStatus), 0, NULL, NULL);   // 获得通道的状态
		unsigned char status = chlStatus.Chl_status; 
		switch (status)
		{
		case 1 :
			GetDlgItem(IDC_STATIC_CHL1)->SetWindowText(_T("状态:运行\n"));  
			break; 
		case 2:
			GetDlgItem(IDC_STATIC_CHL1)->SetWindowText(_T("状态:空闲\n"));
			break; 
		case 5:
			GetDlgItem(IDC_STATIC_CHL1)->SetWindowText(_T("状态:异常\n"));
			break;
		default :
			break; 
		}

		

	}

	/*CString str; 
	str.Format(_T("User ID is %d ,boxCount is %d "), m_id , nBoxCount);*/
	GetDlgItem(IDC_STATIC_LOGIN)->SetWindowText(sBoxConfigInfo);





}

// 设置流程ID
void CDemoDlg::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码

 

	UpdateData(TRUE); 

	TagOperate  cmd_operate;
	ZeroMemory(&cmd_operate, sizeof(cmd_operate));

	cmd_operate.cmd_flag = CONTROL_CMD;
	cmd_operate.cmd_type = CMD_TO_SERVER;
	cmd_operate.nID = m_id ;    // 用户ID 
	cmd_operate.cReg = m_nReg;
	cmd_operate.cBox = m_nBox;
	cmd_operate.cChl = m_nChl;
	cmd_operate.op1 = OP_SEL_FLOW ;
	cmd_operate.op2 = m_nFlowID ;
	SetCheckBit((unsigned char*)&cmd_operate, sizeof(cmd_operate));

	for (int i = 0; i < 3; i++)
	{
		int nRet = sendto(m_sock_control, (char*)&cmd_operate, sizeof(cmd_operate), 0, (SOCKADDR*)&m_servAddr, sizeof(m_servAddr));
		if (SOCKET_ERROR == nRet)
		{
			TRACE1("sendto error ,error code is %d \n", WSAGetLastError());
			Sleep(1);
		}
		else
			break;

	}
	

	ZeroMemory(&cmd_operate, sizeof(cmd_operate));

	// 设置3秒超时，如果超时，重发 
	recvfrom(m_sock_control, (char*)&cmd_operate, sizeof(cmd_operate), 0, NULL, NULL);
	 
	CString str;
	str.Format(_T("procedure is %d "), cmd_operate.op2);
	GetDlgItem(IDC_STATIC_SET_FLOW)->SetWindowText(str);

}


void CDemoDlg::OnDestroy()
{
	CDialogEx::OnDestroy();
	WSACleanup(); 

	// TODO: 在此处添加消息处理程序代码
}

// 启动通道 
void CDemoDlg::OnBnClickedButton3()
{
	// TODO: 在此添加控件通知处理程序代码

	TagOperate  cmd_operate;
	ZeroMemory(&cmd_operate, sizeof(cmd_operate));

	cmd_operate.cmd_flag = CONTROL_CMD;
	cmd_operate.cmd_type = CMD_TO_SERVER;
	cmd_operate.nID = m_id;    // 用户ID 
	cmd_operate.cReg = m_nReg;
	cmd_operate.cBox = m_nBox;
	cmd_operate.cChl = m_nChl;
	cmd_operate.op1 = OP_START;   // 启动通道 
	cmd_operate.op2 = 0;
	SetCheckBit((unsigned char*)&cmd_operate, sizeof(cmd_operate));

	for (int i = 0; i < 3; i++)
	{
		int nRet = sendto(m_sock_control, (char*)&cmd_operate, sizeof(cmd_operate), 0, (SOCKADDR*)&m_servAddr, sizeof(m_servAddr));
		if (SOCKET_ERROR == nRet)
		{
			TRACE1("sendto error ,error code is %d \n", WSAGetLastError());
			Sleep(1);
		}
		else
			break;
	}
	

	ZeroMemory(&cmd_operate, sizeof(cmd_operate));

	// 设置3秒超时，如果超时，重发 
	recvfrom(m_sock_control, (char*)&cmd_operate, sizeof(cmd_operate), 0, NULL, NULL);

	AfxMessageBox(_T("OK")); 

}

void CDemoDlg::SetCheckBit(unsigned char* pBuff, int nLen)
{
	BYTE byteCheckBit(0);
	int iDataBit = 0;
	for (int i = 0; i < nLen - 1; i++)
	{
		byteCheckBit += pBuff[iDataBit];
		iDataBit++;
	}

	pBuff[nLen - 1] = iDataBit;

}

// 停止通道 
void CDemoDlg::OnBnClickedButton4()
{
	// TODO: 在此添加控件通知处理程序代码
	TagOperate  cmd_operate;
	ZeroMemory(&cmd_operate, sizeof(cmd_operate));

	cmd_operate.cmd_flag = CONTROL_CMD;
	cmd_operate.cmd_type = CMD_TO_SERVER;
	cmd_operate.nID = m_id;    // 用户ID 
	cmd_operate.cReg = m_nReg;
	cmd_operate.cBox = m_nBox;
	cmd_operate.cChl = m_nChl;
	cmd_operate.op1 = OP_STOP;   // 启动通道 
	cmd_operate.op2 = 0;
	SetCheckBit((unsigned char*)&cmd_operate, sizeof(cmd_operate));

	for (int i = 0; i < 3; i++)
	{
		int nRet = sendto(m_sock_control, (char*)&cmd_operate, sizeof(cmd_operate), 0, (SOCKADDR*)&m_servAddr, sizeof(m_servAddr));
		if (SOCKET_ERROR == nRet)
		{
			TRACE1("sendto error ,error code is %d \n", WSAGetLastError());
			Sleep(1);
		}
		else
			break;

	}
	

	ZeroMemory(&cmd_operate, sizeof(cmd_operate));

	// 设置3秒超时，如果超时，重发 
	recvfrom(m_sock_control, (char*)&cmd_operate, sizeof(cmd_operate), 0, NULL, NULL);

	AfxMessageBox(_T("OK"));

}


void CDemoDlg::OnBnClickedButton5()
{
	// TODO: 在此添加控件通知处理程序代码
	DWORD dwAddress ; 
	m_ctlIPEdit.GetAddress(dwAddress);
	// u_long ulTemp;
	m_unServerIP = ntohl(dwAddress);
	// m_servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	m_servAddr.sin_addr.s_addr = m_unServerIP ;
	m_servAddr.sin_family = AF_INET;
	m_servAddr.sin_port = htons((short)6000);    // 6000 端口 



	AfxMessageBox(_T("OK")); 
	


}
// 显示通道数据 
void CDemoDlg::ShowChlValue(char* pBuffer, int nBuffLen)
{
	struct TagSampleData cmd_SampleData;   // 显示数据 
	memcpy(&cmd_SampleData, pBuffer, sizeof(cmd_SampleData)); 
	CString sChlLable,sChlMode,sChlIValue,sChlValue,sChlCapcity,sChlEnergy ; 
	sChlLable.Format(_T("%d-%d-%d"), cmd_SampleData.cReg, cmd_SampleData.cBox, cmd_SampleData.cChl); 
	sChlMode.Format(_T("%d"), cmd_SampleData.byteStepMode);  // 工步模式 
	sChlIValue.Format(_T("%d"), cmd_SampleData.nISampleValue);   // 电流
	sChlValue.Format(_T("%d"), cmd_SampleData.nVSampleValue);   // 电压
	sChlCapcity.Format(_T("%d"), cmd_SampleData.nCapcity);    // 容量
	sChlEnergy.Format(_T("%d"), cmd_SampleData.nEnergy);      // 能量 

	CString sShowValue; 
	sShowValue += sChlLable; 
	sShowValue += _T("\r\n"); 
	
	sShowValue += _T("工步模式：  "); 
	sShowValue += sChlMode; 
	sShowValue += _T("\r\n"); ; 

	sShowValue += _T("电压：  ");
	sShowValue += sChlValue;
	sShowValue += _T("\r\n");

	sShowValue += _T("电流：  ");
	sShowValue += sChlIValue;
	sShowValue += _T("\r\n");

	sShowValue += _T("容量：  ");
	sShowValue += sChlCapcity;
	sShowValue += _T("\r\n");

	CStatic* pChl = NULL; 
	CStatic *pChlArray[] = { &m_ctlChl1, &m_ctlChl2 ,&m_ctlChl3 ,&m_ctlChl4 ,&m_ctlChl5 ,&m_ctlChl6 ,&m_ctlChl7 ,&m_ctlChl8 };
	pChl = pChlArray[cmd_SampleData.cChl - 1]; 

	pChl->SetWindowText(sShowValue);

}

bool CDemoDlg::CheckBit(unsigned char* pBuff, int nLen)
{
	BYTE byteCheckBit(0);
	int iDataBit = 0;
	for (int i = 0; i < nLen - 1; i++)
	{
		byteCheckBit += pBuff[iDataBit];
		iDataBit++;
	}

	BYTE check0 = pBuff[nLen - 1];
	if (iDataBit == check0)
		return true;
	else
		return false;
}

// 工步查询 
void CDemoDlg::OnBnClickedButton6()
{
	// TODO: 在此添加控件通知处理程序代码
	 
	TagQuestStep  cmd_queststep;
	ZeroMemory(&cmd_queststep, sizeof(cmd_queststep));

	cmd_queststep.cmd_flag = QUESTSTEPDATA;
	cmd_queststep.cmd_type = CMD_TO_SERVER;
	cmd_queststep.nID = m_id;    // 用户ID 
	cmd_queststep.cReg = m_nReg;
	cmd_queststep.cBox = m_nBox;
	cmd_queststep.cChl = m_nChl;

	cmd_queststep.nCircle = 1;    // 流程号 
	cmd_queststep.nStepIndex = 2 ;  // 工步索引 

	SetCheckBit((unsigned char*)&cmd_queststep, sizeof(cmd_queststep));
    
	for (int i = 0; i < 3; i++)
	{
		int nRet = sendto(m_sock_control, (char*)&cmd_queststep, sizeof(cmd_queststep), 0, (SOCKADDR*)&m_servAddr, sizeof(m_servAddr));
		if (SOCKET_ERROR == nRet)
		{
			TRACE1("sendto error ,error code is %d \n", WSAGetLastError());
			Sleep(1);
		}
		else
			break;

	}


	ZeroMemory(&cmd_queststep, sizeof(cmd_queststep));

	// 设置3秒超时，如果超时，重发 
	recvfrom(m_sock_control, (char*)&cmd_queststep, sizeof(cmd_queststep), 0, NULL, NULL);

	int nStartVolt = cmd_queststep.nStartVoltage; 
	int nEndVolt = cmd_queststep.nEndVoltage; 
	int nStartCurrent = cmd_queststep.nStartCurrent; 
	int nEndCurrent = cmd_queststep.nEndCurrent; 
	int nCapcity  = cmd_queststep.nCapcity;
	int nEnergy = cmd_queststep.nEnergy; 
	int nStepTime = cmd_queststep.nStepTime;
	int byteStepMode = cmd_queststep.byteStepMode;

	m_nStartVote = nStartVolt ;
	m_nEndVote = nEndVolt ;
	m_nStartCurrent = nStartCurrent ;
	m_nEndCurrent = nEndCurrent ;
	m_nCap = nCapcity ;
	m_nEnergy = nEnergy ;
	m_nStepTime = nStepTime; 
	m_nMode = byteStepMode; 

	UpdateData(FALSE); 

}

// 查询流程测试结果
void CDemoDlg::OnBnClickedButton7()
{
	// TODO: 在此添加控件通知处理程序代码
	TagTestResult  cmd_questresult ;
	ZeroMemory(&cmd_questresult, sizeof(cmd_questresult));

	cmd_questresult.cmd_flag = TESTRESULT;
	cmd_questresult.cmd_type = CMD_TO_SERVER;
	cmd_questresult.nID = m_id;    // 用户ID 
	cmd_questresult.cReg = m_nReg;
	cmd_questresult.cBox = m_nBox;
	cmd_questresult.cChl = m_nChl;

	SetCheckBit((unsigned char*)&cmd_questresult, sizeof(cmd_questresult));

	for (int i = 0; i < 3; i++)
	{
		int nRet = sendto(m_sock_control, (char*)&cmd_questresult, sizeof(cmd_questresult), 0, (SOCKADDR*)&m_servAddr, sizeof(m_servAddr));
		if (SOCKET_ERROR == nRet)
		{
			TRACE1("sendto error ,error code is %d \n", WSAGetLastError());
			Sleep(1);
		}
		else
			break;

	}


	ZeroMemory(&cmd_questresult, sizeof(cmd_questresult));

	// 设置3秒超时，如果超时，重发 
	recvfrom(m_sock_control, (char*)&cmd_questresult, sizeof(cmd_questresult), 0, NULL, NULL);

	unsigned char status = cmd_questresult.status; 
	unsigned char code = cmd_questresult.nErrCode; 
	
	if (status == STATUS_OK)
		GetDlgItem(IDC_EDIT10)->SetWindowText(_T("OK")); 
	else
		GetDlgItem(IDC_EDIT10)->SetWindowText(_T("NG"));

	CString sTmp; 
	sTmp.Format(_T("%d"), code); 
	
	GetDlgItem(IDC_EDIT11)->SetWindowText(sTmp);

 

}
