
// TestST_CurveDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "TestST_Curve.h"
#include "TestST_CurveDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define PAGECHANGE		(WM_APP + 1)

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CTestST_CurveDlg 对话框



CTestST_CurveDlg::CTestST_CurveDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CTestST_CurveDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTestST_CurveDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STCURVECTRL, m_ST_Curve);
	DDX_Control(pDX, IDC_STCURVECTRL2, m_ST_Curve2);
}

BEGIN_MESSAGE_MAP(CTestST_CurveDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_WM_LBUTTONDBLCLK()
	ON_BN_CLICKED(IDC_BUTTON1, OnButton1)
	ON_BN_CLICKED(IDC_BUTTON2, OnButton2)
	ON_BN_CLICKED(IDC_BUTTON3, OnButton3)
	ON_BN_CLICKED(IDC_BUTTON4, OnButton4)
	ON_BN_CLICKED(IDC_BUTTON5, OnButton5)
	ON_BN_CLICKED(IDC_BUTTON6, OnButton6)
	ON_BN_CLICKED(IDC_BUTTON7, OnButton7)
	ON_BN_CLICKED(IDC_BUTTON8, OnButton8)
	ON_BN_CLICKED(IDC_BUTTON9, OnButton9)
	ON_BN_CLICKED(IDC_BUTTON10, OnButton10)
	ON_BN_CLICKED(IDC_BUTTON11, OnButton11)
	ON_BN_CLICKED(IDC_BUTTON12, OnButton12)
	ON_BN_CLICKED(IDC_BUTTON13, OnButton13)
	ON_BN_CLICKED(IDC_BUTTON14, OnButton14)
	ON_BN_CLICKED(IDC_BUTTON15, OnButton15)
	ON_BN_CLICKED(IDC_BUTTON16, OnButton16)
	ON_BN_CLICKED(IDC_BUTTON17, OnButton17)
	ON_BN_CLICKED(IDC_BUTTON18, OnButton18)
	ON_BN_CLICKED(IDC_BUTTON19, OnButton19)
	ON_BN_CLICKED(IDC_BUTTON20, OnButton20)
	ON_BN_CLICKED(IDC_BUTTON21, OnButton21)
	ON_BN_CLICKED(IDC_BUTTON22, OnButton22)
	ON_BN_CLICKED(IDC_BUTTON23, OnButton23)
	ON_BN_CLICKED(IDC_BUTTON24, OnButton24)
	ON_BN_CLICKED(IDC_BUTTON25, OnButton25)
	ON_BN_CLICKED(IDC_BUTTON26, OnButton26)
	ON_BN_CLICKED(IDC_BUTTON27, OnButton27)
	ON_BN_CLICKED(IDC_BUTTON28, OnButton28)
	ON_BN_CLICKED(IDC_BUTTON29, OnButton29)
	ON_BN_CLICKED(IDC_BUTTON30, OnButton30)
	ON_BN_CLICKED(IDC_BUTTON31, OnButton31)
	ON_BN_CLICKED(IDC_BUTTON32, OnButton32)
	ON_BN_CLICKED(IDC_BUTTON33, OnButton33)
	ON_BN_CLICKED(IDC_BUTTON34, OnButton34)
	ON_BN_CLICKED(IDC_BUTTON35, OnButton35)
	ON_BN_CLICKED(IDC_BUTTON36, OnButton36)
	ON_BN_CLICKED(IDC_BUTTON37, OnButton37)
	ON_BN_CLICKED(IDC_BUTTON38, OnButton38)
	ON_BN_CLICKED(IDC_BUTTON39, OnButton39)
	ON_BN_CLICKED(IDC_BUTTON40, OnButton40)
	ON_BN_CLICKED(IDC_BUTTON41, OnButton41)
	ON_BN_CLICKED(IDC_BUTTON42, OnButton42)
	ON_BN_CLICKED(IDC_BUTTON43, OnButton43)
	ON_BN_CLICKED(IDC_BUTTON44, OnButton44)
	ON_BN_CLICKED(IDC_BUTTON45, OnButton45)
	ON_BN_CLICKED(IDC_BUTTON46, OnButton46)
	ON_BN_CLICKED(IDC_BUTTON47, OnButton47)
	ON_BN_CLICKED(IDC_BUTTON48, OnButton48)
	ON_BN_CLICKED(IDC_BUTTON49, OnButton49)
	ON_BN_CLICKED(IDC_BUTTON50, OnButton50)
	ON_BN_CLICKED(IDC_BUTTON51, OnButton51)
	ON_BN_CLICKED(IDC_BUTTON52, OnButton52)
	ON_BN_CLICKED(IDC_BUTTON53, OnButton53)
	ON_BN_CLICKED(IDC_BUTTON54, OnButton54)
	ON_BN_CLICKED(IDC_BUTTON55, OnButton55)
	ON_BN_CLICKED(IDC_BUTTON56, OnButton56)
	ON_BN_CLICKED(IDC_BUTTON57, OnButton57)
	ON_BN_CLICKED(IDC_BUTTON58, OnButton58)
	ON_BN_CLICKED(IDC_BUTTON59, OnButton59)
	ON_BN_CLICKED(IDC_BUTTON60, OnButton60)
	ON_BN_CLICKED(IDC_BUTTON61, OnButton61)
	ON_BN_CLICKED(IDC_BUTTON62, OnButton62)
	ON_BN_CLICKED(IDC_BUTTON63, OnButton63)
	ON_BN_CLICKED(IDC_BUTTON64, OnButton64)
	ON_BN_CLICKED(IDC_BUTTON65, OnButton65)
	ON_BN_CLICKED(IDC_BUTTON66, OnButton66)
	ON_BN_CLICKED(IDC_BUTTON67, OnButton67)
	ON_BN_CLICKED(IDC_BUTTON68, OnButton68)
	ON_BN_CLICKED(IDC_BUTTON69, OnButton69)
	ON_BN_CLICKED(IDC_BUTTON70, OnButton70)
	ON_BN_CLICKED(IDC_BUTTON71, OnButton71)
	ON_BN_CLICKED(IDC_BUTTON72, OnButton72)
	ON_BN_CLICKED(IDC_BUTTON73, OnButton73)
	ON_BN_CLICKED(IDC_BUTTON74, OnButton74)
	ON_BN_CLICKED(IDC_BUTTON75, OnButton75)
	ON_BN_CLICKED(IDC_BUTTON76, OnButton76)
	ON_BN_CLICKED(IDC_BUTTON77, OnButton77)
	ON_BN_CLICKED(IDC_BUTTON78, OnButton78)
	ON_BN_CLICKED(IDC_BUTTON79, OnButton79)
	ON_MESSAGE(PAGECHANGE, OnPageChange)
END_MESSAGE_MAP()


// CTestST_CurveDlg 消息处理程序
LRESULT CTestST_CurveDlg::OnPageChange(WPARAM wParam, LPARAM lParam)
{
	CString str;
	str.Format(_T("%u:%u"), (ULONG) wParam, (ULONG) lParam);
	SetDlgItemText(IDC_EDIT10, str);
	return 0;
}

/*
static LPCTSTR ppFileName[] =
{
	_T("D:\\MyProject\\通用组件\\TestST_Curve\\res\\002.bmp"),
	_T("D:\\MyProject\\通用组件\\TestST_Curve\\res\\06.bmp"),
	_T("D:\\MyProject\\通用组件\\TestST_Curve\\res\\44.bmp"),
	_T("D:\\MyProject\\通用组件\\TestST_Curve\\res\\46.bmp"),
	_T("D:\\MyProject\\通用组件\\TestST_Curve\\res\\101.bmp"),
	_T("D:\\MyProject\\通用组件\\TestST_Curve\\res\\102.bmp"),
	_T("D:\\MyProject\\通用组件\\TestST_Curve\\res\\tex6.bmp"),
	_T("D:\\MyProject\\通用组件\\TestST_Curve\\res\\tex7.bmp"),
	_T("D:\\MyProject\\通用组件\\TestST_Curve\\res\\Textmauv.bmp"),
	_T("D:\\MyProject\\通用组件\\TestST_Curve\\res\\Textora.bmp"),
};
*/

BOOL CTestST_CurveDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
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

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	OleTime.ParseDateTime(L"2007-5-8 0:15:0");
//	AfxMessageBox(OleTime.Format());
	Time2 = Time1 = OleTime; //COleDateTime::GetCurrentTime();
	TimeSpan = 0.041666666666666666666666666666;

//	m_ST_Curve.MoveWindow(0, 0, 1000, 380);
//	m_ST_Curve.MoveWindow(0, 0, 980, 380);
	m_ST_Curve.SetMaxLength(1000, 700);
//	m_ST_Curve.SetValueStep(1.0f);
	m_ST_Curve.SetMSGRecWnd(SplitHandle(m_ST_Curve, m_hWnd));
	m_ST_Curve.SetPageChangeMSG(PAGECHANGE);

//	m_ST_Curve.SetBackColor(RGB(255, 255, 255));
//	m_ST_Curve.SetForeColor(RGB(0, 0, 0));

	m_ST_Curve.SetBuddy(SplitHandle(m_ST_Curve, m_ST_Curve2.m_hWnd), 0);
	m_ST_Curve.EnableHZoom(TRUE);

/*	//通过句柄添加位图（HBITMAP）
	CBitmap b;
	b.LoadBitmap(nBitmap);
	m_ST_Curve.AddBitmapHandle(SplitHandle(m_ST_Curve, b.Detach()), FALSE);
*/
	//通过位图句（HBITMAP）添加背景位图
//	for (int i = IDB_1; i <= IDB_10; ++i)
//		m_ST_Curve.AddImageHandle(ppFileName[i - IDB_1]);
	HINSTANCE h = AfxGetResourceHandle();
	m_ST_Curve.SetRegister1(GetH32bit(h));
	for (int i = IDB_1; i <= IDB_10; ++i) //这个循环里面都要使用同一个高32位，所以前面统一做了
		m_ST_Curve.AddBitmapHandle3((long) h, i, FALSE);

// 	m_ST_Curve.EnableFullScreen(TRUE);
// 	m_ST_Curve2.EnableFullScreen(TRUE);
	m_ST_Curve.SetShowMode(0xC);
	m_ST_Curve.SetBottomSpace(0);
	m_ST_Curve.EnableFocusState(FALSE);
	m_ST_Curve2.EnableFocusState(FALSE);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CTestST_CurveDlg::OnSysCommand(UINT nID, LPARAM lParam)
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
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CTestST_CurveDlg::OnPaint()
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
HCURSOR CTestST_CurveDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CTestST_CurveDlg::OnButton1() //背景色
{
	m_ST_Curve2.SetBuddy(SplitHandle(m_ST_Curve2, m_ST_Curve.m_hWnd), 0); //更改联动服务器为m_ST_Curve2
//	m_ST_Curve2.SetBuddy(SplitHandle(m_ST_Curve2, 0), 0);
//	m_ST_Curve.SetBuddy(SplitHandle(m_ST_Curve, m_ST_Curve2.m_hWnd), 1);
//	m_ST_Curve.SetBackColor(0x80000000 | m_ST_Curve.GetBackColor());
//	CColorDialog dlg;
//	if (IDOK == dlg.DoModal())
//		m_ST_Curve.SetBackColor(dlg.GetColor());
}

void CTestST_CurveDlg::OnButton2() //文字色
{
	m_ST_Curve.SetForeColor(0x80000000 | m_ST_Curve.GetForeColor());
//	CColorDialog dlg;
//	if (IDOK == dlg.DoModal())
//		m_ST_Curve.SetForeColor(dlg.GetColor());
}

void CTestST_CurveDlg::OnButton3() //坐标轴色
{
	m_ST_Curve.SetAxisColor(0x80000000 | m_ST_Curve.GetAxisColor());
//	CColorDialog dlg;
//	if (IDOK == dlg.DoModal())
//		m_ST_Curve.SetAxisColor(dlg.GetColor());
}

void CTestST_CurveDlg::OnButton4() 
{
	CString str;
	str.Format(_T("%08X"), m_ST_Curve.GetBackColor());
	AfxMessageBox(str);
}

void CTestST_CurveDlg::OnButton46() //网格色
{
	m_ST_Curve.SetGridColor(0x80000000 | m_ST_Curve.GetGridColor());
//	CColorDialog dlg;
//	if (IDOK == dlg.DoModal())
//		m_ST_Curve.SetGridColor(dlg.GetColor());
}

void CTestST_CurveDlg::OnButton47() 
{
	CString str;
	str.Format(_T("%08X"), m_ST_Curve.GetGridColor());
	AfxMessageBox(str);
}

void CTestST_CurveDlg::OnButton48() 
{
	m_ST_Curve.SetGridMode(3);
}

void CTestST_CurveDlg::OnButton49() 
{
	m_ST_Curve.SetGridMode(0);
}

void CTestST_CurveDlg::OnButton5() 
{
	CString str;
	str.Format(_T("%08X"), m_ST_Curve.GetForeColor());
	AfxMessageBox(str);
}

void CTestST_CurveDlg::OnButton6() 
{
	CString str;
	str.Format(_T("%08X"), m_ST_Curve.GetAxisColor());
	AfxMessageBox(str);
}

void CTestST_CurveDlg::OnButton7() //设置开始时间
{
	CString str;
	GetDlgItemText(IDC_EDIT1, str);
	if (!m_ST_Curve.SetBeginTime(str))
		AfxMessageBox(_T("设置失败！"));
}

void CTestST_CurveDlg::OnButton8() 
{
	SetDlgItemText(IDC_EDIT1, m_ST_Curve.GetBeginTime());
}

void CTestST_CurveDlg::OnButton9() //设置时间步长
{
	CString str;
	GetDlgItemText(IDC_EDIT2, str);
	USES_CONVERSION;
	const char* pstr = W2A(str);
	if (!m_ST_Curve.SetTimeSpan(atof(pstr)))
		AfxMessageBox(_T("时间间隔不正确！"));
}

void CTestST_CurveDlg::OnButton10() 
{
	CString str;
	str.Format(_T("%f"), m_ST_Curve.GetTimeSpan());
	SetDlgItemText(IDC_EDIT2, str);
}

void CTestST_CurveDlg::OnButton11() //设置纵坐标开始值
{
	CString str;
	GetDlgItemText(IDC_EDIT3, str);
	USES_CONVERSION;
	const char* pstr = W2A(str);
	m_ST_Curve2.SetBeginValue((float) atof(pstr));
}

void CTestST_CurveDlg::OnButton12() 
{
	CString str;
	str.Format(_T("%f"), m_ST_Curve.GetBeginValue());
	SetDlgItemText(IDC_EDIT3, str);
}

void CTestST_CurveDlg::OnButton13() //设置纵坐标步长
{
	CString str;
	GetDlgItemText(IDC_EDIT4, str);
	USES_CONVERSION;
	const char* pstr = W2A(str);
	if (!m_ST_Curve.SetValueStep((float) atof(pstr)))
		AfxMessageBox(_T("纵标步长不正确！"));
}

void CTestST_CurveDlg::OnButton14() 
{
	CString str;
	str.Format(_T("%f"), m_ST_Curve.GetValueStep());
	SetDlgItemText(IDC_EDIT4, str);
}

void CTestST_CurveDlg::OnButton15() 
{
	CString str;
	GetDlgItemText(IDC_EDIT5, str);
	if (!m_ST_Curve.SetUnit(str))
		AfxMessageBox(_T("单位太长！"));
}

void CTestST_CurveDlg::OnButton16() 
{
	SetDlgItemText(IDC_EDIT5, m_ST_Curve.GetUnit());
}

void CTestST_CurveDlg::OnButton18() 
{
	int iv = GetDlgItemInt(IDC_EDIT6);
	if (!m_ST_Curve.DelLegend(iv, 0, TRUE))
		AfxMessageBox(_T("删除失败！"));
}

void CTestST_CurveDlg::OnButton19() 
{
	CString str;
	int iv = GetDlgItemInt(IDC_EDIT6);
	str = m_ST_Curve.QueryLegend(iv);
	COLORREF Color;
	if (m_ST_Curve.GetLegend(str, (long*) &Color, 0, 0, 0, 0, 0, 0))
	{
		str.Format(_T("%08X"), Color);
		AfxMessageBox(str);
	}
	else
		AfxMessageBox(_T("未找到指定曲线！"));
}

void CTestST_CurveDlg::OnButton17() //添加图例
{
//	#define HS_HORIZONTAL       0       /* ----- */
//	#define HS_VERTICAL         1       /* ||||| */
//	#define HS_FDIAGONAL        2       /* \\\\\ */
//	#define HS_BDIAGONAL        3       /* ///// */
//	#define HS_CROSS            4       /* +++++ */
//	#define HS_DIAGCROSS        5       /* xxxxx */

	m_ST_Curve.AddLegendHelper(10, _T("第一条曲线"), (unsigned long) RGB(255, 255, 0), PS_SOLID, 1, FALSE);
	m_ST_Curve.AddLegend(11, _T("第二条曲线"),
		(unsigned long) RGB(255, 0, 0), PS_SOLID, 1, (unsigned long) RGB(255, 0, 0), HS_DIAGCROSS, 0, 1, 0xFF, FALSE);
	m_ST_Curve.AddLegendHelper(12, _T("第三条曲线"), (unsigned long) RGB(0, 255, 0), PS_SOLID, 1, FALSE);
	m_ST_Curve.AddLegendHelper(13, _T("第一条曲线"), (unsigned long) RGB(0, 0, 255), PS_SOLID, 1, FALSE);
	m_ST_Curve.AddLegendHelper(14, _T("123"), (unsigned long) RGB(255, 0, 255), PS_SOLID, 1, TRUE);
	m_ST_Curve.AddLegendHelper(15, _T("456"), (unsigned long) RGB(255, 255, 255), PS_SOLID, 1, TRUE);

	m_ST_Curve2.AddLegendHelper(10, _T("第一条曲线"), (unsigned long) RGB(255, 255, 0), PS_SOLID, 1, FALSE);
	m_ST_Curve2.AddLegend(11, _T("第二条曲线"),
		(unsigned long) RGB(255, 0, 0), PS_SOLID, 1, (unsigned long) RGB(255, 0, 0), HS_DIAGCROSS, 0, 1, 0xFF, FALSE);
	m_ST_Curve2.AddLegendHelper(12, _T("第三条曲线"), (unsigned long) RGB(0, 255, 0), PS_SOLID, 1, FALSE);
	m_ST_Curve2.AddLegendHelper(13, _T("第一条曲线"), (unsigned long) RGB(0, 0, 255), PS_SOLID, 1, FALSE);
	m_ST_Curve2.AddLegendHelper(14, _T("123"), (unsigned long) RGB(255, 0, 255), PS_SOLID, 1, TRUE);
	m_ST_Curve2.AddLegendHelper(15, _T("456"), (unsigned long) RGB(255, 255, 255), PS_SOLID, 1, TRUE);
}

void CTestST_CurveDlg::OnButton22() //添加数据
{
	COleDateTime Time = OleTime;

	for (int i = 0; i < 10; i++)
	{
		if (i % 2)
		{
			m_ST_Curve.AddMainData2(11, Time, 90 + .5f * i, i < 5, 0, TRUE);
//			m_ST_Curve.AddMainData2(12, Time, 90 + 2.0f * i, 0, 0, TRUE);
			m_ST_Curve.AddMainData2(13, Time, 95 + .7f * i, 0, 0, TRUE);
			m_ST_Curve2.AddMainData2(13, Time, 95 + .7f * i, 0, 0, TRUE);
		}
		else
		{
			m_ST_Curve.AddMainData2(11, Time, 90 + .6f * i, 1, 0, TRUE);
//			m_ST_Curve.AddMainData2(12, Time, 90 + 2.1f * i, 0, 0, TRUE);
			m_ST_Curve.AddMainData2(13, Time, 95 + .8f * i, 0, 0, TRUE);
			m_ST_Curve2.AddMainData2(13, Time, 95 + .8f * i, 0, 0, TRUE);
		}

		Time += TimeSpan;
	}

//	/*
	Time += 25 * TimeSpan;
	for (int i = 0; i < 10; i++)
	{
//		m_ST_Curve.AddMainData2(11, Time, 90 + 1.0f * i, 0, 0, TRUE);
		m_ST_Curve.AddMainData2(12, Time, 90 + .5f * i, 0, 0, TRUE);
//		m_ST_Curve.AddMainData2(13, Time, 90 + 4.0f * i, 0, 0, TRUE);

		Time += TimeSpan;
	}
//	*/
	m_ST_Curve.FirstPage(FALSE, FALSE);
//	m_ST_Curve2.FirstPage(FALSE, FALSE);
}

void CTestST_CurveDlg::OnTimer(UINT_PTR nIDEvent) 
{
	static float Value1 = -10000.0f;
//	static float Value2 = 80.0f;
	if (100 == nIDEvent)
	{
		Value1 += 5.0f;
		Time1 += TimeSpan;
		m_ST_Curve.AddMainData2(14, (double) Time1, Value1, 0, 1, TRUE);
	}
//	else if (101 == nIDEvent)
//	{
//		Value2 += 11.0f;
//		Time2 += TimeSpan;
//		m_ST_Curve.AddMainData2(13, (double) Time2, Value2, 0, 1, TRUE);
//	}
	else if (1 == nIDEvent)
	{
		m_ST_Curve.DelRange2(0, 0, -1, TRUE, TRUE);
		KillTimer(1);
	}

	CDialogEx::OnTimer(nIDEvent);
}

void CTestST_CurveDlg::OnButton43() 
{
	OnButton17();
	SetTimer(100, 5000, NULL);
//	SetTimer(101, 8000, NULL);
}

void CTestST_CurveDlg::OnButton23() //删除数据
{
	int iv = GetDlgItemInt(IDC_EDIT8);
	m_ST_Curve.DelRange2(iv, 0, -1, FALSE, TRUE);
}

void CTestST_CurveDlg::OnButton24() 
{
	m_ST_Curve.Refresh();
	m_ST_Curve2.Refresh();
	SetTimer(1, 5000, NULL);
}

void CTestST_CurveDlg::OnButton25() 
{
	if (!m_ST_Curve.FirstPage(FALSE, TRUE))
		AfxMessageBox(_T("已是首页！"));
}

void CTestST_CurveDlg::OnButton26() 
{
	if (!m_ST_Curve.GotoPage(-1, TRUE))
		AfxMessageBox(_T("已是首页！"));
}

void CTestST_CurveDlg::OnButton27() 
{
	if (!m_ST_Curve.GotoPage(1, TRUE))
		AfxMessageBox(_T("已是末页！"));
//	m_ST_Curve.GotoPage(1, FALSE);
}

void CTestST_CurveDlg::OnButton28() 
{
	if (!m_ST_Curve.FirstPage(TRUE, TRUE))
		AfxMessageBox(_T("已是末页！"));
}

void CTestST_CurveDlg::OnButton33() 
{
	CString str;
	str.Format(L"%d, %d", m_ST_Curve.GetBuddyCount(), m_ST_Curve2.GetBuddyCount());
	AfxMessageBox(str);
//	int iv = GetDlgItemInt(IDC_EDIT9);
//	m_ST_Curve.SetZoom(iv);
}

void CTestST_CurveDlg::OnButton34() 
{
	SetDlgItemInt(IDC_EDIT9, m_ST_Curve.GetZoom());
}

void CTestST_CurveDlg::OnButton37() 
{
	int iv = GetDlgItemInt(IDC_EDIT11);
	if (!m_ST_Curve.SetVPrecision(iv))
		AfxMessageBox(_T("精度错误！"));
}

void CTestST_CurveDlg::OnButton38() 
{
	SetDlgItemInt(IDC_EDIT11, m_ST_Curve.GetVPrecision());
}

void CTestST_CurveDlg::OnButton39() 
{
	int iv = GetDlgItemInt(IDC_EDIT12);
	if (!m_ST_Curve.AddLegend(0, _T("第一条曲线"), 0, 0, iv, 0, 0, 0, 0, 8, TRUE))
		AfxMessageBox(_T("笔宽错误！"));
}

void CTestST_CurveDlg::OnButton40() 
{
	short iv;
	if (m_ST_Curve.GetLegend(_T("第一条曲线"), 0, 0, &iv, 0, 0, 0, 0))
		SetDlgItemInt(IDC_EDIT12, iv);
	else
		AfxMessageBox(_T("没有找到相应的图例"));
}

void CTestST_CurveDlg::OnButton41() 
{
	/*
	double MinTime, MaxTime;
	if (!m_ST_Curve.GetOneTimeRange(11, &MinTime, &MaxTime))
		return;
	AfxMessageBox(COleDateTime(MinTime).Format(_T("%Y-%m-%d %H:%M:%S")));
	AfxMessageBox(COleDateTime(MaxTime).Format(_T("%Y-%m-%d %H:%M:%S")));
	*/
	/*
	float MinValue, MaxValue;
	if (!m_ST_Curve.GetOneValueRange(11, &MinValue, &MaxValue))
		return;
	CString str;
	str.Format("%f, %f", MinValue, MaxValue);
	AfxMessageBox(str);
	*/
	/*
	double Time;
	float Value;
	if (!m_ST_Curve.GetOneFirstPos(11, &Time, &Value, TRUE))
		return;
	CString str;
	str.Format("%s, %f", COleDateTime(Time).Format("%Y-%m-%d %H:%M:%S"), Value);
	AfxMessageBox(str);
	return;
	*/
	/*
	CPrintDialog dlg(FALSE, PD_RETURNDC);
	dlg.GetDefaults();
	DeleteObject(dlg.m_pd.hDC);
	::GlobalFree(dlg.m_pd.hDevMode);
	::GlobalFree(dlg.m_pd.hDevNames);
	*/
	/*
	CPrintDialog dlg(TRUE, PD_RETURNDC);
	dlg.m_pd.hDevMode = ::GlobalAlloc(GHND, sizeof(DEVMODE));
	DEVMODE FAR* pDevMode = (DEVMODE FAR*)::GlobalLock(dlg.m_pd.hDevMode);
	pDevMode->dmSize = sizeof(DEVMODE);
	pDevMode->dmOrientation = DMORIENT_LANDSCAPE; //默认横向打印DMORIENT_PORTRAIT
	pDevMode->dmPaperSize = DMPAPER_A4; //默认打印A4纸
	pDevMode->dmFields = DM_ORIENTATION | DM_PAPERSIZE;
	::GlobalUnlock(dlg.m_pd.hDevMode);
	dlg.DoModal();
	if (dlg.m_pd.hDC)
		DeleteObject(dlg.m_pd.hDC);
	::GlobalFree(dlg.m_pd.hDevMode);
	::GlobalFree(dlg.m_pd.hDevNames);
	*/

	COleDateTime OleTime = COleDateTime::GetCurrentTime();
	CString Title, FootNote;
	Title.Format(_T("某小区%d月份用电曲线图"), OleTime.GetMonth());
	FootNote.Format(_T("打印时间：%s  操作员：杨狼"), OleTime.Format(VAR_DATEVALUEONLY));

//	double BTime, ETime;
//	m_ST_Curve.GetTimeRange(&BTime, &ETime);
//	m_ST_Curve.GetOneTimeRange(12, &BTime, &ETime);
	m_ST_Curve.PrintCurve(12, //最后一个参数代表打印所有曲线，所以这里的12（曲线地址）将被忽略
						  .0, .0, 0,
						  50, 50, 50, 50,
						  Title, FootNote,
					      0x11, //Flag
//						  FALSE //one curve
						  TRUE  //all curve
						  );
}

void CTestST_CurveDlg::OnButton35() //按时间截左
{
	COleDateTime Time = OleTime;
	Time += 5 * TimeSpan;
//	Time -= 5 * TimeSpan; //失败的截取

	m_ST_Curve.DelRange(11, .0, Time, 2, FALSE, TRUE);
}

void CTestST_CurveDlg::OnButton36() //按数量截左
{
	m_ST_Curve.DelRange2(13, 0, 6, FALSE, TRUE);
//	m_ST_Curve.DelRange2(13, 0, 0, FALSE, TRUE); //失败的截取
}

void CTestST_CurveDlg::OnButton45() //按时间截右
{
	COleDateTime Time = OleTime;
	Time += 5 * TimeSpan;
//	Time += 20 * TimeSpan; //失败的截取

	m_ST_Curve.DelRange(11, Time, .0, 1, FALSE, TRUE);
}

void CTestST_CurveDlg::OnButton50() //按数量截右
{
	m_ST_Curve.DelRange2(11, m_ST_Curve.GetCurveLength(11) - 5, -1, FALSE, TRUE);
//	m_ST_Curve.DelRange2(11, m_ST_Curve.GetCurveLength(11), -1, FALSE, TRUE); //失败的截取
}

void CTestST_CurveDlg::OnButton44() //按时间截中
{
	COleDateTime Time1 = OleTime, Time2 = OleTime;
	Time1 += 5 * TimeSpan;
	Time2 += 7 * TimeSpan;

	m_ST_Curve.DelRange(11, Time1, Time2, 3, FALSE, TRUE);
//	m_ST_Curve.DelRange(11, &dt2, &dt1, FALSE, TRUE); //失败的截取
}

void CTestST_CurveDlg::OnButton51() //按数量截中
{
	m_ST_Curve.DelRange2(11, 3, 5, FALSE, TRUE);
//	m_ST_Curve.DelRange2(11, m_ST_Curve.GetDotNum(11), 5, FALSE, TRUE); //失败的截取
//	m_ST_Curve.DelRange2(11, 5, -1, FALSE, TRUE); //相当于保留左边4个数据，其它的用法可以发挥想像
}

void CTestST_CurveDlg::OnButton55() //按时间截两头
{
	COleDateTime Time1 = OleTime, Time2 = OleTime;
	Time1 += 3 * TimeSpan;
	Time2 += 8 * TimeSpan;

	m_ST_Curve.DelRange(11, .0, Time1, 2, FALSE, TRUE);
	m_ST_Curve.DelRange(11, Time2, .0, 1, FALSE, TRUE);
}

void CTestST_CurveDlg::OnButton52() //按数量截两头
{
	m_ST_Curve.DelRange2(11, 0, 2, FALSE, TRUE);
	//注意，第二次调用的时候，第二个参数，即序号，指的是第一次调用后的曲线中的序号
	m_ST_Curve.DelRange2(11, 6, -1, FALSE, TRUE);
}

void CTestST_CurveDlg::OnButton53() 
{
	m_ST_Curve.TrimCoor();
}

void CTestST_CurveDlg::OnButton29() //获取横标刻度间隔
{
	SetDlgItemInt(IDC_EDIT13, m_ST_Curve.GetScaleInterval() >> 8);
}

void CTestST_CurveDlg::OnButton30() 
{
	int iv = GetDlgItemInt(IDC_EDIT13);
	m_ST_Curve.SetHInterval(iv);
}

void CTestST_CurveDlg::OnButton31()//获取纵标刻度间隔
{
	SetDlgItemInt(IDC_EDIT14, m_ST_Curve.GetScaleInterval() & 0xFF);
}

void CTestST_CurveDlg::OnButton32() 
{
	int iv = GetDlgItemInt(IDC_EDIT14);
	m_ST_Curve.SetVInterval(iv);
}

void CTestST_CurveDlg::OnButton20() 
{
	m_ST_Curve.SetFont(SplitHandle(m_ST_Curve, 0));
	/*
	CFontDialog dlg;
	if (IDOK == dlg.DoModal())
	{
		LOGFONT l;
		dlg.GetCurrentFont(&l);

		m_ST_Curve.SetFont(SplitHandle(m_ST_Curve, ::CreateFontIndirect(&l)));
//		CFont font;
//		font.CreateFontIndirect(&l);
//		m_ST_Curve.SetFont(SplitHandle(m_ST_Curve, font.Detach()));
	}
	*/
}

void CTestST_CurveDlg::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	static int n = 1;
	if (n)
		m_ST_Curve.MoveWindow(11, 11, 991, 361);
	else
		m_ST_Curve.MoveWindow(11, 11, 561, 301);
	n = !n;

	CDialogEx::OnLButtonDblClk(nFlags, point);
}

