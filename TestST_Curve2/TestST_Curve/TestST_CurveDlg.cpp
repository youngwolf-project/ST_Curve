
// TestST_CurveDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "TestST_Curve.h"
#include "TestST_CurveDlg.h"
#include "afxdialogex.h"

#include <locale.h>
#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifdef _DEBUG
#define ShowEvent(str) _tprintf(str _T("\r\n"))
#else
#define ShowEvent(str)
#endif

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
	DDX_Control(pDX, IDC_COMBO1, m_Combo);
	DDX_Control(pDX, IDC_STCURVECTRL, m_ST_Curve);
}

BEGIN_MESSAGE_MAP(CTestST_CurveDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_CBN_SELCHANGE(IDC_COMBO1, OnSelchangeCombo1)
	ON_WM_TIMER()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_DESTROY()
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
	ON_BN_CLICKED(IDC_BUTTON80, OnButton80)
	ON_BN_CLICKED(IDC_BUTTON81, OnButton81)
	ON_BN_CLICKED(IDC_BUTTON82, OnButton82)
	ON_BN_CLICKED(IDC_BUTTON83, OnButton83)
	ON_BN_CLICKED(IDC_BUTTON84, OnButton84)
	ON_BN_CLICKED(IDC_BUTTON85, OnButton85)
	ON_BN_CLICKED(IDC_BUTTON86, OnButton86)
	ON_BN_CLICKED(IDC_BUTTON87, OnButton87)
	ON_BN_CLICKED(IDC_BUTTON88, OnButton88)
	ON_BN_CLICKED(IDC_BUTTON89, OnButton89)
	ON_BN_CLICKED(IDC_BUTTON90, OnButton90)
	ON_BN_CLICKED(IDC_BUTTON91, OnButton91)
	ON_BN_CLICKED(IDC_BUTTON92, OnButton92)
END_MESSAGE_MAP()


// CTestST_CurveDlg 消息处理程序
/*
LRESULT CTestST_CurveDlg::OnPageChange(WPARAM wParam, LPARAM lParam)
{
	CString str;
	str.Format(_T("%u:%u"), (ULONG) wParam, (ULONG) wParam);
	SetDlgItemText(IDC_EDIT10, str);
	return 0;
}
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
//	uMSH_MOUSEWHEEL = RegisterWindowMessage(MSH_MOUSEWHEEL);

	// TODO: Add extra initialization here
	OleTime.ParseDateTime(_T("2007-5-8 0:0:0"));
//	OleTime.ParseDateTime(_T("9999-12-30 3:0:0"));
	Time2 = Time1 = OleTime; //COleDateTime::GetCurrentTime();
	TimeSpan = 1.0 / 24; //一小时

//	m_ST_Curve.MoveWindow(0, 0, 1000, 380);
//	m_ST_Curve.MoveWindow(0, 0, 980, 380);
	m_ST_Curve.SetMaxLength(1000, 700);
//	m_ST_Curve.SetValueStep(1.0f);
//	m_ST_Curve.SetMSGRecWnd(SplitHandle(m_hWnd));
//	m_ST_Curve.SetPageChangeMSG(PAGECHANGE);

//	m_ST_Curve.SetBackColor(RGB(100, 100, 100));
//	m_ST_Curve.SetForeColor(RGB(0, 0, 0));
//	m_ST_Curve.EnableZoom(FALSE);
//	m_ST_Curve.EnableToolTip(FALSE);

	m_ST_Curve.SetFootNote(_T("https://github.com/youngwolf-project/ST_Curve"));
	m_ST_Curve.SetHUnit(_T("MHz")); //由于横坐标显示为时间，所以这个单位显示不出来，除非将横坐标改为显示值

	m_ST_Curve.SetCurveTitle(_T("ST_Curve工作室"));
//	m_ST_Curve.SetTitleColor(RGB(0, 0, 255)); //标题色
//	m_ST_Curve.SetFootNoteColor(RGB(0, 255, 0)); //脚注色
	m_ST_Curve.SetWaterMark(_T("ST_Curve"));

/*	//通过句柄添加位图（HBITMAP）
	CBitmap b;
	b.LoadBitmap(nBitmap);
	m_ST_Curve.AddBitmapHandle(SplitHandle(m_ST_Curve, b.Detach()), FALSE);
*/
	HINSTANCE h = AfxGetResourceHandle();
	m_ST_Curve.SetRegister1(GetH32bit(h));
	for (int i = IDB_1; i <= IDB_12; ++i) //这个循环里面都要使用同一个高32位，所以前面统一做了
		m_ST_Curve.AddBitmapHandle3((long) h, i, FALSE);
	m_ST_Curve.AddImageHandle(_T("xiao.png"), FALSE);

	m_ST_Curve.SetBeginTime(_T("2007-5-7 16:00:00"));
	m_ST_Curve.SetTimeSpan(1.0 / 24 / 4); //15分钟
	m_ST_Curve.SetBeginValue(81.0f);

//	COleDateTimeSpan span;
//	span.SetDateTimeSpan(0, 8, 0, 0);
//	m_ST_Curve.SetVisibleCoorRange(OleTime, OleTime + span, .0f, .0f, 3);
//	m_ST_Curve.SetVisibleCoorRange(.0, .0, 90.0f, 98.0f, 0xc);

	m_ST_Curve.SetSorptionRange(8);
//	m_ST_Curve.EnableAdjustZOrder(FALSE);
	m_ST_Curve.EnableHZoom(TRUE);
//	m_ST_Curve.SetMoveMode(0x87);

	//修整坐标和自定义实施缩放，由插件来实现
//	m_ST_Curve.LoadLuaScript(_T("ST_Curve_PlugIn.lua"), 1, 0x3C);

#ifdef _DEBUG
	m_ST_Curve.SetEventMask(0x1FFF); //开启所有事件

	AllocConsole();
	FILE* stream;
	_tfreopen_s(&stream, _T("CONOUT$"), _T("w"), stdout);
#ifdef _UNICODE
	_tsetlocale(LC_ALL, _T("chs")); //unicode下输出中文需要它，ansi版本不需要
#endif
#endif

	m_Combo.SetCurSel(0);
	OnSelchangeCombo1();

	//快捷键，按位算，从低位起，为1表示开启（有些快捷键只能按组开启或者禁止）
	//缩放键（-+）虽然也是快捷键，但通过EnableZoom开启与禁止
	//1 -F4
	//2 -F5
	//3 -F6
	//4 -F7
	//5 -home/page up/page down/end
	//6 -上下方向键
	//7 -左右方向键
	//8 -数字1键
	//9 -数字2键
	//...
	//16-数字9键
//	m_ST_Curve.SetShortcutKeyMask(0x80); //只开启了数字1键
	m_ST_Curve.SetGridMode(m_ST_Curve.GetGridMode() + 8);

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

//static short HGraduationSize = 21;
//static short VGraduationSize = 21;
void CTestST_CurveDlg::OnButton1() //背景色
{
//	HGraduationSize += 5;
//	m_ST_Curve.SetGraduationSize((HGraduationSize << 16) + VGraduationSize);
//	return;

	m_ST_Curve.SetBackColor(0x80000000 | m_ST_Curve.GetBackColor());
//	m_ST_Curve.ExportMetaFile(0, 11, 1, 5, FALSE, 2);
//	m_ST_Curve.ExportImageFromPage(_T("c:\\***.bmp"), 0, 1, 100, TRUE, 1);
//	m_ST_Curve.EnableSelectCurve(FALSE);
//	m_ST_Curve.SetToolTipDelay(1000);
}

void CTestST_CurveDlg::OnButton4() 
{
//	HGraduationSize -= 5;
//	m_ST_Curve.SetGraduationSize((HGraduationSize << 16) + VGraduationSize);
//	return;

	CString str;
	str.Format(_T("%08X"), m_ST_Curve.GetBackColor());
	AfxMessageBox(str);
//	m_ST_Curve.SelectCurve(13, TRUE);
}

void CTestST_CurveDlg::OnButton2() //文字色
{
//	VGraduationSize += 5;
//	m_ST_Curve.SetGraduationSize((HGraduationSize << 16) + VGraduationSize);
//	return;

	m_ST_Curve.SetForeColor(0x80000000 | m_ST_Curve.GetForeColor());
//	CColorDialog dlg;
//	if (IDOK == dlg.DoModal())
//		m_ST_Curve.SetForeColor(dlg.GetColor());
}

void CTestST_CurveDlg::OnButton5() 
{
//	VGraduationSize -= 5;
//	m_ST_Curve.SetGraduationSize((HGraduationSize << 16) + VGraduationSize);
//	return;

	CString str;
	str.Format(_T("%08X"), m_ST_Curve.GetForeColor());
	AfxMessageBox(str);
}

void CTestST_CurveDlg::OnButton3() //坐标轴色
{
	m_ST_Curve.SetAxisColor(0x80000000 | m_ST_Curve.GetAxisColor());
//	CColorDialog dlg;
//	if (IDOK == dlg.DoModal())
//		m_ST_Curve.SetAxisColor(dlg.GetColor());
}

void CTestST_CurveDlg::OnButton6() 
{
	CString str;
	str.Format(_T("%08X"), m_ST_Curve.GetAxisColor());
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
	const char* pstr = T2A((LPTSTR)(LPCTSTR) str);
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
	const char* pstr = T2A((LPTSTR)(LPCTSTR) str);
	m_ST_Curve.SetBeginValue((float) atof(pstr));
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
	const char* pstr = T2A((LPTSTR)(LPCTSTR) str);
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
	if (!m_ST_Curve.DelLegend(iv, FALSE, TRUE))
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
}

void CTestST_CurveDlg::OnTimer(UINT_PTR nIDEvent) 
{
	static float Value1 = -10000.0f;
	static float Value2 = 77.0f;
	if (100 == nIDEvent)
	{
		Value1 -= 5.0f;
		Time1 -= TimeSpan / 4;
		m_ST_Curve.AddMainData2(14, (double) Time1, Value1, 0, 1/*0x19*//*0x21*/, FALSE);
//		if (2 == m_ST_Curve.AddMainData2(14, (double) Time1, Value1, 0, 1/*0x19*/, FALSE))
//			m_ST_Curve.SetZOffset(14, 41, TRUE);
	}
	else if (101 == nIDEvent)
	{
		Value2 += 5.0f; //10.0f;
		Time2 += TimeSpan / 4;
		m_ST_Curve.AddMainData2(13, (double) Time2, Value2, 0, 1/*0x19*//*7*/, TRUE);
//		if (2 == m_ST_Curve.AddMainData2(13, (double) Time2, Value2, 0, 1/*0x19*//*7*/, TRUE))
//			m_ST_Curve.SetZOffset(13, 20, TRUE);
	}
	else if (1 == nIDEvent)
	{
		m_ST_Curve.DelRange2(0, 0, -1, TRUE, TRUE);
		KillTimer(1);
	}

	CDialogEx::OnTimer(nIDEvent);
}

void CTestST_CurveDlg::OnButton23() //删除数据
{
	int iv = GetDlgItemInt(IDC_EDIT8);
	m_ST_Curve.DelRange2(iv, 0, -1, FALSE, TRUE);
}

void CTestST_CurveDlg::OnButton24() 
{
	m_ST_Curve.Refresh();
//	SetTimer(1, 5000, NULL);
}

void CTestST_CurveDlg::OnButton25() 
{
	if (!m_ST_Curve.FirstPage(FALSE, TRUE))
		AfxMessageBox(_T("已是首页！"));

//	m_ST_Curve.AddLegend(11, _T("第三条曲线"), 0, 0, 0, 0, 0, 0, 0, 1, TRUE);
//	m_ST_Curve.MoveCurveToLegend(11, _T("第三条曲线"));
	//上面两行代码，效果完全一样，可以看出MoveCurveToLegend非常简练

//	m_ST_Curve.ChangeLegendName(_T("第一条曲线"), _T("第十条曲线"));
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
//	m_ST_Curve.ExportBMP("c:\\123.bmp");
}

void CTestST_CurveDlg::OnButton28() 
{
	if (!m_ST_Curve.FirstPage(TRUE, TRUE))
		AfxMessageBox(_T("已是末页！"));
}

void CTestST_CurveDlg::OnButton33() 
{
	int iv = GetDlgItemInt(IDC_EDIT9);
	m_ST_Curve.SetZoom(iv);
}

void CTestST_CurveDlg::OnButton34() 
{
	SetDlgItemInt(IDC_EDIT9, m_ST_Curve.GetZoom());
}

void CTestST_CurveDlg::OnButton37() 
{
	int iv = GetDlgItemInt(IDC_EDIT11);
	m_ST_Curve.SetHPrecision(iv);
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
//	m_ST_Curve.SetCurveIndex(11, 2);

// 	DATE Time = .0;
// 	float Value = .0f;
// 	m_ST_Curve.GetBenchmark(&Time, &Value);
// 	Time -= 1.0;
// 	Value += 1.0f;
// 	m_ST_Curve.SetBenchmark(Time, Value);
//	return;

// 	m_ST_Curve.SetFixedZoomMode('+');
// 	return;

// 	DATE Time;
// 	float Value;
// 	m_ST_Curve.GetOneFirstPos(13, &Time, &Value, FALSE);
// 	long x, y;
// 	m_ST_Curve.GetPixelPoint(Time, Value, &x, &y);
// 	m_ST_Curve.FixedZoom('-', (short) x, (short) y, FALSE); //要保证xy坐标在画面里面，否则无法定点缩放
// 	return;

// 	m_ST_Curve.SetCommentPosition(0);
// 	return;

	COleDateTime OleTime = COleDateTime::GetCurrentTime();
	CString Title, FootNote;
	Title.Format(_T("某小区%d月份用电曲线图"), OleTime.GetMonth());
	FootNote.Format(_T("打印时间：%s  操作员：杨狼"), OleTime.Format(VAR_DATEVALUEONLY));

	static int nPrintTime = 0;
	m_ST_Curve.PrintCurve(12, //最后一个参数代表指定打印所有曲线，所以这里的12（曲线地址）将被忽略
						  .0, .0, 0, //BTime和ETime均无效，则打印最大时间范围
						  50, 50, 50, 50,
						  Title, FootNote,
//						  0, FootNote,
						  (nPrintTime & 1) ? 0x51 : 0x11, //Flag
//						  FALSE //one curve
						  TRUE  //all curve
						  );
	++nPrintTime; //一次正常打印，一次位图打印
}

void CTestST_CurveDlg::OnButton35() //按时间截左
{
	COleDateTime Time = OleTime;
	Time += 5 * TimeSpan;
//	Time -= 5 * TimeSpan; //失败的截取

	m_ST_Curve.DelRange(11, .0, Time, 2, FALSE, TRUE); //只有ETime有效
}

void CTestST_CurveDlg::OnButton36() //按数量截左
{
	/*
	CString str;
	double d;
	double f;

	m_ST_Curve.GetOneTimeRange(13, &d, &f);
	str.Format(L"%f, %f", d, f);
	AfxMessageBox(str);
	*/
	m_ST_Curve.DelRange2(11, 0, 6, FALSE, TRUE);
//	m_ST_Curve.DelRange2(13, 0, 0, FALSE, TRUE); //失败的截取
}

void CTestST_CurveDlg::OnButton45() //按时间截右
{
	COleDateTime Time = OleTime;
	Time += 5 * TimeSpan;
//	Time += 20 * TimeSpan; //失败的截取

	m_ST_Curve.DelRange(11, Time, .0, 1, FALSE, TRUE); //只有BTime有效
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

	double dt1 = Time1, dt2 = Time2;
	m_ST_Curve.DelRange(11, dt1, dt2, 3, FALSE, TRUE); //BTime和ETime均有效
//	m_ST_Curve.DelRange(11, dt2, dt1, 3, FALSE, TRUE); //失败的截取
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

	m_ST_Curve.DelRange(11, .0, Time1, 2, FALSE, TRUE); //只有ETime有效
	m_ST_Curve.DelRange(11, Time2, .0, 1, FALSE, TRUE); //只有BTime有效
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
	/*
	short nCount = m_ST_Curve.GetCurveCount();
	for (short i = 0; i < nCount; i++)
	{
		CString str;
		str.Format(L"%d", m_ST_Curve.GetCurve(i));
		AfxMessageBox(str);
	}
	*/
	/*
	short nCount = m_ST_Curve.GetLegendCount();
	for (short i = 0; i < nCount; i++)
		AfxMessageBox(m_ST_Curve.GetLegend(i));
	*/
	/*
	short nLegendCount = m_ST_Curve.GetLegendCount();
	for (short i = 0; i < nLegendCount; i++)
	{
		short nLegendAddressCount = m_ST_Curve.GetLegendAddressCount(i);
		for (short j = 0; j < nLegendAddressCount; j++)
		{
			CString str;
			str.Format(L"%d", m_ST_Curve.GetLegendAddress(i, j));
			AfxMessageBox(str);
		}
	}
	*/

	m_ST_Curve.SetFont(SplitHandle(m_ST_Curve, 0));
	/*
	CFontDialog dlg;
	if (IDOK == dlg.DoModal())
	{
		LOGFONT l;
		dlg.GetCurrentFont(&l);

		m_ST_Curve.SetHFont((long) ::CreateFontIndirect(&l));
//		CFont font;
//		font.CreateFontIndirect(&l);
//		m_ST_Curve.SetHFont((long) font.Detach());
	}
	*/
}

void CTestST_CurveDlg::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	static int n = 1;
	if (n)
		m_ST_Curve.MoveWindow(11, 11, 991, 361);
	else
		m_ST_Curve.MoveWindow(50, 50, 560, 301);
	n = !n;

	CDialogEx::OnLButtonDblClk(nFlags, point);
}

