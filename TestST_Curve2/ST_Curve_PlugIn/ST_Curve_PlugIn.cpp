// ST_Curve_PlugIn.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <tchar.h>
#include <assert.h>
#include <atlbase.h>
#include "ST_Curve_PlugIn.h"
#include "..\TestST_Curve\ST_Curve_PlugIn.hpp"

//注：在非UNICODE版本下，本工程将无法通过编译，不要擅自修改ST_Curve_PlugIn.hpp以求编译能过
//就算过了，也无法使用，因为ST_Curve只提供unicode版本（相比于COM，dll的弊端得到体现了）
TCHAR PlugInBuff[128];
//用TCHAR，目的是用本工程在非unicode版本下，无法通过编译

/*
extern "C"
__declspec(dllexport) LPCWSTR FormatXCoordinate(long Address, double DateTime, UINT Action)
{
	switch(Action)
	{
	case 1: //绘制坐标轴开始
		break;
	case 2: //绘制坐标轴过程中
		_stprintf(PlugInBuff, _T("%.2f\n绘制X坐标轴"), DateTime);
		break;
	case 3: //绘制坐标轴结束
		break;
	case 4: //显示坐标提示（Tooltip）
		_stprintf(PlugInBuff, _T("%.2f X坐标提示"), DateTime);
		break;
	case 5: //绘制坐标
		_stprintf(PlugInBuff, _T("%.2f 绘制X坐标"), DateTime);
		break;
	case 6: //计算Y轴位置时调用，不会被调用
		break;
	case 7: //绘制填充值，不会被调用
		break;
	default:
		*PlugInBuff = 0;
		break;
	}

	return PlugInBuff;
}
*/
//下面这个演示利用插件的技巧，让横坐标不管在多少缩放比例下，显示都一样
extern "C"
__declspec(dllexport) LPCWSTR FormatXCoordinate(long Address, DATE DateTime, UINT Action)
{
	static int nTimes = -1;
	switch(Action)
	{
	case 1: //绘制坐标轴开始
		nTimes = 0;
		break;
	case 2: //绘制坐标轴过程中
		assert(nTimes >= 0);
		_stprintf_s(PlugInBuff, _T("第%d月\n第%d日"), ++nTimes, nTimes);
		break;
	case 3: //绘制坐标轴结束
		nTimes = -1;
		break;
	case 4: //显示坐标提示（Tooltip）
		_stprintf_s(PlugInBuff, _T("%.2f X坐标提示(dll)"), DateTime);
//		*PlugInBuff = 0; //不显示在tooltip
		break;
	case 5: //绘制坐标
		_stprintf_s(PlugInBuff, _T("%.2f 绘制X坐标"), DateTime);
		break;
	case 6: //计算Y轴位置，不会被调用
	case 7: //绘制填充值，不会被调用
		break;
	default:
		*PlugInBuff = 0;
		break;
	}

	return PlugInBuff;
}

extern "C"
__declspec(dllexport) LPCWSTR FormatYCoordinate(long Address, float Value, UINT Action)
{
	switch(Action)
	{
	case 1: //绘制坐标轴开始
		break;
	case 2: //绘制坐标轴过程中
		_stprintf_s(PlugInBuff, _T("%.2f 绘制Y坐标轴"), Value);
		break;
	case 3: //绘制坐标轴结束
		break;
	case 4: //显示坐标提示（Tooltip）
//		_stprintf(PlugInBuff, _T("%.2f Y坐标提示"), Value);
		*PlugInBuff = 0; //不显示在tooltip
		break;
	case 5: //绘制坐标
		_stprintf_s(PlugInBuff, _T("%.2f 绘制Y坐标"), Value);
		break;
	case 6: //计算Y轴位置时调用
		_stprintf_s(PlugInBuff, _T("%.2f 绘制Y坐标轴"), Value);
		/*
		这里有一个技巧，这里返回的字符串，仅仅是用来计算一个长度，以便确定Y轴的位置
		所以你完全可以返回任意字符串，因为这里个数才是有意义的，字符串内容并没有什么意义
		有了这个技巧，你可以在这里返回恒定的字符串，则其长度也就是恒定的，进而Y轴的显示位置也就是恒定的了
		你还可以返回比Action等于2时返回的字符串更长，这样可以让Y坐标与Y轴空出来一定的距离
		*/
		break;
	case 7: //绘制填充值
		_stprintf_s(PlugInBuff, _T("%.0f"), Value); //无论精度是多少，都显示到整数位
		break;
	default:
		*PlugInBuff = 0;
		break;
	}

	return PlugInBuff;
}

extern "C"
__declspec(dllexport) DATE TrimXCoordinate(DATE DateTime) //序号2
{
	//这就是控件内部的实现，在插件里面仍然这样实现，是为了和以前兼容，
	//二次开发者完全可以随意修改
	_sntprintf_s(PlugInBuff, 128, _T("%.0f"), DateTime + .5); //修正到整数
	USES_CONVERSION;
	return atof(T2A(PlugInBuff));
}

extern "C"
__declspec(dllexport) float TrimYCoordinate(float Value)  //序号3
{
	//这就是控件内部的实现，在插件里面仍然这样实现，是为了和以前兼容，
	//二次开发者完全可以随意修改
	_sntprintf_s(PlugInBuff, 128, _T("%.0f"), Value + .5f); //修正到整数
	USES_CONVERSION;
	return (float) atof(T2A(PlugInBuff));
}

//这就是控件内部的实施缩放的实现，在插件里面仍然这样实现，是为了和以前兼容，
//二次开发者完全可以随意修改
#define GETSTEP(V, ZOOM) (!(ZOOM) ? V : ((ZOOM) > 0 ? V / ((ZOOM) * .25f + 1) : V * (-(ZOOM) * .25f + 1)))

extern "C"
__declspec(dllexport) double CalcTimeSpan(double TimeSpan, short Zoom, short HZoom) //序号4
{
	//这里演示一个变态用法——不缩放横坐标，以显示插件方式的灵活性
	return TimeSpan;
	//下面是控件内容的默认实现
//	return GETSTEP(TimeSpan, Zoom + HZoom);
}

extern "C"
__declspec(dllexport) float CalcValueStep(float ValueStep, short Zoom)  //序号5
{
	//这里在纵轴上演示一下自定义缩放速度，当放大时，我们放大Zoom倍，而不是控件默认的1/4倍
	return Zoom > 0 ? ValueStep / (Zoom + 1) : ValueStep * (-Zoom + 1);
	//下面是控件内容的默认实现
//	return GETSTEP(ValueStep, Zoom);
}
