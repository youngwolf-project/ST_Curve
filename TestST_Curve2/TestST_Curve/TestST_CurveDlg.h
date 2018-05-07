
// TestST_CurveDlg.h : 头文件
//

#pragma once
#include "stcurvectrl.h"

#ifdef _WIN64
#define Format64bitHandle(C, HANDLETYPE, LOW32BIT) ((HANDLETYPE) (((ULONGLONG) C.GetRegister1() << 32) + (ULONG) LOW32BIT))
#define SplitHandle(C, H) (C.SetRegister1(GetH32bit(H)), (OLE_HANDLE) H)
#define GetH32bit(H) ((OLE_HANDLE) ((ULONGLONG) H >> 32))
#else
#define Format64bitHandle(C, HANDLETYPE, LOW32BIT) ((HANDLETYPE) LOW32BIT)
#define SplitHandle(C, H) ((OLE_HANDLE) H)
#define GetH32bit(H) 0
#endif

// CTestST_CurveDlg 对话框
class CTestST_CurveDlg : public CDialogEx
{
// 构造
public:
	CTestST_CurveDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_TESTST_CURVE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	COleDateTime Time1, Time2, OleTime;
	COleDateTimeSpan TimeSpan;
	void ResetAll();

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSelchangeCombo1();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
	afx_msg void OnButton1();
	afx_msg void OnButton2();
	afx_msg void OnButton3();
	afx_msg void OnButton4();
	afx_msg void OnButton5();
	afx_msg void OnButton6();
	afx_msg void OnButton7();
	afx_msg void OnButton8();
	afx_msg void OnButton9();
	afx_msg void OnButton10();
	afx_msg void OnButton11();
	afx_msg void OnButton12();
	afx_msg void OnButton13();
	afx_msg void OnButton14();
	afx_msg void OnButton15();
	afx_msg void OnButton16();
	afx_msg void OnButton17();
	afx_msg void OnButton18();
	afx_msg void OnButton19();
	afx_msg void OnButton20();
	afx_msg void OnButton21();
	afx_msg void OnButton22();
	afx_msg void OnButton23();
	afx_msg void OnButton24();
	afx_msg void OnButton25();
	afx_msg void OnButton26();
	afx_msg void OnButton27();
	afx_msg void OnButton28();
	afx_msg void OnButton29();
	afx_msg void OnButton30();
	afx_msg void OnButton31();
	afx_msg void OnButton32();
	afx_msg void OnButton33();
	afx_msg void OnButton34();
	afx_msg void OnButton35();
	afx_msg void OnButton36();
	afx_msg void OnButton37();
	afx_msg void OnButton38();
	afx_msg void OnButton39();
	afx_msg void OnButton40();
	afx_msg void OnButton41();
	afx_msg void OnButton42();
	afx_msg void OnButton43();
	afx_msg void OnButton44();
	afx_msg void OnButton45();
	afx_msg void OnButton46();
	afx_msg void OnButton47();
	afx_msg void OnButton48();
	afx_msg void OnButton49();
	afx_msg void OnButton50();
	afx_msg void OnButton51();
	afx_msg void OnButton52();
	afx_msg void OnButton53();
	afx_msg void OnButton54();
	afx_msg void OnButton55();
	afx_msg void OnButton56();
	afx_msg void OnButton57();
	afx_msg void OnButton58();
	afx_msg void OnButton59();
	afx_msg void OnButton60();
	afx_msg void OnButton61();
	afx_msg void OnButton62();
	afx_msg void OnButton63();
	afx_msg void OnButton64();
	afx_msg void OnButton65();
	afx_msg void OnButton66();
	afx_msg void OnButton67();
	afx_msg void OnButton68();
	afx_msg void OnButton69();
	afx_msg void OnButton70();
	afx_msg void OnButton71();
	afx_msg void OnButton72();
	afx_msg void OnButton73();
	afx_msg void OnButton74();
	afx_msg void OnButton75();
	afx_msg void OnButton76();
	afx_msg void OnButton77();
	afx_msg void OnButton78();
	afx_msg void OnButton79();
	afx_msg void OnButton80();
	afx_msg void OnButton81();
	afx_msg void OnButton82();
	afx_msg void OnButton83();
	afx_msg void OnButton84();
	afx_msg void OnButton85();
	afx_msg void OnButton86();
	afx_msg void OnButton87();
	afx_msg void OnButton88();
	afx_msg void OnButton89();
	afx_msg void OnButton90();
	afx_msg void OnButton91();
	afx_msg void OnButton92();
	afx_msg void OnMouseDownStcurvectrl(short Button, short Shift, long x, long y);
	afx_msg void OnMouseMoveStcurvectrl(short Button, short Shift, long x, long y);
	afx_msg void OnMouseUpStcurvectrl(short Button, short Shift, long x, long y);
	afx_msg void OnPageChangeStcurvectrl(long wParam, long lParam);
	afx_msg void OnBeginTimeChangeStcurvectrl(DATE NewTime);
	afx_msg void OnBeginValueChangeStcurvectrl(float NewValue);
	afx_msg void OnTimeSpanChangeStcurvectrl(double NewTimeSpan);
	afx_msg void OnValueStepChangeStcurvectrl(float NewValueStep);
	afx_msg void OnZoomChangeStcurvectrl(short NewZoom);
	afx_msg void OnSelectedCurveChangeStcurvectrl(long NewId);
	afx_msg void OnLegendVisableChangeStcurvectrl(long Index, short State);
	afx_msg void OnSorptionChangeStcurvectrl(long Id, long Index, short State);
	afx_msg void OnCurveStateChangeStcurvectrl(long Id, short State);
	afx_msg void OnZoomModeChangeStcurvectrl(short NewMode);
	afx_msg void OnHZoomChangeStcurvectrl(short NewZoom);
	afx_msg void OnBatchExportImageChangeStcurvectrl(long FileNameIndex);
	DECLARE_MESSAGE_MAP()
	DECLARE_EVENTSINK_MAP()
public:
	CComboBox m_Combo;
	CStcurvectrl m_ST_Curve;
};