void CTestST_CurveDlg::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
#ifdef _DEBUG
	_tsystem(_T("cls\r\n"));
#endif

	CDialogEx::OnRButtonDblClk(nFlags, point);
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

void CTestST_CurveDlg::OnButton42() //导出曲线到图片或者到文件
{
	m_ST_Curve.ExportImage(0); //图片
}

void CTestST_CurveDlg::OnButton84() 
{
//	/*
	m_ST_Curve.ExportImageFromPage(0, 11, 1, -1, TRUE, 2); //ansi txt
	m_ST_Curve.ExportImageFromPage(0, 11, 1, -1, TRUE, 3); //unicode txt
	m_ST_Curve.ExportImageFromPage(0, 11, 1, -1, TRUE, 4); //unicode big endian txt
	m_ST_Curve.ExportImageFromPage(0, 11, 1, -1, TRUE, 5); //utf8 txt
	m_ST_Curve.ExportImageFromPage(0, 11, 1, -1, TRUE, 6); //binary
//	*/
}

void CTestST_CurveDlg::OnButton83() //导入曲线
{
	m_ST_Curve.DelRange2(0, 0, -1, TRUE, TRUE);

	//对于ImportFile函数，如果pFileName为空，则Style参数被忽略，导入文件类型将根据用户的选择
	//确定是文本文件还是二进制文件，对于文本文件，又会根据文件头来确定文本文件的具体格式
	long re = m_ST_Curve.ImportFile(0, 0, TRUE);

	m_ST_Curve.FirstPage(FALSE, FALSE);
}

