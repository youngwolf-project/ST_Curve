#pragma once

// ST_CurvePropPage.h : CST_CurvePropPage 属性页类的声明。


// CST_CurvePropPage : 有关实现的信息，请参阅 ST_CurvePropPage.cpp。

class CST_CurvePropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CST_CurvePropPage)
	DECLARE_OLECREATE_EX(CST_CurvePropPage)

// 构造函数
public:
	CST_CurvePropPage();

// 对话框数据
	enum { IDD = IDD_PROPPAGE_ST_CURVE };

// 实现
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 消息映射
protected:
	DECLARE_MESSAGE_MAP()
};

