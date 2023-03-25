
// DemoDlg.h: 头文件
//

#pragma once
#define LOGIN_CMD    1
#define CONTROL_CMD  2
#define QUESTONLINEBOX_CMD  3
#define DATACHANNEL   4        // 打开数据通道 

#define DEVICE_STATUS_CMD 5     // 设备状态命令 
#define TESTRESULT    6        // 测试判断结果 
#define QUESTBOXCONFIG  7     // 查询每个BOX 量程等配置信息 
#define CHLDATALIST   8        //  实时数据 

#define	QUESTCHLSTATUS  9      // 查询通道状态 
#define QUESTSTEPDATA  10     //  查询工步数据 

#define CMD_TO_SERVER  0
#define CMD_TO_CLIENT  1 

#define STATUS_OK  0 
#define STATUS_ERROR 1  
#define STATUS_NG  1 

#define OP_START	1 
#define OP_STOP		2
#define OP_PAUSE	3
#define OP_CONTINUE 4
#define OP_INQUIRE	5
#define OP_SEL_FLOW 6
/////////////////////////////////////////////////////////////////////////
// 工步模式数据类型定义
#define UNAWARE_STEP                    ((unsigned char)0xFF)
#define CONST_CURRENT_PRE_CHARGE		((unsigned char)'B')      //CC-CV  
#define CONST_CURRENT_CHARGE			((unsigned char)'K')
#define CONST_VOLTAGE_CHARGE            ((unsigned char)'I')	  //CC-CV
#define CONST_VOLTAGE_PRE_CHARGE        ((unsigned char)'L')
#define CONST_CURRENT_DISCHARGE         ((unsigned char)'C')
#define IDLE_LEAVE                      ((unsigned char)'D')
#define CONST_POWER_DISCHARGE           ((unsigned char)'H')
#define JUMP_GOTO                       ((unsigned char)'J')

#define CONST_POWER_CHARGE              ((unsigned char)'G')  //恒功率充电
#define CONST_VOLTAGE_DISCHARGE         ((unsigned char)'T')  //恒压放电
#define CONST_CURRENT_PRE_DISCHARGE     ((unsigned char)'U')  //CCD-CVD中CCD
#define CONST_VOLTAGE_PRE_DISCHARGE     ((unsigned char)'V')  //CCD-CVD中CVD
#define CONST_RESIST_DISCHARGE          ((unsigned char)'W')  //恒阻放电
#define CONST_RESIST_CHARGE             ((unsigned char)'X')  //恒阻充电



#pragma pack(1)  
struct TagOperate
{
	unsigned char  cmd_flag;
	unsigned char  cmd_type;
	int      nID;  // 返回用户ID
	unsigned char cReg;  // 柜号
	unsigned char cBox;  // 箱号
	unsigned char cChl;  // 通道号 
	unsigned char op1;
	int  op2;
	BYTE	check;   // 校验和 

};


struct TagDataChannel 
{
	BYTE cmd_flag;
	BYTE cmd_type;
	int  nID;
	BYTE cmd_control; 
	BYTE status;   // 状态 
	BYTE check;   // 校验和 

};


struct TagTestResult
{
	BYTE cmd_flag;
	BYTE cmd_type;
	int      nID;  // 返回用户ID
	unsigned char cReg;  // 柜号
	unsigned char cBox;  // 箱号
	unsigned char cChl;  // 通道号 
	char  szBarcode[50];  // 电池条码 
	char  szBeginTime[50];       // 起始时间
	char  szEndTime[50];         // 结束时间 
	int  nProcedureNO;           // 流程ID 
	char  szProcedureName[50];   // 流程名 
	unsigned char status;   // 状态 
	unsigned char nErrCode;  // 错误码 
	BYTE	check;           // 校验和 

};

struct TagSampleData
{
	BYTE cmd_flag;
	BYTE cmd_type;
	int      nID;  // 返回用户ID
	unsigned char cReg;  // 柜号
	unsigned char cBox;  // 箱号
	unsigned char cChl;  // 通道号 

	unsigned char nStepIndex;  // 工步索引 
	unsigned char byteStepMode;  // 工步模式

	int  nISampleValue;          //电流采样值
	int  nVSampleValue;          //电压采样值
	int  nCapcity;               //电池容量
	int  nEnergy;                //电池能量

	int nDCRValue;     // 内阻值
	int  nTemperatureValue;      //温度值

	int  dStepTime;              //工步所持续的时间（秒）

	unsigned char status;       // 状态 
	unsigned char check;        // 校验和 

};

struct TagQuestStep
{
	BYTE cmd_flag;
	BYTE cmd_type;
	int      nID;        // 用户ID
	unsigned char cReg;  // 柜号
	unsigned char cBox;  // 箱号
	unsigned char cChl;  // 通道号 
// 	int   nFlowID;         // 流程号
	int   nCircle;         // 循环号
	unsigned char nStepIndex;  // 工步模式

	int  nStartVoltage;           // 开始电压
	int  nEndVoltage;            // 结束电压
	int  nStartCurrent;          // 开始电流
	int  nEndCurrent;            // 结束电流 
	
	int  nCapcity;               //电池容量
	int  nEnergy;                //电池能量

	int  nStepTime;             // 工步持续时间 

	unsigned char byteStepMode;  // 工步类型 
	unsigned char status;       // 状态 
	unsigned char check;        // 校验和 

};



#pragma pack()

// CDemoDlg 对话框
class CDemoDlg : public CDialogEx
{
// 构造
public:
	CDemoDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DEMO_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

public :
	int m_id;    // 用户ID 

// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
public :
	SOCKADDR_IN   m_servAddr;    // 服务器地址
	SOCKET        m_sock_control; 
	afx_msg void OnBnClickedButton2();
	afx_msg void OnDestroy();
	// 柜号
	int m_nReg;
	int m_nBox;
	int m_nChl;
	afx_msg void OnBnClickedButton3();
	void SetCheckBit(unsigned char* pBuff, int nLen) ; 
	CListBox m_ctlList;
	afx_msg void OnBnClickedButton4();
	int m_nFlowID;
	CIPAddressCtrl m_ctlIPEdit;
	u_long m_unServerIP; 
	afx_msg void OnBnClickedButton5();
	void ShowChlValue(char* pBuffer, int nBuffLen); 
	bool CheckBit(unsigned char* pBuff, int nLen); 


	CStatic m_ctlChl1;
	CStatic m_ctlChl2;
	CStatic m_ctlChl3;
	CStatic m_ctlChl4;
	CStatic m_ctlChl5;
	CStatic m_ctlChl6;
	CStatic m_ctlChl7;
	CStatic m_ctlChl8;
	afx_msg void OnBnClickedButton6();
	int m_nStartVote;
	int m_nEndVote;
	int m_nStartCurrent;
	int m_nEndCurrent;
	int m_nCap;
	int m_nEnergy;
	int m_nStepTime;
	int m_nMode;
	afx_msg void OnBnClickedButton7();
};