void CTestST_CurveDlg::OnButton77() 
{
	m_ST_Curve.BatchExportImage(_T("d:\\x*"), 10);
}

void CTestST_CurveDlg::OnButton78() 
{
	m_ST_Curve.ExportImageFromPage(_T("c:\\x**y.jpg"), 13, 1, 1, FALSE, 1);
//	COleDateTime BTime, ETime;
//	BTime.ParseDateTime(_T("2007-5-8"));
//	ETime.ParseDateTime(_T("2007-5-8 10:30:00"));
//	m_ST_Curve.ExportImageFromTime(_T("c:\\x**y.jpg"), 13, BTime, ETime, 3, FALSE, 1);
}

void CTestST_CurveDlg::OnButton56() 
{
	m_ST_Curve.SetShowMode(GetDlgItemInt(IDC_EDIT15));
}

void CTestST_CurveDlg::OnButton57() 
{
	SetDlgItemInt(IDC_EDIT15, m_ST_Curve.GetShowMode());
//	m_ST_Curve.SetBottomSpace(GetDlgItemInt(IDC_EDIT15));
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
	m_ST_Curve.SetFillDirection(11, GetDlgItemInt(IDC_EDIT17), TRUE);
}

void CTestST_CurveDlg::OnButton61() 
{
	SetDlgItemInt(IDC_EDIT17, m_ST_Curve.GetFillDirection(11));
}

void CTestST_CurveDlg::OnButton62() 
{
	m_ST_Curve.SetMoveMode(GetDlgItemInt(IDC_EDIT18));
}

void CTestST_CurveDlg::OnButton63() 
{
	SetDlgItemInt(IDC_EDIT18, m_ST_Curve.GetMoveMode());
}

static short nBitmap = IDB_1;
void CTestST_CurveDlg::OnButton64() //添加背景位图
{
	if (nBitmap > IDB_10)
		nBitmap = IDB_1;
	m_ST_Curve.SetBkBitmap(nBitmap - IDB_1);
	m_ST_Curve.SetBkMode(0x80);
	m_ST_Curve.SetCanvasBkBitmap(nBitmap - IDB_1);
	m_ST_Curve.SetCanvasBkMode(1);

	nBitmap++;
}

void CTestST_CurveDlg::OnButton65() //取消背景位图
{
//	m_ST_Curve.SetBkBitmap(-1);
	m_ST_Curve.SetCanvasBkBitmap(-1);
}

void CTestST_CurveDlg::OnButton66() //设置位图显示模式
{
	m_ST_Curve.SetCanvasBkMode(GetDlgItemInt(IDC_EDIT19));
}

void CTestST_CurveDlg::OnButton67() //获取位图显示模式
{
	SetDlgItemInt(IDC_EDIT19, m_ST_Curve.GetBkMode());
}

void CTestST_CurveDlg::OnButton68() //选中曲线
{
	m_ST_Curve.SelectCurve(GetDlgItemInt(IDC_EDIT20), TRUE);
}

void CTestST_CurveDlg::OnButton71() //取消选中
{
	m_ST_Curve.SelectCurve(GetDlgItemInt(IDC_EDIT20), FALSE);
}

void CTestST_CurveDlg::OnButton69() //显示图例
{
	CString str;
	GetDlgItemText(IDC_EDIT21, str);
	m_ST_Curve.ShowLegend(str, TRUE);
}

void CTestST_CurveDlg::OnButton70() //隐藏图例
{
	CString str;
	GetDlgItemText(IDC_EDIT21, str);
	m_ST_Curve.ShowLegend(str, FALSE);
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
//	CString s;
//	s.Format(_T("%d"), m_ST_Curve.QueryLegend4(str));
//	AfxMessageBox(s);
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
}

void CTestST_CurveDlg::OnButton79() 
{
	m_ST_Curve.GotoCurve(12);
}

