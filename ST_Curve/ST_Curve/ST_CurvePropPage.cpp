// ST_CurvePropPage.cpp : CST_CurvePropPage 属性页类的实现。

#include "stdafx.h"
#include "ST_Curve.h"
#include "ST_CurvePropPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE(CST_CurvePropPage, COlePropertyPage)



// 消息映射

BEGIN_MESSAGE_MAP(CST_CurvePropPage, COlePropertyPage)
END_MESSAGE_MAP()



// 初始化类工厂和 guid

IMPLEMENT_OLECREATE_EX(CST_CurvePropPage, "STCURVE.STCurvePropPage.1",
	0xd1e97ac2, 0xde26, 0x4919, 0x87, 0xb, 0x29, 0x4d, 0xc5, 0xf3, 0xed, 0x9b)



// CST_CurvePropPage::CST_CurvePropPageFactory::UpdateRegistry -
// 添加或移除 CST_CurvePropPage 的系统注册表项

BOOL CST_CurvePropPage::CST_CurvePropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_ST_CURVE_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, nullptr);
}



// CST_CurvePropPage::CST_CurvePropPage - 构造函数

CST_CurvePropPage::CST_CurvePropPage() :
	COlePropertyPage(IDD, IDS_ST_CURVE_PPG_CAPTION)
{
}



// CST_CurvePropPage::DoDataExchange - 在页和属性间移动数据

void CST_CurvePropPage::DoDataExchange(CDataExchange* pDX)
{
	DDP_PostProcessing(pDX);
}



// CST_CurvePropPage 消息处理程序