void CTestST_CurveDlg::OnButton54() 
{
	int iv = GetDlgItemInt(IDC_EDIT7);
	m_ST_Curve.SetLegendSpace(iv);
}

void CTestST_CurveDlg::OnButton21() 
{
	SetDlgItemInt(IDC_EDIT7, m_ST_Curve.GetLegendSpace());
}

void CTestST_CurveDlg::OnButton42() 
{
	m_ST_Curve.ExportImage(0);
}

void CTestST_CurveDlg::OnButton77() 
{
	m_ST_Curve.BatchExportImage(0, 1);
}

void CTestST_CurveDlg::OnButton78() 
{
	m_ST_Curve.ExportImageFromPage(0, 13, 1, 100, FALSE, 2); //ansi
}

void CTestST_CurveDlg::OnButton56() 
{
	m_ST_Curve.SetShowMode(GetDlgItemInt(IDC_EDIT15));
	m_ST_Curve2.SetShowMode(GetDlgItemInt(IDC_EDIT15));
}

void CTestST_CurveDlg::OnButton57() 
{
	SetDlgItemInt(IDC_EDIT15, m_ST_Curve.GetShowMode());
}

void CTestST_CurveDlg::OnButton58() 
{
	m_ST_Curve.AddLegend(0, _T("第一条曲线"), 0, 0, 0, 0, 0, GetDlgItemInt(IDC_EDIT16), 0, 0x40, TRUE);
}