void CTestST_CurveDlg::OnButton81() 
{
	short nCurveIndex = m_ST_Curve.GetCurveIndex(11);
	if (nCurveIndex >= 0)
	{
//		m_ST_Curve.DelRange2(11, 5, -1, FALSE, TRUE);

		long CurveLen = m_ST_Curve.GetCurveLength(11);
		for (int i = 0; i < CurveLen; ++i)
		{
			//如果你确定在枚举过程中，曲线不会有任何修改，比如层次位置改变（参看SetCurveIndex），增加删除点等
			//你完全可以不做CanContinueEnum判断，注意第一个参数为正在枚举的曲线的地址
			//由于我们正在枚举11曲线，所以也必须用11调用CanContinueEnum
			if (!m_ST_Curve.CanContinueEnum(11, nCurveIndex, i))
				break; //无法再继续枚举了
			//不能继续枚举的原因有：
			//一：曲线层次发生了变化，此时再次调用GetCurveIndex得到新的nCurveIndex即可接着枚举
			//二：曲线有点被删除并且i已经到达或者超过曲线尾部，此时枚举应该结束
			//注：如果有点被删除，但i仍然在正常的范围之内，则认为是可枚举的，如何解决一边删除一边枚举的问题，请看下面
			//注：如果有增加点，此时i肯定仍然在正常的范围之内，此时也要二次开发者去保证不要重复枚举
			//总之一个原则：删除点可能造成枚举不全；增加点可能造成重复枚举

			float value = m_ST_Curve.GetValueData(nCurveIndex, i);
			CString str;
			str.Format(_T("%s, %f"), ((COleDateTime) m_ST_Curve.GetTimeData(nCurveIndex, i)).Format(), value);
			AfxMessageBox(str);

			if (value > 92.0f) //删除y值大于92.0f的点，模拟一边枚举一边删除
			{
				m_ST_Curve.DelPoint(nCurveIndex, i);
				--i; //当前i的位置删除了（后面的补了上来），所以要继续在这个位置上进行枚举
				--CurveLen; //因为删除了一个点，所以点的总数要减一
			}
		}

		m_ST_Curve.Refresh();
	}
}

void CTestST_CurveDlg::OnButton82() 
{
	m_ST_Curve.SetMoveMode(0x80 | m_ST_Curve.GetMoveMode());
}

void CTestST_CurveDlg::OnButton85() 
{
	static int HelpTipState;
	++HelpTipState;
	m_ST_Curve.EnableHelpTip(HelpTipState % 2);
}

void CTestST_CurveDlg::OnButton86() 
{
	AfxMessageBox(_T("ST_Curven已开源，请从github上自行检查更新。"));
}

void CTestST_CurveDlg::ResetAll()
{
	m_ST_Curve.DelLegend(0, TRUE, FALSE);
	m_ST_Curve.DelRange2(0, 0, -1, TRUE, FALSE);
	m_ST_Curve.DelComment(0, TRUE, FALSE);
	m_ST_Curve.DelInfiniteCurve(0, TRUE, FALSE);

	m_ST_Curve.SetZLength(0);
}

//	#define PS_SOLID            0
//	#define PS_DASH             1
//	#define PS_DOT              2
//	#define PS_DASHDOT          3
//	#define PS_DASHDOTDOT       4
//	#define PS_NULL             5
//	#define PS_INSIDEFRAME      6

