
// TestST_Curve3View.cpp : CTestST_Curve3View 类的实现
//

#include "stdafx.h"
// SHARED_HANDLERS 可以在实现预览、缩略图和搜索筛选器句柄的
// ATL 项目中进行定义，并允许与该项目共享文档代码。
#ifndef SHARED_HANDLERS
#include "TestST_Curve3.h"
#endif

#include "TestST_Curve3Doc.h"
#include "TestST_Curve3View.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTestST_Curve3View

IMPLEMENT_DYNCREATE(CTestST_Curve3View, CView)

BEGIN_MESSAGE_MAP(CTestST_Curve3View, CView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_PRINTCURVE, OnPrintcurve)
	// 标准打印命令
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CTestST_Curve3View::OnFilePrintPreview)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

// CTestST_Curve3View 构造/析构

CTestST_Curve3View::CTestST_Curve3View()
{
	// TODO: 在此处添加构造代码

}

CTestST_Curve3View::~CTestST_Curve3View()
{
}

BOOL CTestST_Curve3View::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: 在此处通过修改
	//  CREATESTRUCT cs 来修改窗口类或样式

	return CView::PreCreateWindow(cs);
}

// CTestST_Curve3View 绘制

void CTestST_Curve3View::OnDraw(CDC* /*pDC*/)
{
	CTestST_Curve3Doc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: 在此处为本机数据添加绘制代码
}


// CTestST_Curve3View 打印


void CTestST_Curve3View::OnFilePrintPreview()
{
#ifndef SHARED_HANDLERS
	AFXPrintPreview(this);
#endif
}

BOOL CTestST_Curve3View::OnPreparePrinting(CPrintInfo* pInfo)
{
	// 默认准备
	return DoPreparePrinting(pInfo);
}

void CTestST_Curve3View::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: 添加额外的打印前进行的初始化过程
}

void CTestST_Curve3View::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: 添加打印后进行的清理过程
}

void CTestST_Curve3View::OnRButtonUp(UINT /* nFlags */, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void CTestST_Curve3View::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
#ifndef SHARED_HANDLERS
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
#endif
}


// CTestST_Curve3View 诊断

#ifdef _DEBUG
void CTestST_Curve3View::AssertValid() const
{
	CView::AssertValid();
}

void CTestST_Curve3View::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CTestST_Curve3Doc* CTestST_Curve3View::GetDocument() const // 非调试版本是内联的
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CTestST_Curve3Doc)));
	return (CTestST_Curve3Doc*)m_pDocument;
}
#endif //_DEBUG


// CTestST_Curve3View 消息处理程序
int CTestST_Curve3View::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO: Add your specialized creation code here
	RECT rect;
	GetClientRect(&rect);
	m_ST_Curve.Create(_T("ST_Curve in view"), WS_CHILD | WS_VISIBLE, rect, this, 1000);
	m_ST_Curve.SetGridMode(m_ST_Curve.GetGridMode() + 8);

	COleDateTime Time;
	Time.ParseDateTime(_T("2007-5-8 0:0:0"));
	COleDateTimeSpan TimeSpan = 0.041666666666666666666666666666;

	m_ST_Curve.AddLegendHelper(10, _T("第一条曲线"), (unsigned long) RGB(255, 255, 0), PS_SOLID, 1, FALSE);
	m_ST_Curve.AddLegend(11, _T("第二条曲线"),
		(unsigned long) RGB(255, 0, 0), PS_SOLID, 1, (unsigned long) RGB(0, 0, 255), 6/*HS_HORIZONTAL*/, 3, 1, 0xFF, FALSE);
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

	for (int i = 0; i < 12; i++)
	{
		if (i % 2)
		{
			m_ST_Curve.AddMainData2(11, Time, 90 + .5f * i, i < 5, 0, TRUE);
			m_ST_Curve.AddMainData2(13, Time, 89 + .7f * i, 0, 0, TRUE);
		}
		else
		{
			m_ST_Curve.AddMainData2(11, Time, 90 + .6f * i, i < 5, 0, TRUE);
			m_ST_Curve.AddMainData2(13, Time, 89 + .8f * i, 0, 0, TRUE);
		}
	
		Time += TimeSpan;
	}

	//		Time += TimeSpan * 12;
	for (int i = 0; i < 12; i++)
	{
		m_ST_Curve.AddMainData2(12, Time, 88 + .5f * i, 0, 0, TRUE);
		
		Time += TimeSpan;
	}

	m_ST_Curve.SetFillDirection(11, 0x31, TRUE); //显示平均值、向下填充

	return 0;
}

void CTestST_Curve3View::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	if (SIZE_MAXIMIZED == nType || SIZE_RESTORED == nType)
	{
		m_ST_Curve.MoveWindow(0, 0, cx, cy, TRUE);
		m_ST_Curve.SetLegendSpace(0);
	}
}

void CTestST_Curve3View::OnPrintcurve() 
{
	COleDateTime OleTime = COleDateTime::GetCurrentTime();
	CString Title, FootNote;
	Title.Format(_T("某小区%d月份用电曲线图"), OleTime.GetMonth());
	FootNote.Format(_T("打印时间：%s  操作员：杨狼"), OleTime.Format(VAR_DATEVALUEONLY));

	m_ST_Curve.PrintCurve(12, //最后一个参数代表指定打印所有曲线，所以这里的12（曲线地址）将被忽略
						  .0, .0, 0, //BTime和ETime均无效，则打印最大时间范围
						  50, 50, 50, 50,
						  Title, FootNote,
//						  0, FootNote,
						  0x11, //Flag
//						  FALSE //one curve
						  TRUE  //all curve
						  );
}