void CTestST_CurveDlg::OnButton59() 
{
	short vi;
	m_ST_Curve.GetLegend(_T("第一条曲线"), 0, 0, 0, 0, 0, &vi, 0);
	SetDlgItemInt(IDC_EDIT16, vi);
}

void CTestST_CurveDlg::OnButton60() 
{
}

void CTestST_CurveDlg::OnButton61() 
{
}

void CTestST_CurveDlg::OnButton62() 
{
	m_ST_Curve.SetMoveMode(GetDlgItemInt(IDC_EDIT18));
}

void CTestST_CurveDlg::OnButton63() 
{
	SetDlgItemInt(IDC_EDIT18, m_ST_Curve.GetMoveMode());
}

void CTestST_CurveDlg::OnButton64() //添加背景位图
{
	static UINT nBitmap = IDB_1;
	if (nBitmap > IDB_10)
		nBitmap = IDB_1;
	m_ST_Curve.SetBkBitmap(nBitmap - IDB_1);

	nBitmap++;
}

void CTestST_CurveDlg::OnButton65() //取消背景位图
{
	m_ST_Curve.SetBkBitmap(-1);
}

void CTestST_CurveDlg::OnButton66() //设置位图显示模式
{
	m_ST_Curve.SetBkMode(GetDlgItemInt(IDC_EDIT19));
}