//	#define HS_HORIZONTAL       0       /* ----- */
//	#define HS_VERTICAL         1       /* ||||| */
//	#define HS_FDIAGONAL        2       /* \\\\\ */
//	#define HS_BDIAGONAL        3       /* ///// */
//	#define HS_CROSS            4       /* +++++ */
//	#define HS_DIAGCROSS        5       /* xxxxx */
void CTestST_CurveDlg::OnSelchangeCombo1() 
{
	ResetAll();

	COleDateTime Time = OleTime;
	switch (m_Combo.GetCurSel())
	{
	case 0: //最普通的显示方式
		m_ST_Curve.AddLegendHelper(10, _T("第一条曲线"), (unsigned long) RGB(255, 255, 0), PS_SOLID, 1, FALSE);
//		m_ST_Curve.AddLegend(10, _T("第一条曲线"),
//			(unsigned long) RGB(255, 0, 0), PS_SOLID, 1, (unsigned long) RGB(0, 255, 255), 5/*HS_DIAGCROSS*/, 0, 1, 0xFF, FALSE);

//		m_ST_Curve.AddLegendHelper(11, _T("第二条曲线"), (unsigned long) RGB(255, 0, 0), PS_SOLID, 1, FALSE);
		m_ST_Curve.AddLegend(11, _T("第二条曲线"),
			(unsigned long) RGB(255, 0, 0), PS_SOLID, 1, (unsigned long) RGB(0, 0, 255), 6/*HS_HORIZONTAL*/, 3, 2, 0xFF, FALSE);
		//平滑曲线，Hatch填充，注意，样式6是GDI+定义的，因为平滑曲线使用GDI+技术，所以可用，如果非平滑曲线，将使用
		//GDI，此时将不再支持样式6（样式6在GDI+中定义为HatchStyle05Percent，这里直接使用6，因为VC6是没有这个定义头文件的）

		m_ST_Curve.AddLegend(12, _T("第三条曲线"), //不可使用AddLegendHelper函数，因为倒数第3、4个参数不是默认的
			(unsigned long) RGB(0, 255, 0), PS_SOLID, 1, 0, 255, 2, 1, 0xFF, FALSE);

		//将13曲线添加到“第一条曲线”这个图例，有两种方法
		//一：
//		m_ST_Curve.AddLegend(13, _T("第一条曲线"), 0, 0, 0, 0, 0, 0, 0, 1, FALSE); //Mask等于1，只有Address有效
		//二：
		m_ST_Curve.AddLegendHelper(13, _T("第一条曲线"), (unsigned long) RGB(255, 255, 0), PS_SOLID, 1, FALSE);
		//因为AddLegendHelper函数没有Mask，所以PenColor PenStyle LineWidth要照写，否则会改变这些量

		//标识起始点与结束点
		m_ST_Curve.AppendLegendEx(_T("第一条曲线"),
			(unsigned long) RGB(0, 255, 255), (unsigned long) RGB(255, 0, 255), (unsigned long) RGB(0, 0, 255), 7);

		for (int i = 0; i < 12; i++)
		{
			if (i % 2)
			{
				m_ST_Curve.AddMainData2(11, Time, 90 + .5f * i, i < 5, 0, TRUE);
				m_ST_Curve.AddMainData2(13, Time, 89 + .7f * i, 0, 0, TRUE);

				//m_ST_Curve.AddComment(Time, 89 + .7f * i, 4, 12, 0, 0, 0, NULL, 0, 7, 7, FALSE);
			}
			else
			{
				m_ST_Curve.AddMainData2(11, Time, 90 + .6f * i, i < 5, 0, TRUE);
				m_ST_Curve.AddMainData2(13, Time, 89 + .8f * -i, 0x100, 0, TRUE);

				//m_ST_Curve.AddComment(Time, 89 + .8f * -i, 4, 12, 0, 0, 0, NULL, 0, 7, 7, FALSE);
			}

			Time += TimeSpan;
		}

//		Time += TimeSpan * 12;
		for (int i = 0; i < 12; i++)
		{
			m_ST_Curve.AddMainData2(12, Time, 88 + .5f * i, 0, 0, TRUE);

			Time += TimeSpan;
		}

//		m_ST_Curve.SetFillDirection(10, 1, FALSE);
		m_ST_Curve.SetFillDirection(11, 0x31, TRUE); //显示平均值、向下填充
		//由于11曲线所在的图例没有指定填充，所以上面的设置不会起作用

		//将曲线12加到曲线11上（其实什么都不会做，因为曲线12与曲线11没有相同的横坐标）
//		m_ST_Curve.ArithmeticOperate(11, 12, 0, -1, '+');
/*
		//将曲线13的所有纵坐标的十五分之一加到曲线11上
		m_ST_Curve.CloneCurve(13, 100); //100是临时曲线
		m_ST_Curve.OffSetCurve(100, .0, 1.0f / 15, '*'); //将临时曲线所有纵坐标减为原来的十五分之一（乘以1.0f / 15）
		m_ST_Curve.ArithmeticOperate(11, 100, '+'); //将临时曲线加到曲线11上
		m_ST_Curve.DelRange2(100, 0, -1, FALSE, TRUE); //删除临时曲线
*/
		//将曲线12插入到11的最后面
//		m_ST_Curve.UniteCurve(11, m_ST_Curve.GetCurveLength(11), 12, 0, 3);
		//将曲线12插入到11的中间位置
//		m_ST_Curve.UniteCurve(11, 5, 12, 0, 3);

//		m_ST_Curve.CloneCurve(11, 14); //以11为基础复制出来一条曲线14
		//偏移一下曲线14，否则与11完全重叠，注意，由于图例里面没有14曲线，所以显示为虚线
//		m_ST_Curve.OffSetCurve(14, 1.0 / 24 / 2, .95f, ('+' << 8) + '*'); //对纵坐标执行乘操作，对横坐标执行加操作
//		m_ST_Curve.OffSetCurve(14, 1.5, .0f, '*' << 8); //对横坐标执行乘操作（操作不失败，因为横坐标显示为时间，显示为值时可以）

		//下面测试注解相关功能
		m_ST_Curve.AddComment((double) Time - 1.0, 92.0f, 3, 10, 0, 0, 0xFFFFFF,
			_T("测试注解\n第二排第二二排\n第三排"), 0xFFFFFF, 20, 10, FALSE);
		m_ST_Curve.AddComment((double) Time - 1.0, 92.0f, 0, 11, 0, 0, 0xFFFFFF, //Time Value一样，但Position不同，测试效果
			_T("测试注解"), 0xFFFFFF, 20, 10, FALSE);
//		m_ST_Curve.AddComment(Time - 1.0, 92.0f, 0, 10, 0, 0, 0xFFFFFF, NULL, 0xFFFFFF, 20, 10, TRUE);

		//下面的代码没有实质性效果，因为两个注解不交叠，把第一个注解的Position改为0，就有效果了
		m_ST_Curve.SwapCommentIndex(0, 1, TRUE); //第一个注解（位图是粉红色的，将被提到最上层，没有这行代码时，它在最下层）
//		m_ST_Curve.ShowComment(0, FALSE, FALSE); //由于前面有交换语句，所以这里隐藏的是第二个注解（蓝色那个）

		//下面演示无限曲线
		m_ST_Curve.AddInfiniteCurve(11, (double) Time - 1.0, .0f, (8 << 8) + 1, FALSE); //垂直无限直线，向左填充
		m_ST_Curve.AddInfiniteCurve(12, .0, 92.0f, 0, FALSE); //水平无限直线，不填充
		//注意，最好为无限直线增加单独的图例，我这里用的是普通曲线的图例，并不很好，因为：
		//一：无限直线一般来说，颜色上应该与普通曲线有所区别，所以也应该单独为其添加图例
		//二：可能会有效率损失
		//要达到最佳效率，无限直线的图例，请一定关闭如下图例基本属性（请对照AddLegend接口看）：
		//CurveMode = 0;
		//NodeMode = 0;
		//和如下图例扩展属性（请对照AppendLegendEx接口看）：
		//BeginNodeColor = 0;
		//EndNodeColor = 0;
		//SelectedNodeColor = 0;
		//NodeModeEx = 0;
		//和如下图例坐标标记属性（请对照SetXYFormat接口看）：
		//Format = 0;
		//对于AddLegend接口，由于必须要调用，所以需要指定CurveMode和NodeMode均为0
		//对于AppendLegendEx和SetXYFormat接口，你不调用它，默认就是上面的情况，所以就简单了，不调用即可
		//另外还有一个简单的方法，如果不需要填充的话，调用AddLegendHelper添加出来的图例，也满足上面的要求
		break;
	case 1:
		//最普通的显示方式
		m_ST_Curve.AddLegendHelper(10, _T("第一条曲线"), (unsigned long) RGB(255, 255, 0), PS_SOLID, 1, FALSE);
		//Hatch填充，反显节点
		m_ST_Curve.AddLegend(11, _T("第二条曲线"),
			(unsigned long) RGB(255, 0, 0), PS_SOLID, 1, (unsigned long) RGB(255, 0, 0), HS_DIAGCROSS, 0, 2, 0xFF, FALSE);
		//方波显示，Solid填充，隐藏节点
		m_ST_Curve.AddLegend(12, _T("第三条曲线"),
			(unsigned long) RGB(0, 255, 0), PS_SOLID, 1, (unsigned long) RGB(0, 255, 0), 127, 1, 0, 0xFF, FALSE);

		//将13曲线添加到“第一条曲线”这个图例，有两种方法
		//一：
//		m_ST_Curve.AddLegend(13, _T("第一条曲线"), 0, 0, 0, 0, 0, 0, 0, 1, FALSE); //Mask等于1，只有Address有效
		//二：
		m_ST_Curve.AddLegendHelper(13, _T("第一条曲线"), (unsigned long) RGB(255, 255, 0), PS_SOLID, 1, FALSE);
		//因为AddLegendHelper函数没有Mask，所以PenColor PenStyle LineWidth要照写，否则会改变这些量

		for (int i = 0; i < 12; i++)
		{
			if (i % 2)
			{
				m_ST_Curve.AddMainData2(11, Time, 90 + .5f * i, i < 2, 0, TRUE);
				m_ST_Curve.AddMainData2(13, Time, 89 + .7f * (i < 6 ? i : (12 - i)), 0, 0, TRUE);
			}
			else
			{
				m_ST_Curve.AddMainData2(11, Time, 90 + .6f * i, 1, 0, TRUE);
				m_ST_Curve.AddMainData2(13, Time, 89 + .8f * (i < 6 ? i : (12 - i)), 0, 0, TRUE);
			}

			Time += TimeSpan;
		}

//		Time += TimeSpan * 12;
		for (int i = 0; i < 12; i++)
		{
			m_ST_Curve.AddMainData2(12, Time, 88 + .5f * i, 0, 0, TRUE);

			Time += TimeSpan;
		}

		m_ST_Curve.SetFillDirection(11, 0x11, TRUE); //显示平均值、向下填充（使用前景色显示第一个值）
		m_ST_Curve.SetFillDirection(12, 0x61, TRUE); //显示右边值、向下填充（使用曲线颜色，没有效果，因为是Solid填充）
		//只有Hatch和Pattern填充时才可以指定显示颜色
		//12这条曲线由于是多于2个点连在一起的柱状图，所以右边的值是什么，要看曲线的位置，控件绘制到第一个超出画布右边杠的点，
		//这个点就是将要显示的值，所以柱状图应该是两个点为一个柱，多个点为一柱的话，在显示值的时候，随着曲线位置的不同，
		//结果会不同。
		break;
	case 2:
		//Hatch填充，隐藏节点
		m_ST_Curve.AddLegend(10, _T("第一条曲线"),
			(unsigned long) RGB(255, 255, 0), PS_NULL, 1, (unsigned long) RGB(255, 255, 0), HS_VERTICAL, 0, 0, 0xFF, FALSE);
		//Solid填充，隐藏节点
		m_ST_Curve.AddLegend(11, _T("第二条曲线"),
			(unsigned long) RGB(255, 0, 0), PS_NULL, 1, (unsigned long) RGB(255, 0, 0), 127, 0, 0, 0xFF, FALSE);
		//Pattern填充，隐藏节点
		m_ST_Curve.AddLegend(12, _T("第三条曲线"),
			(unsigned long) RGB(0, 255, 0), PS_NULL, 1, (unsigned long) RGB(0, 255, 0), 128, 0, 0, 0xFF, FALSE);

		//将13曲线添加到“第一条曲线”这个图例，有两种方法
		//一：
//		m_ST_Curve.AddLegend(13, _T("第一条曲线"), 0, 0, 0, 0, 0, 0, 0, 1, FALSE); //Mask等于1，只有Address有效
		//二：
		m_ST_Curve.AddLegendHelper(13, _T("第一条曲线"), (unsigned long) RGB(255, 255, 0), PS_NULL, 1, FALSE);
		//因为AddLegendHelper函数没有Mask，所以PenColor PenStyle LineWidth要照写，否则会改变这些量

		//下面的设置不会起作用，原因是画笔为空，就算画笔不为空，但NodeMode为0（参看上面的AddLegend接口调用）
		//所以仍然不会起作用
		//要想标识起始点与结束点，必须是画笔不为空，则NodeMode不为0
		m_ST_Curve.AppendLegendEx(_T("第一条曲线"), (unsigned long) RGB(255, 0, 0), (unsigned long) RGB(0, 255, 0), 0, 3);
		//SelectedNodeColor未使用到，这通过最后一个参数NodeModeEx指定

		for (int i = 0; i < 3; ++i) //三条曲线
		{
			Time = OleTime;
			Time += i * TimeSpan;
			for (int j = 0; j < 4; ++j) //四组
				for (int k = 0; k < 4; ++k) //三个一组，空一组
				{
					if (k < 2)
						m_ST_Curve.AddMainData2(11 + i, Time, 88 + 1.0f * (i + j), !k, 0, TRUE);
					Time += TimeSpan;
				}
		}

		m_ST_Curve.SetFillDirection(13, 4, TRUE); //向上填充
		break;
	case 3:
		//Hatch填充，隐藏节点
		m_ST_Curve.AddLegend(10, _T("第一条曲线"),
			(unsigned long) RGB(255, 255, 0), PS_NULL, 1, (unsigned long) RGB(255, 255, 0), HS_VERTICAL, 0, 0, 0xFF, FALSE);
		//Solid填充，隐藏节点
		m_ST_Curve.AddLegend(11, _T("第二条曲线"),
			(unsigned long) RGB(255, 0, 0), PS_NULL, 1, (unsigned long) RGB(255, 255, 255), 127, 0, 0, 0xFF, FALSE);
		//Pattern填充，隐藏节点
		m_ST_Curve.AddLegend(12, _T("第三条曲线"),
			(unsigned long) RGB(0, 255, 0), PS_NULL, 1, (unsigned long) RGB(0, 255, 0), 128, 0, 0, 0xFF, FALSE);

		//将13曲线添加到“第一条曲线”这个图例，有两种方法
		//一：
//		m_ST_Curve.AddLegend(13, _T("第一条曲线"), 0, 0, 0, 0, 0, 0, 0, 1, FALSE); //Mask等于1，只有Address有效
		//二：
		m_ST_Curve.AddLegendHelper(13, _T("第一条曲线"), (unsigned long) RGB(255, 255, 0), PS_NULL, 1, FALSE);
		//因为AddLegendHelper函数没有Mask，所以PenColor PenStyle LineWidth要照写，否则会改变这些量

		m_ST_Curve.AddLegend(14, _T("123"),
			(unsigned long) RGB(255, 0, 255), PS_NULL, 1, (unsigned long) RGB(255, 0, 255), 129, 0, 0, 0xFF, FALSE);

		Time = OleTime - TimeSpan;
		m_ST_Curve.AddMainData2(14, Time, 88.0f, 0, 0, TRUE);
		m_ST_Curve.AddMainData2(14, Time, 90.0f, 0, 0, TRUE);

		{
			BYTE MemData[2048];
			//申请一个足够在的内存空间，这个数组容纳了三条曲线，每个点占用18个字节
			//具体是：地址（4）+ 时间（8）+ 值（4）+ State（2）
			//所以，如果要精确计算至少需要申请多少内存，可以通过点数乘以18来计算得到

			LPBYTE pMemData = MemData;
			for (int i = 0; i < 3; ++i) //三条曲线
			{
				Time = OleTime;
				Time += i * TimeSpan;

				for (int j = 0; j < 4; ++j) //四组
					for (int k = 0; k < 4; ++k) //三个一组，空一组
					{
						if (k < 2)
						{
							*(long*) pMemData = 11 + i;
							*(double*) (pMemData + 4) = Time;
							*(float*) (pMemData + 4 + 8) = 88 + 1.0f * (i + j);
							*(short*) (pMemData + 16) = !k;

							pMemData += 18;
						}
						Time += TimeSpan;
					}
			}
			m_ST_Curve.AddMemMainData(SplitHandle(m_ST_Curve, MemData), (long) (pMemData - MemData), TRUE); //一次添加三条曲线
		}

		m_ST_Curve.SetFillDirection(11, 0x21, TRUE); //显示第二个值、向下填充
		m_ST_Curve.SetFillDirection(13, 4, TRUE); //向上填充
		m_ST_Curve.SetFillDirection(14, 0x98, TRUE); //显示第一个值、向左填充（使用前景色的反色）
		return; //防止后面的FirstPage调用
		break;
	case 4: //椭圆，不常用的用法
//		m_ST_Curve.AddLegend(14, _T("123"),
//			(unsigned long) RGB(255, 0, 255), PS_NULL, 1, (unsigned long) RGB(255, 0, 255), 129, 0, 0, 0xFF, FALSE);
		m_ST_Curve.AddLegend(14, _T("123"),
			(unsigned long) RGB(255, 0, 255), PS_SOLID, 1, (unsigned long) RGB(255, 0, 255), 0xFF, 0, 0, 0xFF, FALSE);
		m_ST_Curve.AddLegend(15, _T("456"),
			(unsigned long) RGB(255, 255, 255), PS_SOLID, 1, (unsigned long) RGB(255, 255, 255), 131, 3, 0, 0xFF, FALSE);

		//先用普通方法添加一条2次曲线
		Time = OleTime;
		Time -= 5 * TimeSpan;

		m_ST_Curve.AddMainData2(15, Time, 87.0f, 0, 0, TRUE);
		Time += TimeSpan;
		m_ST_Curve.AddMainData2(15, Time, 86.0f, 0, 0, TRUE);
		Time += TimeSpan;
		m_ST_Curve.AddMainData2(15, Time, 85.0f, 0, 0, TRUE);
		m_ST_Curve.AddMainData2(15, Time, 87.0f, 0, 0, TRUE);
		Time -= TimeSpan;
		m_ST_Curve.AddMainData2(15, Time, 86.0f, 0, 0, TRUE);
		Time -= TimeSpan;
//		m_ST_Curve.AddMainData2(15, Time - 0.01, 85.0f, 0, 0, TRUE);
		m_ST_Curve.AddMainData2(15, Time, 85.0f, 0, 0, TRUE);
		m_ST_Curve.AddMainData2(15, Time, 87.0f, 0, 0, TRUE);

		Time = OleTime;
		{
			BYTE MemData[2048];
			//画一个圆
			LPBYTE pMemData = MemData;
			for (int i = 0; i >= -350; i -= (i >= -180 ? 10 : 5))
			{
				*(long*) pMemData = 14;
				double h = i * 3.1415926 / 180;
				*(double*) (pMemData + 4) = (double) Time + .1 * cos(h);
				*(float*) (pMemData + 4 + 8) = 88 + 5.0f * (float) sin(h);
				*(short*) (pMemData + 16) = 0;

				pMemData += 18;
			}
			m_ST_Curve.AddMemMainData(SplitHandle(m_ST_Curve, MemData), (long) (pMemData - MemData), TRUE);
		}
		m_ST_Curve.Refresh();
		break;
	case 5: //演示三维效果
		m_ST_Curve.SetZLength(2);
		m_ST_Curve.AddLegendHelper(10, _T("第一条曲线"), (unsigned long) RGB(0, 255, 0), PS_SOLID, 1, FALSE);
		m_ST_Curve.AddLegendHelper(11, _T("第二条曲线"), (unsigned long) RGB(255, 0, 0), PS_SOLID, 1, FALSE);
		m_ST_Curve.AddLegend(12, _T("第三条曲线"), //不可使用AddLegendHelper函数，因为倒数第3、4个参数不是默认的
			(unsigned long) RGB(0, 0, 255), PS_SOLID, 1, 0, 255, 2, 1, 0xFF, FALSE);

		//将13曲线添加到“第一条曲线”这个图例，有两种方法
		//一：
//		m_ST_Curve.AddLegend(13, _T("第一条曲线"), 0, 0, 0, 0, 0, 0, 0, 1, FALSE); //Mask等于1，只有Address有效
		//二：
		m_ST_Curve.AddLegendHelper(13, _T("第一条曲线"), (unsigned long) RGB(0, 255, 0), PS_SOLID, 1, FALSE);
		//因为AddLegendHelper函数没有Mask，所以PenColor PenStyle LineWidth要照写，否则会改变这些量

		for (int i = 0; i < 12; i++)
		{
			float Value = 90 + .5f * i;
			//三条完全重合的曲线
			m_ST_Curve.AddMainData2(11, Time, Value, 0, 0, TRUE);
			m_ST_Curve.AddMainData2(12, Time, Value, 0, 0, TRUE);
			m_ST_Curve.AddMainData2(13, Time, Value, 0, 0, TRUE);
			Time += TimeSpan;
		}
		m_ST_Curve.SetZOffset(13, 30, TRUE); //用Z轴来偏移13曲线（偏移了1个Z轴单位）
		m_ST_Curve.SetZOffset(12, 61, TRUE); //用Z轴来隐藏12曲线（偏移了2个Z轴单位多，所以被隐藏掉了）
		//60虽然已经大于2个单位Z轴了，但由于浮点数转整数的误差，所以仍然可见12曲线，这里最小必须为61
		//每一个单位的Z轴，屏幕象素为29.698484809834996个，所以这里只需要大于它的两倍，就可以隐藏曲线了
		break;
	}

	m_ST_Curve.FirstPage(FALSE, FALSE);
}

