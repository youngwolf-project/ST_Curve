
// TestST_Curve3View.h : CTestST_Curve3View 类的接口
//

#pragma once
#include "CDST_Curve.h"

class CTestST_Curve3View : public CView
{
protected: // 仅从序列化创建
	CTestST_Curve3View();
	DECLARE_DYNCREATE(CTestST_Curve3View)

// 特性
public:
	CTestST_Curve3Doc* GetDocument() const;

// 操作
public:

// 重写
public:
	virtual void OnDraw(CDC* pDC);  // 重写以绘制该视图
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// 实现
public:
	virtual ~CTestST_Curve3View();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CDST_Curve m_ST_Curve;

// 生成的消息映射函数
protected:
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPrintcurve();
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // TestST_Curve3View.cpp 中的调试版本
inline CTestST_Curve3Doc* CTestST_Curve3View::GetDocument() const
   { return reinterpret_cast<CTestST_Curve3Doc*>(m_pDocument); }
#endif