void CTestST_CurveDlg::OnButton67() //获取位图显示模式
{
	SetDlgItemInt(IDC_EDIT19, m_ST_Curve.GetBkMode());
}

void CTestST_CurveDlg::OnButton68() //选中曲线
{
	m_ST_Curve.SelectCurve(GetDlgItemInt(IDC_EDIT20), 1);
}

void CTestST_CurveDlg::OnButton71() //取消选中
{
	m_ST_Curve.SelectCurve(GetDlgItemInt(IDC_EDIT20), 0);
}

void CTestST_CurveDlg::OnButton69() //显示图例
{
	CString str;
	GetDlgItemText(IDC_EDIT21, str);
	m_ST_Curve.ShowLegend(str, 1);
}

void CTestST_CurveDlg::OnButton70() //隐藏图例
{
	CString str;
	GetDlgItemText(IDC_EDIT21, str);
	m_ST_Curve.ShowLegend(str, 0);
}

void CTestST_CurveDlg::OnButton72() //显示节点
{
	m_ST_Curve.AddLegend(0, _T("第一条曲线"), 0, 0, 0, 0, 0, 0, 1, 0x80, TRUE);
}

void CTestST_CurveDlg::OnButton73() //隐藏节点
{
	m_ST_Curve.AddLegend(0, _T("第一条曲线"), 0, 0, 0, 0, 0, 0, 0, 0x80, TRUE);
}

void CTestST_CurveDlg::OnButton74()
{
	m_ST_Curve.TrimCurve(GetDlgItemInt(IDC_EDIT22), GetDlgItemInt(IDC_EDIT23),
		GetDlgItemInt(IDC_EDIT24), GetDlgItemInt(IDC_EDIT25), GetDlgItemInt(IDC_EDIT26), FALSE);
}

void CTestST_CurveDlg::OnButton75() //删除图例
{
	CString str;
	GetDlgItemText(IDC_EDIT27, str);
	m_ST_Curve.DelLegend2(str, TRUE);
}

void CTestST_CurveDlg::OnButton76() //获取图例
{
	CString str;
	GetDlgItemText(IDC_EDIT27, str);
	COLORREF Color;
	if (m_ST_Curve.GetLegend(str, (long*) &Color, 0, 0, 0, 0, 0, 0))
	{
		str.Format(_T("%08X"), Color);
		AfxMessageBox(str);
	}
	else
		AfxMessageBox(_T("未找到指定曲线！"));
}

void CTestST_CurveDlg::OnButton79() 
{
	m_ST_Curve.GotoCurve(12);
}