void CTestST_CurveDlg::OnButton43() 
{
	if (KillTimer(100) || KillTimer(101))
		return;

	ResetAll();

//	/*
	m_ST_Curve.AddLegend(13, _T("第一条曲线"),
		(unsigned long) RGB(255, 255, 0), PS_SOLID, 1, (unsigned long) RGB(255, 255, 0), 255, 0, 1, 0xFF, FALSE);
	m_ST_Curve.AddLegend(14, _T("123"),
		(unsigned long) RGB(0, 255, 0), PS_SOLID, 1, (unsigned long) RGB(0, 255, 0), 128, 0, 1, 0xFF, FALSE);
	m_ST_Curve.AppendLegendEx(_T("123"), (unsigned long) RGB(255, 0, 0), (unsigned long) RGB(0, 0, 255), 0, 3);
//	*/
//	m_ST_Curve.AddLegendHelper(13, _T("第一条曲线"), (unsigned long) RGB(255, 255, 0), PS_SOLID, 1, FALSE);
//	m_ST_Curve.AddLegendHelper(14, _T("第二条曲线"), (unsigned long) RGB(255, 0, 0), PS_SOLID, 1, FALSE);

	m_ST_Curve.SetValueStep(20.0f);
//	m_ST_Curve.SetAutoRefresh(31, 0); //3.1 seconds
//	m_ST_Curve.SetAutoRefresh(0, 3);
//	m_ST_Curve.LimitOnePage(TRUE);
//	m_ST_Curve.SetLimitOnePageMode(1);

	SetTimer(100, 1000, NULL);
//	SetTimer(101, 1000, NULL);
}

void CTestST_CurveDlg::OnButton17() 
{
	static int ct;
	if (ct & 1)
	{
		m_ST_Curve.AddLegend(14, _T("123"),
			(unsigned long) RGB(255, 0, 255), PS_SOLID, 1, (unsigned long) RGB(255, 0, 255), 129, 0, 0, 0xA0, FALSE); //只更改画刷样式和节点模式

		//让隐藏的点显示出来
		m_ST_Curve.TrimCurve(14, 0, 1, -1, 2, FALSE);
//		m_ST_Curve.TrimCurve2(14, 0, OleTime + TimeSpan, .0, 1, 2, FALSE);
		//生成断点
		m_ST_Curve.TrimCurve(14, 1, 2, -1, 2, FALSE);
//		m_ST_Curve.TrimCurve2(14, 1, OleTime + TimeSpan + TimeSpan, .0, 1, 2, FALSE);
		m_ST_Curve.Refresh();
	}
	else
	{
		m_ST_Curve.DelLegend(0, TRUE, FALSE);
		m_ST_Curve.DelRange2(0, 0, -1, TRUE, FALSE);

		m_ST_Curve.AddLegend(14, _T("123"),
			(unsigned long) RGB(255, 0, 255), PS_SOLID, 1, (unsigned long) RGB(255, 0, 255), 255, 0, 1, 0xFF, FALSE);

		//下面生成曲线，但其中有一些隐藏点，这样方便以后改为柱状图显示
		COleDateTime Time = OleTime;
		for (int i = 0; i < 11; ++i)
		{
			m_ST_Curve.AddMainData2(14, Time, 87.0f + i / 2, i & 1 ? 2 : 0, 0, TRUE);
			Time += TimeSpan;
		}

		AfxMessageBox(_T("这是原始曲线，再次点击该按钮切换到柱状图显示。"));
	}

	++ct;
}

void CTestST_CurveDlg::OnButton22() 
{
	static int bFullScreen = 0;
	++bFullScreen;
	m_ST_Curve.EnableFullScreen(bFullScreen & 1);
	SetDlgItemText(IDC_BUTTON22, bFullScreen & 1 ? _T("取消全屏") : _T("全屏"));
}

void CTestST_CurveDlg::OnButton80() //限制一页
{
	m_ST_Curve.SetZoom(0);
	m_ST_Curve.EnableZoom(FALSE);
	m_ST_Curve.SetMoveMode(0);
	m_ST_Curve.LimitOnePage(TRUE);
}

void CTestST_CurveDlg::OnButton87() //限制坐标
{
//	m_ST_Curve.SetZoom(0);
//	m_ST_Curve.EnableZoom(FALSE);
//	m_ST_Curve.SetMoveMode(0);
	double BeginTime, EndTime;
	float BeginValue, EndValue;
	if (m_ST_Curve.GetOneTimeRange(11, &BeginTime, &EndTime))
	{
		m_ST_Curve.GetOneValueRange(11, &BeginValue, &EndValue);
		m_ST_Curve.FixCoor(BeginTime, EndTime, BeginValue, EndValue, 2); //只限制横坐标结束值
	}
}

void CTestST_CurveDlg::OnButton88() //刷新限制坐标
{
	m_ST_Curve.RefreshLimitedOrFixedCoor();
}

void CTestST_CurveDlg::OnButton89() //位置预览窗口
{
	static int bPreview = 1;
	++bPreview;
	m_ST_Curve.EnablePreview(bPreview & 1);
}

void CTestST_CurveDlg::OnButton90() 
{
	m_ST_Curve.SetXYFormat(_T("第二条曲线"), GetDlgItemInt(IDC_EDIT28));
}

void CTestST_CurveDlg::OnButton91() 
{
	long re = m_ST_Curve.LoadPlugIn(_T("ST_Curve_PlugIn.dll"), 1, 3); //对XY轴都使用插件
	if (re)
		AfxMessageBox(_T("加载ST_Curve_PlugIn.dll失败！"));

	re = m_ST_Curve.LoadLuaScript(_T("ST_Curve_PlugIn.lua"), 1, 0x3E); //对应二进制：11 1110
	//对Y轴使用lua函数（这会覆盖ST_Curve_PlugIn.dll插件对Y轴的使用）
	//同时，在OnInitDialog里面有如下调用：
	//m_ST_Curve.LoadLuaScript(_T("ST_Curve_PlugIn.lua"), 1, 0x3C);
	//为了让0x3C位置上的仍然有效，这里使用了0x3E = 0x3C + 2(对Y轴使用插件)
	if (re)
		AfxMessageBox(_T("加载ST_Curve_PlugIn.lua失败！"));
}

void CTestST_CurveDlg::OnButton92() 
{
	long TempBuffSize;
	long AllBuffSize;
	float UseRate;
	long Address;
	if (m_ST_Curve.GetMemInfo(&TempBuffSize, &AllBuffSize, &UseRate, &Address))
	{
		CString str;
		str.Format(_T("临时缓存大小（POINT）：%d\r\n曲线缓存大小（点）：%d\r\n缓存利用率：%f\r\n最小缓存利用率曲线：%d\r\n"),
			TempBuffSize, AllBuffSize, UseRate, Address);
		AfxMessageBox(str);
	}
}

BEGIN_EVENTSINK_MAP(CTestST_CurveDlg, CDialogEx)
    //{{AFX_EVENTSINK_MAP(CTestST_CurveDlg)
	ON_EVENT(CTestST_CurveDlg, IDC_STCURVECTRL, -605 /* MouseDown */, OnMouseDownStcurvectrl, VTS_I2 VTS_I2 VTS_I4 VTS_I4)
	ON_EVENT(CTestST_CurveDlg, IDC_STCURVECTRL, -606 /* MouseMove */, OnMouseMoveStcurvectrl, VTS_I2 VTS_I2 VTS_I4 VTS_I4)
	ON_EVENT(CTestST_CurveDlg, IDC_STCURVECTRL, -607 /* MouseUp */, OnMouseUpStcurvectrl, VTS_I2 VTS_I2 VTS_I4 VTS_I4)
	ON_EVENT(CTestST_CurveDlg, IDC_STCURVECTRL, 1 /* PageChange */, OnPageChangeStcurvectrl, VTS_I4 VTS_I4)
	ON_EVENT(CTestST_CurveDlg, IDC_STCURVECTRL, 2 /* BeginTimeChange */, OnBeginTimeChangeStcurvectrl, VTS_DATE)
	ON_EVENT(CTestST_CurveDlg, IDC_STCURVECTRL, 3 /* BeginValueChange */, OnBeginValueChangeStcurvectrl, VTS_R4)
	ON_EVENT(CTestST_CurveDlg, IDC_STCURVECTRL, 4 /* TimeSpanChange */, OnTimeSpanChangeStcurvectrl, VTS_R8)
	ON_EVENT(CTestST_CurveDlg, IDC_STCURVECTRL, 5 /* ValueStepChange */, OnValueStepChangeStcurvectrl, VTS_R4)
	ON_EVENT(CTestST_CurveDlg, IDC_STCURVECTRL, 6 /* ZoomChange */, OnZoomChangeStcurvectrl, VTS_I2)
	ON_EVENT(CTestST_CurveDlg, IDC_STCURVECTRL, 7 /* SelectedCurveChange */, OnSelectedCurveChangeStcurvectrl, VTS_I4)
	ON_EVENT(CTestST_CurveDlg, IDC_STCURVECTRL, 8 /* LegendVisableChange */, OnLegendVisableChangeStcurvectrl, VTS_I4 VTS_I2)
	ON_EVENT(CTestST_CurveDlg, IDC_STCURVECTRL, 9 /* SorptionChange */, OnSorptionChangeStcurvectrl, VTS_I4 VTS_I4 VTS_I2)
	ON_EVENT(CTestST_CurveDlg, IDC_STCURVECTRL, 10 /* CurveStateChange */, OnCurveStateChangeStcurvectrl, VTS_I4 VTS_I2)
	ON_EVENT(CTestST_CurveDlg, IDC_STCURVECTRL, 11 /* ZoomModeChange */, OnZoomModeChangeStcurvectrl, VTS_I2)
	ON_EVENT(CTestST_CurveDlg, IDC_STCURVECTRL, 12 /* HZoomChange */, OnHZoomChangeStcurvectrl, VTS_I2)
	ON_EVENT(CTestST_CurveDlg, IDC_STCURVECTRL, 13 /* BatchExportImageChange */, OnBatchExportImageChangeStcurvectrl, VTS_I4)
	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()

void CTestST_CurveDlg::OnMouseDownStcurvectrl(short Button, short Shift, long x, long y) 
{
	ShowEvent(_T("左键按下"));
}

void CTestST_CurveDlg::OnMouseMoveStcurvectrl(short Button, short Shift, long x, long y) 
{
	//ShowEvent(_T("鼠标移动")); //这个事件太多，不输出（输出会影响观看其它事件）
}

void CTestST_CurveDlg::OnMouseUpStcurvectrl(short Button, short Shift, long x, long y) 
{
	ShowEvent(_T("左键弹起"));

	if (2 == Button)
	{
		/*鼠标点击插入或者修改点
		double Time;
		float Value;

		short nCurveIndex = m_ST_Curve.GetCurveIndex(13);
		m_ST_Curve.GetOneFirstPos(13, &Time, &Value, FALSE);

		//操作一：
		//把13曲线的第2个点修改，参数Time Value State均有效（当然也可以部分有效）
//		m_ST_Curve.InsertMainData2(nCurveIndex, 1, Time, Value + 2.0f, 7 << 8, 0);

		//操作二：
		//在13曲线的第2个点的前面增加一个点，此时，参数Time Value State当然均有效，因为是一个新点
//		m_ST_Curve.InsertMainData2(nCurveIndex, 1, Time, Value + 2.0f, 0, -1);

		//操作三：
		//在13曲线的第2个点的后面增加一个点，此时，参数Time Value State当然均有效，因为是一个新点
		m_ST_Curve.InsertMainData2(nCurveIndex, 1, Time, Value + 2.0f, 0, 1);

		m_ST_Curve.Refresh();
		*/

		/*鼠标点击增加点
		double Time;
		float Value;
		long offset = m_ST_Curve.GetZOffset(13); //考虑Z坐标的影响，否则添加的点不会落在鼠标点击处（如果Z坐标不为0的话）
		x -= (offset >> 16);
		y += (offset & 0xFFFF);
		if (m_ST_Curve.GetActualPoint(x, y, &Time, &Value)) //点击在画布里面
			m_ST_Curve.AddMainData2(13, Time, Value, 0, 1, FALSE); //插入数据，马上刷新
		*/

		/*鼠标点击删除点，本函数不用考虑Z坐标的影响
		long re = m_ST_Curve.GetPointFromScreenPoint(13, x, y, 8);
		if (-1 != re) //鼠标击中了点，并且点属于13曲线，下面删除该点
			m_ST_Curve.DelRange2(13, re, 1, FALSE, TRUE);
		*/
	}
}

void CTestST_CurveDlg::OnPageChangeStcurvectrl(long wParam, long lParam) 
{
	CString str;
	str.Format(_T("%d:%d"), wParam, lParam);
	SetDlgItemText(IDC_EDIT10, str);
}

void CTestST_CurveDlg::OnBeginTimeChangeStcurvectrl(DATE NewTime) 
{
	ShowEvent(_T("原点横坐标改变"));
}

void CTestST_CurveDlg::OnBeginValueChangeStcurvectrl(float NewValue) 
{
	ShowEvent(_T("原点纵坐标改变"));
}

void CTestST_CurveDlg::OnTimeSpanChangeStcurvectrl(double NewTimeSpan) 
{
	ShowEvent(_T("横坐标间隔改变"));
}

void CTestST_CurveDlg::OnValueStepChangeStcurvectrl(float NewValueStep) 
{
	ShowEvent(_T("纵坐标间隔改变"));
}

void CTestST_CurveDlg::OnZoomChangeStcurvectrl(short NewZoom) 
{
	ShowEvent(_T("缩放改变"));
}

void CTestST_CurveDlg::OnSelectedCurveChangeStcurvectrl(long NewId) 
{
	ShowEvent(_T("选中曲线改变"));
}

void CTestST_CurveDlg::OnLegendVisableChangeStcurvectrl(long Index, short State) 
{
	ShowEvent(_T("图例可见状态改变"));
}

void CTestST_CurveDlg::OnSorptionChangeStcurvectrl(long Id, long Index, short State) 
{
	ShowEvent(_T("吸附点改变"));
}

void CTestST_CurveDlg::OnCurveStateChangeStcurvectrl(long Id, short State) 
{
	ShowEvent(_T("曲线状态改变"));
}

void CTestST_CurveDlg::OnZoomModeChangeStcurvectrl(short NewMode) 
{
	ShowEvent(_T("缩放模式改变"));
}

void CTestST_CurveDlg::OnHZoomChangeStcurvectrl(short NewZoom) 
{
	ShowEvent(_T("水平缩放改变"));
}


void CTestST_CurveDlg::OnBatchExportImageChangeStcurvectrl(long FileNameIndex) 
{
	ShowEvent(_T("批量导出改变"));
}

void CTestST_CurveDlg::OnDestroy() 
{
#ifdef _DEBUG
	FreeConsole();
#endif

	CDialogEx::OnDestroy();
}
