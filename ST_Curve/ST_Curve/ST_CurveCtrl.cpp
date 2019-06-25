// ST_CurveCtrl.cpp : CST_CurveCtrl ActiveX 控件类的实现。

#include "stdafx.h"
#include "ST_Curve.h"
#include "ST_CurveCtrl.h"
#include "ST_CurvePropPage.h"
#include "afxdialogex.h"

#include <functional>
#ifndef _USING_V110_SDK71_
#include <VersionHelpers.h>
#endif // !_USING_V110_SDK71_

#define LUA_LIB_NAME "Lua_" LUA_VERSION_MAJOR "." LUA_VERSION_MINOR "." LUA_VERSION_RELEASE

#ifdef _WIN64
#pragma comment(lib, "..\\..\\lua_5.3.5\\" LUA_LIB_NAME "x64.lib")
#else
#pragma comment(lib, "..\\..\\lua_5.3.5\\" LUA_LIB_NAME ".lib")
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static lua_State* g_L;

static COLORREF CustClr[16];
static long	nRef; //本控件已运行的实例
static TCHAR Time[16]; //时间提示符
static TCHAR Legend[16]; //图例提示符
static TCHAR OverFlow[32];
static UINT OverFlowLen;
/*
static TCHAR YMD[32]; //年、月、日打印字符串
static TCHAR HMS[32]; //时、分、秒打印字符串
static TCHAR Time2[16]; //时间提示符2
static TCHAR ValueStr[16]; //值提示符
*/
extern CST_CurveApp theApp;
static HHOOK MsgHook = nullptr; //	Handle to the Windows Message hook.
//	Hook procedure for WH_GETMESSAGE hook type.
static LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	// Switch the module state for the correct handle to be used.
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	// If this is a keystrokes message, translate it in controls' PreTranslateMessage().
	auto lpMsg = (LPMSG) lParam;
	if (nCode >= 0 && PM_REMOVE == wParam && WM_KEYFIRST <= lpMsg->message && lpMsg->message <= WM_KEYLAST && theApp.PreTranslateMessage(lpMsg))
	{
		// The value returned from this hookproc is ignored, and it cannot
		// be used to tell Windows the message has been handled. To avoid
		// further processing, convert the message to WM_NULL before returning.
		lpMsg->message = WM_NULL;
		lpMsg->wParam = 0;
		lpMsg->lParam = 0L;
	}

	// Passes the hook information to the next hook procedure in the current hook chain.
	return ::CallNextHookEx(MsgHook, nCode, wParam, lParam);
}

extern "C"
LPBITMAPINFO __stdcall GetDIBFromDDB(HDC hDC, HBITMAP hBitmap)
{
	if (!hBitmap || !hDC)
		return 0;

	BITMAP Bitmap;
	GetObject(hBitmap, sizeof(BITMAP), &Bitmap); //获取位图信息

	BITMAPINFO bi;
	memset(&bi, 0, sizeof(BITMAPINFO));
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	::GetDIBits(hDC, hBitmap, 0, Bitmap.bmHeight, 0, &bi, DIB_RGB_COLORS);
	bi.bmiHeader.biCompression = BI_RGB;

	DWORD dwPaletteSize = bi.bmiHeader.biBitCount > 8 ? 0 : sizeof(RGBQUAD) * ((1 << bi.bmiHeader.biBitCount) - 1);
	DWORD dwBmBitsSize = ((Bitmap.bmWidth * bi.bmiHeader.biBitCount + 31) & ~31) / 8 * Bitmap.bmHeight;

	LPBITMAPINFO lpbi = (LPBITMAPINFO) LocalAlloc(LMEM_FIXED, sizeof(BITMAPINFO) + dwPaletteSize + dwBmBitsSize);
	*lpbi = bi;
	::GetDIBits(hDC, hBitmap, 0, Bitmap.bmHeight,
		(LPBYTE)lpbi + sizeof(BITMAPINFO) + dwPaletteSize, lpbi, DIB_RGB_COLORS);

	return lpbi;
}
/*
static UINT CALLBACK OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	if (WM_NOTIFY == uiMsg)
		Beep(1500, 100);

	return 0;
}
*/

extern "C"
BOOL __stdcall ExportImage(HBITMAP hBitmap, LPCTSTR pFileName)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState()); //下面调用了MFC的东西，所以添加这行代码
	if (!hBitmap)
		return FALSE;

	TCHAR FileName[MAX_PATH];
	*FileName = 0;
	auto bWarn = TRUE;
	if (!IsBadStringPtr(pFileName, -1) && *pFileName) //可读，不为空
		_tcsncpy_s(FileName, pFileName, MAX_PATH - 4 - 1);

	if (!*FileName)
	{
		/*
		OPENFILENAME of;
		memset(&of, 0, sizeof(OPENFILENAME));
		of.lStructSize = sizeof(OPENFILENAME);
		of.hwndOwner = AfxGetMainWnd()->m_hWnd;
		of.lpstrFilter = _T("Microsoft Bitmap Files (*.bmp)\0*.bmp\0PNG Files (*.png)\0*.png\0JPEG Files (*.jpeg;*.jpg)\0*.jpeg;*.jpg\0GIF Files (*.gif)\0*.gif\0\0");
		of.lpstrFile = FileName;
		of.nMaxFile = MAX_PATH;
		of.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_EXPLORER;
		of.lpstrDefExt = _T("");
		if (!GetSaveFileName(&of))
			return FALSE;
		*/
		CFileDialog Save_Dialog(FALSE, _T(""), nullptr, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_EXPLORER/* | OFN_ENABLEHOOK*/,
			_T("Microsoft Bitmap Files (*.bmp)|*.bmp|PNG Files (*.png)|*.png|JPEG Files (*.jpeg;*.jpg)|*.jpeg;*.jpg|GIF Files (*.gif)|*.gif||"), nullptr);
		static DWORD nFilterIndex = 1;
		Save_Dialog.m_ofn.nFilterIndex = nFilterIndex;
//		Save_Dialog.m_ofn.lpfnHook = OFNHookProc;
		if (IDOK == Save_Dialog.DoModal())
		{
			nFilterIndex = Save_Dialog.m_ofn.nFilterIndex;
			_tcsncpy_s(FileName, Save_Dialog.GetPathName(), _TRUNCATE);
		}
		else
			return FALSE;
	}
	else //后台输出
	{
		bWarn = FALSE;

		auto Len = _tcslen(FileName);
		BOOL bNoType = Len <= 4;
		if (!bNoType)
		{
			auto pBeginPos = FileName + Len - 4;
			bNoType = 0 != _tcsicmp(pBeginPos, _T(".bmp")) && 0 != _tcsicmp(pBeginPos, _T(".png")) &&
				0 != _tcsicmp(pBeginPos, _T(".jpg")) && 0 != _tcsicmp(pBeginPos, _T(".gif"));
			if (bNoType && Len > 5)
			{
				--pBeginPos;
				bNoType = 0 != _tcsicmp(pBeginPos, _T(".jpeg"));
			}
		}
		if (bNoType) //如果没有指定格式，则按bmp导出
			_tcscat_s(FileName, _T(".bmp"));
	}

	auto re = TRUE;
	ATL::CImage Image;
	Image.Attach(hBitmap);
	auto hr = Image.Save(FileName);
	if (FAILED(hr))
	{
		re = FALSE;
		if (bWarn) //因为本控件的其它接口会调用该方法，所以增加了错误提示
			AfxMessageBox(IDS_WRITEFILEFAIL);
	}
	/*
	CFile Output;
	if (Output.Open(FileName, CFile::modeCreate | CFile::modeWrite | CFile::shareDenyRead | CFile::shareDenyWrite, nullptr))
	{
		LPBITMAPINFO lpbi = GetDIBFromDDB(hWnd, hBitmap);
		DWORD dwPaletteSize = lpbi->bmiHeader.biBitCount > 8 ? 0 : sizeof(RGBQUAD) * ((1 << lpbi->bmiHeader.biBitCount) - 1);
		//设置位图文件头
		BITMAPFILEHEADER bmfHdr;
		bmfHdr.bfType = 0x4D42; //"BM"
		bmfHdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + (DWORD) sizeof(BITMAPINFO) + dwPaletteSize;
		bmfHdr.bfSize = bmfHdr.bfOffBits + lpbi->bmiHeader.biSizeImage;
		bmfHdr.bfReserved1 = 0;
		bmfHdr.bfReserved2 = 0;

		Output.Write((LPBYTE) &bmfHdr, sizeof(BITMAPFILEHEADER)); //写入位图文件头
		Output.Write((LPBYTE) lpbi, sizeof(BITMAPINFO) + dwPaletteSize + lpbi->bmiHeader.biSizeImage); //写入位图文件其余内容
		Output.Close();

		LocalFree((HLOCAL) lpbi);
	}
	else
	{
		re = FALSE;
		if (bWarn)
			AfxMessageBox(IDS_WRITEFILEFAIL);
	}
	*/
	Image.Detach(); //重要，否则CImage会释放掉hBitmap，在本控件中之所以没有问题，是因为hBitmap被选入了dc，所以释放不掉
	//但本函数是可以外部当成dll来调用的，如果使用者传入一个未被选入dc的hBitmap进来，将造成本函数返回后，使用者再也不能
	//使用hBitmap参数了，因为它已经被彻底释放

	return re;
}

IMPLEMENT_DYNCREATE(CST_CurveCtrl, COleControl)

// 消息映射
BEGIN_MESSAGE_MAP(CST_CurveCtrl, COleControl)
	ON_WM_CREATE()
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()

// 调度映射
BEGIN_DISPATCH_MAP(CST_CurveCtrl, COleControl)
	DISP_PROPERTY_NOTIFY_ID(CST_CurveCtrl, "ForeColor", dispidForeColor, m_foreColor, OnForeColorChanged, VT_COLOR)
	DISP_PROPERTY_NOTIFY_ID(CST_CurveCtrl, "BackColor", dispidBackColor, m_backColor, OnBackColorChanged, VT_COLOR)
	DISP_PROPERTY_NOTIFY_ID(CST_CurveCtrl, "AxisColor", dispidAxisColor, m_axisColor, OnAxisColorChanged, VT_COLOR)
	DISP_PROPERTY_NOTIFY_ID(CST_CurveCtrl, "GridColor", dispidGridColor, m_gridColor, OnGridColorChanged, VT_COLOR)
	DISP_PROPERTY_NOTIFY_ID(CST_CurveCtrl, "PageChangeMSG", dispidPageChangeMSG, m_pageChangeMSG, OnPageChangeMSGChanged, VT_I4)
	DISP_PROPERTY_NOTIFY_ID(CST_CurveCtrl, "MSGRecWnd", dispidMSGRecWnd, m_MSGRecWnd, OnMSGRecWndChanged, VT_HANDLE)
	DISP_PROPERTY_NOTIFY_ID(CST_CurveCtrl, "TitleColor", dispidTitleColor, m_titleColor, OnTitleColorChanged, VT_COLOR)
	DISP_PROPERTY_NOTIFY_ID(CST_CurveCtrl, "FootNoteColor", dispidFootNoteColor, m_footNoteColor, OnFootNoteColorChanged, VT_COLOR)
	DISP_PROPERTY_NOTIFY_ID(CST_CurveCtrl, "Register1", dispidRegister1, m_register1, OnRegister1Changed, VT_HANDLE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetVInterval", dispidSetVInterval, SetVInterval, VT_BOOL, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetHInterval", dispidSetHInterval, SetHInterval, VT_BOOL, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetScaleInterval", dispidGetScaleInterval, GetScaleInterval, VT_I2, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "EnableHelpTip", dispidEnableHelpTip, EnableHelpTip, VT_EMPTY, VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetLegendSpace", dispidSetLegendSpace, SetLegendSpace, VT_BOOL, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetLegendSpace", dispidGetLegendSpace, GetLegendSpace, VT_I2, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetBeginValue", dispidSetBeginValue, SetBeginValue, VT_BOOL, VTS_R4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetBeginValue", dispidGetBeginValue, GetBeginValue, VT_R4, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetBeginTime", dispidSetBeginTime, SetBeginTime, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetBeginTime2", dispidSetBeginTime2, SetBeginTime2, VT_BOOL, HCOOR_VTS_TYPE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetBeginTime", dispidGetBeginTime, GetBeginTime, VT_BSTR, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetBeginTime2", dispidGetBeginTime2, GetBeginTime2, HCOOR_VT_TYPE, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetTimeSpan", dispidSetTimeSpan, SetTimeSpan, VT_BOOL, VTS_R8)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetTimeSpan", dispidGetTimeSpan, GetTimeSpan, VT_R8, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetValueStep", dispidSetValueStep, SetValueStep, VT_BOOL, VTS_R4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetValueStep", dispidGetValueStep, GetValueStep, VT_R4, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetVPrecision", dispidSetVPrecision, SetVPrecision, VT_BOOL, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetVPrecision", dispidGetVPrecision, GetVPrecision, VT_I2, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetUnit", dispidSetUnit, SetUnit, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetUnit", dispidGetUnit, GetUnit, VT_BSTR, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "TrimCoor", dispidTrimCoor, TrimCoor, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "AddLegend", dispidAddLegend, AddLegend, VT_I2, VTS_I4 VTS_BSTR VTS_COLOR VTS_I2 VTS_I2 VTS_COLOR VTS_I2 VTS_I2 VTS_I2 VTS_I2 VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetLegend", dispidGetLegend, GetLegend, VT_BOOL, VTS_BSTR VTS_PCOLOR VTS_PI2 VTS_PI2 VTS_PCOLOR VTS_PI2 VTS_PI2 VTS_PI2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "QueryLegend", dispidQueryLegend, QueryLegend, VT_BSTR, VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetLegendCount", dispidGetLegendCount, GetLegendCount, VT_I2, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetLegend2", dispidGetLegend2, GetLegend2, VT_BOOL, VTS_I2 VTS_PCOLOR VTS_PI2 VTS_PI2 VTS_PCOLOR VTS_PI2 VTS_PI2 VTS_PI2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetLegendIdCount", dispidGetLegendIdCount, GetLegendIdCount, VT_I2, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetLegendId", dispidGetLegendId, GetLegendId, VT_I4, VTS_I2 VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "DelLegend", dispidDelLegend, DelLegend, VT_BOOL, VTS_I4 VTS_BOOL VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "DelLegend2", dispidDelLegend2, DelLegend2, VT_BOOL, VTS_BSTR VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "AddMainData", dispidAddMainData, AddMainData, VT_I2, VTS_I4 VTS_BSTR VTS_R4 VTS_I2 VTS_I2 VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "AddMainData2", dispidAddMainData2, AddMainData2, VT_I2, VTS_I4 HCOOR_VTS_TYPE VTS_R4 VTS_I2 VTS_I2 VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetVisibleCoorRange", dispidSetVisibleCoorRange, SetVisibleCoorRange, VT_EMPTY, HCOOR_VTS_TYPE HCOOR_VTS_TYPE VTS_R4 VTS_R4 VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetVisibleCoorRange", dispidGetVisibleCoorRange, GetVisibleCoorRange, VT_EMPTY, HCOOR_VTS_PTYPE HCOOR_VTS_PTYPE VTS_PR4 VTS_PR4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "DelRange", dispidDelRange, DelRange, VT_EMPTY, VTS_I4 HCOOR_VTS_TYPE HCOOR_VTS_TYPE VTS_I2 VTS_BOOL VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "DelRange2", dispidDelRange2, DelRange2, VT_EMPTY, VTS_I4 VTS_I4 VTS_I4 VTS_BOOL VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "FirstPage", dispidFirstPage, FirstPage, VT_BOOL, VTS_BOOL VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GotoPage", dispidGotoPage, GotoPage, VT_I2, VTS_I2 VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetZoom", dispidSetZoom, SetZoom, VT_BOOL, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetZoom", dispidGetZoom, GetZoom, VT_I2, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetMaxLength", dispidSetMaxLength, SetMaxLength, VT_BOOL, VTS_I4 VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetMaxLength", dispidGetMaxLength, GetMaxLength, VT_I4, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetCutLength", dispidGetCutLength, GetCutLength, VT_I4, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetShowMode", dispidSetShowMode, SetShowMode, VT_BOOL, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetShowMode", dispidGetShowMode, GetShowMode, VT_I2, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetBkBitmap", dispidSetBkBitmap, SetBkBitmap, VT_BOOL, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetBkBitmap", dispidGetBkBitmap, GetBkBitmap, VT_I2, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetFillDirection", dispidSetFillDirection, SetFillDirection, VT_BOOL, VTS_I4 VTS_I2 VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetFillDirection", dispidGetFillDirection, GetFillDirection, VT_I2, VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetMoveMode", dispidSetMoveMode, SetMoveMode, VT_BOOL, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetMoveMode", dispidGetMoveMode, GetMoveMode, VT_I2, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetFont", dispidSetFont, SetFont, VT_EMPTY, VTS_HANDLE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "AddImageHandle", dispidAddImageHandle, AddImageHandle, VT_BOOL, VTS_BSTR VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "AddBitmapHandle", dispidAddBitmapHandle, AddBitmapHandle, VT_EMPTY, VTS_HANDLE VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "AddBitmapHandle2", dispidAddBitmapHandle2, AddBitmapHandle2, VT_BOOL, VTS_HANDLE VTS_BSTR VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "AddBitmapHandle3", dispidAddBitmapHandle3, AddBitmapHandle3, VT_BOOL, VTS_HANDLE VTS_I4 VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetBitmapCount", dispidGetBitmapCount, GetBitmapCount, VT_I4, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetBkMode", dispidSetBkMode, SetBkMode, VT_BOOL, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetBkMode", dispidGetBkMode, GetBkMode, VT_I2, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "ExportImage", dispidExportImage, ExportImage, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION_ID(CST_CurveCtrl, "ExportImageFromPage", dispidExportImageFromPage, ExportImageFromPage, VT_I4, VTS_BSTR VTS_I4 VTS_I4 VTS_I4 VTS_BOOL VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "ExportImageFromTime", dispidExportImageFromTime, ExportImageFromTime, VT_I4, VTS_BSTR VTS_I4 HCOOR_VTS_TYPE HCOOR_VTS_TYPE VTS_I2 VTS_BOOL VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "BatchExportImage", dispidBatchExportImage, BatchExportImage, VT_BOOL, VTS_BSTR VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "EnableAutoTrimCoor", dispidEnableAutoTrimCoor, EnableAutoTrimCoor, VT_EMPTY, VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "ImportFile", dispidImportFile, ImportFile, VT_I4, VTS_BSTR VTS_I2 VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetOneTimeRange", dispidGetOneTimeRange, GetOneTimeRange, VT_BOOL, VTS_I4 HCOOR_VTS_PTYPE HCOOR_VTS_PTYPE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetOneValueRange", dispidGetOneValueRange, GetOneValueRange, VT_BOOL, VTS_I4 VTS_PR4 VTS_PR4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetOneFirstPos", dispidGetOneFirstPos, GetOneFirstPos, VT_BOOL, VTS_I4 HCOOR_VTS_PTYPE VTS_PR4 VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetTimeRange", dispidGetTimeRange, GetTimeRange, VT_BOOL, HCOOR_VTS_PTYPE HCOOR_VTS_PTYPE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetValueRange", dispidGetValueRange, GetValueRange, VT_BOOL, VTS_PR4 VTS_PR4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetViableTimeRange", dispidGetViableTimeRange, GetViableTimeRange, VT_EMPTY, HCOOR_VTS_PTYPE HCOOR_VTS_PTYPE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "AddMemMainData", dispidAddMemMainData, AddMemMainData, VT_I4, VTS_HANDLE VTS_I4 VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "ShowCurve", dispidShowCurve, ShowCurve, VT_BOOL, VTS_I4 VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetFootNote", dispidSetFootNote, SetFootNote, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetFootNote", dispidGetFootNote, GetFootNote, VT_BSTR, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "TrimCurve", dispidTrimCurve, TrimCurve, VT_I4, VTS_I4 VTS_I2 VTS_I4 VTS_I4 VTS_I2 VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "PrintCurve", dispidPrintCurve, PrintCurve, VT_I2, VTS_I4 HCOOR_VTS_TYPE HCOOR_VTS_TYPE VTS_I2 VTS_I2 VTS_I2 VTS_I2 VTS_I2 VTS_BSTR VTS_BSTR VTS_I2 VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetEventMask", dispidGetEventMask, GetEventMask, VT_I4, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetScaleNums", dispidGetScaleNums, GetScaleNums, VT_I4, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "ReportPageInfo", dispidReportPageInfo, ReportPageInfo, VT_I4, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "ShowLegend", dispidShowLegend, ShowLegend, VT_BOOL, VTS_BSTR VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SelectCurve", dispidSelectCurve, SelectCurve, VT_BOOL, VTS_I4 VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "DragCurve", dispidDragCurve, DragCurve, VT_I2, VTS_I2 VTS_I2 VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "VCenterCurve", dispidVCenterCurve, VCenterCurve, VT_BOOL, VTS_I4 VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetSelectedCurve", dispidGetSelectedCurve, GetSelectedCurve, VT_BOOL, VTS_PI4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "EnableAdjustZOrder", dispidEnableAdjustZOrder, EnableAdjustZOrder, VT_EMPTY, VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "IsSelected", dispidIsSelected, IsSelected, VT_BOOL, VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "IsLegendVisible", dispidIsLegendVisible, IsLegendVisible, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION_ID(CST_CurveCtrl, "IsCurveVisible", dispidIsCurveVisible, IsCurveVisible, VT_BOOL, VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "IsCurveInCanvas", dispidIsCurveInCanvas, IsCurveInCanvas, VT_BOOL, VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GotoCurve", dispidGotoCurve, GotoCurve, VT_BOOL, VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "EnableZoom", dispidEnableZoom, EnableZoom, VT_EMPTY, VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetCurveLength", dispidGetCurveLength, GetCurveLength, VT_I4, VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetLuaVer", dispidGetLuaVer, GetLuaVer, VT_BSTR, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetTimeData", dispidGetTimeData, GetTimeData, HCOOR_VT_TYPE, VTS_I2 VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetValueData", dispidGetValueData, GetValueData, VT_R4, VTS_I2 VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetState", dispidGetState, GetState, VT_I2, VTS_I2 VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "InsertMainData", dispidInsertMainData, InsertMainData, VT_BOOL, VTS_I2 VTS_I4 VTS_BSTR VTS_R4 VTS_I2 VTS_I2 VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "InsertMainData2", dispidInsertMainData2, InsertMainData2, VT_BOOL, VTS_I2 VTS_I4 HCOOR_VTS_TYPE VTS_R4 VTS_I2 VTS_I2 VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "CanContinueEnum", dispidCanContinueEnum, CanContinueEnum, VT_BOOL, VTS_I4 VTS_I2 VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "DelPoint", dispidDelPoint, DelPoint, VT_BOOL, VTS_I2 VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetCurveCount", dispidGetCurveCount, GetCurveCount, VT_I2, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetCurve", dispidGetCurve, GetCurve, VT_I4, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "RemoveBitmapHandle", dispidRemoveBitmapHandle, RemoveBitmapHandle, VT_BOOL, VTS_HANDLE VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "RemoveBitmapHandle2", dispidRemoveBitmapHandle2, RemoveBitmapHandle2, VT_BOOL, VTS_I2 VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetBitmap", dispidGetBitmap, GetBitmap, VT_HANDLE, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetBitmapState", dispidGetBitmapState, GetBitmapState, VT_I2, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetBitmapState2", dispidGetBitmapState2, GetBitmapState2, VT_I2, VTS_HANDLE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetBuddy", dispidSetBuddy, SetBuddy, VT_BOOL, VTS_HANDLE VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetBuddyCount", dispidGetBuddyCount, GetBuddyCount, VT_I2, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetBuddy", dispidGetBuddy, GetBuddy, VT_HANDLE, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetCurveTitle", dispidSetCurveTitle, SetCurveTitle, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetCurveTitle", dispidGetCurveTitle, GetCurveTitle, VT_BSTR, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetHUnit", dispidSetHUnit, SetHUnit, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetHUnit", dispidGetHUnit, GetHUnit, VT_BSTR, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetHPrecision", dispidSetHPrecision, SetHPrecision, VT_BOOL, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetHPrecision", dispidGetHPrecision, GetHPrecision, VT_I2, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetCurveIndex", dispidSetCurveIndex, SetCurveIndex, VT_BOOL, VTS_I4 VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetCurveIndex", dispidGetCurveIndex, GetCurveIndex, VT_I2, VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetGridMode", dispidSetGridMode, SetGridMode, VT_BOOL, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetGridMode", dispidGetGridMode, GetGridMode, VT_I2, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetBenchmark", dispidSetBenchmark, SetBenchmark, VT_EMPTY, HCOOR_VTS_TYPE VTS_R4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetBenchmark", dispidGetBenchmark, GetBenchmark, VT_EMPTY, HCOOR_VTS_PTYPE VTS_PR4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetPower", dispidGetPower, GetPower, VT_I2, VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "TrimCurve2", dispidTrimCurve2, TrimCurve2, VT_I4, VTS_I4 VTS_I2 HCOOR_VTS_TYPE HCOOR_VTS_TYPE VTS_I2 VTS_I2 VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "ChangeId", dispidChangeId, ChangeId, VT_BOOL, VTS_I4 VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "CloneCurve", dispidCloneCurve, CloneCurve, VT_BOOL, VTS_I4 VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "UniteCurve", dispidUniteCurve, UniteCurve, VT_BOOL, VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "UniteCurve2", dispidUniteCurve2, UniteCurve2, VT_BOOL, VTS_I4 VTS_I4 VTS_I4 HCOOR_VTS_TYPE HCOOR_VTS_TYPE VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "UniteCurve3", dispidUniteCurve3, UniteCurve3, VT_BOOL, VTS_I4 HCOOR_VTS_TYPE VTS_I4 VTS_I4 VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "UniteCurve4", dispidUniteCurve4, UniteCurve4, VT_BOOL, VTS_I4 HCOOR_VTS_TYPE VTS_I4 HCOOR_VTS_TYPE HCOOR_VTS_TYPE VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "OffSetCurve", dispidOffSetCurve, OffSetCurve, VT_BOOL, VTS_I4 VTS_R8 VTS_R4 VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "ArithmeticOperate", dispidArithmeticOperate, ArithmeticOperate, VT_I4, VTS_I4 VTS_I4 VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "ClearTempBuff", dispidClearTempBuff, ClearTempBuff, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "PreMallocMem", dispidPreMallocMem, PreMallocMem, VT_BOOL, VTS_I4 VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetMemSize", dispidGetMemSize, GetMemSize, VT_I4, VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "IsCurve", dispidIsCurve, IsCurve, VT_BOOL, VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetSorptionRange", dispidSetSorptionRange, SetSorptionRange, VT_EMPTY, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetSorptionRange", dispidGetSorptionRange, GetSorptionRange, VT_I2, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "IsLegend", dispidIsLegend, IsLegend, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION_ID(CST_CurveCtrl, "AddLegendHelper", dispidAddLegendHelper, AddLegendHelper, VT_I2, VTS_I4 VTS_BSTR VTS_COLOR VTS_I2 VTS_I2 VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetActualPoint", dispidGetActualPoint, GetActualPoint, VT_BOOL, VTS_I4 VTS_I4 HCOOR_VTS_PTYPE VTS_PR4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetPointFromScreenPoint", dispidGetPointFromScreenPoint, GetPointFromScreenPoint, VT_I4, VTS_I4 VTS_I4 VTS_I4 VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "EnableFullScreen", dispidEnableFullScreen, EnableFullScreen, VT_EMPTY, VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetEndTime", dispidGetEndTime, GetEndTime, HCOOR_VT_TYPE, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetEndValue", dispidGetEndValue, GetEndValue, VT_R4, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetZLength", dispidSetZLength, SetZLength, VT_EMPTY, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetZLength", dispidGetZLength, GetZLength, VT_I2, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetCanvasBkBitmap", dispidSetCanvasBkBitmap, SetCanvasBkBitmap, VT_BOOL, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetCanvasBkBitmap", dispidGetCanvasBkBitmap, GetCanvasBkBitmap, VT_I2, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetLeftBkColor", dispidSetLeftBkColor, SetLeftBkColor, VT_EMPTY, VTS_COLOR)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetLeftBkColor", dispidGetLeftBkColor, GetLeftBkColor, VT_COLOR, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetBottomBkColor", dispidSetBottomBkColor, SetBottomBkColor, VT_EMPTY, VTS_COLOR)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetBottomBkColor", dispidGetBottomBkColor, GetBottomBkColor, VT_COLOR, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetZOffset", dispidSetZOffset, SetZOffset, VT_BOOL, VTS_I4 VTS_I2 VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetZOffset", dispidGetZOffset, GetZOffset, VT_I4, VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "EnableFocusState", dispidEnableFocusState, EnableFocusState, VT_EMPTY, VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetReviseToolTip", dispidSetReviseToolTip, SetReviseToolTip, VT_BOOL, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetReviseToolTip", dispidGetReviseToolTip, GetReviseToolTip, VT_I2, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "ExportMetaFile", dispidExportMetaFile, ExportMetaFile, VT_I4, VTS_BSTR VTS_I4 VTS_I4 VTS_I4 VTS_BOOL VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "LimitOnePage", dispidLimitOnePage, LimitOnePage, VT_EMPTY, VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "FixCoor", dispidFixCoor, FixCoor, VT_BOOL, HCOOR_VTS_TYPE HCOOR_VTS_TYPE VTS_R4 VTS_R4 VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetFixCoor", dispidGetFixCoor, GetFixCoor, VT_I2, HCOOR_VTS_PTYPE HCOOR_VTS_PTYPE VTS_PR4 VTS_PR4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "RefreshLimitedOrFixedCoor", dispidRefreshLimitedOrFixedCoor, RefreshLimitedOrFixedCoor, VT_BOOL, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetCanvasBkMode", dispidSetCanvasBkMode, SetCanvasBkMode, VT_BOOL, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetCanvasBkMode", dispidGetCanvasBkMode, GetCanvasBkMode, VT_I2, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "EnablePreview", dispidEnablePreview, EnablePreview, VT_EMPTY, VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetWaterMark", dispidSetWaterMark, SetWaterMark, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetSysState", dispidGetSysState, GetSysState, VT_I4, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetTension", dispidSetTension, SetTension, VT_EMPTY, VTS_R4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetTension", dispidGetTension, GetTension, VT_R4, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetFont", dispidGetFont, GetFont, VT_HANDLE, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetXYFormat", dispidSetXYFormat, SetXYFormat, VT_BOOL, VTS_BSTR VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetXYFormat", dispidGetXYFormat, GetXYFormat, VT_I2, VTS_BSTR)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetXYFormat2", dispidGetXYFormat2, GetXYFormat2, VT_I2, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "LoadPlugIn", dispidLoadPlugIn, LoadPlugIn, VT_I4, VTS_BSTR VTS_I2 VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "AppendLegendEx", dispidAppendLegendEx, AppendLegendEx, VT_BOOL, VTS_BSTR VTS_COLOR VTS_COLOR VTS_COLOR VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetLegendEx", dispidGetLegendEx, GetLegendEx, VT_BOOL, VTS_BSTR VTS_PCOLOR VTS_PCOLOR VTS_PCOLOR VTS_PI2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetLegendEx2", dispidGetLegendEx2, GetLegendEx2, VT_BOOL, VTS_I2 VTS_PCOLOR VTS_PCOLOR VTS_PCOLOR VTS_PI2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetSelectedNodeIndex", dispidGetSelectedNodeIndex, GetSelectedNodeIndex, VT_I4, VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetSelectedNodeIndex", dispidSetSelectedNodeIndex, SetSelectedNodeIndex, VT_BOOL, VTS_I4 VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "LoadLuaScript", dispidLoadLuaScript, LoadLuaScript, VT_I4, VTS_BSTR VTS_I2 VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetShortcutKeyMask", dispidSetShortcutKeyMask, SetShortcutKeyMask, VT_EMPTY, VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetShortcutKeyMask", dispidGetShortcutKeyMask, GetShortcutKeyMask, VT_I4, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetFrceHDC", dispidGetFrceHDC, GetFrceHDC, VT_HANDLE, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetBottomSpace", dispidSetBottomSpace, SetBottomSpace, VT_BOOL, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetBottomSpace", dispidGetBottomSpace, GetBottomSpace, VT_I2, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetEndTime2", dispidGetEndTime2, GetEndTime2, VT_BSTR, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetTimeData2", dispidGetTimeData2, GetTimeData2, VT_BSTR, VTS_I2 VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "AddComment", dispidAddComment, AddComment, VT_I2, HCOOR_VTS_TYPE VTS_R4 VTS_I2 VTS_I2 VTS_I2 VTS_I2 VTS_COLOR VTS_BSTR VTS_COLOR VTS_I2 VTS_I2 VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "DelComment", dispidDelComment, DelComment, VT_BOOL, VTS_I4 VTS_BOOL VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetCommentNum", dispidGetCommentNum, GetCommentNum, VT_I4, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetComment", dispidGetComment, GetComment, VT_BOOL, VTS_I4 HCOOR_VTS_PTYPE VTS_PR4 VTS_PI2 VTS_PI2 VTS_PI2 VTS_PI2 VTS_PCOLOR VTS_PBSTR VTS_PCOLOR VTS_PI2 VTS_PI2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetComment", dispidSetComment, SetComment, VT_I2, VTS_I4 HCOOR_VTS_TYPE VTS_R4 VTS_I2 VTS_I2 VTS_I2 VTS_I2 VTS_COLOR VTS_BSTR VTS_COLOR VTS_I2 VTS_I2 VTS_I2 VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SwapCommentIndex", dispidSwapCommentIndex, SwapCommentIndex, VT_BOOL, VTS_I4 VTS_I4 VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "ShowComment", dispidShowComment, ShowComment, VT_BOOL, VTS_I4 VTS_BOOL VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "IsCommentVisiable", dispidIsCommentVisiable, IsCommentVisiable, VT_BOOL, VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetEventMask", dispidSetEventMask, SetEventMask, VT_EMPTY, VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetFixedZoomMode", dispidSetFixedZoomMode, SetFixedZoomMode, VT_BOOL, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetFixedZoomMode", dispidGetFixedZoomMode, GetFixedZoomMode, VT_I2, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "FixedZoom", dispidFixedZoom, FixedZoom, VT_BOOL, VTS_I2 VTS_I2 VTS_I2 VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetCommentPosition", dispidSetCommentPosition, SetCommentPosition, VT_BOOL, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetCommentPosition", dispidGetCommentPosition, GetCommentPosition, VT_I2, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetPixelPoint", dispidGetPixelPoint, GetPixelPoint, VT_BOOL, HCOOR_VTS_TYPE VTS_R4 VTS_PI4 VTS_PI4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetMemInfo", dispidGetMemInfo, GetMemInfo, VT_BOOL, VTS_PI4 VTS_PI4 VTS_PR4 VTS_PI4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "IsCurveClosed", dispidIsCurveClosed, IsCurveClosed, VT_BOOL, VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetPosData", dispidGetPosData, GetPosData, VT_BOOL, VTS_I2 VTS_I4 VTS_PI4 VTS_PI4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "EnableHZoom", dispidEnableHZoom, EnableHZoom, VT_EMPTY, VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetHZoom", dispidSetHZoom, SetHZoom, VT_BOOL, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetHZoom", dispidGetHZoom, GetHZoom, VT_I2, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "MoveCurveToLegend", dispidMoveCurveToLegend, MoveCurveToLegend, VT_BOOL, VTS_I4 VTS_BSTR)
	DISP_FUNCTION_ID(CST_CurveCtrl, "ChangeLegendName", dispidChangeLegendName, ChangeLegendName, VT_BOOL, VTS_BSTR VTS_BSTR)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetAutoRefresh", dispidSetAutoRefresh, SetAutoRefresh, VT_BOOL, VTS_I2 VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetAutoRefresh", dispidGetAutoRefresh, GetAutoRefresh, VT_I4, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "EnableSelectCurve", dispidEnableSelectCurve, EnableSelectCurve, VT_EMPTY, VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetToolTipDelay", dispidSetToolTipDelay, SetToolTipDelay, VT_EMPTY, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetToolTipDelay", dispidGetToolTipDelay, GetToolTipDelay, VT_I2, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetLimitOnePageMode", dispidSetLimitOnePageMode, SetLimitOnePageMode, VT_BOOL, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetLimitOnePageMode", dispidGetLimitOnePageMode, GetLimitOnePageMode, VT_I2, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "AddInfiniteCurve", dispidAddInfiniteCurve, AddInfiniteCurve, VT_BOOL, VTS_I4 HCOOR_VTS_TYPE VTS_R4 VTS_I2 VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "DelInfiniteCurve", dispidDelInfiniteCurve, DelInfiniteCurve, VT_BOOL, VTS_I4 VTS_BOOL VTS_BOOL)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetGraduationSize", dispidSetGraduationSize, SetGraduationSize, VT_BOOL, VTS_I4)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetGraduationSize", dispidGetGraduationSize, GetGraduationSize, VT_I4, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetMouseWheelMode", dispidSetMouseWheelMode, SetMouseWheelMode, VT_EMPTY, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetMouseWheelMode", dispidGetMouseWheelMode, GetMouseWheelMode, VT_I2, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetMouseWheelSpeed", dispidSetMouseWheelSpeed, SetMouseWheelSpeed, VT_BOOL, VTS_I2)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetMouseWheelSpeed", dispidGetMouseWheelSpeed, GetMouseWheelSpeed, VT_I2, VTS_NONE)
	DISP_FUNCTION_ID(CST_CurveCtrl, "SetHLegend", dispidSetHLegend, SetHLegend, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION_ID(CST_CurveCtrl, "GetHLegend", dispidGetHLegend, GetHLegend, VT_BSTR, VTS_NONE)
	DISP_STOCKFUNC_REFRESH()
END_DISPATCH_MAP()



// 事件映射

BEGIN_EVENT_MAP(CST_CurveCtrl, COleControl)
	EVENT_CUSTOM("PageChange", FirePageChange, VTS_I4 VTS_I4)
	EVENT_CUSTOM("BeginTimeChange", FireBeginTimeChange, HCOOR_VTS_TYPE)
	EVENT_CUSTOM("BeginValueChange", FireBeginValueChange, VTS_R4)
	EVENT_CUSTOM("TimeSpanChange", FireTimeSpanChange, VTS_R8)
	EVENT_CUSTOM("ValueStepChange", FireValueStepChange, VTS_R4)
	EVENT_CUSTOM("ZoomChange", FireZoomChange, VTS_I2)
	EVENT_CUSTOM("SelectedCurveChange", FireSelectedCurveChange, VTS_I4)
	EVENT_CUSTOM("LegendVisableChange", FireLegendVisableChange, VTS_I4 VTS_I2)
	EVENT_CUSTOM("SorptionChange", FireSorptionChange, VTS_I4 VTS_I4 VTS_I2)
	EVENT_CUSTOM("CurveStateChange", FireCurveStateChange, VTS_I4 VTS_I2)
	EVENT_CUSTOM("ZoomModeChange", FireZoomModeChange, VTS_I2)
	EVENT_CUSTOM("HZoomChange", FireHZoomChange, VTS_I2)
	EVENT_CUSTOM("BatchExportImageChange", FireBatchExportImageChange, VTS_I4)
	EVENT_STOCK_MOUSEDOWN()
	EVENT_STOCK_MOUSEMOVE()
	EVENT_STOCK_MOUSEUP()
END_EVENT_MAP()



// 属性页

// TODO: 按需要添加更多属性页。请记住增加计数!
BEGIN_PROPPAGEIDS(CST_CurveCtrl, 1)
	PROPPAGEID(CST_CurvePropPage::guid)
END_PROPPAGEIDS(CST_CurveCtrl)



// 初始化类工厂和 guid

IMPLEMENT_OLECREATE_EX(CST_CurveCtrl, "STCURVE.STCurveCtrl.1",
	0x315e7f0e, 0x6f9c, 0x41a3, 0xa6, 0x69, 0xa7, 0xe9, 0x62, 0x6d, 0x7c, 0xa0)



// 键入库 ID 和版本

IMPLEMENT_OLETYPELIB(CST_CurveCtrl, _tlid, _wVerMajor, _wVerMinor)



// 接口 ID

const IID IID_DST_Curve = { 0xb8f65d5c, 0xca0b, 0x494f, { 0x8b, 0x39, 0x6c, 0xb7, 0xe1, 0xa, 0x2d, 0xd4 } };
const IID IID_DST_CurveEvents = { 0x890ba0f6, 0x1786, 0x4e90, { 0x93, 0xe9, 0xf3, 0xc5, 0x24, 0xe1, 0xd0, 0xdc } };


// 控件类型信息

static const DWORD _dwST_CurveOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CST_CurveCtrl, IDS_ST_CURVE, _dwST_CurveOleMisc)



// CST_CurveCtrl::CST_CurveCtrlFactory::UpdateRegistry -
// 添加或移除 CST_CurveCtrl 的系统注册表项

BOOL CST_CurveCtrl::CST_CurveCtrlFactory::UpdateRegistry(BOOL bRegister)
{
	// TODO: 验证您的控件是否符合单元模型线程处理规则。
	// 有关更多信息，请参考 MFC 技术说明 64。
	// 如果您的控件不符合单元模型规则，则
	// 必须修改如下代码，将第六个参数从
	// afxRegApartmentThreading 改为 0。

	if (bRegister)
		return AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			m_clsid,
			m_lpszProgID,
			IDS_ST_CURVE,
			IDB_ST_CURVE,
			afxRegApartmentThreading,
			_dwST_CurveOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}

static HCOOR_TYPE TrimDateTime(HCOOR_TYPE DateTime) //规整时间
{
	TCHAR StrBuff[StrBuffLen];
	_stprintf_s(StrBuff, _T("%.0f")/*HPrecisionExp*/, DateTime + .5);
	return _tcstod(StrBuff, nullptr);
}

static float TrimValue(float Value) //规整值
{
	TCHAR StrBuff[StrBuffLen];
	_stprintf_s(StrBuff , _T("%.0f")/*VPrecisionExp*/, Value + .5f);
	return (float) _tcstod(StrBuff, nullptr);
}

// CST_CurveCtrl::CST_CurveCtrl - 构造函数

CST_CurveCtrl::CST_CurveCtrl()
{
	InitializeIIDs(&IID_DST_Curve, &IID_DST_CurveEvents);

	auto pApp = AfxGetApp();
	hZoomIn = pApp->LoadCursor(IDC_ZOOMIN);
//	hZoomIn = pApp->LoadStandardCursor(IDC_SIZENS);
	hZoomOut = pApp->LoadCursor(IDC_ZOOMOUT);
	hMove = pApp->LoadCursor(IDC_MOVE);
	hDrag = pApp->LoadCursor(IDC_DRAG);
	MouseMoveMode = 0; //自由模式
	Zoom = 0; //不缩放
	HZoom = 0; //不缩放
	pWebBrowser = 0;
	Tension = .5f; //绘制基数样条曲线时的系数
	SysState = 0x805C2C08; //允许选中曲线、允许缩放、允许坐标提示、自动更改曲线ZOrder、显示帮助、显示纵横网格、显示焦点状态、显示全局位置窗口
	EventState = 0; //默认不开启任何事件
	m_iMSGRecWnd = nullptr;
	m_MSGRecWnd = m_register1 = 0;
	hAxisPen = nullptr;
	hScreenRgn = nullptr;
	hFont = nullptr;
	hTitleFont = nullptr;
	nBkBitmap = -1;
	hFrceBmp = hBackBmp = nullptr;
	hFrceDC = hBackDC = hTempDC = nullptr;
	fWidth = fHeight = 0; //这里赋0是必须的，代表是第一次调用SetFont函数

	m_gdiplusToken = 0;

	nZLength = 0; //Z轴长度，单位为刻度数
	LeftBkColor = RGB(130, 130, 130);
	BottomBkColor = RGB(200, 200, 200);
	nCanvasBkBitmap = -1; //画布背景位图

	hBuddyServer = nullptr;
	pBuddys = nullptr;
	BuddyState = 0;
	/*已经放弃添加此功能，因为二次开发者来实现此功能将更加的灵活多样
	memset(pMoveBuddy, 0, sizeof(pMoveBuddy));
	*/

	SorptionRange = -1; //无效的吸附范围
	LastMousePoint.x = -1; //在屏幕外面，代表没有上一次鼠标坐标
	CurveTitle[0] = 0;
	FootNote[0] = 0;
	WaterMark[0] = 0;
	m_BE = nullptr;

	m_ShowMode = 0; //从低位起，第一位：1－X轴显示在右边，0－X轴显示在左边；第二位：1－Y轴显示在上边，0－Y轴显示在下边
	m_MoveMode = 7; //快速移动模式，可在水平和垂直方向上移动
	m_CanvasBkMode = m_BkMode = 0; //平铺位图
	m_ReviseToolTip = 0; //始终按Z坐标等于0来校正坐标提示

	MaxLength = -1; //不限曲线最大点数
	CutLength = 0;
	nVisibleCurve = 0;

	m_hInterval = 3; m_vInterval = 0;
	m_LegendSpace = 30;
	LeftSpace = 0;
	BottomSpaceLine = 2; //下空白两行
	CommentPosition = 1; //最高层
	VCoorData.fCurStep = VCoorData.fStep = 1.0f;
	HCoorData.fCurStep = HCoorData.fStep = .5 / 24;
	m_MinTime = MAXTIME;
	m_MinValue = 1.0f;
	m_MaxTime = MINTIME;
	m_MaxValue = -1.0f;

	OriginPoint.Time = TrimDateTime(COleDateTime::GetCurrentTime());
	OriginPoint.Value = .0f;

	_tcscpy_s(VPrecisionExp, _T("%.2f"));
	_tcscpy_s(HPrecisionExp, _T("%.2f"));

	auto hResource = AfxGetResourceHandle();
	*Unit = *HUnit = 0;

	if (!*Time) //第一次，装载全局变量
	{
		LoadString(hResource, IDS_TIME, Time, sizeof(Time) / sizeof(TCHAR));
		LoadString(hResource, IDS_LEGEND, Legend, sizeof(Legend) / sizeof(TCHAR));
//		LoadString(hResource, IDS_TIME2, Time2, sizeof(Time2) / sizeof(TCHAR));
//		LoadString(hResource, IDS_VALUE, ValueStr, sizeof(ValueStr) / sizeof(TCHAR));
		LoadString(hResource, IDS_OVERFLOW, OverFlow, sizeof(OverFlow) / sizeof(TCHAR));

		OverFlowLen = (UINT) _tcslen(OverFlow);
#ifdef _UNICODE
	#if 1 == LANG_TYPE
		size_t len = 0;
		wcstombs_s(&len, nullptr, 0, OverFlow, _TRUNCATE);
		OverFlowLen = (UINT) len - 1;
	#endif
#endif
	}

	CurveTitleRect.left = CurveTitleRect.top = 0;
	FootNoteRect.left = 0;
	UnitRect.top = LegendMarkRect.top = 2;
	HLabelRect.left = 0;
	UnitRect.left = VLabelRect.left = LEFTSPACE;

	CurCurveIndex = -1; //当前没有选中的曲线
	points.reserve(512); //默认分配512个空间

	//第1类插件
	pFormatXCoordinate = nullptr;
	pFormatYCoordinate = nullptr;
	pTrimXCoordinate = nullptr;
	pTrimYCoordinate = nullptr;
	pCalcTimeSpan = nullptr;
	pCalcValueStep = nullptr;
	hPlugIn = nullptr;

	ShortcutKey = ALL_SHORTCUT_KEY; //所有快捷键都开启
	LimitOnePageMode = 0;
	AutoRefresh = 0;
	ToolTipDely = SHOWDELAY;

	VSTEP = 21; //纵坐标屏幕步长
	HSTEP = 21; //横坐标屏幕步长

	//0-垂直移动， 1-缩放，2-水平缩放， 3-水平移动
	//从低位起，每两位为一组，依次是：shift alt ctrl和不按键
	SetMouseWheelMode(1 + (2 << 2) + (3 << 4) + (0 << 6));
	MouseWheelSpeed = 1;
}

// CST_CurveCtrl::~CST_CurveCtrl - 析构函数

CST_CurveCtrl::~CST_CurveCtrl()
{
	DeleteGDIObject();
}

// CST_CurveCtrl::OnDraw - 绘图函数

void CST_CurveCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid) //打印时不会调用本函数
{
	if (!hFrceDC) //这句用于解决在 AcitveX Control Test Container 中出错的问题
		return;

	if (SysState & 0x200) //有新东西需要刷新
	{
		SysState &= ~0x200;
		UpdateRect(hFrceDC, AllRectMask); //没有记录哪些需要刷新，所以只能全部刷新
	}
	else
	{
		BitBlt(pdc->m_hDC, rcInvalid.left, rcInvalid.top, rcInvalid.Width(), rcInvalid.Height(), hFrceDC, rcInvalid.left, rcInvalid.top, SRCCOPY);

		if (SysState & 0x100)
		{
			InvalidRect = rcInvalid & rcBounds;
			if (!IsRectEmpty(&InvalidRect))
				REFRESHFOCUS(HDC hDC = pdc->m_hDC, ;);
		}

		if (LastMousePoint.x > -1)
		{
			InvalidRect = rcInvalid & CanvasRect[0];
			if (!IsRectEmpty(&InvalidRect))
				RepairAcrossLine(pdc->m_hDC, &InvalidRect);
		}
	}
}

void CST_CurveCtrl::DrawTime(HDC hDC)
{
	if (m_ShowMode & 0x80)
	{
		if (!*HUnit)
			return;

		auto x = TimeRect.left, y = TimeRect.top;
		if (m_ShowMode & 1)
		{
			SIZE size = {0};
			GetTextExtentPoint32(hDC, HUnit, (int) _tcslen(HUnit), &size);
			x += size.cx;
		}
		if (m_ShowMode & 2)
			y += fHeight;

		TextOut(hDC, x, y, HUnit, (int) _tcslen(HUnit));
	}
	else if (*Time)
	{
		auto x = TimeRect.left, y = TimeRect.top;
		if (m_ShowMode & 1)
		{
			SIZE size = {0};
			GetTextExtentPoint32(hDC, Time, (int) _tcslen(Time), &size);
			x += size.cx;
		}
		if (m_ShowMode & 2)
			y += fHeight;

		TextOut(hDC, x, y, Time, (int) _tcslen(Time));
	}
}

void CST_CurveCtrl::DrawLegendSign(HDC hDC)
{
	if (!*Legend)
		return;

	auto x = LegendMarkRect.left, y = LegendMarkRect.top;
	if (m_ShowMode & 1)
	{
		SIZE size = {0};
		GetTextExtentPoint32(hDC, Legend, (int) _tcslen(Legend), &size);
		x += size.cx;
	}
	if (m_ShowMode & 2)
		y += fHeight;

	TextOut(hDC, x, y, Legend, (int) _tcslen(Legend));
}

void CST_CurveCtrl::DrawUnit(HDC hDC)
{
	if (!*Unit)
		return;

	auto x = UnitRect.left, y = UnitRect.top;
	if (m_ShowMode & 1)
	{
		SIZE size = {0};
		GetTextExtentPoint32(hDC, Unit, (int) _tcslen(Unit), &size);
		x += size.cx;
	}
	if (m_ShowMode & 2)
		y += fHeight;

	TextOut(hDC, x, y, Unit, (int) _tcslen(Unit));
}

void CST_CurveCtrl::DrawCurveTitle(HDC hDC)
{
	auto tr = CurveTitleRect;
	if (m_ShowMode & 1)
	{
		DrawText(hDC, CurveTitle, -1, &tr, DT_CALCRECT | DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		auto step = tr.left - tr.right;
		tr = CurveTitleRect;
		OffsetRect(&tr, step, 0);
		InflateRect(&tr, step, 0); //居中时，这两条语句保证在映射坐标后，仍然处于居中，并且显示的内容一样多
	}
//	Rectangle(hDC, tr.left, tr.top, tr.right, tr.bottom);
	DrawText(hDC, CurveTitle, -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

static BOOL IsColorsSimilar(COLORREF lc, COLORREF rc) //每个颜色分量的差的绝对值加起来，小于60的为相似
{
	auto sum = 0;
	auto diff = (int) (lc & 0xff) - (int) (rc & 0xff);
	if (diff < 0)
		diff = -diff;
	sum += diff;

	diff = (int) ((lc & 0xff00) >> 8) - (int) ((rc & 0xff00) >> 8);
	if (diff < 0)
		diff = -diff;
	sum += diff;

	diff = (int) ((lc & 0xff0000) >> 16) - (int) ((rc & 0xff0000) >> 16);
	if (diff < 0)
		diff = -diff;
	sum += diff;

	return sum < 60; //小于60的为相似
}

void CST_CurveCtrl::DrawFootNote(HDC hDC)
{
	auto tr = FootNoteRect;
	if (m_ShowMode & 1)
	{
		DrawText(hDC, FootNote, -1, &tr, DT_CALCRECT | DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		auto step = tr.left - tr.right;
		tr = FootNoteRect;
		OffsetRect(&tr, step, 0);
		InflateRect(&tr, step, 0);
	}
//	Rectangle(hDC, tr.left, tr.top, tr.right, tr.bottom);
	DrawText(hDC, FootNote, -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	auto pCPInfo = _T("ST_Curve (C)2008-2018 young wolf");
	auto OldColor = GetTextColor(hDC);
	SetThreeQuarterColor(hDC);
	if (m_ShowMode & 1)
	{
		DrawText(hDC, pCPInfo, -1, &tr, DT_CALCRECT | DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
		auto step = tr.left - tr.right;
		tr = FootNoteRect;
		OffsetRect(&tr, step, 0);
		tr.left -= step; //居右时，这两条语句保证在映射坐标后，仍然处于居右，并且显示的内容一样多
	}
	else
		tr = FootNoteRect;
//	Rectangle(hDC, tr.left, tr.top, tr.right, tr.bottom);
	DrawText(hDC, pCPInfo, -1, &tr, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
	SetTextColor(hDC, OldColor);
}

void CST_CurveCtrl::DrawVAxis(HDC hDC)
{
	auto pPoint = new POINT[(VCoorData.nScales + 1) * 2 + 5]; //加5是坐标轴的两条线
	auto pNum = new DWORD[VCoorData.nScales + 1 + 2]; //加2是坐标轴的两条线

	//“/”
	pPoint[0].x = LeftSpace - 5;
	pPoint[0].y = TopSpace + 10;
	pPoint[1].x = LeftSpace;
	pPoint[1].y = TopSpace;
	pNum[0] = 2;

	//“\”
	pPoint[2].x = LeftSpace + 5;
	pPoint[2].y = TopSpace + 10;
	pPoint[3] = pPoint[1];
	//“|”
	pPoint[4].x = LeftSpace;
	pPoint[4].y = WinHeight - BottomSpace;
	pNum[1] = 3;

	int n = 0, len;
	auto iy = WinHeight - BottomSpace - 5 - 1;
	while (iy > TopSpace + 10)
	{
		if (!(n % (m_vInterval + 1)))
			len = 5;
		else
			len = 2;
		pPoint[5 + 2 * n].x = LeftSpace + len;
		pPoint[5 + 2 * n].y = iy;
		pPoint[5 + 2 * n + 1].x = LeftSpace;
		pPoint[5 + 2 * n + 1].y = iy;
		pNum[2 + n] = 2;

		iy -= VSTEP;

		++n;
	}

	SelectObject(hDC, hAxisPen);
	PolyPolyline(hDC, pPoint, pNum, VCoorData.nScales + 1 + 2);

	delete[] pPoint;
	delete[] pNum;
}

void CST_CurveCtrl::DrawHAxis(HDC hDC)
{
	auto iy = WinHeight - BottomSpace;

	auto pPoint = new POINT[(HCoorData.nScales + 1) * 2 + 5]; //加5是坐标轴的两条线
	auto pNum = new DWORD[HCoorData.nScales + 1 + 2]; //加2是坐标轴的两条线

	//“\”
	pPoint[0].x = WinWidth - RightSpace - 10;
	pPoint[0].y = iy - 5;
	pPoint[1].x = WinWidth - RightSpace;
	pPoint[1].y = iy;
	pNum[0] = 2;

	//“/”
	pPoint[2].x = WinWidth - RightSpace - 10;
	pPoint[2].y = iy + 5;
	pPoint[3] = pPoint[1];
	//“-”
	pPoint[4].x = LeftSpace - 1;
	pPoint[4].y = iy;
	pNum[1] = 3;

	int n = 0, len;
	auto ix = LeftSpace + 5 + 1;
	while (ix < WinWidth - RightSpace - 10)
	{
		if (!(n % (m_hInterval + 1)))
			len = 5;
		else
			len = 2;
		pPoint[5 + 2 * n].x = ix;
		pPoint[5 + 2 * n].y = iy - len;
		pPoint[5 + 2 * n + 1].x = ix;
		pPoint[5 + 2 * n + 1].y = iy;
		pNum[2 + n] = 2;

		ix += HSTEP;

		++n;
	}

	SelectObject(hDC, hAxisPen);
	PolyPolyline(hDC, pPoint, pNum, HCoorData.nScales + 1 + 2);

	delete[] pPoint;
	delete[] pNum;
}

void CST_CurveCtrl::DrawVLabel(HDC hDC)
{
	auto iy = WinHeight - BottomSpace - 5 - 1;
	auto fy = OriginPoint.Value;
	auto VLabelBegin = LEFTSPACE - (m_ShowMode & 1 ? 1 : 0);
	auto VLabelEnd = LeftSpace - 5 - (m_ShowMode & 1 ? 1 : 0);
	auto n = 0, nIndex = 0;

	if (pFormatYCoordinate) //插件
		pFormatYCoordinate(0x7fffffff, .0f, 1);

	while (iy > TopSpace + 10)
	{
		if (!(n % (m_vInterval + 1))  && nIndex < VCoorData.nPolyText &&
			(!(VCoorData.RangeMask & 1) || fy >= VCoorData.fMinVisibleValue) && (!(VCoorData.RangeMask & 2) || fy <= VCoorData.fMaxVisibleValue))
		{
			if (pFormatYCoordinate) //插件
			{
				//如果使用插件控制输出，则本控件忽略坐标是显示为值还是时间，其它原本由控件控制的还照旧，比如坐标显示范围等
				auto pStr = pFormatYCoordinate(0x7fffffff, fy, 2);
				ASSERT(pStr);

				if (!pStr)
					pStr = _T("");

				//由于必须要写一个结束符，所以只能复制PolyTextLen - 1个字符
				//用_sntprintf_s代替_tcsncpy_s，以减少一次_tcslen调用
				VCoorData.pPolyTexts[nIndex].n = _sntprintf_s(VCoorData.pTexts[nIndex], _TRUNCATE, _T("%s"), pStr);
			}
			else
				//虽然有n成员，pPolyTexts里面可以没有结束符，但_sntprintf_s必须要写一个结束符，这也是新版本crt安全性方面的考虑
				//由于上面的原因，vc6版本下面的控件将能够多显示一个字符
				VCoorData.pPolyTexts[nIndex].n = _sntprintf_s(VCoorData.pTexts[nIndex], _TRUNCATE, VPrecisionExp, fy);

			adjust_poly_len(VCoorData);

			if (m_ShowMode & 1)
				//插件 右对齐，以免转化为窄字符集并求长度
				//变宽字符显然也做相同处理，否则计算显示宽度会有消耗
				if (pFormatYCoordinate || VARIABLE_PITCH == FontPitch)
					VCoorData.pPolyTexts[nIndex].x = VLabelEnd;
				else //等宽
				{
					VCoorData.pPolyTexts[nIndex].x = VLabelBegin;
					VCoorData.pPolyTexts[nIndex].x += fWidth * VCoorData.pPolyTexts[nIndex].n;
				}
			else
				//插件 左对齐，以免转化为窄字符集并求长度
				//变宽字符显然也做相同处理，否则计算显示宽度会有消耗
				if (pFormatYCoordinate || VARIABLE_PITCH == FontPitch)
					VCoorData.pPolyTexts[nIndex].x = VLabelBegin;
				else
				{
					VCoorData.pPolyTexts[nIndex].x = VLabelEnd;
					VCoorData.pPolyTexts[nIndex].x -= fWidth * VCoorData.pPolyTexts[nIndex].n;
				}
			VCoorData.pPolyTexts[nIndex].y = iy - fHeight / 2;

			if (m_ShowMode & 2)
				VCoorData.pPolyTexts[nIndex].y += fHeight;

			++nIndex;
		}

		fy += VCoorData.fCurStep;
		iy -= VSTEP;

		++n;
	}

	if (pFormatYCoordinate) //插件
		pFormatYCoordinate(0x7fffffff, .0f, 3);

	PolyTextOut(hDC, VCoorData.pPolyTexts, nIndex);
}

#ifdef _UNICODE
#define DATETIMEFORMAT	L"%s"
#else
#define DATETIMEFORMAT	"%S"
#endif
void CST_CurveCtrl::DrawHLabel(HDC hDC)
{
	if (BottomSpaceLine <= 0) //至少应该要输出一行
		return;

	auto ix = LeftSpace + 5 + 1;
	auto dx = OriginPoint.Time;
	auto hd = m_ShowMode & 1 ? -1 : 1;
	auto iy = HLabelRect.top + (m_ShowMode & 2 ? fHeight : 0);
	auto n = 0, nIndex = 0, row = 1;
	auto bOverflow = FALSE, bDrawLabel = TRUE;

	if (pFormatXCoordinate) //插件
		pFormatXCoordinate(0x7fffffff, .0, 1);

	while (ix < WinWidth - RightSpace - 10)
	{
		if (!(n % (m_hInterval + 1)) && nIndex < HCoorData.nPolyText &&
			(!(HCoorData.RangeMask & 1) || dx >= HCoorData.fMinVisibleValue) && (!(HCoorData.RangeMask & 2) || dx <= HCoorData.fMaxVisibleValue))
			if (pFormatXCoordinate) //插件
			{
				//如果使用插件控制输出，则本控件忽略坐标是显示为值还是时间，其它原本由控件控制的还照旧，比如坐标显示范围等
				auto pStr = pFormatXCoordinate(0x7fffffff, dx, 2);
				ASSERT(pStr);

				const TCHAR* pEnter = nullptr;
				if (!pStr)
					pStr = _T("");
				else
					pEnter = _tcschr(pStr, _T('\n'));

				size_t len = PolyTextLen - 1;
				if (pEnter)
				{
					len = min(len, (size_t) (pEnter - pStr));
					++pEnter;
				}

				HCoorData.pPolyTexts[nIndex].n = _sntprintf_s(HCoorData.pTexts[nIndex], len, _T("%s"), pStr);
				adjust_poly_len2(HCoorData.pPolyTexts[nIndex].n, (UINT) len); //并非是因为目的缓存满

				auto n = HCoorData.pPolyTexts[nIndex].n;
				//考虑到插件里面可能有双字节字符，所以下面要先转化为窄字符集在求字符串长度
				//非插件的情况下，因为没有双字节字符，所以不需要此步骤
#ifdef _UNICODE
	#if 1 == LANG_TYPE
				wcstombs_s(&len, nullptr, 0, HCoorData.pTexts[nIndex], _TRUNCATE);
				n = (UINT) len - 1;
	#endif
#endif
				HCoorData.pPolyTexts[nIndex].x = ix - n * fWidth / 2 * hd;
				HCoorData.pPolyTexts[nIndex++].y = iy;

				if (pEnter && BottomSpaceLine > 1)
				{
					row = 2;

					HCoorData.pPolyTexts[nIndex].n = _sntprintf_s(HCoorData.pTexts[nIndex], _TRUNCATE, _T("%s"), pEnter);
					adjust_poly_len(HCoorData);

					n = HCoorData.pPolyTexts[nIndex].n;
#ifdef _UNICODE
	#if 1 == LANG_TYPE
					wcstombs_s(&len, nullptr, 0, HCoorData.pTexts[nIndex], _TRUNCATE);
					n = (UINT) len - 1;
	#endif
#endif
					HCoorData.pPolyTexts[nIndex].x = ix - n * fWidth / 2 * hd;
					HCoorData.pPolyTexts[nIndex++].y = iy + fHeight + 2;
				}
				else
					HCoorData.pPolyTexts[nIndex++].n = 0;
			} //插件
			else if (m_ShowMode & 0x80) //显示为值
			{
				if (m_ShowMode & 0xC)
				{
					bDrawLabel = FALSE;
					break;
				}

				HCoorData.pPolyTexts[nIndex].n = _sntprintf_s(HCoorData.pTexts[nIndex], _TRUNCATE, HPrecisionExp, dx);
				adjust_poly_len(HCoorData);
				HCoorData.pPolyTexts[nIndex].x = ix - HCoorData.pPolyTexts[nIndex].n * fWidth / 2 * hd;
				HCoorData.pPolyTexts[nIndex].y = iy;

				HCoorData.pPolyTexts[nIndex + 1].n = 0;
				nIndex += 2;
			}
			else if (!bOverflow && ISHVALUEVALID(dx))
			{
				if (0xC == (m_ShowMode & 0xC))
				{
					bDrawLabel = FALSE;
					break;
				}

				BSTR bstr;
				HRESULT hr;

				auto OldIndex = nIndex;
				if (!(m_ShowMode & 4))
				{
					hr = VarBstrFromDate(dx, LANG_USER_DEFAULT, VAR_DATEVALUEONLY, &bstr);
					ASSERT(SUCCEEDED(hr));
					HCoorData.pPolyTexts[nIndex].n = _sntprintf_s(HCoorData.pTexts[nIndex], _TRUNCATE, DATETIMEFORMAT, bstr);
					adjust_poly_len(HCoorData);
					HCoorData.pPolyTexts[nIndex].x = ix - HCoorData.pPolyTexts[nIndex].n * fWidth / 2 * hd; //这里假设时间字符串里面没有汉字，如果有汉字，会偏移居中位置
					HCoorData.pPolyTexts[nIndex].y = iy;
					SysFreeString(bstr);
					++nIndex;
				}

				if (!(m_ShowMode & 8) && (BottomSpaceLine > 1 || m_ShowMode & 4))
				{
					hr = VarBstrFromDate(dx, LANG_USER_DEFAULT, VAR_TIMEVALUEONLY, &bstr);
					ASSERT(SUCCEEDED(hr));
					HCoorData.pPolyTexts[nIndex].n = _sntprintf_s(HCoorData.pTexts[nIndex], _TRUNCATE, DATETIMEFORMAT, bstr);
					adjust_poly_len(HCoorData);
					HCoorData.pPolyTexts[nIndex].x = ix - HCoorData.pPolyTexts[nIndex].n * fWidth / 2 * hd;
					HCoorData.pPolyTexts[nIndex].y = iy;
					if (nIndex > OldIndex)
						HCoorData.pPolyTexts[nIndex].y += fHeight + 2;
					SysFreeString(bstr);
					++nIndex;
				}

				while (nIndex < OldIndex + 2)
					HCoorData.pPolyTexts[nIndex++].n = 0;
			}
			else //溢出
			{
				bOverflow = TRUE;
				HCoorData.pPolyTexts[nIndex].n = _stprintf_s(HCoorData.pTexts[nIndex], _T("%s"), OverFlow);
				HCoorData.pPolyTexts[nIndex].x = ix - OverFlowLen * fWidth / 2 * hd; //这里可以做到居中显示，因为是做过处理的，在本类的构造函数里面
				HCoorData.pPolyTexts[nIndex].y = iy;

				HCoorData.pPolyTexts[nIndex + 1].n = 0;
				nIndex += 2;
			}

		dx += HCoorData.fCurStep;
		ix += HSTEP;

		++n;
	} //while (ix < WinWidth - RightSpace - 10)

	if (pFormatXCoordinate) //插件
		pFormatXCoordinate(0x7fffffff, .0, 3);

	if (bDrawLabel)
		PolyTextOut(hDC, HCoorData.pPolyTexts, nIndex);

	auto LeftLine = (int) BottomSpaceLine - 1 - row;
	for (auto iter = begin(HLegend); LeftLine > 0 && iter < end(HLegend); --LeftLine, ++iter)
	{
		auto tr = FootNoteRect;
		OffsetRect(&tr, 0, HLabelRect.bottom - FootNoteRect.top);
		if (m_ShowMode & 1)
		{
			DrawText(hDC, *iter, -1, &tr, DT_CALCRECT | DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			auto step = tr.left - tr.right;
			tr = FootNoteRect;
			OffsetRect(&tr, step, 0);
			InflateRect(&tr, step, 0);
		}
		OffsetRect(&tr, 0, -LeftLine * fHeight);
//		Rectangle(hDC, tr.left, tr.top, tr.right, tr.bottom);
		DrawText(hDC, *iter, -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}
}

void CST_CurveCtrl::DrawLegend(HDC hDC)
{
	auto iy = TopSpace;
	auto yStep = m_ShowMode & 2 ? fHeight : 0;
	for (auto i = begin(LegendArr); i < end(LegendArr) && iy < WinHeight - BottomSpace - fHeight; ++i)
	{
		RECT rect = {WinWidth - RightSpace + 1, iy, WinWidth - RightSpace + 1 + fHeight, iy + fHeight};
		auto hPen = CreatePen(i->PenStyle, 1, *i);
		SelectObject(hDC, hPen);

		HBRUSH hBrush = nullptr;
		if (i->State)
		{
			if (i->BrushStyle < 127) //hatch brush样式，参看CreateHatchBrush函数，颜色为BrushColor
			{
				if (3 != i->CurveMode || i->BrushStyle <= HS_DIAGCROSS) //高级（GDI不支持的样式）Hatch画刷单独处理
					hBrush = CreateHatchBrush(i->BrushStyle, i->BrushColor);
			}
			else if (127 < i->BrushStyle && i->BrushStyle < 255) //pattern brush样式，参看CreatePatternBrush函数，(BrushStyle - 128)即为位图在BitBmps里面的序号
			{
				if ((size_t) (i->BrushStyle - 128) < BitBmps.size())
					hBrush = CreatePatternBrush(BitBmps[i->BrushStyle - 128]);
			}
			else //不填充与Solid填充都执行下面的代码
				hBrush = CreateSolidBrush(*i);
//				SetBkColor(hDC, *i);
//				ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rect, nullptr, 0, nullptr);

			if (3 == i->CurveMode && HS_DIAGCROSS < i->BrushStyle && i->BrushStyle < 127) //高级（GDI不支持的样式）Hatch画刷单独处理
			{
				Gdiplus::Graphics graphics(hDC);
				Gdiplus::HatchBrush brush((Gdiplus::HatchStyle) i->BrushStyle,
					Gdiplus::Color(GetRValue(i->BrushColor), GetGValue(i->BrushColor), GetBValue(i->BrushColor)),
					Gdiplus::Color(GetRValue(m_backColor), GetGValue(m_backColor), GetBValue(m_backColor)));
				graphics.FillRectangle(&brush, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
			}
		}
		SelectObject(hDC, hBrush ? hBrush : GetStockObject(NULL_BRUSH)); //API中没有SelectStockObject函数
		Rectangle(hDC, rect.left, rect.top, rect.right, rect.bottom);

		DELETEOBJECT(hBrush);
		DELETEOBJECT(hPen);

		TextOut(hDC, rect.left + fHeight + 1 + (m_ShowMode & 1 ? i->SignWidth : 0), iy + yStep, *i, (int) _tcslen(*i));
		iy += fHeight + 1;
	} //每个图例
}

BOOL CST_CurveCtrl::CHLeftSpace(short ThisLeftSpace, BOOL bCanvasWidthCh/* = FALSE*/)
{
	auto re = FALSE;
	auto XOff = 0;
	auto OldCanvasWidth = CanvasRect[1].right - CanvasRect[1].left;

	if (SysState & 0x200000)
	{
		auto CanvasLeft = WinWidth % HSTEP;
		if (CanvasLeft < 2)
			CanvasLeft += HSTEP;
		CanvasRect[1].right = WinWidth - CanvasLeft / 2;
		CanvasLeft -= CanvasLeft / 2 + 1;
		if (CanvasLeft != CanvasRect[1].left)
		{
			XOff = CanvasRect[1].left - CanvasLeft;
			CanvasRect[1].left = CanvasLeft;

			re = TRUE;
		}
	}
	else if (LeftSpace != ThisLeftSpace)
	{
		LeftSpace = ThisLeftSpace;

		VAxisRect.left = LeftSpace - 5;
		VAxisRect.right = LeftSpace + 5 + 1;
		HAxisRect.left = VAxisRect.right; //LeftSpace;
		VLabelRect.right = VAxisRect.left;
		XOff = CanvasRect[1].left - VAxisRect.right;
		CanvasRect[1].left = VAxisRect.right;

		re = TRUE;
	}

	if (bCanvasWidthCh || re)
	{
		if (!(SysState & 0x200000)) //对于全屏状态时，CanvasRect[1].right已经在前面计算出来了
		{
			CanvasRect[1].right = WinWidth - RightSpace - 10 - CanvasRect[1].left - 1;
			CanvasRect[1].right -= CanvasRect[1].right % HSTEP -  1;
			CanvasRect[1].right += CanvasRect[1].left;
		}
		CanvasRect[0] = CanvasRect[1];
		MOVERECT(CanvasRect[0], m_ShowMode);

		MakeHotspotRect;
		MakePreviewRect;

		UINT nOldScales = HCoorData.nScales;
//		HCoorData.nScales = (USHORT) ((WinWidth - RightSpace - 10 - LeftSpace - 5 - 1 - 1) / HSTEP);
		HCoorData.nScales = (USHORT) ((CanvasRect[1].right - CanvasRect[1].left) / HSTEP);
		BOOL bScaleChanged = nOldScales != HCoorData.nScales;
		HCoorData.reserve(2 * (1 + HCoorData.nScales / (m_hInterval + 1)));

		if (XOff)
			CalcOriginDatumPoint(OriginPoint, 0, XOff);

		DrawBkg();
		if (!(SysState & 1))
		{
			DELETEOBJECT(hScreenRgn);
			hScreenRgn = CreateRectRgnIndirect(CanvasRect);

			if (bScaleChanged || bCanvasWidthCh) //刷新坐标限制
				RefreshLimitedOrFixedCoor();

			//纵坐标的改变，可能会引起整个画布的改变（除了图例和标题），以后不再注释
			UpdateRect(hFrceDC, bCanvasWidthCh ? AllRectMask : MostRectMask);

			//页面信息可能改变
			if (OldCanvasWidth != CanvasRect[1].right - CanvasRect[1].left)
				ReportPageChanges;
		}
	} //if (bCanvasWidthCh || re)

	return re;
}

short CST_CurveCtrl::RestoreLeftSpace()
{
	auto MaxValue = OriginPoint.Value + VCoorData.nScales * VCoorData.fCurStep;
	if (pFormatYCoordinate)
	{
		auto pStr = pFormatYCoordinate(0x7fffffff, MaxValue, 6);
		ASSERT(pStr);

		if (!pStr)
			pStr = _T("");

		_tcsncpy_s(StrBuff, pStr, _TRUNCATE);
	}
	else
		_stprintf_s(StrBuff, VPrecisionExp, MaxValue);

	SIZE size = {0};
	GetTextExtentPoint32(hFrceDC, StrBuff, (int) _tcslen(StrBuff), &size);
	auto ThisLeftSpace = (short) size.cx;
	if (OriginPoint.Value < .0f)
	{
		if (pFormatYCoordinate)
		{
			auto pStr = pFormatYCoordinate(0x7fffffff, OriginPoint.Value, 6);
			ASSERT(pStr);

			if (!pStr)
				pStr = _T("");

			_tcsncpy_s(StrBuff, pStr, _TRUNCATE);
		}
		else
			_stprintf_s(StrBuff, VPrecisionExp, OriginPoint.Value);

		GetTextExtentPoint32(hFrceDC, StrBuff, (int) _tcslen(StrBuff), &size);
		if (ThisLeftSpace < size.cx)
			ThisLeftSpace = (short) size.cx;
	}

	ThisLeftSpace += LEFTSPACE + 5;
	return ThisLeftSpace;
}

BOOL CST_CurveCtrl::SetLeftSpace(BOOL bCanvasWidthCh/*= FALSE*/)
{
	short MaxLeftSpace = 0;
	if (!(SysState & 0x200000)) //全屏时LeftSpace无意义
	{
		ActualLeftSpace = RestoreLeftSpace();
		MaxLeftSpace = GetMaxLeftSpace(ActualLeftSpace);
		if (MaxLeftSpace != LeftSpace)
			if (hBuddyServer)
				MaxLeftSpace = (short) ::SendMessage(hBuddyServer, BUDDYMSG, 7, 0);
			else
				BROADCASTLEFTSPACE(MaxLeftSpace);
	}

	return CHLeftSpace(MaxLeftSpace, bCanvasWidthCh);
}

short CST_CurveCtrl::GetMaxLeftSpace(short RootLeftSpace)
{
	auto MaxLeftSpace = RootLeftSpace;
	if (pBuddys)
		for (auto i = begin(*pBuddys); i < end(*pBuddys); ++i)
		{
			auto thisLeftSpace = (short) ::SendMessage(*i, BUDDYMSG, 6, 0);
			if (thisLeftSpace > MaxLeftSpace)
				MaxLeftSpace = thisLeftSpace;
		}

	return MaxLeftSpace;
}

void CST_CurveCtrl::UpdateOneRange(vector<DataListHead<MainData>>::iterator DataListIter, vector<MainData>::iterator DataIter/* = NullDataIter*/)
{
	ASSERT(NullDataListIter != DataListIter && DataListIter < end(MainDataListArr));
	auto pDataVector = DataListIter->pDataVector;
	if (NullDataIter == DataIter) //以前这里没有对LeftTopPoint和RightBottomPoint重新置位，是一个BUG，在删除了曲线两端的时候，会出错
		DataListIter->LeftTopPoint = DataListIter->RightBottomPoint = pDataVector->front();
	for (auto i = NullDataIter == DataIter ? begin(*pDataVector) : DataIter; i < end(*pDataVector); ++i)
	{
		if (2 == DataListIter->Power) //二次曲线，最小最大时间不一定在两头
			CHANGERANGE(DataListIter->LeftTopPoint.Time, DataListIter->RightBottomPoint.Time, i->Time,
				DataListIter->LeftTopPoint.ScrPos.x, DataListIter->RightBottomPoint.ScrPos.x, i->ScrPos.x);
		CHANGERANGE(DataListIter->RightBottomPoint.Value, DataListIter->LeftTopPoint.Value, i->Value,
			DataListIter->RightBottomPoint.ScrPos.y, DataListIter->LeftTopPoint.ScrPos.y, i->ScrPos.y);

		if (NullDataIter != DataIter)
			break;
	}

	if (1 == DataListIter->Power)
	{
		CHANGEONERANGE(DataListIter->LeftTopPoint.Time, pDataVector->front().Time, >, DataListIter->LeftTopPoint.ScrPos.x, pDataVector->front().ScrPos.x);
		CHANGEONERANGE(DataListIter->RightBottomPoint.Time, pDataVector->back().Time, <, DataListIter->RightBottomPoint.ScrPos.x, pDataVector->back().ScrPos.x);
	}
}

void CST_CurveCtrl::UpdateTotalRange(BOOL bRectOnly/* = FALSE*/) //更新m_MinTime、m_MaxTime值和m_MinValue、m_MaxValue值
{
	if (!bRectOnly)
	{
		m_MinTime = MAXTIME; //表示当前没有曲线数据，所以无法确定最小时间
		m_MaxTime = MINTIME; //表示当前没有曲线数据，所以无法确定最大时间
		m_MinValue = 1.0f; //表示当前没有曲线数据，所以无法确定最小值
		m_MaxValue = -1.0f; //表示当前没有曲线数据，所以无法确定最大值
	}

	auto bFirstData = TRUE;
	for (auto i = begin(MainDataListArr); i < end(MainDataListArr); ++i)
		if (ISCURVESHOWN(i)) //没有图例时也显示曲线，只不过是虚线
			if (bFirstData) //不可用i == begin(MainDataListArr)来判断，因为本函数只统计非隐藏的曲线
			{
				bFirstData = FALSE;
				if (!bRectOnly)
				{
					m_MinTime = i->LeftTopPoint.Time;
					m_MaxTime = i->RightBottomPoint.Time;
					m_MinValue = i->RightBottomPoint.Value;
					m_MaxValue = i->LeftTopPoint.Value;
				}

				LeftTopPoint.ScrPos = i->LeftTopPoint.ScrPos;
				RightBottomPoint.ScrPos = i->RightBottomPoint.ScrPos;
			}
			else
			{
				if (!bRectOnly)
				{
					if (m_MinTime > i->LeftTopPoint.Time)
						m_MinTime = i->LeftTopPoint.Time;
					if (m_MaxTime < i->RightBottomPoint.Time)
						m_MaxTime = i->RightBottomPoint.Time;
					if (m_MinValue > i->RightBottomPoint.Value)
						m_MinValue = i->RightBottomPoint.Value;
					if (m_MaxValue < i->LeftTopPoint.Value)
						m_MaxValue = i->LeftTopPoint.Value;
				}

				if (LeftTopPoint.ScrPos.x > i->LeftTopPoint.ScrPos.x)
					LeftTopPoint.ScrPos.x = i->LeftTopPoint.ScrPos.x;
				if (LeftTopPoint.ScrPos.y > i->LeftTopPoint.ScrPos.y)
					LeftTopPoint.ScrPos.y = i->LeftTopPoint.ScrPos.y;
				if (RightBottomPoint.ScrPos.x < i->RightBottomPoint.ScrPos.x)
					RightBottomPoint.ScrPos.x = i->RightBottomPoint.ScrPos.x;
				if (RightBottomPoint.ScrPos.y < i->RightBottomPoint.ScrPos.y)
					RightBottomPoint.ScrPos.y = i->RightBottomPoint.ScrPos.y;
			}

	ReportPageChanges;
}

long CST_CurveCtrl::ReportPageInfo()
{
	WPARAM wParam = -1;
	LPARAM lParam = -1;
	if (nVisibleCurve > 0)
	{
		auto PageLen = (ULONG) OnePageLength;
		if (PageLen > 0)
		{
			wParam = 0;
			lParam = 0;

			auto TotalLen = CanvasRect[1].left - LeftTopPoint.ScrPos.x;
			if (TotalLen > 0)
			{
				auto nPageNum = TotalLen / PageLen;
				if (TotalLen % PageLen)
					++nPageNum;

				wParam = nPageNum;
			}

			TotalLen = RightBottomPoint.ScrPos.x - CanvasRect[1].right + nZLength * HSTEP;
			if (TotalLen > 0)
			{
				auto nPageNum = TotalLen / PageLen;
				if (TotalLen % PageLen)
					++nPageNum;

				lParam = nPageNum;
			}
		}
	}

	if (m_pageChangeMSG && m_iMSGRecWnd)
		::SendMessage(m_iMSGRecWnd, m_pageChangeMSG, wParam, lParam);
	FIRE_PageChange((long) wParam, (long) lParam);

	m_register1 = (OLE_HANDLE) wParam;
	return (long) lParam;
}

long CST_CurveCtrl::GetCurveLength(long Address) 
{
	auto DataListIter = FindMainData(Address);
	return NullDataListIter != DataListIter ? (long) DataListIter->pDataVector->size() : -1;
}

BSTR CST_CurveCtrl::GetLuaVer()
{
	USES_CONVERSION;
	CComBSTR strResult = A2OLE(LUA_RELEASE);
	return strResult.Copy();
}

void CST_CurveCtrl::DrawGrid(HDC hDC)
{
	auto XBegin = CanvasRect[1].left + nZLength * HSTEP;
	auto YEnd = CanvasRect[1].bottom - nZLength * VSTEP;

	long hd = m_ShowMode & 1 ? 1 : 0;
	long vd = m_ShowMode > 1 ? 1 : 0;
	if (nCanvasBkBitmap >= 0)
	{
		RECT rect = {XBegin - hd, CanvasRect[1].top - vd, CanvasRect[1].right - hd, YEnd - vd};
		DrawBkgImage(hDC, BitBmps[nCanvasBkBitmap].hBitBmp, rect, m_CanvasBkMode);
	}

	if (m_ShowMode & 3)
		CHANGE_MAP_MODE(hBackDC, m_ShowMode);

	int ix, iy;
	HPEN hPen = nullptr;
	if ((SysState & 0x980000) > 0x800000) //绘制实线风格
	{
		hPen = CreatePen(PS_SOLID, 1, m_gridColor);
		SelectObject(hDC, hPen);

		if (SysState & 0x100000) //垂直网格线
			DRAWSOLIDGRIP(ix, XBegin, CanvasRect[1].right, HSTEP, ix, CanvasRect[1].top, ix, YEnd);
		if (SysState & 0x80000) //水平网格线
			DRAWSOLIDGRIP2(iy, CanvasRect[1].top, YEnd, VSTEP, XBegin, iy, CanvasRect[1].right, iy);
	}
	else
	{
		if (SysState & 0x100000) //垂直网格线
			DRAWGRID(ix, XBegin, CanvasRect[1].right, HSTEP, iy, CanvasRect[1].top, YEnd);
		if (SysState & 0x80000) //水平网格线
			DRAWGRID2(iy, CanvasRect[1].top, YEnd, VSTEP, ix, XBegin, CanvasRect[1].right);
	}

	if (nZLength && (!(LeftBkColor & 0x40000000) || !(BottomBkColor & 0x40000000))) //绘制三维效果
	{
		//先填充左边和下边颜色
		HBRUSH hBrush;
		SelectObject(hDC, GetStockObject(NULL_PEN));

		if (!(LeftBkColor & 0x40000000))
		{
			hBrush = CreateSolidBrush(LeftBkColor);
			SelectObject(hDC, hBrush);
			BeginPath(hDC);
			MoveToEx(hDC, CanvasRect[1].left - hd, CanvasRect[1].top + nZLength * VSTEP, nullptr);
			LineTo(hDC, XBegin - hd, CanvasRect[1].top);
			LineTo(hDC, XBegin - hd, YEnd);
			LineTo(hDC, CanvasRect[1].left - hd, CanvasRect[1].bottom);
			EndPath(hDC);
			FillPath(hDC);
			DELETEOBJECT(hBrush);
		}

		if (!(BottomBkColor & 0x40000000))
		{
			hBrush = CreateSolidBrush(BottomBkColor);
			SelectObject(hDC, hBrush);
			BeginPath(hDC);
			MoveToEx(hDC, CanvasRect[1].left - 1 + vd, CanvasRect[1].bottom - vd, nullptr);
			LineTo(hDC, XBegin - 1 + vd, YEnd - vd);
			LineTo(hDC, CanvasRect[1].right - 1 - 1 + vd, YEnd - vd);
			LineTo(hDC, CanvasRect[1].right - nZLength * HSTEP - 1 - 1 + vd, CanvasRect[1].bottom - vd);
			EndPath(hDC);
			FillPath(hDC);
			DELETEOBJECT(hBrush);
		}

		AbortPath(hDC); //节省内存

		if (SysState & 0x180000)
		{
			if (!hPen)
			{
				hPen = CreatePen(PS_SOLID, 1, m_gridColor);
				SelectObject(hDC, hPen);
			}

			if (SysState & 0x100000)
				for (ix = XBegin; ix < CanvasRect[1].right; ix += HSTEP)
				{
					MoveToEx(hDC, ix - nZLength * HSTEP, CanvasRect[1].bottom - 1, nullptr);
					LineTo(hDC, ix, YEnd - 1);
				}
			if (SysState & 0x80000)
				for (iy = CanvasRect[1].top; iy < YEnd; iy += VSTEP)
				{
					MoveToEx(hDC, CanvasRect[1].left, iy + nZLength * VSTEP, nullptr);
					LineTo(hDC, XBegin, iy);
				}
		} //if (SysState & 0x180000)
	} //绘制三维效果

	DELETEOBJECT(hPen);

	if (m_ShowMode & 3)
		CHANGE_MAP_MODE(hBackDC, 0);
}

void CST_CurveCtrl::InitFrce(BOOL bReCreate/*= FALSE*/)
{
	auto hDC = ::GetDC(m_hWnd);
	if (bReCreate)
	{
		DELETEDC(hFrceDC);
		DELETEDC(hTempDC);
	}
	if (!hFrceDC)
	{
		hFrceDC = CreateCompatibleDC(hDC);
		::SetBkMode(hFrceDC, TRANSPARENT);
		SelectObject(hFrceDC, hFont);
	}
	DELETEOBJECT(hFrceBmp);
	hFrceBmp = CreateCompatibleBitmap(hDC, WinWidth, WinHeight);
	SelectObject(hFrceDC, hFrceBmp);

	if (!hTempDC)
		hTempDC = CreateCompatibleDC(hDC);
	::ReleaseDC(m_hWnd, hDC);
}

void CST_CurveCtrl::DrawBkgImage(HDC hDC, HBITMAP hBitmap, RECT& rect, UINT BkMode)
{
	if (!BkMode) //平铺
	{
		auto hBrush = CreatePatternBrush(hBitmap);
		FillRect(hDC, &rect, hBrush);
		DELETEOBJECT(hBrush);
	}
	else if (1 == BkMode || 2 == BkMode) //居中、拉伸
	{
		ASSERT(hTempDC);
		SelectObject(hTempDC, hBitmap);

		BITMAP Bitmap;
		::GetObject(hBitmap, sizeof(BITMAP), &Bitmap);

		if (1 == BkMode)
		{
			auto Left = (rect.right - rect.left - Bitmap.bmWidth) / 2;
			if (Left < 0) //位图宽于矩形
				Left = -Left;
			else
			{
				rect.left += Left;
				rect.right = rect.left + Bitmap.bmWidth;
				Left = 0;
			}

			auto Top = (rect.bottom - rect.top - Bitmap.bmHeight) / 2;
			if (Top < 0) //位图高于矩形
				Top = -Top;
			else
			{
				rect.top += Top;
				rect.bottom = rect.top + Bitmap.bmHeight;
				Top = 0;
			}

			BitBlt(hDC, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, hTempDC, Left, Top, SRCCOPY);
		}
		else
			StretchBlt(hDC, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, hTempDC, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight, SRCCOPY);
	}
}

void CST_CurveCtrl::DrawBkg(BOOL bReCreate/*= FALSE*/)
{
	auto hDC = ::GetDC(m_hWnd);
	if (bReCreate && hBackDC)
		DELETEDC(hBackDC);

	if (!hBackDC)
	{
		hBackDC = CreateCompatibleDC(hDC);
		::SetBkMode(hBackDC, TRANSPARENT);
	}
	DELETEOBJECT(hBackBmp);
	hBackBmp = CreateCompatibleBitmap(hDC, WinWidth, WinHeight);
	::ReleaseDC(m_hWnd, hDC);

	SelectObject(hBackDC, hBackBmp);
	SetBkColor(hBackDC, m_backColor);

	RECT rect = {0, 0, WinWidth, WinHeight};
	if (nBkBitmap >= 0)
	{
		if (1 == (m_BkMode & 0x7F))
		{
			BITMAP Bitmap;
			::GetObject(BitBmps[nBkBitmap].hBitBmp, sizeof(BITMAP), &Bitmap);
			auto Left = (WinWidth - Bitmap.bmWidth) / 2;
			auto Top = (WinHeight - Bitmap.bmHeight) / 2;
			if (WinWidth > Bitmap.bmWidth || WinHeight > Bitmap.bmHeight)
				ExtTextOut(hBackDC, 0, 0, ETO_OPAQUE, &rect, nullptr, 0, nullptr);
		}
		DrawBkgImage(hBackDC, BitBmps[nBkBitmap].hBitBmp, rect, (m_BkMode & 0x7F));

		if (m_BkMode & 0x80) //裁剪画面区域
		{
			if (m_ShowMode & 3)
				CHANGE_MAP_MODE(hBackDC, m_ShowMode);

			long hd = m_ShowMode & 1 ? 1 : 0;
			long vd = m_ShowMode > 1 ? 1 : 0;
			RECT rect = {CanvasRect[1].left + nZLength * HSTEP - hd, CanvasRect[1].top - vd,
				CanvasRect[1].right - hd, CanvasRect[1].bottom - nZLength * VSTEP - vd};
			ExtTextOut(hBackDC, 0, 0, ETO_OPAQUE, &rect, nullptr, 0, nullptr);

			if (m_ShowMode & 3)
				CHANGE_MAP_MODE(hBackDC, 0);
		}
	}
	else
		ExtTextOut(hBackDC, 0, 0, ETO_OPAQUE, &rect, nullptr, 0, nullptr); //这个函数效率最高

	if (SysState & 0x180000 || nCanvasBkBitmap >= 0 || nZLength && (!(LeftBkColor & 0x40000000) || !(BottomBkColor & 0x40000000)))
		DrawGrid(hBackDC);

	if (WaterMark[0]) //显示水印
	{
		SetOneHalfColor(hBackDC);
		CFont font;
		font.CreatePointFont(400, _T("")); //API中没有类似于CreatePointFont的函数，所以只能使用CFont类
		SelectObject(hBackDC, font);
		DrawText(hBackDC, WaterMark, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}

	if (SysState & 0x40000) //显示简单的帮助信息在背景这上
	{
		SetOneHalfColor(hBackDC);
		SelectObject(hBackDC, hFont);

		RECT rect = {CanvasRect[1].left, (WinHeight - 13 * (fHeight + 1)) / 2, CanvasRect[1].right, WinHeight};
		DrawText(hBackDC,
			_T("F4键显示/隐藏本帮助\n")
			_T("F7全屏/取消全屏\n")
			_T("按住鼠标左键拖动曲线\n")
			_T("鼠标滚轮上下移动曲线\n")
			_T("1-9数字键按序号选中曲线\n")
			_T("-+键定点缩放曲线(鼠标取点)\n")
			_T("ctrl加鼠标滚轮左右移动曲线\n")
			_T("shift加鼠标滚轮从原点缩放曲线\n")
			_T("alt加鼠标滚轮从原点水平缩放曲线\n")
			_T("ctrl键限制鼠标只能在水平方向上运动\n")
			_T("shift键限制鼠标只能在垂直方向上运动\n")
			_T("F5键按当前选中的曲线在垂直方向上居中\n")
			_T("在图例上点击鼠标右键隐藏/显示曲线\n")
			_T("在图例上点击鼠标左键选中/取消选中曲线\n")
			_T("F6/左击原点处小正方形显示/隐藏全局位置预览窗口\n")
			_T("在全局位置预览窗口里面，右击隐藏全局位置预览窗口")
			, -1, &rect, DT_CENTER);
	}
}

int CST_CurveCtrl::FormatToolTipString(long Address, HCOOR_TYPE DateTime, float Value, UINT Action)
{
	TCHAR TempBuf[StrBuffLen];
	const TCHAR* pX = nullptr;
	const TCHAR* pY = nullptr;

	if (pFormatXCoordinate)
		pX = pFormatXCoordinate(Address, DateTime, Action);
	if (pFormatYCoordinate)
		pY = pFormatYCoordinate(Address, Value, Action);

	if (!pY)
	{
		auto n = _stprintf_s(TempBuf, VPrecisionExp, Value); //TempBuf缓存空间绝对足够
		TempBuf[n++] = _T(' ');
		_tcscpy_s(TempBuf + n, StrBuffLen - n, Unit);
		pY = TempBuf;
	}

	auto n = 0;
	if (pX)
	{
		n = _sntprintf_s(StrBuff, _TRUNCATE, _T("%s"), pX);
		adjust_poly_len2(n, StrBuffLen - 1);
	}
	//StrBuff缓存空间绝对足够
	else if (m_ShowMode & 0x80)
	{
		if (!(m_ShowMode & 0xC))
		{
			n = _stprintf_s(StrBuff, HPrecisionExp, DateTime);
			StrBuff[n++] = _T(' ');
			n += _stprintf_s(StrBuff + n, StrBuffLen - 1 - n, _T("%s"), HUnit);
		}
	}
	else
	{
		BSTR bstr = nullptr;
		if (ISHVALUEVALID(DateTime))
		{
			if (0xC != (m_ShowMode & 0xC))
			{
				auto hr = VarBstrFromDate(DateTime, LANG_USER_DEFAULT, !(m_ShowMode & 0xC) ? 0 : (!(m_ShowMode & 4) ? VAR_DATEVALUEONLY : VAR_TIMEVALUEONLY), &bstr);
				ASSERT(SUCCEEDED(hr));
			}
			else
				bstr = SysAllocString(L"");
		}
#ifdef _UNICODE
		n = _stprintf_s(StrBuff, _T("%s"), bstr ? bstr : OverFlow);
#else
		if (bstr)
			n = _stprintf_s(StrBuff, _T("%S"), bstr);
		else
			n = _stprintf_s(StrBuff, _T("%s"), OverFlow);
#endif
		if (bstr)
			SysFreeString(bstr);
	} //!if (m_ShowMode & 0x80)

	if (n < StrBuffLen - 1 - 1) //一个结束符和一个回车符
	{
		ASSERT(pY);

		StrBuff[n] = _T('\n');
		_tcsncpy_s(StrBuff + n + 1, StrBuffLen - n - 1, pY, StrBuffLen - 1 - n - 1); //这个不计算在返回的字符个数之内
	}

	//有回车的话，只返回回车符之前的字符个数
	//没有写入回车符（因为空间不问题）的话，此时返回前面已经写入的有效字符数，不影响使用该返回值的地方
	return n;
}

//每调用这个函数一次，得到从BeginPos开始的一组连续的点，存放于points里面
//如果是2次曲线，则结束条件为：取值到了EndPos；出现了断点
//如果是1次曲线，则结束条件为：取值到了EndPos；出现了断点；出现了超出画布右边框的点
//返回值按位算，从低位起：1-取点结束；2-出来了首点；3-出来了末点；4-出来了选中点
//bDrawNode为FALSE时，返回值只有第1位有效；bDrawNode为TRUE时，返回值只有第2、3、4位有效
//EndDrawPos返回绘制结束点（已经被绘制）
UINT CST_CurveCtrl::GetPoints(vector<DataListHead<MainData>>::iterator DataListIter,
							  vector<MainData>::iterator BeginPos, vector<MainData>::iterator EndPos, vector<MainData>::iterator& EndDrawPos,
							  UINT CurveMode, BOOL bClosedAndFilled, BOOL bDrawNode, HDC hDC, UINT NodeMode, UINT PenWidth)
{
	ASSERT(NullDataListIter != DataListIter && NullDataIter != BeginPos && NullDataIter != EndPos);

	UINT re = bDrawNode ? 0 : 1;
	UINT Pos = 0; //1-左；2-上；4-右；8-下
	UINT LastPos = 0; //上一次的Pos值
	UINT OutTimes = 0; //连续出屏次数
	UINT FlexTimes = 0; //拐点次数，points容器的最后的连续拐点将被删除，无论有多少个
	vector<MainData>::iterator LastOutPos = NullDataIter;

	auto pDataVector = DataListIter->pDataVector;
	for (auto i = BeginPos; i < EndPos; ++i)
	{
		if (2 == i->State && i > begin(*pDataVector) && i < prev(end(*pDataVector))) //无论如何，首尾两个点必须要绘制
			continue;
		else if (1 == i->State && i > BeginPos) //断点
		{
			re &= ~1;
			break;
		}
		auto ppoint = &i->ScrPos;

		//////////////////////////////////////////////////////////////////////////
		UINT thisPos = 0;

		if (!bClosedAndFilled || i < prev(end(*pDataVector))) //封闭填充曲线，最后一个点需要绘制，无论在不在画布之内
		{
			if (ppoint->x < CanvasRect[1].left + DataListIter->Zx)
				thisPos |= 1; //1-左
			else if (ppoint->x > CanvasRect[1].right)
				thisPos |= 4; //4-右

			if (ppoint->y < CanvasRect[1].top)
				thisPos |= 2; //2-上
			else if (ppoint->y > CanvasRect[1].bottom - DataListIter->Zy)
				thisPos |= 8; //8-下
		}

		//一定是 thisPos & Pos 判断先出现为假的情况
		//注意后面的判断非常重要，有可能 thisPos 和 Pos 在同一侧，而与 LastPos 不在同一侧
		if (thisPos & Pos && thisPos & LastPos) //优化，2.0.1.9
		{
			++OutTimes; //画布之外的点数量（连续的）
			LastPos = thisPos; //重要，要保证 LastPos 始终为 thisPos 的上一个点
			continue;
		}
		//可能跑到了画布内部，或者跑到了画布另一侧（或者邻侧）

		if (1 == DataListIter->Power &&
			thisPos && LastPos && //两个点都在画布之外
			!IsPointVisible(DataListIter, i, TRUE, FALSE, 2)) //不可见（只向前检测，这里只有这一种写法，考虑到隐藏点的问题，其它写法都可能有BUG）
			break; //一次曲线优化——当两个点都在画布之外，并且不在同侧，此时如果不部分可见，则绘制一定是可以结束了

		if (OutTimes > 1) //多个连续的画布外点，可能出现拐点（拐点即为了保持曲线外观而必须要绘制的画布外点，只有二次曲线才会有拐点）
		{
			if (thisPos & LastPos) //出现拐点，拐点必须与上一点在同一侧
			{
				//既然出来了拐点，则从拐点开始，一定会有两个以上的点在画布之外同一侧，所以下次一定有机会运行到 OutTimes > 1 这个判断里面
				//如果运行不到，则说明 points 后面有连续的拐点，这正是我们想要的，所以不用担心 FlexTimes 无法归零
				if (1 == ++FlexTimes)
					LastOutPos = EndDrawPos; //如果 points 后面有连续拐点，则这个点才是真正的绘制结束点
			}
			else //假拐点
				FlexTimes = 0;

			OutTimes = 0; //重要，防止接下来添加拐点的时候，进入本 if 语句内部
			LastPos = Pos = 0; //重要，保证拐点一定会被添加到 points
			i -= 2; //从前一个点（拐点）重新开始，多减了1是为了中和接下来要执行的 ++i 语句

			continue;
		}

		OutTimes = thisPos ? 1 : 0;
		LastPos = Pos = thisPos;
		//////////////////////////////////////////////////////////////////////////

		if (!bDrawNode)
		{
			EndDrawPos = i; //每次循环里面修改，以减少复杂度

			if (!points.empty())
				if (1 == CurveMode)
				{
					if (points.back().y != ppoint->y)
					{
						POINT NoUse = {points.back().x, ppoint->y};
						points.push_back(NoUse);
					}
				}
				else if (2 == CurveMode && ppoint->x != points.back().x)
				{
					POINT NoUse = {ppoint->x, points.back().y};
					points.push_back(NoUse);
				}

			if (1 == DataListIter->Power && Pos & 4)
			{
				if (!points.empty())
					points.push_back(*ppoint);
				break;
			}
			else
				points.push_back(*ppoint);
		}
		else if (1 == DataListIter->Power && Pos & 4)
			break;
		else if (!Pos) //在画布之内
		{
			if (distance(begin(*pDataVector), i) == DataListIter->SelectedIndex)
				re |= 8; //选中点

			if (i == prev(end(*pDataVector)))
				re |= 4; //末点
			else if (i == begin(*pDataVector))
				re |= 2; //首点

			BOOL bDrawThisNode = NodeMode;
			auto ThisNodeMode = NodeMode;
			if (i->StateEx & 1) //不显示节点，哪怕图例指示需要显示
				ThisNodeMode = bDrawThisNode = FALSE;

			if (!bDrawThisNode) //虽然图例或者点的状态指示不显示节点，但在某些情况下仍然会显示（不会增加宽度），情况如下面代码所示
			{
				bDrawThisNode = 1 == pDataVector->size(); //整条曲线只有一个点
				if (!bDrawThisNode && 1 == i->State)
				{
					bDrawThisNode = i == prev(end(*pDataVector)); //最后一个点
					if (!bDrawThisNode)
					{
						vector<MainData>::iterator tj = i;
						bDrawThisNode = 1 == (++tj)->State; //孤立点
					}
				}
			} //if (!bDrawThisNode)

			//SetViewportOrgEx对ExtTextOut不管用
			if (bDrawThisNode)
			{
				RECT rect;
				DrawNodeRect(rect, *ppoint, PenWidth, ThisNodeMode);

				auto LegendIter = DataListIter->LegendIter; //这里仅仅是为了后面的书写方便，并且减少一次寻址
				if (NullLegendIter != LegendIter && LegendIter->Lable & 3) //标记坐标点，从低位起：1-是否显示X值，2-是否显示Y值，3-是否隐藏单位，4-是否显示单行
				{
					//目前暂时先按坐标提示的格式打印字符串，下面再删除不使用的
					//这样存在一些浪费，因为某个坐标可能是不显示的
					auto nIndex = FormatToolTipString(DataListIter->Address, i->Time, i->Value, 5);
					ASSERT(0 <= nIndex && nIndex < StrBuffLen);

					//unit test
					//_tcscpy_s(StrBuff, _T("\n")); //至少都是这个样子，不可能得到一个空字符串，因为FormatToolTipString为添加回车符
					//nIndex = 0;
					//_tcscpy_s(StrBuff, _T("\n456"));
					//nIndex = 0;
					//_tcscpy_s(StrBuff, _T("123\n"));
					//nIndex = 3;
					//_tcscpy_s(StrBuff, _T("123\n456"));
					//nIndex = 3;
					//_tcscpy_s(StrBuff, _T("123"));
					//nIndex = 3;
					//unit test

					auto pStrBuff = StrBuff;
					auto pEnter = StrBuff + nIndex; //pEnter这个地方可能是\n也可能是\0字符，只有这两种情况

					UINT Lable = LegendIter->Lable;
					if (0 == nIndex) //回车出现在最前面，等于不显示X值
						Lable &= ~1;
					if (!*pEnter || !pEnter[1]) //回车出现在最后面，等于不显示Y值
						Lable &= ~2;

					if (!(Lable & 3)) //XY都不显示
						continue;

					if (!(Lable & 1)) //不显示X值
						pStrBuff = pEnter + 1;
					else if (!(Lable & 2)) //不显示Y值（注意，不可能XY值都不显示）
						*pEnter = 0;
					else if (Lable & 8) //显示为单行
						*pEnter = _T(' ');

					if (Lable & 4) //隐藏单位，在插件状态下，无法隐藏单位
					{
						if (!pFormatYCoordinate && Lable & 2 && *Unit) //隐藏Y单位
						{
							auto pUnit = _tcsstr(pEnter + 1, Unit);
							if (pUnit)
								*(pUnit - 1) = 0; //单位前面有一空格
						}

						if (!pFormatXCoordinate && Lable & 1 && m_ShowMode & 0x80 && *HUnit) //隐藏X单位
						{
							auto pUnit = _tcsstr(pStrBuff, HUnit);
							if (pUnit && pUnit < pEnter) //y坐标里面碰巧有一个x坐标单位，不处理这种情况
							{
								--pUnit;
								while(pStrBuff <= pUnit && isspace(*pUnit)) //排除空格
									--pUnit;
								++pUnit;

								_tcscpy_s(pUnit, StrBuffLen - 1 - distance(pStrBuff, pUnit), pEnter);
								pEnter = pUnit;
							}
						}
					} //if (Lable & 4)

					//由于rect里面考虑过PenWidth的因素，下面要排除这种因素（为了让绘制坐标结果与tooltip完全重合）
					//下面这套偏移方式，只适合于PenWidth等于1的情况，当大于1时，还原成等于1的情况
					if (PenWidth > 1)
						MakeNodeRect(rect, *ppoint, 1, ThisNodeMode);
					auto x = rect.right + 1, y = rect.bottom + 1;
					if (m_ShowMode & 1)
						x -= 5;
					if (m_ShowMode & 2)
						y -= 5;

					//当不存在两行时（插件可能返回一个空字符串），当成一行处理
					if (3 == (Lable & 0xB))
					{
						*pEnter = 0;
						TextOut(hDC, x, y, pStrBuff, (int) _tcslen(pStrBuff));
						y += m_ShowMode & 2 ? -fHeight : fHeight;
						TextOut(hDC, x, y, pEnter + 1, (int) _tcslen(pEnter + 1));
					}
					else
						TextOut(hDC, x, y, pStrBuff, (int) _tcslen(pStrBuff));
				} //if (NullLegendIter != LegendIter && LegendIter->Lable & 3)
			} //if (bDrawThisNode)
		} //else if (PtInRect(CanvasRect + 1, *ppoint))
	} //for (auto i = BeginPos; i < EndPos; ++i)

	if (!bDrawNode && FlexTimes)
	{
		ASSERT(points.size() > FlexTimes);
		points.erase(prev(end(points), FlexTimes), end(points)); //删除无用拐点

		EndDrawPos = LastOutPos; //修改结束点
	}

	return re;
}

//Mask代表参数的有效性，从低起，依次是（低4位）：MinTime、MaxTime、MinValue、MaxValue
//返回值从低位起，依次是：SetBeginTime2、SetTimeSpan、SetBeginValue、SetValueStep函数的执行结果——TRUE则置1，否则置0（这些函数可返回FALSE、TRUE、2）
UINT CST_CurveCtrl::UpdateFixedValues(HCOOR_TYPE MinTime, HCOOR_TYPE MaxTime, float MinValue, float MaxValue, UINT Mask)
{
	UINT re = 0;

	SetRedraw(FALSE);
	SysState |= 0x20000000;
	BOOL bResult;
	if (Mask & 2 && HCoorData.nScales > 0) //MaxTime有效
		if (Mask & 1) //MinTime有效
		{
			auto TimeSpan = (MaxTime - MinTime) / HCoorData.nScales;
			TimeSpan = pCalcTimeSpan ? pCalcTimeSpan(TimeSpan, -Zoom, -HZoom) : GETSTEP(TimeSpan, -(Zoom + HZoom));
			bResult = SetTimeSpan(TimeSpan);
			if (TRUE == bResult) //由于bResult可能会等于FALSE、TRUE、2，所以TRUE == bResult这个判断不可改为bResult，下同
				re |= 2;
		}
		else
		{
			MinTime = MaxTime - HCoorData.nScales * HCoorData.fCurStep;
			Mask |= 1;
		}
	if (Mask & 1) //MinTime有效
	{
		bResult = SetBeginTime2(MinTime);
		if (TRUE == bResult)
			re |= 1;
	}

	if (Mask & 8 && VCoorData.nScales > 0) //MaxValue有效
		if (Mask & 4) //MinValue有效
		{
			float ValueStep = (MaxValue - MinValue) / VCoorData.nScales;
			ValueStep = pCalcValueStep ? pCalcValueStep(ValueStep, -Zoom) : GETSTEP(ValueStep, -Zoom);
			bResult = SetValueStep(ValueStep);
			if (TRUE == bResult)
				re |= 8;
		}
		else
		{
			MinValue = MaxValue - VCoorData.nScales * VCoorData.fCurStep;
			Mask |= 4;
		}
	if (Mask & 4) //MinValue有效
	{
		bResult = SetBeginValue(MinValue);
		if (TRUE == bResult)
			re |= 4;
	}
	SysState &= ~0x20000000;
	SetRedraw();
	if (re)
		UpdateRect(hFrceDC, MostRectMask);

	return re;
}

void CST_CurveCtrl::DrawComment(HDC hDC) //绘制注解
{
	for (auto iterComment = begin(CommentDataArr); iterComment < end(CommentDataArr); ++iterComment)
		if (iterComment->State && PtInRect(CanvasRect + 1, iterComment->ScrPos))
		{
			auto PosX = iterComment->ScrPos.x;
			auto PosY = iterComment->ScrPos.y;

			int Width = iterComment->Width;
			int Height = iterComment->Height;

			if (0 <= iterComment->nBkBitmap && (size_t) iterComment->nBkBitmap < BitBmps.size())
			{
				BITMAP bmp;
				GetObject(BitBmps[iterComment->nBkBitmap].hBitBmp, sizeof BITMAP, &bmp);

				if (!Width) //无效时使用位图的宽度
					Width = bmp.bmWidth;
				if (!Height) //无效时使用位图的高度
					Height = bmp.bmHeight;

				CreateCommentRect(iterComment->Position);

				SelectObject(hTempDC, BitBmps[iterComment->nBkBitmap].hBitBmp);
				if (iterComment->TransColor & 0x80000000) //不做透明处理
					BitBlt(hDC, PosX, PosY, bmp.bmWidth, bmp.bmHeight, hTempDC, 0, 0, SRCCOPY);
				else
					TransparentBlt(hDC, PosX, PosY, bmp.bmWidth, bmp.bmHeight, hTempDC, 0, 0, bmp.bmWidth, bmp.bmHeight, iterComment->TransColor);
			}
			else
				CreateCommentRect(iterComment->Position);

			auto pComment = iterComment->Comment;
			if (nullptr == pComment || _T('\0') == *pComment)
				continue;

			//因为有映射关系，这里要处理
			if (m_ShowMode & 1)
				PosX += Width - 2 * iterComment->XOffSet;
			if (m_ShowMode & 2)
				PosY += Height - 2 * iterComment->YOffset;

			//开始绘制文字
			SetTextColor(hDC, iterComment->TextColor);
			PosX += iterComment->XOffSet;
			PosY += iterComment->YOffset;

			while(*pComment)
			{
				auto pEnter = _tcschr(pComment, _T('\n'));
				TextOut(hDC, PosX, PosY, pComment, pEnter ? (int) distance(pComment, pEnter) : (int) _tcslen(pComment));
				PosY += m_ShowMode & 2 ? -(fHeight + 2) : (fHeight + 2);

				if (pEnter)
					pComment = pEnter + 1;
				else
					break;
			}
		} //if (PtInRect(CanvasRect + 1, iterComment->ScrPos))
}

BOOL CST_CurveCtrl::IsCurveClosed2(vector<DataListHead<MainData>>::iterator DataListIter)
{
	ASSERT(NullDataListIter != DataListIter);

	auto pDataVector = DataListIter->pDataVector;
	return 2 == DataListIter->Power && !memcmp(&pDataVector->front().ScrPos, &pDataVector->back().ScrPos, sizeof(POINT));
}

BOOL CST_CurveCtrl::IsCurveClosed(long Address)
{
	auto DataListIter = FindMainData(Address);
	return NullDataListIter != DataListIter ? IsCurveClosed2(DataListIter) : FALSE;
}

void CST_CurveCtrl::DrawCurve(HDC hDC, HRGN hRgn, vector<DataListHead<MainData>>& DataListArr, UINT& CurvePosition, BOOL bInfiniteCurve/* = FALSE*/,
							  vector<DataListHead<MainData>>::iterator DataListIter/* = NullDataListIter*/, vector<MainData>::iterator BeginPos/* = NullDataIter*/)
{
	for (auto i = begin(DataListArr); i < end(DataListArr); ++i)
	{
		if (i == begin(DataListArr) && NullDataListIter != DataListIter) //i == begin(DataListArr)这个判断非常重要，不可去掉，因为后面用i覆盖了DataListIter
			i = prev(end(DataListArr)); //为了退出for循环
		else
			DataListIter = i;

		//判断可见性只需要Zx和Zy中的随便一个（约定使用Zx），但真正执行偏移的时候，就要分别使用了（虽然目前Zx和Zy是相等的）
		if (!ISCURVESHOWN(DataListIter) || DataListIter->Zx > nZLength * HSTEP) //曲线不可见，包括从图例隐藏和用Z轴隐藏两种情况
			continue;

		auto pDataVector = DataListIter->pDataVector;
		vector<MainData>::iterator j = NullDataIter;
		if (NullDataIter != BeginPos)
			j = BeginPos;
		else
		{
			UINT thisPosition;
			j = GetFirstVisiblePos(DataListIter, TRUE, FALSE, &thisPosition); //部分可见即可
			if (!bInfiniteCurve)
				if (0xFF == CurvePosition)
					CurvePosition = thisPosition;
				else
					CurvePosition &= thisPosition;

			if (NullDataIter == j) //DataListIter曲线完全不可见
				continue;
		}

		if (nZLength) //裁减区域需要更改，因为hDC已经做了坐标映射，所以用CanvasRect[1]这个矩形
		{
			if (DataListIter->Zx)
				ExcludeClipRect(hDC, CanvasRect[1].left - 1, CanvasRect[1].top, CanvasRect[1].left + DataListIter->Zx, CanvasRect[1].bottom);
			if (DataListIter->Zy)
				ExcludeClipRect(hDC, CanvasRect[1].left, CanvasRect[1].bottom - DataListIter->Zy, CanvasRect[1].right, CanvasRect[1].bottom);
				//这里CanvasRect[1].left减不减1无所谓了，因为左边已经截掉一块了，本来都可以从CanvasRect[1].left + DataListIter->Zx开始的
		}

		UINT PenStyle;
		UINT PenWidth;
		COLORREF PenColor;
		auto LegendIter = DataListIter->LegendIter; //这里仅仅是为了后面的书写方便，并且减少一次寻址

		if (NullLegendIter != LegendIter)
		{
			PenStyle = LegendIter->PenStyle;
			PenWidth = LegendIter->LineWidth;
			PenColor = *LegendIter;
			if (!bInfiniteCurve && CurCurveIndex == distance(begin(DataListArr), DataListIter))
				PenWidth *= 2;
		}
		else
		{
			PenStyle = PS_DASH;
			PenWidth = 1;
			PenColor = RGB(255, 255, 255);
		}

		Gdiplus::Bitmap* pBitmap = nullptr;
		Gdiplus::Brush* pBrush = nullptr;
		Gdiplus::GraphicsPath* pPath = nullptr;
		Gdiplus::Graphics* pGraphics = nullptr;

		HBRUSH hBrush = nullptr;
		UINT CurveMode = 0;
		UINT NodeMode = 1;
		if (NullLegendIter != LegendIter)
		{
			CurveMode = LegendIter->CurveMode;
			NodeMode = LegendIter->NodeMode;
			if (3 == CurveMode)
			{
				pGraphics = Gdiplus::Graphics::FromHDC(hDC);
				LockGdiplus; //先不让GDI+生效
			}

			if (255 != LegendIter->BrushStyle && DataListIter->FillDirection & 0xF) //如果没有填充或者没有填充方向（没有填充方向认为是不填充），省去创建刷子
			{
				if (3 == CurveMode)
				{
					if (LegendIter->BrushStyle <= 127)
					{
						Gdiplus::Color color;
						color.SetFromCOLORREF(LegendIter->BrushColor);

						if (LegendIter->BrushStyle < 127)
							pBrush = new Gdiplus::HatchBrush((Gdiplus::HatchStyle) LegendIter->BrushStyle, color,
								Gdiplus::Color(GetRValue(m_backColor), GetGValue(m_backColor), GetBValue(m_backColor)));
						else
							pBrush = new Gdiplus::SolidBrush(color);
					}
					else if ((size_t) (LegendIter->BrushStyle - 128) < BitBmps.size()) //Pattern填充模式下，BrushColor无效
					{
						pBitmap = new Gdiplus::Bitmap(BitBmps[LegendIter->BrushStyle - 128], (HPALETTE) nullptr);
						pBrush = new Gdiplus::TextureBrush(pBitmap);
					}

					if (pBrush)
					{
						hBrush = (HBRUSH) 1; //不使用hBrush，但不可让其等于nullptr，后面对它有判断
						pPath = new Gdiplus::GraphicsPath();
					}
				} //if (3 == CurveMode)
				else
				{
					if (LegendIter->BrushStyle < 127)
						hBrush = CreateHatchBrush(LegendIter->BrushStyle, LegendIter->BrushColor);
					else if (127 == LegendIter->BrushStyle)
						hBrush = CreateSolidBrush(LegendIter->BrushColor);
					else if ((size_t) (LegendIter->BrushStyle - 128) < BitBmps.size()) //Pattern填充模式下，BrushColor无效
						hBrush = CreatePatternBrush(BitBmps[LegendIter->BrushStyle - 128]);

					if (hBrush)
						SelectObject(hDC, hBrush);
				}
			} //if (255 != LegendIter->BrushStyle && DataListIter->FillDirection & 0xF)
		} //if (NullLegendIter != LegendIter)

		Gdiplus::Pen* pPen = nullptr;
		HPEN hPen = nullptr;
		if (PS_NULL != PenStyle)
			if (3 == CurveMode)
			{
				hPen = (HPEN) 1;
				pPen = new Gdiplus::Pen(Gdiplus::Color(GetRValue(PenColor), GetGValue(PenColor), GetBValue(PenColor)), (Gdiplus::REAL) PenWidth);
				pPen->SetDashStyle((Gdiplus::DashStyle) PenStyle);
			}
			else
				hPen = CreatePen(PenStyle, PenWidth, PenColor);

		if (hBrush || hPen) //要么填充，要么非空画笔，或者两者都有，不可全无
		{
			COLORREF NodeColor = PenColor;
			if (2 == NodeMode) //用曲线颜色的反色显示节点
				NodeColor = ~NodeColor & 0xFFFFFF;
			SetBkColor(hDC, NodeColor);
			SetTextColor(hDC, NodeColor);

			//找到绘制的结束点
			vector<MainData>::iterator EndPos = NullDataIter;
			if (NullDataIter != BeginPos)
			{
				ASSERT(BeginPos < end(*pDataVector));

				EndPos = BeginPos;
				++EndPos;
				if (EndPos < end(*pDataVector))
					++EndPos; //绘制实时曲线，每次需绘制两个点
			}
			else
				EndPos = end(*pDataVector);

			BOOL bClosedAndFilled = hBrush && IsCurveClosed2(DataListIter);
			//如果是二次封闭填充曲线，必须从头开始，但是前面的GetFirstVisiblePos也是必不可少的，因为需要更新Position这个重要的量
			if (bClosedAndFilled)
				j = begin(*pDataVector);

			for (; j < EndPos; ++j)
			{
				points.clear(); //在vc2010及其以上的版本中，用clear不会释放缓存，但之前的版本会的，所以要用erase

				vector<MainData>::iterator EndDrawPos = NullDataIter;
				BOOL bQuit = 1 & GetPoints(DataListIter, j, EndPos, EndDrawPos, CurveMode, bClosedAndFilled, FALSE, 0, 0, 0); //后面三个参数不会使用
				ASSERT(NullDataIter != EndDrawPos);

				auto nPoints = points.size(); //先保存点的数量，因为FILLPATH宏里面可能会增加点
				if (hBrush && nPoints > 1) //填充至少需要两个点
				{
					if (3 != CurveMode)
						SelectObject(hDC, GetStockObject(NULL_PEN));

					if (DataListIter->FillDirection & 0x30) //绘制填充值，准备颜色，不要在FillValue里面做，在这里做只需要做一次
					{
						UINT FillDirection = DataListIter->FillDirection;
						COLORREF TextColor;

						if (127 == LegendIter->BrushStyle) //Solid填充时，采用填充色的反色
						{
							TextColor = LegendIter->BrushColor;
							FillDirection |= 0x80; //让下面的程序执行求反
						}
						else
							TextColor = FillDirection & 0x40 ? LegendIter->PenColor : m_foreColor; //确定是采用前景色还是本条曲线颜色

						if (FillDirection & 0x80) //求反
							TextColor = ~TextColor & 0xFFFFFF;

						SetTextColor(hDC, TextColor);
					}

					//Windows 95/98/Me对一次传给Polygon函数的点的个数有限制，幸好控件只运行于Windows 2000及其以后的系统
					//因为hBrush不为空，所以LegendIter一定是一个有效的迭代器
					FILLPATH(1, EndDrawPos->ScrPos.x, CanvasRect[1].bottom, j->ScrPos.x, CanvasRect[1].bottom); //向下填充
					FILLPATH(2, CanvasRect[1].right, EndDrawPos->ScrPos.y, CanvasRect[1].right, j->ScrPos.y); //向右填充
					FILLPATH(4, EndDrawPos->ScrPos.x, CanvasRect[1].top, j->ScrPos.x, CanvasRect[1].top); //向上填充
					FILLPATH(8, CanvasRect[1].left, EndDrawPos->ScrPos.y, CanvasRect[1].left, j->ScrPos.y); //向左填充

					if (DataListIter->FillDirection & 0x30) //还原颜色
						SetTextColor(hDC, NodeColor);
				}

				if (hPen && nPoints) //空画笔不用再绘制
				{
					if (nPoints > 1) //画线至少需要两个点
						if (3 == CurveMode) //如果只有两个点，也得绘制平滑曲线，因为hPen要么未创建，要么是为另外某条曲线创建的
						{
							UnlockGdiplus; //让GDI+生效，马上要使用了
							pGraphics->DrawCurve(pPen, (Gdiplus::Point*) &points.front(), (int) nPoints, Tension); //Gdiplus::Point结构与POINT结构在win32下是兼容的
							LockGdiplus; //用完后，马上让GDI+失效
						}
						else
						{
							/*
							Windows 95/98/Me对一次传给Polyline函数的点的个数有限制，幸好控件只运行于Windows 2000及其以后的系统
							for (auto i = 0; nPoints;)
							{
								auto nThisDraw = nPoints > 16 * 1024 ? 16 * 1024 : nPoints;
								Polyline(hDC, next(begin(points), i)._Ptr, nThisDraw);
								i += nThisDraw;
								nPoints -= nThisDraw;
							}
							*/
							SelectObject(hDC, hPen);
							Polyline(hDC, &points.front(), (int) nPoints);
						}

					vector<MainData>::iterator NoUse = NullDataIter;
					auto re = GetPoints(DataListIter, j, EndPos, NoUse, CurveMode, bClosedAndFilled, TRUE, hDC, NodeMode, PenWidth); //需要使用后面三个参数
					//绘制第一点和最后一点，如果颜色不相同的话
					if (re & 0xE && NodeMode && NullLegendIter != LegendIter) //出现了首末点、或者选中点
					{
						if (LegendIter->NodeModeEx & 3) //按位算，从低位起：1-是否显示头节点；2-是否显示尾节点；3-是否显示选中点；
						{
							POINT point;
							if (re & 2 && LegendIter->NodeModeEx & 1 && NodeColor != LegendIter->BeginNodeColor)
							{
								SetBkColor(hDC, LegendIter->BeginNodeColor);
								point = pDataVector->front().ScrPos;
								DrawNode(point, PenWidth, NodeMode);

								SetBkColor(hDC, NodeColor);
							}

							if (re & 4 && LegendIter->NodeModeEx & 2 && NodeColor != LegendIter->EndNodeColor)
							{
								SetBkColor(hDC, LegendIter->EndNodeColor);
								point = pDataVector->back().ScrPos;
								DrawNode(point, PenWidth, NodeMode);
							}
						}

						if (re & 8 && -1 != DataListIter->SelectedIndex && LegendIter->NodeModeEx & 4)
							DrawSelectedNode(hDC, nullptr, DataListIter, PenWidth, DataListIter->SelectedIndex);
					} //if (re & 0xE && NodeMode && NullLegendIter != LegendIter)
				} //if (hPen && nPoints)

				if (bQuit)
					break;

				++EndDrawPos;
				if (EndDrawPos >= EndPos)
					break;

				j = GetFirstVisiblePos(DataListIter, TRUE, FALSE, nullptr, EndDrawPos);
				if (NullDataIter == j) //DataListIter曲线完全不可见
					break;

				--j; //抵消接下来马上执行的++j语句
			} //for (; j < EndPos; ++j)
		} //if (hBrush || hPen)

		if (pBitmap)
			delete pBitmap;
		if (pBrush)
			delete pBrush;
		if (pPath)
			delete pPath;
		if (pPen)
			delete pPen;
		if (pGraphics)
		{
			UnlockGdiplus; //重要，否则内存泄漏
			delete pGraphics;
		}

		if ((UINT_PTR) hPen > 1)
			DeleteObject(hPen);
		if ((UINT_PTR) hBrush > 1)
			DeleteObject(hBrush);

		if (nZLength && (DataListIter->Zx || DataListIter->Zy)) //恢复裁减区域，以便再一次执行ExcludeClipRect
			SelectClipRgn(hDC, hRgn);
	} //for (auto i = begin(DataListArr); i < end(DataListArr); ++i)
}

void CST_CurveCtrl::DrawCurve(HDC hDC, HRGN hRgn,
							  vector<DataListHead<MainData>>::iterator DataListIter/* = NullDataListIter*/, vector<MainData>::iterator BeginPos/* = NullDataIter*/)
{
	if (nVisibleCurve <= 0)
		return;

	if (!InvalidCurveSet.empty())
	{
		for (auto i = begin(InvalidCurveSet); i != end(InvalidCurveSet); ++i)
		{
			auto DataListIter = FindMainData(*i);
			if (NullDataListIter != DataListIter)
			{
				UpdatePower(DataListIter);
				UpdateOneRange(DataListIter); //这两个函数的调用次序非常重要，不可调换
			}
		}
		free_container(InvalidCurveSet);

		UpdateTotalRange();
		if (SysState & 0x10000000 && RefreshLimitedOrFixedCoor())
			return;
	}

	ASSERT(NullDataIter == BeginPos || NullDataListIter != DataListIter);

	UINT CurvePosition = 0xFF;
	SelectClipRgn(hDC, hRgn);

	//在最下层绘制注解
	if (!CommentPosition)
		DrawComment(hDC);

	if (!InfiniteCurveArr.empty())
	{
		//生成无限曲线
		vector<DataListHead<MainData>> InfiniteCurveDataListArr;
		for (auto iter = begin(InfiniteCurveArr); iter < end(InfiniteCurveArr); ++iter)
		{
			DataListHead<MainData> NoUse;

			NoUse.Address = iter->Address;
			NoUse.LegendIter = iter->LegendIter;
			NoUse.SelectedIndex = -1; //未选中点
			NoUse.FillDirection = iter->FillDirection;
			NoUse.Power = 0;

			NoUse.Zx = NoUse.Zy = 0;

			UINT PenWidth = 1;
			auto LegendIter = iter->LegendIter; //这里仅仅是为了后面的书写方便，并且减少一次寻址
			if (NullLegendIter != LegendIter)
				PenWidth = LegendIter->LineWidth;

			InfiniteCurveDataListArr.push_back(NoUse);
			auto i = prev(end(InfiniteCurveDataListArr));
			{
				MainData NoUse;
				NoUse.AllState = 0;

				if (0 == iter->State)
				{
					NoUse.ScrPos.x = CanvasRect[1].left - PenWidth;
					NoUse.ScrPos.y = iter->ScrPos.y;
					i->pDataVector->push_back(NoUse);

					NoUse.ScrPos.x = CanvasRect[1].right + PenWidth;
					i->pDataVector->push_back(NoUse);
				}
				else //1 == iter->State
				{
					NoUse.ScrPos.y = CanvasRect[1].top - PenWidth;
					NoUse.ScrPos.x = iter->ScrPos.x;
					i->pDataVector->push_back(NoUse);

					NoUse.ScrPos.y = CanvasRect[1].bottom + PenWidth;
					i->pDataVector->push_back(NoUse);
				}
			}
			//不可调用UpdateOneRange，它是根据实际值来计算的，无限曲线的实际值是随机的，不使用
			i->LeftTopPoint = i->pDataVector->front();
			i->RightBottomPoint = i->pDataVector->back();
		} //for (auto iter = begin(InfiniteCurveArr); iter < end(InfiniteCurveArr); ++iter)

		//绘制无限曲线
		DrawCurve(hDC, hRgn, InfiniteCurveDataListArr, CurvePosition, TRUE);

		//释放无限曲线
		for (auto iter = begin(InfiniteCurveDataListArr); iter < end(InfiniteCurveDataListArr); ++iter)
			delete iter->pDataVector;
	} //if (!InfiniteCurveArr.empty())

	//绘制普通曲线
	DrawCurve(hDC, hRgn, MainDataListArr, CurvePosition, FALSE, DataListIter, BeginPos);

	//在最上层绘制注解
	if (CommentPosition)
		DrawComment(hDC);

//	ExtSelectClipRgn(hDC, nullptr, RGN_COPY);
	SelectClipRgn(hDC, nullptr);

	if (0xFF != CurvePosition)
	{
		//下面的代码纯粹为绝对的杜绝因后面的MoveCurve调用引起的堆栈溢出（递归调用层次太深）
		static UINT LastPosition; //上次运行到这里时的CurvePosition值
		static short MoveDistance; //上次移动时的移动量
		if (CurvePosition) //本页中没有任何可显示的曲线
		{
			if (!(SysState & 0x40000000) || CurvePosition & 0xC) //第31位阻止不了在垂直方向上的移动
			{
				if (LastPosition == CurvePosition) //上次的状态和本次一样，所以加速移动
				{
					if (MoveDistance <= 0x2000)
						MoveDistance <<= 1; //加速移动
					else
						MoveDistance = 0x7fff;
				}
				else
				{
					LastPosition = CurvePosition;
					MoveDistance = 1;
				}

				//GetFirstVisiblePos函数采用短路算法，所以水平和垂直上的偏出不会并存
				//如果在水平移动后，垂直方向上仍然偏出，此时自然会在下一次DrawCurve时，调用到下面的else分支的
				if (CurvePosition & 3) //曲线全部偏出了画布（水平上）
					MoveCurve(CurvePosition & 1 ? MoveDistance : -MoveDistance, 0, FALSE, FALSE); //不能刷新，看下面的注释
				//不要检测移动的合法性，否则移动不成功，造成死循环，如果移动后，曲线仍然在画布之外，会继续向同一个方向移动
				//CurvePosition不可能出现第1、2位或者第3、4位同时为1，具体看GetFirstVisiblePos函数
				else if (CurvePosition & 0xC) //曲线全部偏出了画布（垂直上）
					MoveCurve(0, CurvePosition & 4 ? -MoveDistance : MoveDistance, FALSE, FALSE); //不能刷新
				//至于为什么不能刷新，正常情况下只在UpdateRect里面调用本函数，所以刷新应该由UpdateRect来控制
			} //if (!(SysState & 0x40000000) || !(CurvePosition & 0xC))
		}
		else
		{
			LastPosition = 0;
			MoveDistance = 0;
		}
	}
}

void CST_CurveCtrl::FillValue(HDC hDC, vector<DataListHead<MainData>>::iterator DataListIter,
							  vector<MainData>::iterator BeginPos, vector<MainData>::iterator EndPos, UINT FillDirection, vector<LegendData>::iterator LegendIter)
{
	ASSERT(NullDataListIter != DataListIter);
	ASSERT(NullLegendIter != LegendIter);
	auto ValueMask = FillDirection & 0x30;
	auto Value = 0x20 == ValueMask ? EndPos->Value : (0x30 == ValueMask ? .0f : BeginPos->Value);

	auto nNum = 0;
	auto i = BeginPos;
	RECT rect;
	//FillDirection里面的低4位，每次只有一位为1，参看FILLPATH宏及DrawCurve函数
	switch (FillDirection & 0xF)
	{
	case 1: //向下填充
		FILLVALUE(rect.top, y);

		rect.left = BeginPos->ScrPos.x;
		rect.right = EndPos->ScrPos.x;
		rect.bottom = CanvasRect[1].bottom - DataListIter->Zy;
		break;
	case 2: //向右填充
		FILLVALUE(rect.left, x);

		rect.top = BeginPos->ScrPos.y;
		rect.right = CanvasRect[1].right;
		rect.bottom = EndPos->ScrPos.y;
		break;
	case 4: //向上填充
		FILLVALUE(rect.bottom, y);

		rect.left = BeginPos->ScrPos.x;
		rect.right = EndPos->ScrPos.x;
		rect.top = CanvasRect[1].top;
		break;
	case 8: //向左填充
		FILLVALUE(rect.right, x);

		rect.top = BeginPos->ScrPos.y;
		rect.left = CanvasRect[1].left + DataListIter->Zx;
		rect.bottom = EndPos->ScrPos.y;
		break;
	}
	NormalizeRect(rect);

	if (pFormatYCoordinate)
	{
		auto pStr = pFormatYCoordinate(DataListIter->Address, Value, 7);
		ASSERT(pStr);

		if (!pStr)
			pStr = _T("");

		_tcsncpy_s(StrBuff, pStr, _TRUNCATE);
	}
	else //StrBuff缓存空间绝对足够
		_stprintf_s(StrBuff, VPrecisionExp, Value);

	if (m_ShowMode & 1)
	{
		RECT tr = {0};
		DrawText(hDC, StrBuff, -1, &tr, DT_CALCRECT | DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		auto step = tr.left - tr.right;
		OffsetRect(&rect, step, 0);
		InflateRect(&rect, step, 0);
	}

//	Rectangle(hDC, rect.left, rect.top, rect.right, rect.bottom);
	DrawText(hDC, StrBuff, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// CST_CurveCtrl::DoPropExchange - 持久性支持

void CST_CurveCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	// TODO: 为每个持久的自定义属性调用 PX_ 函数。
	PX_Color(pPX, _T("ForeColor"), m_foreColor, DEFAULT_foreColor);
	PX_Color(pPX, _T("BackColor"), m_backColor, DEFAULT_backColor);
	PX_Color(pPX, _T("AxisColor"), m_axisColor, DEFAULT_axisColor);
	PX_Color(pPX, _T("GridColor"), m_gridColor, DEFAULT_gridColor);
	PX_Color(pPX, _T("TitleColor"), m_titleColor, DEFAULT_titleColor);
	PX_Color(pPX, _T("FootNoteColor"), m_footNoteColor, DEFAULT_footNoteColor);
	PX_Long(pPX, _T("PageChangeMSG"), m_pageChangeMSG, DEFAULT_pageChangeMSG);
}

// CST_CurveCtrl::GetControlFlags -
// 自定义 MFC 的 ActiveX 控件实现的标志。
//
DWORD CST_CurveCtrl::GetControlFlags()
{
	auto dwFlags = COleControl::GetControlFlags();

	// 当前未剪辑控件的输出。
	// 控件保证它不会绘制到它的
	// 矩形工作区之外。
	dwFlags &= ~clipPaintDC;

	// 在活动和不活动状态之间进行转换时，
	// 不会重新绘制控件。
	dwFlags |= noFlickerActivate;

	// 控件通过不还原设备上下文中的
	// 原始 GDI 对象，可以优化它的 OnDraw 方法。
	dwFlags |= canOptimizeDraw;

	return dwFlags;
}

void CST_CurveCtrl::DrawPreviewView(HDC hDC)
{
	::SetBkMode(hDC, OPAQUE);
	SelectObject(hDC, hAxisPen);
	SetBkColor(hDC, m_backColor);
	ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &PreviewRect, nullptr, 0, nullptr);
	::SetBkMode(hDC, TRANSPARENT);
	SelectObject(hDC, GetStockObject(NULL_BRUSH));
	Rectangle(hDC, PreviewRect.left, PreviewRect.top, PreviewRect.right, PreviewRect.bottom);
	if (nVisibleCurve > 0)
	{
		PreviewPoint.x = PreviewRect.left +
			(int) ((float) (4 * HSTEP - 20) / (RightBottomPoint.ScrPos.x - LeftTopPoint.ScrPos.x + CanvasRect[1].right - CanvasRect[1].left) *
			(CanvasRect[1].right - LeftTopPoint.ScrPos.x));
		PreviewPoint.y = PreviewRect.top +
			(int) ((float) (2 * VSTEP - 9) / (RightBottomPoint.ScrPos.y - LeftTopPoint.ScrPos.y + CanvasRect[1].bottom - CanvasRect[1].top) *
			(CanvasRect[1].bottom - LeftTopPoint.ScrPos.y));
		RECT rect = {PreviewPoint.x, PreviewPoint.y, PreviewPoint.x + 20, PreviewPoint.y + 9};
		if (m_ShowMode)
		{
			MOVERECTLIMIT(rect, PreviewRect, m_ShowMode);
			PreviewPoint.x = rect.left;
			PreviewPoint.y = rect.top;
		}
		Rectangle(hDC, rect.left, rect.top, rect.right, rect.bottom);
	} //if (nVisibleCurve > 0)
}

//特别注意：调用UpdateRect函数的时候，对于Mask参数，要么等于PreviewRectMask，要么刷新矩形必须要包括PreviewRectMask指向的矩形
//举个例子，AllRectMask、MostRectMask、MostRectMask这些掩码对应的矩形都包括了PreviewRectMask对应的矩形
//至于为什么要这样，是因为对于PreviewRectMask没有调用ERASEBKG宏，所以刷新区域（InvalidRect）里面将不会包括它，控件的窗口也就得不到刷新
//至于为什么不调用ERASEBKG宏，是因为调用DrawPreviewView前，需要进行屏幕反映射
//注意，虽然本函数可以只绘制某一条曲线，但其余曲线会被消除
void CST_CurveCtrl::UpdateRect(HDC hDC, UINT Mask, vector<DataListHead<MainData>>::iterator DataListIter/*= NullDataListIter*/, BOOL bUpdate/*= TRUE*/)
{
	if (SysState & 0x20000000) //刷新已经禁止掉了
		return;

	if (PreviewRectMask == Mask)
	{
		if (SysState & 0x80000000)
		{
			DrawPreviewView(hDC);
			InvalidateControl(&PreviewRect, FALSE);
		}
		return;
	}

	if (SysState & 0x200000 && MostRectMask == Mask) //全屏时MostRectMask必须用AllRectMask代替
		Mask = AllRectMask;

	//刷背景
	SetRectEmpty(&InvalidRect);
	if (!(SysState & 1) && //非打印才执行的程序
		(!(SysState & 0x200000) || Mask & CanvasRectMask)) //Mask & CanvasRectMask这个判断非常重要，否则可能出现刷了背景，前景却没有得到绘制（在全屏的时候会发生）
		if (AllRectMask == Mask) //所有的矩形并起来的矩形，并不等于控件的整个窗口
		{
			InvalidRect.right = WinWidth;
			InvalidRect.bottom = WinHeight;
			BitBlt(hDC, 0, 0, WinWidth, WinHeight, hBackDC, 0, 0 , SRCCOPY);
		}
		else if (MostRectMask == Mask) //在这种情况下，TotalRect的计算是写死的，所以在修改MostRectMask后，一定要修改这里
		{
			InvalidRect.top = VAxisRect.top;
			InvalidRect.right = min(LegendMarkRect.left, LegendRect[1].left);
			InvalidRect.bottom = WinHeight;
			MOVERECT(InvalidRect, m_ShowMode);
			BitBlt(hDC, InvalidRect.left, InvalidRect.top, InvalidRect.right - InvalidRect.left, InvalidRect.bottom - InvalidRect.top,
				hBackDC, InvalidRect.left, InvalidRect.top , SRCCOPY);

			//重要，因为横坐标刻度值可能会写到LegendRect内部去，如果没有下面的代码，屏幕上可能会留下脏数据
			InvalidRect.left = min(LegendMarkRect.left, LegendRect[1].left);
			InvalidRect.top = HLabelRect.top;
			InvalidRect.right = WinWidth;
			InvalidRect.bottom = WinHeight;
			MOVERECT(InvalidRect, m_ShowMode);
			BitBlt(hDC, InvalidRect.left, InvalidRect.top, InvalidRect.right - InvalidRect.left, InvalidRect.bottom - InvalidRect.top,
				hBackDC, InvalidRect.left, InvalidRect.top , SRCCOPY);

			//生成真正的InvalidRect（两次BitBlt的并集）
			InvalidRect.left = 0;
			InvalidRect.top = VAxisRect.top;
			InvalidRect.right = WinWidth;
			InvalidRect.bottom = WinHeight;
			MOVERECT(InvalidRect, m_ShowMode);
		}
		else
		{
			RECT rect;
			ERASEBKG(UnitRectMask, UnitRect, TRUE);
			ERASEBKG(LegendMarkRectMask, LegendMarkRect, TRUE);
			ERASEBKG(TimeRectMask, TimeRect, TRUE);
			ERASEBKG(VAxisRectMask, VAxisRect, TRUE);
			ERASEBKG(HAxisRectMask, HAxisRect, TRUE);
			ERASEBKG(VLabelRectMask, VLabelRect, TRUE);
			ERASEBKG(HLabelRectMask, HLabelRect, TRUE);
			ERASEBKG(LegendRectMask, (*LegendRect), FALSE);
			ERASEBKG(CanvasRectMask, (*CanvasRect), FALSE);
			ERASEBKG(CurveTitleRectMask, CurveTitleRect, TRUE);
			ERASEBKG(FootNoteRectMask, FootNoteRect, TRUE);
		}

	if (!(SysState & 1) && m_ShowMode & 3) //映射坐标，打印的时候会在别处映射坐标
		CHANGE_MAP_MODE(hDC, m_ShowMode);

	if (!(SysState & 0x200000))
	{
		SetTextColor(hDC, m_foreColor); //打印或刷新都要执行的程序
		UPDATERECT(VAxisRectMask, DrawVAxis);
//		Rectangle(hDC, VAxisRect.left, VAxisRect.top, VAxisRect.right, VAxisRect.bottom);
		UPDATERECT(HAxisRectMask, DrawHAxis);
//		Rectangle(hDC, HAxisRect.left, HAxisRect.top, HAxisRect.right, HAxisRect.bottom);
	}
	if (Mask & CanvasRectMask)
	{
		DrawCurve(hDC, hScreenRgn, DataListIter); //本函数可能会更改TextColor属性
		if (SysState & 0x80000000) //绘制全局位置窗口时，要保持坐标映射为初始化状态（即未映射状态）
		{
			if (m_ShowMode & 3) //恢复坐标
				CHANGE_MAP_MODE(hDC, 0);
			DrawPreviewView(hDC); //打印时，不会绘制全局位置窗口
			if (m_ShowMode & 3) //映射坐标
				CHANGE_MAP_MODE(hDC, m_ShowMode);
		}
	}
	if (!(SysState & 0x200000))
	{
		SetTextColor(hDC, m_foreColor); //打印或刷新都要执行的程序
		UPDATERECT(UnitRectMask, DrawUnit);
//		Rectangle(hDC, UnitRect.left, UnitRect.top, UnitRect.right, UnitRect.bottom);
		UPDATERECT(LegendMarkRectMask, DrawLegendSign);
//		Rectangle(hDC, LegendMarkRect.left, LegendMarkRect.top, LegendMarkRect.right, LegendMarkRect.bottom);
		UPDATERECT(LegendRectMask, DrawLegend);
//		Rectangle(hDC, LegendRect[1].left, LegendRect[1].top, LegendRect[1].right, LegendRect[1].bottom);
		UPDATERECT(TimeRectMask, DrawTime);
//		Rectangle(hDC, TimeRect.left, TimeRect.top, TimeRect.right, TimeRect.bottom);
		UPDATERECT(VLabelRectMask, DrawVLabel);
//		Rectangle(hDC, VLabelRect.left, VLabelRect.top, VLabelRect.right, VLabelRect.bottom);
		UPDATERECT(HLabelRectMask, DrawHLabel);
//		Rectangle(hDC, HLabelRect.left, HLabelRect.top, HLabelRect.right, HLabelRect.bottom);
		if (!(SysState & 1)) //非打印才执行的程序
		{
			//2012.8.5
			//CHVIEWORG这个宏调用有什么作用？不调用后果是什么？
//			CHVIEWORG(hDC, WinWidth, WinHeight, m_ShowMode); //打印时，少了这个映射，文字稍微有点错位
			if (*CurveTitle && Mask & CurveTitleRectMask)
			{
				SetTextColor(hDC, m_titleColor);
				SelectObject(hDC, hTitleFont);
				DrawCurveTitle(hDC);
//				Rectangle(hDC, CurveTitleRect.left, CurveTitleRect.top, CurveTitleRect.right, CurveTitleRect.bottom);
				SelectObject(hDC, hFont);
			}

			SetTextColor(hDC, m_footNoteColor);
			UPDATERECT(FootNoteRectMask, DrawFootNote);
//			Rectangle(hDC, FootNoteRect.left, FootNoteRect.top, FootNoteRect.right, FootNoteRect.bottom);
		}
	} //if (!(SysState & 0x200000))
	if (!(SysState & 1)) //非打印才执行的程序，打印的时候会在别处调用SelectClipRgn选择打印区域，并调用DrawCurve函数
	{
		if (m_ShowMode & 3)
			CHANGE_MAP_MODE(hDC, 0);

		if (bUpdate)
			InvalidateControl(&InvalidRect, FALSE);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CST_CurveCtrl message handlers
void CST_CurveCtrl::OnForeColorChanged() 
{
	CHECKCOLOR(m_foreColor);
	UINT UpdateMask = ForeRectMask;
	if (WaterMark[0] || SysState & 0x40000)
	{
		UpdateMask = AllRectMask;
		DrawBkg();
	}
	UpdateRect(hFrceDC, UpdateMask);

	SetModifiedFlag();
}

void CST_CurveCtrl::OnBackColorChanged() 
{
	CHECKCOLOR(m_backColor);
	if (-1 == nBkBitmap || 0 != (m_BkMode & 0x81) || -1 != nCanvasBkBitmap)
	{
		DrawBkg();
		UpdateRect(hFrceDC, AllRectMask);
	}
	else //全局位置窗口需要背景色来填充
		UpdateRect(hFrceDC, PreviewRectMask);

	SetModifiedFlag();
}

void CST_CurveCtrl::OnAxisColorChanged() 
{
	CHECKCOLOR(m_axisColor);
	DELETEOBJECT(hAxisPen);
	hAxisPen = CreatePen(PS_SOLID, 1, m_axisColor);

	//下面两行语句，不能集中用一个UpdateRect语句来调用，具体原因参看UpdateRect参数说明
	UpdateRect(hFrceDC, VAxisRectMask | HAxisRectMask);
	UpdateRect(hFrceDC, PreviewRectMask);
	RECT rect = {VAxisRect.left, VAxisRect.bottom, HAxisRect.left, HAxisRect.bottom};
	InvalidateControl(&rect, FALSE);
	//靠近原点纵坐标和横坐标都有5个不属于VAxisRect也不属于VAxisRect的像素，但DrawHAxis函数与DrawVAxis都会去绘制它们，所以这里只需要刷新这个区域即可

	SetModifiedFlag();
}

void CST_CurveCtrl::OnGridColorChanged() 
{
	CHECKCOLOR(m_gridColor);
	DrawBkg();
	UpdateRect(hFrceDC, CanvasRectMask);

	SetModifiedFlag();
}

void CST_CurveCtrl::OnTitleColorChanged() 
{
	CHECKCOLOR(m_titleColor);
	UpdateRect(hFrceDC, TitleRectMask); //CurveTitleRect包括UnitRect和LegendMarkRect

	SetModifiedFlag();
}

void CST_CurveCtrl::OnFootNoteColorChanged() 
{
	CHECKCOLOR(m_footNoteColor);
	UpdateRect(hFrceDC, FootNoteRectMask);

	SetModifiedFlag();
}

BOOL CST_CurveCtrl::SetVInterval(short VInterval)
{
	if (0 <= VInterval && VInterval < 100)
	{
		if (VInterval != m_vInterval)
		{
			m_vInterval = VInterval;
			if (VCoorData.nScales > 0)
				VCoorData.reserve(1 + VCoorData.nScales / (m_vInterval + 1));

			DrawBkg();
			UpdateRect(hFrceDC, VAxisRectMask | VLabelRectMask | CanvasRectMask);
		}
		return TRUE;
	}

	return FALSE;
}

BOOL CST_CurveCtrl::SetHInterval(short HInterval)
{
	if (0 <= HInterval && HInterval < 100)
	{
		if (HInterval != m_hInterval)
		{
			m_hInterval = HInterval;
			if (HCoorData.nScales > 0)
				HCoorData.reserve(2 * (1 + HCoorData.nScales / (m_hInterval + 1)));

			DrawBkg();
			UpdateRect(hFrceDC, HAxisRectMask | HLabelRectMask | CanvasRectMask);
		}
		return TRUE;
	}

	return FALSE;
}
short CST_CurveCtrl::GetScaleInterval() {return (m_hInterval << 8) + m_vInterval;}

BOOL CST_CurveCtrl::SetLegendSpace(short LegendSpace)
{
	if (WinWidth - m_LegendSpace > CanvasRect[1].left + HSTEP)
	{
		BOOL bKeep = LegendSpace < 0; //在LegendSpace小于零的时候，如果图例可以显示完全，则不更改图例宽度，在LegendSpace等于零时，计算最小图例宽度并应用
		if (LegendSpace <= 0) //此时计算最小的m_LegendSpace值
		{
			LegendSpace = 30;
			for (auto i = begin(LegendArr); i < end(LegendArr); ++i)
				if (i->SignWidth > LegendSpace)
					LegendSpace = i->SignWidth;

			++LegendSpace;
		}

		if (LegendSpace != m_LegendSpace && (!bKeep || m_LegendSpace < LegendSpace))
		{
			m_LegendSpace = LegendSpace;
			RightSpace = fHeight + 1 + 1 + m_LegendSpace; //右空白
			ReSetUIPosition(WinWidth, WinHeight);
		}

		return TRUE;
	}

	return FALSE;
}
short CST_CurveCtrl::GetLegendSpace() {return m_LegendSpace;}

void CST_CurveCtrl::AddBitmap(HBITMAP hBitmap, UINT State)
{
	ASSERT(hBitmap);
	auto i = find(begin(BitBmps), end(BitBmps), hBitmap);
	if (i >= end(BitBmps))
	{
		BitBmp NoUse = {hBitmap, State};
		BitBmps.push_back(NoUse);
	}
}

BOOL CST_CurveCtrl::AddImageHandle(LPCTSTR pFileName, BOOL bShared)
{
	TCHAR FileName[MAX_PATH];
	if (IsBadStringPtr(pFileName, -1) || !*pFileName) //不可读或为空字符串
	{
		/*
		OPENFILENAME of;
		memset(&of, 0, sizeof(OPENFILENAME));
		of.lStructSize = sizeof(OPENFILENAME);
		of.hwndOwner = m_hWnd;
		of.lpstrFilter = _T("Microsoft Bitmap Files (*.bmp)\0*.bmp\0PNG Files (*.png)\0*.png\0JPEG Files (*.jpeg;*.jpg)\0*.jpeg;*.jpg\0GIF Files (*.gif)\0*.gif\0\0");
		of.lpstrFile = FileName;
		of.nMaxFile = MAX_PATH;
		of.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
		if (!GetOpenFileName(&of))
			return FALSE;
		*/
		CFileDialog Open_Dialog(TRUE, nullptr, nullptr, OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER,
			_T("Microsoft Bitmap Files (*.bmp)|*.bmp|PNG Files (*.png)|*.png|JPEG Files (*.jpeg;*.jpg)|*.jpeg;*.jpg|GIF Files (*.gif)|*.gif||"), nullptr);
		static DWORD nFilterIndex = 1;
		Open_Dialog.m_ofn.nFilterIndex = nFilterIndex;
		if (IDOK == Open_Dialog.DoModal())
		{
			nFilterIndex = Open_Dialog.m_ofn.nFilterIndex;
			_tcsncpy_s(FileName, Open_Dialog.GetPathName(), _TRUNCATE);
		}
		else
			return FALSE;

		pFileName = FileName;
	}

	ATL::CImage Image;
	auto hr = Image.Load(pFileName);
	if (SUCCEEDED(hr))
	{
		AddBitmap(Image.Detach(), 2 + bShared); //位图由控件自己创建
		return TRUE;
	}
	else
		return FALSE;
}

void CST_CurveCtrl::AddBitmapHandle(OLE_HANDLE hBitmap, BOOL bShared)
{
	if (hBitmap)
		AddBitmap(Format64bitHandle(HBITMAP, hBitmap), bShared); //位图由外界创建
}

BOOL CST_CurveCtrl::AddBitmapHandle2(OLE_HANDLE hInstance, LPCTSTR pszResourceName, BOOL bShared)
{
	if (hInstance)
	{
		auto hBitmap = LoadImage(Format64bitHandle(HINSTANCE, hInstance), pszResourceName, IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
		if (hBitmap)
		{
			AddBitmap((HBITMAP) hBitmap, 2 + bShared); //位图由控件自己创建
			return TRUE;
		}
	}

	return FALSE;
}
BOOL CST_CurveCtrl::AddBitmapHandle3(OLE_HANDLE hInstance, long nIDResource, BOOL bShared) //通过资源ID添加背景位图，hInstance为资源的来源
	{return AddBitmapHandle2(hInstance, MAKEINTRESOURCE(nIDResource), bShared);}

BOOL CST_CurveCtrl::SetBkBitmap(short nIndex)
{
	if (-1 == nIndex || 0 <= nIndex && (size_t) nIndex < BitBmps.size())
	{
		if (nBkBitmap != nIndex)
		{
			nBkBitmap = nIndex;
			DrawBkg();
			UpdateRect(hFrceDC, AllRectMask);
		}

		return TRUE;
	}

	return FALSE;
}
short CST_CurveCtrl::GetBkBitmap() {return nBkBitmap;}

BOOL CST_CurveCtrl::SetBkMode(short BkMode)
{
	short HMode = BkMode & 0x80;
	BkMode &= ~0x80;
	if (0 > BkMode || BkMode >= 3)
		return FALSE;

	UINT UpdateMask = 0;
	if ((HMode ^ m_BkMode) & 0x80 || m_BkMode != (BYTE) BkMode) //第8位有变化
	{
		m_BkMode = (BYTE) BkMode;
		m_BkMode |= (BYTE) HMode;
		DrawBkg();
		UpdateRect(hFrceDC, AllRectMask);
	}

	return TRUE;
}
short CST_CurveCtrl::GetBkMode() {return m_BkMode;}

BOOL CST_CurveCtrl::RemoveBitmapHandle(OLE_HANDLE hBitmap, BOOL bDel)
{
	auto i = find(begin(BitBmps), end(BitBmps), Format64bitHandle(HBITMAP, hBitmap)); //如果未找到，不需要做特殊处理，处理结果仍然正确
	return RemoveBitmapHandle2((short) distance(begin(BitBmps), i), bDel);
}

BOOL CST_CurveCtrl::RemoveBitmapHandle2(short nIndex, BOOL bDel)
{
	if (0 <= nIndex && (size_t) nIndex < BitBmps.size())
	{
		if (bDel || BitBmps[nIndex].State & 2) //控件由自己创建时，不管bDel是否为真，都将释放资源
			DeleteObject(BitBmps[nIndex]);
		BitBmps.erase(next(begin(BitBmps), nIndex));

		UINT UpdateMask = 0;
		for (auto LegendIter = begin(LegendArr); LegendIter < end(LegendArr); ++LegendIter)
			if (255 != LegendIter->BrushStyle && LegendIter->BrushStyle > 127) //采用了位图画刷
			{
				auto nBitmap = LegendIter->BrushStyle - 128;
				UpdateImageIndex(nBitmap, LegendIter->BrushStyle = 0xFF, ;, CanvasRectMask | LegendRectMask);
			}

		for (auto iter = begin(CommentDataArr); iter < end(CommentDataArr); ++iter)
			UpdateImageIndex(iter->nBkBitmap, iter->nBkBitmap = -1, ;, CanvasRectMask);

		auto bDrawBkg = FALSE;
		UpdateImageIndex(nBkBitmap, nBkBitmap = -1, bDrawBkg = TRUE, AllRectMask);
		UpdateImageIndex(nCanvasBkBitmap, nCanvasBkBitmap = -1, bDrawBkg = TRUE, CanvasRectMask);

		if (UpdateMask)
		{
			if (bDrawBkg)
				DrawBkg();
			UpdateRect(hFrceDC, UpdateMask);
		}

		return TRUE;
	}

	return FALSE;
}

long CST_CurveCtrl::GetBitmapCount() {return (long) BitBmps.size();}
OLE_HANDLE CST_CurveCtrl::GetBitmap(short nIndex)
{
	if (0 <= nIndex && (size_t) nIndex < BitBmps.size() && BitBmps[nIndex].State & 1) //只有共享的位图才能从外部获取控件内部的位图句柄
		return SplitHandle((HBITMAP) BitBmps[nIndex]);

	return NULL;
}

short CST_CurveCtrl::GetBitmapState(short nIndex)
{
	ASSERT(0 <= nIndex && (size_t) nIndex < BitBmps.size());
	if (0 <= nIndex && (size_t) nIndex < BitBmps.size())
		return (short)(UINT) BitBmps[nIndex];

	return -1;
}

short CST_CurveCtrl::GetBitmapState2(OLE_HANDLE hBitmap)
{
	auto i = find(begin(BitBmps), end(BitBmps), Format64bitHandle(HBITMAP, hBitmap));
	return GetBitmapState((short) distance(begin(BitBmps), i)); //如果未找到，不需要做特殊处理，处理结果仍然正确
}

void CST_CurveCtrl::iSetFont(HFONT hFont)
{
	if (this->hFont != hFont)
	{
		DELETEOBJECT(this->hFont);
		this->hFont = hFont;

		DELETEOBJECT(hTitleFont);
		LOGFONT lf;
		GetObject(this->hFont, sizeof(LOGFONT), &lf);
		lf.lfHeight += lf.lfHeight / 5;
		lf.lfWeight = 700;
		hTitleFont = CreateFontIndirect(&lf);

		UINT OldfHeight = fHeight;
		if (hFrceDC)
			SelectObject(hFrceDC, this->hFont);

		HDC hDC;
		if (hFrceDC)
			hDC = hFrceDC;
		else
		{
			hDC = ::GetDC(m_hWnd); //这里有可能hFrceDC还没有被创建
			::SelectObject(hDC, this->hFont);
		}

		TEXTMETRIC tm;
		GetTextMetrics(hDC, &tm);
		fHeight = (BYTE) tm.tmHeight;
		fWidth = (BYTE) tm.tmAveCharWidth;

		SIZE size_min = {0}, size_max = {0};
		GetTextExtentPoint32(hDC, _T("iI"), 2, &size_min);
		GetTextExtentPoint32(hDC, _T("wW"), 2, &size_max);
		if (size_min.cx == size_max.cx)
			FontPitch = FIXED_PITCH;
		else
			FontPitch = VARIABLE_PITCH;

		if (!hFrceDC)
			::ReleaseDC(m_hWnd, hDC);

		TopSpace	= 2 + fHeight + 5; //上空白
		RightSpace	= fHeight + 1 + m_LegendSpace; //右空白
		BottomSpace = 5 + (BottomSpaceLine + 1) * (2 +  fHeight); //下空白

		UnitRect.right = LEFTSPACE + 16 * fWidth;
		UnitRect.bottom = UnitRect.top + fHeight;

		VAxisRect.top = TopSpace;
		VLabelRect.top = TopSpace + 11 - fHeight / 2;
		LegendMarkRect.bottom = LegendMarkRect.top + fHeight;

		if (!OldfHeight)
		{
			CurveTitleRect.bottom = CurveTitleRect.top + fHeight + 5;
			LegendRect[1].top =  LegendRect[1].bottom = TopSpace;
		}
		else
		{
			LegendRect[1].bottom += (LegendRect[1].bottom - LegendRect[1].top) / (OldfHeight + 1) * (fHeight - OldfHeight);
			//LegendRect[1].bottom - LegendRect[1].top一定是OldfHeight + 1整数陪，所以不用考虑取整造成的损失
			ReSetUIPosition(WinWidth, WinHeight);

			if (hFrceDC)
				hDC = hFrceDC;
			else
			{
				hDC = ::GetDC(m_hWnd); //这里有可能hFrceDC还没有被创建
				::SelectObject(hDC, this->hFont);
			}
			SIZE size = {0};
			for (auto i = begin(LegendArr); i < end(LegendArr); ++i)
			{
				GetTextExtentPoint32(hDC, *i, (int) _tcslen(*i), &size);
				i->SignWidth = (short) size.cx + 1;
			}
			if (!hFrceDC)
				::ReleaseDC(m_hWnd, hDC);
			SetLegendSpace(0);
		}

		LegendRect[0] = LegendRect[1];
		MOVERECT(LegendRect[0], m_ShowMode);
	}
}

void CST_CurveCtrl::SetFont(OLE_HANDLE hFont) 
{
	HFONT hThisFont = nullptr;
	if (!hFont) //为0时，本控件弹出字体选择框，让用户选择
	{
		LOGFONT lf;
		GetObject(this->hFont, sizeof(LOGFONT), &lf); //能调用SetFont函数的时候，this->hFont不可能为0

		CHOOSEFONT cf;
		memset(&cf, 0, sizeof(CHOOSEFONT));
		cf.lStructSize = sizeof(CHOOSEFONT);
		cf.hwndOwner = m_hWnd;
		cf.lpLogFont = &lf;
		cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;
		if (ChooseFont(&cf))
			hThisFont = CreateFontIndirect(&lf);
		else
			return;
	}
	else
		hThisFont = Format64bitHandle(HFONT, hFont);

	iSetFont(hThisFont);
}

void CST_CurveCtrl::OnPageChangeMSGChanged() {SetModifiedFlag(FALSE);}
void CST_CurveCtrl::OnMSGRecWndChanged() 
{
	if (!AmbientUserMode()) //设计模式下不允许更改这个值
	{
		m_MSGRecWnd = 0;
		MessageBeep(-1);
	}
	else
		m_iMSGRecWnd = Format64bitHandle(HWND, m_MSGRecWnd);

	SetModifiedFlag(FALSE);
}

short CST_CurveCtrl::GetVPrecision() {return VPrecisionExp[2] - 48;}
BOOL CST_CurveCtrl::SetVPrecision(short Precision)
{
	if (0 <= Precision && Precision <= 6)
	{
		if (VPrecisionExp[2] != 48 + Precision)
		{
			VPrecisionExp[2] = 48 + Precision;
			//本来在设置纵坐标精度的时候，因为精度更改，肯定会移动纵坐标的，但现在由于有了联动关系，所以有可能不会移动纵坐标
			if (!SetLeftSpace())
			{
				UINT UpdateMask = VLabelRectMask;
				for (auto i = begin(MainDataListArr); i < end(MainDataListArr); ++i)
				{
					if (NullLegendIter != i->LegendIter && i->LegendIter->Lable & 2) //显示Y坐标，这里需要更新
					{
						UpdateMask |= CanvasRectMask;
						break;
					}
				}

				UpdateRect(hFrceDC, UpdateMask);
			} //if (!SetLeftSpace())
		} //if (VPrecisionExp[2] != 48 + Precision)

		return TRUE;
	} //if (0 <= Precision && Precision <= 6)

	return FALSE;
}

short CST_CurveCtrl::GetHPrecision() {return HPrecisionExp[2] - 48;}
BOOL CST_CurveCtrl::SetHPrecision(short Precision)
{
	if (0 <= Precision && Precision <= 6)
	{
		if (HPrecisionExp[2] != 48 + Precision)
		{
			HPrecisionExp[2] = 48 + Precision;
			if (m_ShowMode & 0x80)
			{
				UINT UpdateMask = HLabelRectMask;
				for (auto i = begin(MainDataListArr); i < end(MainDataListArr); ++i)
				{
					if (NullLegendIter != i->LegendIter && i->LegendIter->Lable & 1) //显示X坐标，这里需要更新
					{
						UpdateMask |= CanvasRectMask;
						break;
					}
				}

				UpdateRect(hFrceDC, UpdateMask);
			} //if (m_ShowMode & 0x80)
		} //if (HPrecisionExp[2] != 48 + Precision)

		return TRUE;
	} //if (0 <= Precision && Precision <= 6)

	return FALSE;
}

double CST_CurveCtrl::GetTimeSpan() {return HCoorData.fStep;}
BOOL CST_CurveCtrl::SetTimeSpan(double TimeStep)
{
	if (HCoorData.fStep != TimeStep)
	{
		auto dStepTime = pCalcTimeSpan ? pCalcTimeSpan(TimeStep, Zoom, HZoom) : GETSTEP(TimeStep, Zoom + HZoom);
		if (IsTimeInvalidate)
			return FALSE;

		HCoorData.fStep = TimeStep;
		HCoorData.fCurStep = dStepTime;
		CalcOriginDatumPoint(OriginPoint, 5);

		SYNBUDDYS(3, &HCoorData.fStep);

		UINT UpdateMask = HLabelRectMask;
		if (nVisibleCurve > 0)
			UpdateMask |= CanvasRectMask;

		UpdateRect(hFrceDC, UpdateMask);

		return TRUE;
	}

	return 2;
}

float CST_CurveCtrl::GetValueStep() {return VCoorData.fStep;}
BOOL CST_CurveCtrl::SetValueStep(float ValueStep)
{
	if (VCoorData.fStep != ValueStep)
	{
		auto thisValueStep = pCalcValueStep ? pCalcValueStep(ValueStep, Zoom) : GETSTEP(ValueStep, Zoom);
		if (IsValueInvalidate)
			return FALSE;

		VCoorData.fStep = ValueStep;
		VCoorData.fCurStep = thisValueStep;
		CalcOriginDatumPoint(OriginPoint, 6);

		if (!SetLeftSpace())
		{
			UINT UpdateMask = VLabelRectMask;
			if (nVisibleCurve > 0)
				UpdateMask |= CanvasRectMask;

			UpdateRect(hFrceDC, UpdateMask);
		}

		return TRUE;
	}

	return 2;
}

BSTR CST_CurveCtrl::GetUnit() 
{
	CComBSTR strResult = Unit;
	return strResult.Copy();
}

BOOL CST_CurveCtrl::SetUnit(LPCTSTR pUnit)
{
	ASSERT(pUnit);
	if (!IsBadStringPtr(pUnit, -1)) //pUnit为空指针时也能正确判断
	{
		auto n = _tcslen(pUnit);
		if (n < StrUintLen)
		{
			if (0 != _tcscmp(Unit, pUnit))
			{
				_tcscpy_s(Unit, pUnit);

				UINT UpdateMask = UnitRectMask;
				for (auto i = begin(MainDataListArr); i < end(MainDataListArr); ++i)
				{
					if (NullLegendIter != i->LegendIter && 2 == (i->LegendIter->Lable & 6)) //显示Y坐标，并且没有隐藏单位，这里需要更新
					{
						UpdateMask |= CanvasRectMask;
						break;
					}
				}

				UpdateRect(hFrceDC, UpdateMask);
			} //if (0 != _tcscmp(Unit, pUnit))

			return TRUE;
		} //if (n < 16)
	} //if (!IsBadStringPtr(pUnit, -1))

	return FALSE;
}

BSTR CST_CurveCtrl::GetHUnit() 
{
	CComBSTR strResult = HUnit;
	return strResult.Copy();
}

BOOL CST_CurveCtrl::SetHUnit(LPCTSTR pHUnit)
{
	ASSERT(pHUnit);
	if (!IsBadStringPtr(pHUnit, -1)) //pHUnit为空指针时也能正确判断
	{
		auto n = _tcslen(pHUnit);
		if (n < StrUintLen)
		{
			if (0 != _tcscmp(HUnit, pHUnit))
			{
				_tcscpy_s(HUnit, pHUnit);

				if (m_ShowMode & 0x80)
				{
					UINT UpdateMask = TimeRectMask;
					for (auto i = begin(MainDataListArr); i < end(MainDataListArr); ++i)
					{
						if (NullLegendIter != i->LegendIter && 1 == (i->LegendIter->Lable & 5)) //显示X坐标，并且没有隐藏单位，这里需要更新
						{
							UpdateMask |= CanvasRectMask;
							break;
						}
					}

					UpdateRect(hFrceDC, UpdateMask);
				} //if (m_ShowMode & 0x80)
			} //if (0 != _tcscmp(HUnit, pHUnit))

			return TRUE;
		} //if (n < 16)
	} //if (!IsBadStringPtr(pHUnit, -1))

	return FALSE;
}

BOOL CST_CurveCtrl::ExportImage(LPCTSTR pFileName) {return hFrceDC && ::ExportImage(hFrceBmp, pFileName);}

//Style取值为：
//1－导出为图片；2－导出为ANSI文件；3－导出为Unicode文件；4－导出为Unicode big endian文件；5－导出为utf8文件；6－导出为二制文件
long CST_CurveCtrl::ExportImageFromPage(LPCTSTR pFileName, long Address, long nBegin, long nCount, BOOL bAll, short Style)
{
	if (nBegin < 1 || (-1 != nCount && nCount < 1))
		return 0;

	HCOOR_TYPE BTime = GetNearFrontPos(m_MinTime, OriginPoint.Time), ETime;
	//ETime没有初始化，在调试的时候，可能会弹出警告，不用理会，因为有Mask参数，没有初始化的值，一定用不上
	//至于BTime，由于后面在计算ETime时可能还需要，所以直接计算出来，不再使用Mask来代表其有效性（恒有效）
	short Mask = 3;
	if (nBegin > 1)
		BTime = GETPAGESTARTTIME(BTime, nBegin);
	if (-1 == nCount)
		Mask &= ~2; //ETime无效
	else
		ETime = GETPAGESTARTTIME(BTime, nCount + 1); //BTime肯定有效，由于从BTime开始（不是从首页开始），所以只需要计算nCount页

	if (1 < Style && Style <= 6)
	{
		vector<DataListHead<MainData>>::iterator ExportIter = NullDataListIter;
		if (!bAll)
		{
			ExportIter = FindMainData(Address);
			if (NullDataListIter == ExportIter)
				return 0;
		}

		auto MinTime = !(Mask & 1) || BTime <= m_MinTime ? m_MinTime : BTime;
		MinTime = GetNearFrontPos(MinTime, OriginPoint.Time);
		auto MaxTime = !(Mask & 2) || ETime >= m_MaxTime ? m_MaxTime : ETime;
		int nPageNum;
		GETPAGENUM(MinTime, MaxTime);
		if (!nPageNum)
			return 0; //无数据

		return ExportMetaFileFromTime(pFileName, Address, ExportIter, MinTime, MaxTime, Style);
	}
	else
		return ExportImageFromTime(pFileName, Address, BTime, ETime, Mask, bAll, Style); //这里使用ETime，可能出现上面提到的警告
}

#define BUFF_A_W_LEN	(16 + 32 + 16 + 8)
//ShowMode就是CST_CurveCtrl::m_ShowMode，用于确定横坐标的导出方式（DATE或者double值，对于时间，不能只导出日期或者只导出时间），只有最高位有用
static void WriteFile(HANDLE hFile, long Address, vector<MainData>::iterator MainDataIter, short Style, UINT ShowMode, char* pBuffA, wchar_t* pBuffW)
{
	DWORD WritedLen;
	if (6 == Style)
	{
		DWORD WriteLen = 0;
		WriteFile(hFile, &Address, 4, &WritedLen, nullptr);
		ASSERT(4 == WritedLen);
		WriteFile(hFile, MainDataIter._Ptr, 8 + 4 + 2, &WritedLen, nullptr);
		//低位在前的系统才有效，否则将写入State的最高两字节（目前并且在很长一段时间之内都将只使用低两字节）
		ASSERT(8 + 4 + 2 == WritedLen);
	}
	else if (2 == Style || 5 == Style)
		WRITEFILE(sprintf_s, pBuffA, "", len += sprintf_s(pBuffA + len, BUFF_A_W_LEN - len, "%S,", bstr), ;)
	else
		WRITEFILE(swprintf_s, pBuffW, L, len += swprintf_s(pBuffW + len, BUFF_A_W_LEN - len, L"%s,", bstr), if (4 == Style) for (auto i = 0; i < len; ++i) pBuffW[i] = htons(pBuffW[i]);)
}

long CST_CurveCtrl::ExportMetaFile(LPCTSTR pFileName, long Address, long nBegin, long nCount, BOOL bAll, short Style)
{
	if (2 > Style || Style > 6 || nCount < 0 && -1 != nCount) //Style只能取值2到6
		return 0;

	vector<DataListHead<MainData>>::iterator ExportIter = NullDataListIter;
	if (!bAll)
	{
		ExportIter = FindMainData(Address);
		if (NullDataListIter == ExportIter)
			return 0;
	}

	TCHAR FileName[MAX_PATH];
	*FileName = 0;
	if (!IsBadStringPtr(pFileName, -1) && *pFileName) //可读，不为空
		_tcsncpy_s(FileName, pFileName, _TRUNCATE);
	if (!*FileName)
	{
		CFileDialog Save_Dialog(FALSE, _T(""), nullptr, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_EXPLORER,
			6 == Style ? _T("ST_Curve binary Files (*.urv)|*.urv||") :
		_T("ANSI Text Files (*.txt)|*.txt|Unicode Text Files (*.txt)|*.txt|Unicode big endian Text Files (*.txt)|*.txt|UTF-8 Text Files (*.txt)|*.txt||"), nullptr);
		if (6 != Style)
			Save_Dialog.m_ofn.nFilterIndex = Style - 1;
		if (IDOK == Save_Dialog.DoModal())
		{
			if (6 != Style)
				Style = (short) (1 + Save_Dialog.m_ofn.nFilterIndex);
			_tcsncpy_s(FileName, Save_Dialog.GetPathName(), _TRUNCATE);
		}
		else
			return 0;
	}

	auto nTotalData = 0;
	auto hFile = CreateFile(FileName, FILE_GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (INVALID_HANDLE_VALUE != hFile)
	{
		if (2 < Style && Style < 6)
		{
			DWORD HeadLen;
			DWORD WritedLen;
			DWORD Head;
			switch (Style)
			{
			case 3:
				Head = 0xFEFF;
				HeadLen = 2;
				break;
			case 4:
				Head = 0xFFFE;
				HeadLen = 2;
				break;
			default:
				Head = 0xBFBBEF;
				HeadLen = 3;
				break;
			}
			WriteFile(hFile, &Head, HeadLen, &WritedLen, nullptr);
			ASSERT(HeadLen == WritedLen);
		}

		auto DataListIter = NullDataListIter == ExportIter ? begin(MainDataListArr) : ExportIter;
		while (DataListIter < end(MainDataListArr))
		{
			auto pDataVector = DataListIter->pDataVector;
			auto MainDataIter = begin(*pDataVector);

			char BuffA[BUFF_A_W_LEN];
			wchar_t BuffW[BUFF_A_W_LEN];

			MainDataIter += nBegin;
			for (auto nThisCount = 0; MainDataIter < end(*pDataVector) && (-1 == nCount || ++nThisCount <= nCount); ++MainDataIter, ++nTotalData)
				WriteFile(hFile, DataListIter->Address, MainDataIter, Style, m_ShowMode, BuffA, BuffW);

			if (!bAll) //只导出一条曲线
				break;
			else
				++DataListIter;
		} //while (DataListIter < end(MainDataListArr))

		CloseHandle(hFile);
	} //if (INVALID_HANDLE_VALUE != hFile)

	return nTotalData; //导出文件成功
}

long CST_CurveCtrl::ExportMetaFileFromTime(LPCTSTR pFileName, long Address, vector<DataListHead<MainData>>::iterator ExportIter, HCOOR_TYPE MinTime, HCOOR_TYPE MaxTime, short Style)
{
	TCHAR FileName[MAX_PATH];
	*FileName = 0;
	if (!IsBadStringPtr(pFileName, -1) && *pFileName) //可读，不为空
		_tcsncpy_s(FileName, pFileName, _TRUNCATE);
	if (!*FileName)
	{
		CFileDialog Save_Dialog(FALSE, _T(""), nullptr, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_EXPLORER,
			6 == Style ? _T("ST_Curve binary Files (*.urv)|*.urv||") :
		_T("ANSI Text Files (*.txt)|*.txt|Unicode Text Files (*.txt)|*.txt|Unicode big endian Text Files (*.txt)|*.txt|UTF-8 Text Files (*.txt)|*.txt||"), nullptr);
		if (6 != Style)
			Save_Dialog.m_ofn.nFilterIndex = Style - 1;
		if (IDOK == Save_Dialog.DoModal())
		{
			if (6 != Style)
				Style = (short) (1 + Save_Dialog.m_ofn.nFilterIndex);
			_tcsncpy_s(FileName, Save_Dialog.GetPathName(), _TRUNCATE);
		}
		else
			return 0;
	}

	auto nTotalData = 0;
	auto hFile = CreateFile(FileName, FILE_GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (INVALID_HANDLE_VALUE != hFile)
	{
		if (2 < Style && Style < 6)
		{
			DWORD HeadLen;
			DWORD WritedLen;
			DWORD Head;
			switch (Style)
			{
			case 3:
				Head = 0xFEFF;
				HeadLen = 2;
				break;
			case 4:
				Head = 0xFFFE;
				HeadLen = 2;
				break;
			default:
				Head = 0xBFBBEF;
				HeadLen = 3;
				break;
			}
			WriteFile(hFile, &Head, HeadLen, &WritedLen, nullptr);
			ASSERT(HeadLen == WritedLen);
		}

		auto DataListIter = NullDataListIter == ExportIter ? begin(MainDataListArr) : ExportIter;
		while (DataListIter < end(MainDataListArr))
		{
			auto pDataVector = DataListIter->pDataVector;
			auto MainDataIter = begin(*pDataVector);

			char BuffA[BUFF_A_W_LEN];
			wchar_t BuffW[BUFF_A_W_LEN];
			if (2 == DataListIter->Power) //二次曲线的导出效率是很低的
				for (; MainDataIter < end(*pDataVector); ++MainDataIter)
				{
					if (MinTime <= MainDataIter->Time && MainDataIter->Time <= MaxTime)
					{
						WriteFile(hFile, DataListIter->Address, MainDataIter, Style, m_ShowMode, BuffA, BuffW);
						++nTotalData;
					}
				}
			else
			{
				for (; MainDataIter < end(*pDataVector) && MainDataIter->Time < MinTime; ++MainDataIter);
				for (; MainDataIter < end(*pDataVector) && MainDataIter->Time <= MaxTime; ++MainDataIter, ++nTotalData)
					WriteFile(hFile, DataListIter->Address, MainDataIter, Style, m_ShowMode, BuffA, BuffW);
			}

			if (NullDataListIter != ExportIter) //只导出一条曲线
				break;
			else
				++DataListIter;
		} //while (DataListIter < end(MainDataListArr))

		CloseHandle(hFile);
	} //if (INVALID_HANDLE_VALUE != hFile)

	return nTotalData; //导出文件成功
}

long CST_CurveCtrl::ExportImageFromTime(LPCTSTR pFileName, long Address, HCOOR_TYPE BTime, HCOOR_TYPE ETime, short Mask, BOOL bAll, short Style)
{
	if (!hFrceDC || Style < 1 || Style > 6)
		return 0;

	vector<DataListHead<MainData>>::iterator ExportIter = NullDataListIter;
	if (!bAll)
	{
		ExportIter = FindMainData(Address);
		if (NullDataListIter == ExportIter || (1 == Style && (!ISCURVESHOWN(ExportIter) || ExportIter->Zx > nZLength * HSTEP))) //导出为图元文件时，不管曲线可见与否
			return 0;
	}

	auto MinTime = !(Mask & 1) || BTime <= m_MinTime ? m_MinTime : BTime;
	MinTime = GetNearFrontPos(MinTime, OriginPoint.Time);
	auto MaxTime = !(Mask & 2) || ETime >= m_MaxTime ? m_MaxTime : ETime;
	int nPageNum;
	GETPAGENUM(MinTime, MaxTime);
	if (!nPageNum)
		return 0; //无数据

	if (1 < Style) //导出为图元文件
		return ExportMetaFileFromTime(pFileName, Address, ExportIter, MinTime, MaxTime, Style);

	if (IsBadStringPtr(pFileName, -1) || !*pFileName)
		return 0;

	auto len = _tcslen(pFileName) + 1;
	BatchExport be;
	be.pFileName = new TCHAR[len];
	_tcscpy_s(be.pFileName, len, pFileName);
//	be.pStart = _tcschr(be.pFileName, _T('*'));
	be.pStart = find(be.pFileName, be.pFileName + len, _T('*'));
	if (be.pStart >= be.pFileName + len)
	{
		delete[] be.pFileName;
		return 0;
	}

	//正向找到'*'后，反向一定也可以找得到
//	be.nWidth = _tcsrchr(be.pFileName, _T('*')) - be.pStart + 1;
	be.nWidth = (UINT) ((find(reverse_iterator<TCHAR*>(be.pFileName + len), reverse_iterator<TCHAR*>(be.pFileName), _T('*')) + 1).base() - be.pStart + 1);
	_stprintf_s(be.cNumFormat, _T("%%0%uu"), be.nWidth);
	be.nFileNum = 0;

	SetRedraw(FALSE);
	SysState |= 2;

	BOOL bPrintAllPage = m_MaxTime == MaxTime;
	auto OldBeginValue = OriginPoint.Value; //保存当前页纵坐标开始值
	auto OldBeginTime = OriginPoint.Time;  //保存当前页开始时间
	SetBeginTime2(MinTime);
	auto bNeedCHPage = FALSE;
	long nTotalPage = 0;
	for (auto i = 0; i < nPageNum || bPrintAllPage; ++i)
	{
		while (bNeedCHPage || !ReSetCurvePosition(5, FALSE, ExportIter))
		{
			auto PageStep = GotoPage(1, FALSE);
			if (!PageStep)
				goto EXPORTOVER;

			i += PageStep;
			--i; //中和掉for循环里面的++i
			if (!bPrintAllPage && i >= nPageNum) //这里的判断是非常有必要的，因为GotoPage有可能一次翻多页
				goto EXPORTOVER;

			bNeedCHPage = FALSE;
		}
		bNeedCHPage = TRUE;

		UpdateRect(hFrceDC, AllRectMask, ExportIter, FALSE);
		while (++be.nFileNum)
		{
			auto nNum = _sntprintf_s(StrBuff, _TRUNCATE, be.cNumFormat, be.nFileNum);
			if (nNum != be.nWidth) //已经溢出了，比如pFileName为：c:\****.bmp，而此时nFileNum已经大于4位数了，比如10000
				goto EXPORTOVER;

			memcpy(be.pStart, StrBuff, be.nWidth * sizeof(TCHAR));
			if (_taccess(be.pFileName, 0)) //文件不存在
				break;
		}
		if (be.nFileNum)
			::ExportImage(hFrceBmp, be.pFileName);
		else //文件名序号溢出，也就是UINT数据溢出
			break;

		++nTotalPage;
	} //for (auto i = 0; i < nPageNum || bPrintAllPage; ++i)

EXPORTOVER:
	SetBeginTime2(OldBeginTime);  //恢复当前页的开始时间
	SetBeginValue(OldBeginValue); //恢复当前页的纵坐标开始值

	SysState &= ~2;
	SetRedraw();
	UpdateRect(hFrceDC, AllRectMask);

	delete[] be.pFileName;
	return nTotalPage;
}

BOOL CST_CurveCtrl::BatchExportImage(LPCTSTR pFileName, long nSecond) //nSecond的单位为秒
{
	if (nSecond <= 0)
	{
		KillTimer(BATCHEXPORTBMP);
		if (m_BE)
		{
			if (m_BE->pFileName)
				delete[] m_BE->pFileName;
			delete m_BE;
			m_BE = nullptr;

			FIRE_BatchExportImageChange(0); //0代表批量导出结束
		}
	}
	else if (!m_BE) //最多只能有一个定时器定时导出图片
	{
		if (IsBadStringPtr(pFileName, -1) || !*pFileName)
			return FALSE;

		auto Len = _tcslen(pFileName) + 1;
		m_BE = new BatchExport;
		m_BE->pFileName = new TCHAR[Len];
		_tcscpy_s(m_BE->pFileName, Len, pFileName);
//		m_BE->pStart = _tcschr(m_BE->pFileName, _T('*'));
		m_BE->pStart = find(m_BE->pFileName, m_BE->pFileName + Len, _T('*'));
		if (m_BE->pStart >= m_BE->pFileName + Len)
		{
			BatchExportImage(0, 0); //释放m_BE
			return FALSE;
		}

		//正向找到'*'后，反向一定也可以找得到
//		m_BE->nWidth = _tcsrchr(m_BE->pStart, _T('*')) - m_BE->pStart + 1;
		m_BE->nWidth = (UINT) ((find(reverse_iterator<TCHAR*>(m_BE->pFileName + Len), reverse_iterator<TCHAR*>(m_BE->pFileName), _T('*')) + 1).base() - m_BE->pStart + 1);
		_stprintf_s(m_BE->cNumFormat, _T("%%0%uu"), m_BE->nWidth);
		m_BE->nFileNum = 0;

		SetTimer(BATCHEXPORTBMP, nSecond * 1000, nullptr);
	}

	return TRUE;
}

//Style取值为：
//2－从文本文件导入；6－二制文件导入
//返回值：-1表示参数错误或者用户取消导出
//否则高2字节代表文件里面的总数据条数（文本文件一行为一条数据，二进制文件18字节为一条数据） ，低2字节代表成功添加的数据条数，均当成无符号数看待
long CST_CurveCtrl::ImportFile(LPCTSTR pFileName, short Style, BOOL bAddTrail)
{
	bAddTrail = TRUE;

	TCHAR FileName[MAX_PATH];
	*FileName = 0;
	if (!IsBadStringPtr(pFileName, -1) && *pFileName) //可读，不为空
		_tcsncpy_s(FileName, pFileName, _TRUNCATE);
	if (!*FileName)
	{
		CFileDialog Open_Dialog(TRUE, nullptr, nullptr, OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER,
			_T("ST_Curve binary Files (*.urv)|*.urv|Text Files (*.txt)|*.txt||"), nullptr);
		static DWORD nFilterIndex = 1;
		Open_Dialog.m_ofn.nFilterIndex = nFilterIndex;
		if (IDOK == Open_Dialog.DoModal())
		{
			nFilterIndex = Open_Dialog.m_ofn.nFilterIndex;
			_tcsncpy_s(FileName, Open_Dialog.GetPathName(), _TRUNCATE);
		}
		else
			return -1;

		Style = 2 == Open_Dialog.m_ofn.nFilterIndex ? 2 : 6; //nFilterIndex为2时，从文本文件中导入（nFilterIndex从1开始）
	}
	else if (2 != Style && 6 != Style)
		return -1;

	auto hFile = CreateFile(FileName, FILE_GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (INVALID_HANDLE_VALUE == hFile)
		return 0;

#define BUFFLEN		(1024 * 1024)
#define BINBUFFLEN	(BUFFLEN / 18 * 18) //读取二进制文件时，按18的整数倍数来读取

	auto nTotalRow = 0, nImportRow = 0;
	DWORD len;
	if (6 == Style) //读取二进制文件
	{
		auto BinBuff = new BYTE[BINBUFFLEN];
		while(ReadFile(hFile, BinBuff, BINBUFFLEN, &len, nullptr) && len)
		{
			nTotalRow += len / 18; //总行数
			nImportRow += iAddMemMainData(BinBuff, len, bAddTrail); //添加成功的行数
		}
		delete[] BinBuff;
	}
	else //文本文件，先读取文本文件头，以确定文件格式：ansi、unicode、unicode big endian、utf8
	{
		DWORD Head = 0;
		if (!ReadFile(hFile, &Head, 3, &len, nullptr) || len < 3)
			return 0;

		if (0xBFBBEF == Head) //utf8
			Style = 5;
		else
		{
			Head &= 0xFFFF;
			if (0xFEFF == Head) //3 unicode
				Style = 3;
			else if (0xFFFE == Head) //4 unicode big endian
				Style = 4;
			SetFilePointer(hFile, 2 == Style ? -3 : -1, nullptr, FILE_CURRENT);
		}

		auto BinBuff = new BYTE[BINBUFFLEN];
		auto pBinBuff = BinBuff;
		DWORD pos = 0;
		if (3 == Style || 4 == Style) //读取unicode文件
		{
			UINT State = 0; //0－未知格式；1－普通时间；2－浮点数
			auto Buff = new wchar_t[BUFFLEN];
			while (ReadFile(hFile, (char*) Buff + pos, BUFFLEN * sizeof(wchar_t) - pos, &len, nullptr) && len)
			{
				auto TotalLen = len + pos; //缓冲里面的字节数
				BOOL bReadOver = TotalLen < BUFFLEN * sizeof(wchar_t); //数据是否已读完
				len = TotalLen / 2; //缓冲里面的字符数
				if (4 == Style) //对于unicode big endian文件，需要高低字节互换
					for (UINT i = 0; i < len; ++i)
						Buff[i] = ntohs(Buff[i]);
				--len;
				auto pRowStart = Buff; //swscanf读取位置
				UINT i = 0;
				for (; i < len; ++i)
					if (0xA000D == *(DWORD*) (Buff + i))
					{
						SCANDATA(swscanf_s, L"%d,%lf,%f,%hd", L"%d", L"%f,%hd");
						++i; //现在i指向0x000A
						pRowStart = Buff + i + 1; //跳过0x000A
					}
				TotalLen -= (UINT) distance(Buff, pRowStart) * sizeof(wchar_t); //剩下的数据的字节数
				if (TotalLen)
				{
					memcpy(Buff, pRowStart, TotalLen); //将剩下的数据移动到缓冲区最前面
					len = TotalLen / 2; //缓冲里面的字符数
					if (len > 0)
						if (bReadOver && len > 0) //数据已经读取完毕，最后一行没有回车，这里要对最后一行进行读取
						{
							Buff[len] = 0; //由于要赋这个结束符，所以将数据拷贝到数组最前面（至少要在最后面让出一个位置出来），这样防止写数组溢出的可能
							SCANDATA(swscanf_s, L"%d,%lf,%f,%hd", L"%d", L"%f,%hd");
							break;
						}
						else if (4 == Style)
							for (i = 0; i < len; ++i)
								Buff[i] = htons(Buff[i]);
				}
				pos = TotalLen; //下次数据开始写入位置
			} //while
			delete[] Buff;
		}
		else //读取ansi和utf8文件
		{
			UINT State = 0; //0－未知格式；1－普通时间；2－浮点数
			auto Buff = new char[BUFFLEN];
			while (ReadFile(hFile, Buff + pos, BUFFLEN - pos, &len, nullptr) && len)
			{
				len += pos; //缓冲区里面的总字节数
				--len;
				auto pRowStart = Buff; //sscanf读取位置
				UINT i = 0;
				for (; i < len; ++i)
					if (0xA0D == *(WORD*) (Buff + i))
					{
						SCANDATA(sscanf_s, "%d,%lf,%f,%hd", "%d", "%f,%hd");
						++i; //现在i指向0x0A
						pRowStart = Buff + i + 1; //跳过0x0A
					}
				++len; //缓冲区里面的总字节数
				BOOL bReadOver = len < BUFFLEN; //数据是否读取完毕
				len -= (UINT) distance(Buff, pRowStart); //缓冲区里面还剩下的字节数
				if (len > 0)
				{
					memcpy(Buff, pRowStart, len); //将剩下的数据移动到缓冲区最前面
					if (bReadOver) //数据已经读取完毕，最后一行没有回车，这里要对最后一行进行读取
					{
						Buff[len] = 0; //由于要赋这个结束符，所以将数据拷贝到数组最前面，这样防止写数组溢出的可能
						SCANDATA(sscanf_s, "%d,%lf,%f,%hd", "%d", "%f,%hd");
						break;
					}
				}
				pos = len; //下次数据开始写入位置
			} //while
			delete[] Buff;
		} //读取ansi和utf8文件

		nImportRow += iAddMemMainData(BinBuff, (long) distance(BinBuff, pBinBuff), bAddTrail);
		delete[] BinBuff;
	} //if (2 == Style)
	CloseHandle(hFile);

	return (nTotalRow << 16) + nImportRow;
}

long CST_CurveCtrl::iAddMemMainData(LPBYTE pMemMainData, long MemSize, BOOL bAddTrail)
{
	long re = 0;
	if (pMemMainData)
	{
		while (MemSize >= 18)
		{
			if (AddMainData2(*(long*) pMemMainData, *(HCOOR_TYPE*) (pMemMainData + 4), *(float*) (pMemMainData + 4 + 8),
				*(short*) (pMemMainData + 4 + 8 + 4), 0, bAddTrail))
				++re;

			pMemMainData += 18;
			MemSize -= 18;
		}
	}

	return re;
}

long CST_CurveCtrl::AddMemMainData(OLE_HANDLE pMemMainData, long MemSize, BOOL bAddTrail) //MemSize指的是总的字节长度
	{return pMemMainData ? iAddMemMainData(Format64bitHandle(LPBYTE, pMemMainData), MemSize, bAddTrail) : 0;}

BOOL CST_CurveCtrl::SetZoom(short Zoom)
{
	if (nVisibleCurve <= 0)
		return FALSE;

	if (this->Zoom != Zoom)
	{
		auto thisValueStep = pCalcValueStep ? pCalcValueStep(VCoorData.fStep, Zoom) : GETSTEP(VCoorData.fStep, Zoom);
		if (IsValueInvalidate)
			return FALSE;

		auto dStepTime = pCalcTimeSpan ? pCalcTimeSpan(HCoorData.fStep, Zoom, HZoom) : GETSTEP(HCoorData.fStep, Zoom + HZoom);
		if (IsTimeInvalidate)
			return FALSE;

		VCoorData.fCurStep = thisValueStep;
		HCoorData.fCurStep = dStepTime;

		CalcOriginDatumPoint(OriginPoint, 0xF); //不触发坐标间隔事件
		if (!SetLeftSpace()) //SetLeftSpace函数只会在画布变小的时候才调用ReSetCurvePosition函数，所以下面要补上
			UpdateRect(hFrceDC, VLabelRectMask | HLabelRectMask | CanvasRectMask);
		this->Zoom ^= Zoom;
		Zoom ^= this->Zoom;
		this->Zoom ^= Zoom;
		SYNBUDDYS(4, this->Zoom);
		FIRE_ZoomChange(this->Zoom);

		//2012.10.29 感觉这个可以不加，因为在SetValueStep和SetTimeSpan里面，都有可能存在曲线被移出屏幕的可能性，但它们都没有加
//		if (Zoom < this->Zoom) //本页中可能有曲线被移出屏幕
//			ReSetCurvePosition(0, TRUE); //可任意移动曲线，但ReSetCurvePosition函数会优先考虑移动纵坐标
	}

	return TRUE;
}
short CST_CurveCtrl::GetZoom() {return Zoom;}

BOOL CST_CurveCtrl::SetHZoom(short Zoom)
{
	if (nVisibleCurve <= 0)
		return FALSE;

	if (HZoom != Zoom)
	{
		auto dStepTime = pCalcTimeSpan ? pCalcTimeSpan(HCoorData.fStep, this->Zoom, Zoom) : GETSTEP(HCoorData.fStep, this->Zoom + Zoom);
		if (IsTimeInvalidate)
			return FALSE;

		HCoorData.fCurStep = dStepTime;

		CalcOriginDatumPoint(OriginPoint, 0xD); //不触发坐标间隔事件
		UpdateRect(hFrceDC, HLabelRectMask | CanvasRectMask);
		HZoom ^= Zoom;
		Zoom ^= HZoom;
		HZoom ^= Zoom;
		SYNBUDDYS(8, HZoom);
		FIRE_HZoomChange(HZoom);

		if (Zoom < HZoom) //本页中可能有曲线被移出屏幕
			ReSetCurvePosition(0, TRUE); //可任意移动曲线，但ReSetCurvePosition函数会优先考虑移动纵坐标
	}

	return TRUE;
}
short CST_CurveCtrl::GetHZoom() {return HZoom;}

BOOL CST_CurveCtrl::SetBeginValue(float fBeginValue)
{
	if (OriginPoint.Value == fBeginValue)
		return 2;

	if (MainDataListArr.empty() || !CheckVPosition(fBeginValue))
	{
		OriginPoint.Value = fBeginValue;
		CalcOriginDatumPoint(OriginPoint, 2);

		if (!SetLeftSpace())
		{
			UINT UpdateMask = VLabelRectMask;
			if (nVisibleCurve > 0)
				UpdateMask |= CanvasRectMask;

			UpdateRect(hFrceDC, UpdateMask);
		}

		return TRUE;
	}

	return FALSE;
}
float CST_CurveCtrl::GetBeginValue() {return OriginPoint.Value;}

BOOL CST_CurveCtrl::SetBeginTime(LPCTSTR pBeginTime)
{
	ASSERT(pBeginTime);
	if (IsBadStringPtr(pBeginTime, -1)) //空指针也能正确判断
		return FALSE;

	HCOOR_TYPE Time;
	USES_CONVERSION;
	auto hr = VarDateFromStr((LPOLESTR) T2COLE(pBeginTime), LANG_USER_DEFAULT, 0, &Time);
	return SUCCEEDED(hr) && SetBeginTime2(Time);
}

BOOL CST_CurveCtrl::SetBeginTime2(HCOOR_TYPE fBeginTime)
{
	if (OriginPoint.Time == fBeginTime)
		return 2;

	if (MainDataListArr.empty())
	{
		if (ISHVALUEINVALID(fBeginTime))
			return FALSE;
	}
	else if (CheckHPosition(fBeginTime))
		return FALSE;

	OriginPoint.Time = fBeginTime;
	CalcOriginDatumPoint(OriginPoint, 1);
	if (!(SysState & 0x20000001)) //在打印的时候，也会调用到本函数，最低位那个1，就是打印标志
	{
		UINT UpdateMask = HLabelRectMask;
		if (nVisibleCurve > 0 && !(4 & ReSetCurvePosition(4, TRUE)))
			UpdateMask |= CanvasRectMask;

		UpdateRect(hFrceDC, UpdateMask);
	}

	SYNBUDDYS(2, &fBeginTime);

	return TRUE;
}

BSTR CST_CurveCtrl::GetBeginTime() 
{
	BSTR bstr = nullptr;
	if (ISHVALUEVALID(OriginPoint.Time))
	{
		auto hr = VarBstrFromDate(OriginPoint.Time, LANG_USER_DEFAULT, 0, &bstr);
		ASSERT(SUCCEEDED(hr));
	}

	return bstr;
}
HCOOR_TYPE CST_CurveCtrl::GetBeginTime2() {return OriginPoint.Time;}

BOOL CST_CurveCtrl::ChangeLegendName(LPCTSTR pFrom, LPCTSTR pTo)
{
	ASSERT(pFrom && pTo);
	if (IsBadStringPtr(pFrom, -1) || IsBadStringPtr(pTo, -1)) //空指针也能正确判断
		return FALSE;

	auto LegendIter = FindLegend(pFrom);
	if (NullLegendIter == LegendIter || IsLegend(pTo))
		return FALSE;

	auto Len = _tcslen(pTo) + 1;
	if (Len > _tcslen(pFrom) + 1)
	{
		delete[] LegendIter->pSign;
		LegendIter->pSign = new TCHAR[Len];
	}

	_tcscpy_s(LegendIter->pSign, Len, pTo);
	UpdateRect(hFrceDC, LegendRectMask);

	return TRUE;
}

BOOL CST_CurveCtrl::MoveCurveToLegend(long Address, LPCTSTR pSign)
{
	auto LegendIter = FindLegend(pSign);
	if (NullLegendIter == LegendIter)
		return FALSE;

	DoMoveCurveToLegend(Address, LegendIter, TRUE);
	return TRUE;
}

//把Address从原来的图例中删除，然后添加到LegendIter里面，如果已经在LegendIter里面，则不做任何操作
void CST_CurveCtrl::DoMoveCurveToLegend(long Address, vector<LegendData>::iterator& LegendIter, BOOL bUpdate)
{
	if (NullLegendIter != LegendIter && 0x80000000 == FindLegend(LegendIter, Address)) //地址不存在，则添加到组中
	{
		auto pSign = LegendIter->pSign;

		DelLegend(Address, FALSE, bUpdate);

		LegendIter = FindLegend(pSign); //调用DelLegend之后，LegendIter可能已经失效
		LegendIter->Addrs.push_back(Address);

		auto DataListIter = FindMainData(Address);
		if (NullDataListIter != DataListIter)
		{
			DataListIter->LegendIter = LegendIter;

			if (bUpdate)
			{
				if (nVisibleCurve > 0) //有更新，刷新数据
					UpdateRect(hFrceDC, CanvasRectMask);
			}
			else
				SysState |= 0x200;
		}
	}
}

//Mask代表Address, PenColor, PenStyle, LineWidth, BrushColor, BrushStyle, CurveMode, NodeMode的有效性，按位算，按前面罗列的顺序

//当要添加的图例（pSign）已存在时：
//将会把Address地址添加到图例中（如果它不存在并且Mask最低位为1的话），并根据Mask来决定哪些值用来更新图例。

//当要添加的图例（pSign）不存在时：
//如果Address已存在，则首先把Address从它所在的图例中删除，再以pSign为图例新建，并将Address添加到其中，此时所有参数都必须有效
//也就是说，Mask必须等于0xFF

//BrushStyle取值如下：
//255－不填充；
//127－solid brush样式，参看CreateSolidBrush函数，颜色为BrushColor；
//0-126－hatch brush样式（没有这么多的样式，留着以后扩展，所以控件没有判断参数的值是否在CreateHatchBrush函数可识别的范围之内，不在范围之内是不会出错的），参看CreateHatchBrush函数，颜色为BrushColor；
//128-254－pattern brush样式，参看CreatePatternBrush函数，(BrushStyle - 128)即为位图序号（位图由AddBitmap等函数添加，具体看相关文档）。
//NodeMode取值如下：
//0：不显示节点；1按曲线颜色显示节点；2按曲线颜色的反色显示节点
short CST_CurveCtrl::AddLegend(long Address, LPCTSTR pSign, OLE_COLOR PenColor, short PenStyle, short LineWidth, OLE_COLOR BrushColor, short BrushStyle, short CurveMode, short NodeMode, short Mask, BOOL bUpdate)
{
	ASSERT(pSign);
	if (IsBadStringPtr(pSign, -1)) //空指针也能正确判断
		return Mask;

	size_t LegendIndex;
	auto LegendIter = FindLegend(pSign);
	if (NullLegendIter != LegendIter)
	{
		if (Mask & 1)
			DoMoveCurveToLegend(Address, LegendIter, FALSE);

		//PenColor, PenStyle, LineWidth, BrushColor, BrushStyle, CurveMode, NodeMode
		if (Mask & 2)
			LegendIter->PenColor = (COLORREF) (PenColor & 0xFFFFFF); //更改整个组的颜色，需要刷新图例
		if (Mask & 4)
			LegendIter->PenStyle = (BYTE) PenStyle;
		if (Mask & 8)
			if (0 <= LineWidth && LineWidth <= 255)
			{
				LegendIter->LineWidth = (BYTE) LineWidth;
				Mask &= ~8;
			}

		if (Mask & 16)
			LegendIter->BrushColor = (COLORREF) (BrushColor & 0xFFFFFF); //更改整个组的颜色
		if (Mask & 32)
			LegendIter->BrushStyle = (BYTE) BrushStyle;
		if (Mask & 64)
			if (0 <= CurveMode && CurveMode <= 3)
			{
				LegendIter->CurveMode = (BYTE) CurveMode;
				Mask &= ~64;
			}

		if (Mask & 128)
			if (0 <= NodeMode && NodeMode <= 2)
			{
				LegendIter->NodeMode = (BYTE) NodeMode;
				Mask &= ~128;
			}

		if (Mask & 2)
			LegendIndex = distance(begin(LegendArr), LegendIter);
		else
			LegendIndex = -1; //图例不需要刷新

		Mask &= ~( 1 | 2 | 4 | 16 | 32); //有五个值不会失败
	}
	else if (0xFF == (Mask & 0xFF)) //添加新的图例
	{
		LegendData NoUse(pSign);
		NoUse.PenColor = (COLORREF) (PenColor & 0xFFFFFF);
		NoUse.PenStyle = (BYTE) PenStyle;
		if (0 <= LineWidth && LineWidth <= 255)
			NoUse.LineWidth = (BYTE) LineWidth;
		else
			return 8;
		NoUse.State = 1;
		NoUse.BrushColor = (COLORREF) (BrushColor & 0xFFFFFF);
		NoUse.BrushStyle = (BYTE) BrushStyle;
		if (0 <= CurveMode && CurveMode <= 3)
			NoUse.CurveMode = (BYTE) CurveMode;
		else
			return 64;
		if (0 <= NodeMode && NodeMode <= 2)
			NoUse.NodeMode = (BYTE) NodeMode;
		else
			return 128;
		NoUse.NodeModeEx = 0;
		NoUse.BeginNodeColor = 0;
		NoUse.EndNodeColor = 0;
		NoUse.SelectedNodeColor = 0;
		NoUse.Lable = 0; //不显示坐标

		//不可用DoMoveCurveToLegend代替，因为多出了下面两行//中间部分的代码
		DelLegend(Address, FALSE, bUpdate);

		//////////////////////////////////////////////////////////////////////////
		BOOL bUpdateLegend = !LegendArr.empty() && LegendArr.capacity() == LegendArr.size(); //vector要重新分配内存了，DataListHead.LegendIter迭代器将失效

		LegendArr.push_back(NoUse);
		LegendIter = prev(end(LegendArr));
		LegendIter->Addrs.push_back(Address);

		if (bUpdateLegend)
			ReSetDataListLegend(begin(LegendArr), prev(end(LegendArr))); //最后一个图例是新添加的，不用考虑
		//////////////////////////////////////////////////////////////////////////

		auto DataListIter = FindMainData(Address);
		if (NullDataListIter != DataListIter) //添加DataListHead的相应的LegendIter
			DataListIter->LegendIter = LegendIter;

		SIZE size = {0};
		GetTextExtentPoint32(hFrceDC, pSign, (int) _tcslen(pSign), &size);
		LegendIter->SignWidth = (short) size.cx;
		if (LegendIter->SignWidth + 1 > m_LegendSpace)
			SetLegendSpace(LegendIter->SignWidth + 1);

		LegendRect[1].bottom += fHeight + 1;
		LegendRect[0] = LegendRect[1];
		MOVERECT(LegendRect[0], m_ShowMode);

		LegendIndex = LegendArr.size() - 1;
	}
	else
		return Mask;

	if (bUpdate)
	{
		UINT UpdateMask = 0;
		if (-1 != LegendIndex) //这是刚刚添加的或者更新的图例
			UpdateMask |= LegendRectMask;
		if (nVisibleCurve > 0) //有更新，刷新数据
			UpdateMask |= CanvasRectMask;

		if (UpdateMask)
			UpdateRect(hFrceDC, UpdateMask);
	}
	else
		SysState |= 0x200;

	return Mask;
}

BOOL CST_CurveCtrl::GetLegend(LPCTSTR pSign, OLE_COLOR* pPenColor, short* pPenStyle, short* pLineWidth, OLE_COLOR* pBrushColor, short* pBrushStyle, short* pCurveMode, short* pNodeMode)
{
	auto LegendIter = FindLegend(pSign);
	if (NullLegendIter != LegendIter)
		return GetLegend2((short) distance(begin(LegendArr), LegendIter), pPenColor, pPenStyle, pLineWidth, pBrushColor, pBrushStyle, pCurveMode, pNodeMode);

	return FALSE;
}

BOOL CST_CurveCtrl::AppendLegendEx(LPCTSTR pSign, OLE_COLOR BeginNodeColor, OLE_COLOR EndNodeColor, OLE_COLOR SelectedNodeColor, short NodeModeEx)
{
	if (0 > NodeModeEx || NodeModeEx > 7)
		return FALSE;

	auto LegendIter = FindLegend(pSign);
	if (NullLegendIter != LegendIter)
	{
		if (LegendIter->NodeModeEx != (BYTE) NodeModeEx)
		{
			LegendIter->NodeModeEx = (BYTE) NodeModeEx;
			if (NodeModeEx & 1)
				LegendIter->BeginNodeColor = (COLORREF) (BeginNodeColor & 0xFFFFFF);
			if (NodeModeEx & 2)
				LegendIter->EndNodeColor = (COLORREF) (EndNodeColor & 0xFFFFFF);
			if (NodeModeEx & 4)
				LegendIter->SelectedNodeColor = (COLORREF) (SelectedNodeColor & 0xFFFFFF);

			if (nVisibleCurve > 0) //注意这里不能只绘制受到影响的曲线，因为这样会破坏曲线之间的层次结构
				UpdateRect(hFrceDC, CanvasRectMask);
		}

		return TRUE;
	}

	return FALSE;
}

BOOL CST_CurveCtrl::GetLegendEx(LPCTSTR pSign, OLE_COLOR* pBeginNodeColor, OLE_COLOR* pEndNodeColor, OLE_COLOR* pSelectedNodeColor, short* pNodeModeEx)
{
	auto LegendIter = FindLegend(pSign);
	if (NullLegendIter != LegendIter)
		return GetLegendEx2((short) distance(begin(LegendArr), LegendIter), pBeginNodeColor, pEndNodeColor, pSelectedNodeColor, pNodeModeEx);

	return FALSE;
}

BOOL CST_CurveCtrl::GetLegendEx2(short nIndex, OLE_COLOR* pBeginNodeColor, OLE_COLOR* pEndNodeColor, OLE_COLOR* pSelectedNodeColor, short* pNodeModeEx)
{
	ASSERT(0 <= nIndex && (size_t) nIndex < LegendArr.size());
	if (0 <= nIndex && (size_t) nIndex < LegendArr.size())
	{
		auto LegendIter = next(begin(LegendArr), nIndex);
		DOFILL4VALUE(pBeginNodeColor, pEndNodeColor, pSelectedNodeColor, pNodeModeEx,
			LegendIter->BeginNodeColor, LegendIter->EndNodeColor, LegendIter->SelectedNodeColor, LegendIter->NodeModeEx);

		return TRUE;
	}

	return FALSE;
}

long CST_CurveCtrl::GetSelectedNodeIndex(long Address)
{
	auto DataListIter = FindMainData(Address);
	if (NullDataListIter != DataListIter)
		return (long) DataListIter->SelectedIndex;

	return -1;
}

BOOL CST_CurveCtrl::SetSelectedNodeIndex(long Address, long NewNodeIndex)
{
	if (NewNodeIndex < -1)
		return FALSE;

	auto DataListIter = FindMainData(Address);
	if (NullDataListIter != DataListIter)
	{
		auto pDataVector = DataListIter->pDataVector;
		ASSERT(pDataVector);

		if (-1 == NewNodeIndex || 0 <= NewNodeIndex && (size_t) NewNodeIndex < pDataVector->size())
		{
			auto OldSelectedNode = DataListIter->SelectedIndex;
			if (NewNodeIndex != OldSelectedNode)
			{
				DataListIter->SelectedIndex = NewNodeIndex; //修改了这个选中节点，但不一定能显示出来，还要看图例是否支持，这个在UpdateSelectedNode会容错
				UpdateSelectedNode(DataListIter, OldSelectedNode);
			}

			return TRUE;
		}
	}

	return FALSE;
}

BSTR CST_CurveCtrl::QueryLegend(long Address) 
{
	CComBSTR strResult;
	auto LegendIter = FindLegend(Address);
	if (NullLegendIter != LegendIter)
		strResult = *LegendIter;

	return strResult.Copy();
}

//当删除或者添加图例（添加时，只有在内存中需要重新分配内存的时候）时，调用本函数更新DataListHead的LegendIter成员
void CST_CurveCtrl::ReSetDataListLegend(vector<LegendData>::iterator BeginPos, vector<LegendData>::iterator EndPos)
{
	ASSERT(NullLegendIter != BeginPos && NullLegendIter != EndPos);
	for (auto LegendIter = BeginPos; LegendIter < EndPos; ++LegendIter)
	{
		auto pAddrs = &LegendIter->Addrs;
		for (auto AddrIter = begin(*pAddrs); AddrIter < end(*pAddrs); ++AddrIter)
		{
			auto DataListIter = FindMainData(*AddrIter);
			if (NullDataListIter != DataListIter)
				DataListIter->LegendIter = LegendIter;

			auto iter = FindInfiniteCurve(*AddrIter);
			if (NullInfiniteCurveIter != iter)
				iter->LegendIter = LegendIter;
		}
	}
}

COLORREF CST_CurveCtrl::FindLegend(vector<LegendData>::iterator LegendPos, long Address, BOOL bDel/*= FALSE*/, BOOL bAll/*= FALSE*/)
{
	ASSERT(NullLegendIter != LegendPos && LegendPos < end(LegendArr) && (!bAll || bDel)); //如果bAll要为真，只能是在bDel为真的情况下
	COLORREF Color = 0x80000000;
	for (auto i = begin(LegendPos->Addrs); i < end(LegendPos->Addrs);)
	{
		auto tempIter = i++;
		if (bAll || Address == *tempIter)
		{
			if (bDel)
			{
				auto DataListIter = FindMainData(*tempIter);
				if (NullDataListIter != DataListIter) //删除DataListHead的相应的LegendIter
					DataListIter->LegendIter = NullLegendIter;

				auto iter = FindInfiniteCurve(*tempIter);
				if (NullInfiniteCurveIter != iter)
					iter->LegendIter = NullLegendIter;

				i = LegendPos->Addrs.erase(tempIter);
			}

			if (0x80000000 == Color)
				Color = *LegendPos;

			if (!bAll)
				break;
		}
	}

	return Color;
}

vector<LegendData>::iterator CST_CurveCtrl::FindLegend(long Address, BOOL bInLegend/* = FALSE*/)
{
	if (bInLegend)
	{
		for (auto LegendIter = begin(LegendArr); LegendIter < end(LegendArr); ++LegendIter)
			if (0x80000000 != FindLegend(LegendIter, Address))
				return LegendIter;

		return NullLegendIter;
	}

	auto DataListIter = FindMainData(Address);
	return NullDataListIter != DataListIter ? DataListIter->LegendIter : NullLegendIter;
}

vector<LegendData>::iterator CST_CurveCtrl::FindLegend(LPCTSTR pSign)
{
	ASSERT(pSign);
	if (!IsBadStringPtr(pSign, -1))
	{
		auto LegendIter = find(begin(LegendArr), end(LegendArr), pSign);
		if (LegendIter < end(LegendArr))
			return LegendIter;
	}

	return NullLegendIter;
}

void CST_CurveCtrl::DeleteGDIObject()
{
	DELETEDC(hBackDC);
	DELETEDC(hFrceDC);
	DELETEDC(hTempDC);

	DELETEOBJECT(hFont);
	DELETEOBJECT(hTitleFont);
	DELETEOBJECT(hScreenRgn);
	DELETEOBJECT(hAxisPen);
	DELETEOBJECT(hBackBmp);
	DELETEOBJECT(hFrceBmp);

	for (auto i = begin(BitBmps); i < end(BitBmps); ++i)
		if (i->State & 2 || !(i->State & 1))
			DeleteObject(*i);
	free_container(BitBmps);
	nCanvasBkBitmap = nBkBitmap = -1;
}

BOOL CST_CurveCtrl::DoDelLegend(vector<LegendData>::iterator LegendPos, long Address, BOOL bAll, BOOL bUpdate)
{
	auto re = FALSE, bDelLegend = FALSE;
	for (auto i = begin(LegendArr); i < end(LegendArr);)
	{
		auto tempIter = i++;
		if (NullLegendIter != LegendPos && LegendPos != tempIter)
			continue;
		else if (0x80000000 != FindLegend(tempIter, Address, TRUE, bAll)) //调用FindLegend函数有必要，该函数会更新相应DataListHead结构的LegendIter成员
		{
			if (bAll || tempIter->Addrs.empty())
			{
				delete[] tempIter->pSign;
				i = LegendArr.erase(tempIter);
				ReSetDataListLegend(i, end(LegendArr));
				bDelLegend = TRUE;
			}

			re = TRUE;

			if (!bAll) //删除结束
				break;
		}

		if (NullLegendIter != LegendPos) //pLegendData为空时，bAll代表是否删除所有图例，否则代表是否删除pLegendData这一个图例
			break;
	}

	if (bDelLegend)
	{
		//当有图例被删除时，必须完全重新刷新一次，因为LegendRect在紧接着下面就要修改了，不完全刷新的话，会留下残留（因为图例减少了，增加就没事）
		SysState |= 0x200;

		if (NullLegendIter == LegendPos && bAll)
			LegendRect[1].bottom = LegendRect[1].top;
		else
			LegendRect[1].bottom -= fHeight + 1;

		LegendRect[0] = LegendRect[1];
		MOVERECT(LegendRect[0], m_ShowMode);
	}

	if (re && nVisibleCurve > 0)
		if (bUpdate)
			UpdateRect(hFrceDC, CanvasRectMask);
		else
			SysState |= 0x200;

	return re;
}

BOOL CST_CurveCtrl::DelLegend2(LPCTSTR pSign, BOOL bUpdate)
{
	ASSERT(pSign);
	if (!IsBadStringPtr(pSign, -1))
	{
		auto LegendIter = FindLegend(pSign);
		if (NullLegendIter != LegendIter)
			return DoDelLegend(LegendIter, 0, TRUE, bUpdate); //由于LegendPos有效，所以bDelAll（TRUE）的意思是删除pLegendData，而不是所有图例
	}

	return FALSE;
}

BOOL CST_CurveCtrl::DelLegend(long Address, BOOL bAll, BOOL bUpdate)
{
	return DoDelLegend(NullLegendIter, Address, bAll, bUpdate); //由于LegendPos无效，所以bDelAll（客户端传入的bAll参数）意思是删除所有图例（如果为真的话）
}

void CST_CurveCtrl::ClearLegend()
{
	for (auto i = begin(LegendArr); i < end(LegendArr); ++i)
		delete[] i->pSign;
	free_container(LegendArr);
}

void CST_CurveCtrl::UpdatePower(vector<DataListHead<MainData>>::iterator DataListIter)
{
	ASSERT(NullDataListIter != DataListIter);

	auto pDataVector = DataListIter->pDataVector;
	auto EndPos = prev(end(*pDataVector));
	for (auto i = begin(*pDataVector); i < EndPos;)
	{
		auto j = i++; //不可用前辍加加
		if (j->Time > i->Time)
		{
			DataListIter->Power = 2;
			return;
		}
	}

	DataListIter->Power = 1;
}

//计算让指定点可见时需要的移动量

//VisibleState从低位起：
//1－是否马上绘制添加的点（不判断该位）
//2－保持纵坐标不变（不判断该位）
//3－保持横坐标不变（不判断该位）
//4－在纵坐标上做最少的移动
//5－在横坐标上做最少的移动
//6－只有当上一点在画布中可见时，才自动移动曲线（不判断该位）
SIZE CST_CurveCtrl::MakePointVisible(vector<DataListHead<MainData>>::iterator DataListIter, vector<MainData>::iterator DataIter, short VisibleState /*= 0*/)
{
	ASSERT(NullDataListIter != DataListIter);
	SIZE size = {0, 0};
	if (!IsPointVisible(DataListIter, DataIter, FALSE, FALSE)) //如果点已经在画布中，则不需要移动曲线
	{
		int Position;
		int step;

		//////////////////////////////////////////////////////////////////////////
		Position = CanvasRect[1].right < DataIter->ScrPos.x ? 1 : 0; //1表示在右边
		if (!Position)
			Position = CanvasRect[1].left + DataListIter->Zx > DataIter->ScrPos.x ? 2 : 0; //2表示在左边

		if (Position) //需要移动
			if (VisibleState & 0x10) //最少移动
				if (1 == Position) //在右边
				{
					step = CanvasRect[1].right - DataIter->ScrPos.x;
					size.cx = step / HSTEP;
					if (step % HSTEP)
						--size.cx;
				}
				else //if (2 == Position) //在左边
				{
					step = CanvasRect[1].left + DataListIter->Zx - DataIter->ScrPos.x;
					size.cx = step / HSTEP;
					if (step % HSTEP)
						++size.cx;
				}
			else
				size.cx = ((CanvasRect[1].right + CanvasRect[1].left + DataListIter->Zx) / 2 - DataIter->ScrPos.x) / HSTEP;
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		Position = CanvasRect[1].bottom - DataListIter->Zy < DataIter->ScrPos.y ? 1 : 0; //1表示在下边
		if (!Position)
			Position = CanvasRect[1].top > DataIter->ScrPos.y ? 2 : 0; //2表示在上边

		if (Position) //需要移动
			if (VisibleState & 8) //最少移动
				if (1 == Position) //在下边
				{
					step = DataIter->ScrPos.y - (CanvasRect[1].bottom - DataListIter->Zy);
					size.cy = step / VSTEP;
					if (step % VSTEP)
						++size.cy;
				}
				else //if (2 == Position) //在上边
				{
					step = DataIter->ScrPos.y - CanvasRect[1].top;
					size.cy = step / VSTEP;
					if (step % VSTEP)
						--size.cy;
				}
			else
				size.cy = (DataIter->ScrPos.y - (CanvasRect[1].bottom - DataListIter->Zy + CanvasRect[1].top) / 2) / VSTEP;
		//////////////////////////////////////////////////////////////////////////
	} //if (!IsPointVisible(DataListIter, DataIter, FALSE, FALSE))

	return size;
}

//ChMask从位起：
//1－水平上重新计算ScrPos.x
//2－垂直上重新计算ScrPos.y
//3－步长改变，具体改变的是什么步长由1、2位决定
//4－只在第3位有效时有效，如果为1，则是由于Zoom或者刻度间隔改变引起的，所以不触发坐标间隔改变事件
//5－只计算ap点（哪怕它是OriginPoint点），这在修改基点时有用
//当XOff或者YOff不为0时，则ChMask当成整体来看，只有0和非0之分，非0代表OriginPoint也要平移
void CST_CurveCtrl::CalcOriginDatumPoint(MainData& ap, UINT ChMask/* = 3*/, int XOff/* = 0*/, int YOff/* = 0*/, vector<DataListHead<MainData>>::iterator DataListIter/* = NullDataListIter*/)
{
	//平移坐标包括：
	//XOff = LeftSpace(old) - LeftSpace(new)
	//YOff = WinHeight(old) - WinHeight(new)
	//YOff = BottomSpace(new) - BottomSpace(old)
	//以及移动曲线时
	if (XOff || YOff) //平移曲线
	{
		ASSERT(OriginPoint == ap); //平移坐标时，只需要对OriginPoint调用CalcOriginDatumPoint函数即可

		for (auto j = begin(MainDataListArr); j < end(MainDataListArr); ++j) //每条曲线
		{
			auto pDataVector = j->pDataVector;
			for (auto i = begin(*pDataVector); i < end(*pDataVector); ++i) //每个点
			{
				if (XOff)
					i->ScrPos.x -= XOff;
				if (YOff)
					i->ScrPos.y -= YOff;
			}

			//每条曲线占据的矩形
			if (XOff)
			{
				j->LeftTopPoint.ScrPos.x -= XOff;
				j->RightBottomPoint.ScrPos.x -= XOff;
			}
			if (YOff)
			{
				j->LeftTopPoint.ScrPos.y -= YOff;
				j->RightBottomPoint.ScrPos.y -= YOff;
			}
		} //for

		for (auto k = begin(InfiniteCurveArr); k < end(InfiniteCurveArr); ++k) //每条无限曲线
		{
			if (XOff && 1 == k->State)
				k->ScrPos.x -= XOff;
			if (YOff && 0 == k->State)
				k->ScrPos.y -= YOff;
		}

		for (auto i = begin(CommentDataArr); i < end(CommentDataArr); ++i) //每个注解
		{
			if (XOff)
				i->ScrPos.x -= XOff;
			if (YOff)
				i->ScrPos.y -= YOff;
		}

		if (ChMask) //当XOff和YOff不全为0的时候，ChMask代表是否要对OriginPoint进行偏移
		{
			if (XOff)
				OriginPoint.ScrPos.x += XOff; //这里的加很重要
			if (YOff)
				OriginPoint.ScrPos.y -= YOff;
		}

		if (nVisibleCurve > 0) //所有曲线占据的矩形
		{
			if (XOff)
			{
				LeftTopPoint.ScrPos.x -= XOff;
				RightBottomPoint.ScrPos.x -= XOff;
			}
			if (YOff)
			{
				LeftTopPoint.ScrPos.y -= YOff;
				RightBottomPoint.ScrPos.y -= YOff;
			}
		}

		return;
	} //if (XOff || YOff)

	if (!(ChMask & 3))
		return;

	if (ChMask & 1) //水平上有变化
	{
		if (!(ChMask & 0x14) && OriginPoint == ap) //第3、5位
			XOff = ap.ScrPos.x;

		BOOL bNegative = ap.Time < BenchmarkData.Time;
		ap.ScrPos.x = (long) ((ap.Time - BenchmarkData.Time) / HCoorData.fCurStep * HSTEP + (bNegative ? -.5 : .5));
	}
	if (ChMask & 2) //垂直上有变化
	{
		if (!(ChMask & 0x14) && OriginPoint == ap) //第3、5位
			YOff = ap.ScrPos.y;

		BOOL bNegative = ap.Value < BenchmarkData.Value;
		ap.ScrPos.y = (long) ((ap.Value - BenchmarkData.Value) / VCoorData.fCurStep * VSTEP + (bNegative ? -.5f : .5f));
	}

	if (ChMask & 0x10)
	{
		ASSERT(OriginPoint == ap); //修改基线
		return;
	}

	if (ChMask & 4) //步长有变化，需要重新计算所有点的ScrPos值
	{
		if (ChMask & 3)
		{
			ASSERT(OriginPoint == ap); //步长改变时，只需要对OriginPoint调用CalcOriginDatumPoint函数即可

			//触发事件
			if (!(ChMask & 8)) //非Zoom引起
			{
				if (ChMask & 1)
					FIRE_TimeSpanChange(HCoorData.fStep);
				if (ChMask & 2)
					FIRE_ValueStepChange(VCoorData.fStep);
			}

			ChMask &= 3;
			for (auto j = begin(MainDataListArr); j < end(MainDataListArr); ++j) //每条曲线
			{
				auto pDataVector = j->pDataVector;
				for (auto i = begin(*pDataVector); i < end(*pDataVector); ++i) //每个点
					CalcOriginDatumPoint(*i, ChMask, 0, 0, j);
				//每条曲线占据的矩形
				CalcOriginDatumPoint(j->LeftTopPoint, ChMask, 0, 0, j);
				CalcOriginDatumPoint(j->RightBottomPoint, ChMask, 0, 0, j);
			}

			for (auto k = begin(InfiniteCurveArr); k < end(InfiniteCurveArr); ++k) //每条无限曲线
				CalcOriginDatumPoint(*k, ChMask & (0 == k->State ? 2 : 1));

			for (auto i = begin(CommentDataArr); i < end(CommentDataArr); ++i) //每个注解
				CalcOriginDatumPoint(*i, ChMask); //注解不考虑Z轴影响

			UpdateTotalRange(TRUE);
		}

		return;
	}

	if (OriginPoint == ap)
	{
		if (ChMask & 1) //水平上有变化
		{
			XOff -= ap.ScrPos.x;
			FIRE_BeginTimeChange(OriginPoint.Time); //触发事件
		}
		if (ChMask & 2) //垂直上有变化
		{
			YOff -= ap.ScrPos.y;
			FIRE_BeginValueChange(OriginPoint.Value); //触发事件
		}

		CalcOriginDatumPoint(ap, 0, -XOff, YOff); //平移曲线
	}
	else
	{
		if (ChMask & 1) //水平上有变化
			ap.ScrPos.x = CanvasRect[1].left + ap.ScrPos.x - OriginPoint.ScrPos.x;
		if (ChMask & 2) //垂直上有变化
			ap.ScrPos.y = CanvasRect[1].bottom - 1 - (ap.ScrPos.y - OriginPoint.ScrPos.y); //减一是必须的，因为在ReSetUIPosition对CanvasRect[1]偏移了一个像素

		if (NullDataListIter != DataListIter) //这个判断很重要，在添加某条曲线的第一个点的时候，在计算页数量的时候，DataListIter都无效
		{
			if (ChMask & 1) //水平上有变化
				ap.ScrPos.x += DataListIter->Zx;
			if (ChMask & 2) //垂直上有变化
				ap.ScrPos.y -= DataListIter->Zy;
		}
	}
}

ActualPoint CST_CurveCtrl::CalcActualPoint(const POINT& point)
{
	ActualPoint ap = {OriginPoint.Time, OriginPoint.Value};

	if (m_ShowMode & 1)
		ap.Time += HCoorData.fCurStep * (CanvasRect[0].right - point.x - 1) / HSTEP;
	else
		ap.Time += HCoorData.fCurStep * (point.x - CanvasRect[0].left) / HSTEP;

	if ((m_ShowMode & 3) < 2)
		ap.Value += VCoorData.fCurStep * (CanvasRect[0].bottom - point.y - 1) / VSTEP;
	else
		ap.Value += VCoorData.fCurStep * (point.y - CanvasRect[0].top) / VSTEP;

	return ap;
}
/*
int CST_CurveCtrl::IsPointOutdrop(POINT& p)
{
	auto re = 0; //返回低四位有效，从低位起依次是曲线在画布的右，下，左，上方
	if (p.y < CanvasRect[1].top)
		re |= 8; //上
	else if (p.y > CanvasRect[1].bottom)
		re |= 2; //下

	if (p.x < CanvasRect[1].left)
		re |= 1; //右
	else if (p.x > CanvasRect[1].right)
		re |= 4; //左

	return re;
}

int CST_CurveCtrl::IsLineOutdrop(POINT& p1, POINT& p2)
{
	auto re = 0, re1 = IsPointOutdrop(p1), re2 = IsPointOutdrop(p2); //返回低四位有效，从低位起依次是曲线在画布的右，下，左，上方
	if (re1 & 8 && re2 & 8)
		re |= 8; //上
	else if (re1 & 2 && re2 & 2)
		re |= 2; //下

	if (re1 & 1 && re2 & 1)
		re |= 1; //右
	else if (re1 & 4 && re2 & 4)
		re |= 4; //左

	return re;
}
*/
BOOL CST_CurveCtrl::IsLineInCanvas(vector<LegendData>::iterator LegendIter, const POINT& p1, const POINT& p2)
{
//	if (IsLineOutdrop(p1, p2)) //线段在矩形的某一侧，肯定无法与矩形相交
//		return FALSE;
	if (p1.y < CanvasRect[1].top && p2.y < CanvasRect[1].top ||
		p1.y > CanvasRect[1].bottom && p2.y > CanvasRect[1].bottom ||
		p1.x < CanvasRect[1].left && p2.x < CanvasRect[1].left ||
		p1.x > CanvasRect[1].right && p2.x > CanvasRect[1].right)
		return FALSE;
	else if (p1.x == p2.x || p1.y == p2.y) //水平线段或者垂直线段，此时肯定穿过矩形
		return TRUE;

	if (NullLegendIter == LegendIter || 1 != LegendIter->CurveMode && 2 != LegendIter->CurveMode) //只要不是方波，都在这里处理，包括基础样条曲线
	{
		if (p1.y < CanvasRect[1].top) //从上向下倾斜的直线
		{
			auto x = p1.x + (p2.x - p1.x) * (CanvasRect[1].top - p1.y) / (p2.y - p1.y);
			if (CanvasRect[1].left <= x && x <= CanvasRect[1].right)
				return TRUE;
		}
		else if (p1.y > CanvasRect[1].bottom) //从下向上倾斜的直线
		{
			auto x = p1.x + (p2.x - p1.x) * (p1.y - CanvasRect[1].bottom) / (p1.y - p2.y);
			if (CanvasRect[1].left <= x && x <= CanvasRect[1].right)
				return TRUE;
		}
		else if (p1.x < CanvasRect[1].left) //从左向右倾斜的直线
		{
			auto y = p1.y + (p2.y - p1.y) * (CanvasRect[1].left - p1.x) / (p2.x - p1.x);
			if (CanvasRect[1].top <= y && y <= CanvasRect[1].bottom)
				return TRUE;
		}
		else //if (p1.x > CanvasRect[1].right) //从右向左倾斜的直线
		{
			auto y = p1.y + (p2.y - p1.y) * (p1.x - CanvasRect[1].right) / (p1.x - p2.x);
			if (CanvasRect[1].top <= y && y <= CanvasRect[1].bottom)
				return TRUE;
		}
		//还有一种方法，思想大意是：
		//求线段p1p2和画布两条对角线是否相交，如果相交，则线段p1p2与矩形相交，这种方法使用了矢量减法和矢量叉积
		//通过组合两条线段与另一条原始线段求矢量叉积来解答问题
		//矢量叉积其实用于得出一个结果，就是线段1是在线段2的顺时针方向还是逆时针方向，或者重合（此时两线段在方向上可能相等或者相差180度）。
		//以上方法可以解答倾斜的矩形，而且效率已经不错了，但考虑到本控件实际的情况，画布不是倾斜的，所以有更优的算法
		//矢量叉积方案在最好的情况下：减运算10次，乘运算5次；最坏的情况下：减运算20次，乘运算10次；平均为：减运算15次，乘运算7.5次
		//而本控件使用的方案，每次执行的运算完全相同，为：减（加）运算4次，乘（除）运算2次
	}
	else //1－先垂直后水平的方波；2－先水平后垂直的方波
	{
		POINT MidPoint;
		if (1 == LegendIter->CurveMode) //先垂直后水平的方波
		{
			MidPoint.x = p1.x;
			MidPoint.y = p2.y;
		}
		else //if (2 == LegendIter->CurveMode) //先水平后垂直的方波
		{
			MidPoint.x = p2.x;
			MidPoint.y = p1.y;
		}

		return IsLineInCanvas(LegendIter, p1, MidPoint) || IsLineInCanvas(LegendIter, MidPoint, p2);
	}

	return FALSE;
}

BOOL CST_CurveCtrl::IsPointVisible(vector<DataListHead<MainData>>::iterator DataListIter, vector<MainData>::iterator DataIter, BOOL bPart, BOOL bXOnly, UINT Mask/*= 3*/)
{
	ASSERT(NullDataListIter != DataListIter);
	RECT rect = {CanvasRect[1].left + DataListIter->Zx, CanvasRect[1].top, CanvasRect[1].right, CanvasRect[1].bottom - DataListIter->Zy};
	if (bPart) //判断DataIter点与它的前一点和后一点（如果有并且非隐藏非断点的话）组成的线条是否（部分）可见
	{
//		if (IsPointVisible(DataListIter, DataIter, FALSE, FALSE))
		if (PtInRect(&rect, DataIter->ScrPos)) //如果点就在画布中，直接返回真
			return TRUE;

		auto pDataVector = DataListIter->pDataVector;
		ASSERT(begin(*pDataVector) <= DataIter && DataIter < end(*pDataVector));
		vector<MainData>::iterator DataIter2 = NullDataIter;
		if (2 & Mask && DataIter > begin(*pDataVector)) //可向前
		{
			DataIter2 = DataIter;
			--DataIter2;
			while (DataIter2 > begin(*pDataVector))
			{
				if (2 != DataIter2->State) //跳过隐藏点
					break;
				--DataIter2;
			}

			if (PtInRect(&rect, DataIter2->ScrPos)) //如果点就在画布中，直接返回真
				return TRUE;

			//计算DataIter2、DataIter两点与画布的相交性
			if (IsLineInCanvas(DataListIter->LegendIter, DataIter2->ScrPos, DataIter->ScrPos))
				return TRUE;
		} //if (2 & Mask && DataIter > begin(*pDataVector))

		if (1 & Mask) //可向后
		{
			DataIter2 = DataIter;
			++DataIter2;
			if (DataIter2 < end(*pDataVector))
			{
				while (DataIter2 < prev(end(*pDataVector)))
				{
					if (2 != DataIter2->State) //跳过隐藏点
						break;
					++DataIter2;
				}

				if (PtInRect(&rect, DataIter2->ScrPos)) //如果点就在画布中，直接返回真
					return TRUE;

				//计算DataIter、DataIter2两点与画布的相交性
				return IsLineInCanvas(DataListIter->LegendIter, DataIter->ScrPos, DataIter2->ScrPos);
			} //if (DataIter2 < end(*pDataVector))
		} //if (1 & Mask)
	} //if (bPart)
	else if (bXOnly)
	{
		if (rect.left <= DataIter->ScrPos.x && DataIter->ScrPos.x <= rect.right)
			return TRUE;
	}
	else if (PtInRect(&rect, DataIter->ScrPos))
		return TRUE;

	return FALSE;
}

//VisibleState从低位起：
//1－是否马上绘制添加的点（不判断该位）
//2－保持纵坐标不变
//3－保持横坐标不变
//4－在纵坐标上做最少的移动
//5－在横坐标上做最少的移动
//6－只有当上一点在画布中可见时，才自动移动曲线
void CST_CurveCtrl::RefreshRTData(vector<DataListHead<MainData>>::iterator DataListIter, vector<MainData>::iterator Pos, short VisibleState)
{
	ASSERT(NullDataListIter != DataListIter && DataListIter < end(MainDataListArr));
	auto pDataVector = DataListIter->pDataVector;
	ASSERT(NullDataIter != Pos && Pos < end(*pDataVector));

	if (!ISCURVESHOWN(DataListIter))
	{
		auto LegendIter = find(begin(LegendArr), end(LegendArr), *DataListIter->LegendIter);
		if (LegendIter < end(LegendArr))
			ShowLegendFromIndex(distance(begin(LegendArr), LegendIter));
	}

	if (1 == nVisibleCurve && 1 == pDataVector->size()) //第一条可见曲线的第一个点，初始化所有曲线所占据的区域
	{
		m_MinTime = m_MaxTime = Pos->Time;
		m_MinValue = m_MaxValue = Pos->Value;
		RightBottomPoint.ScrPos = LeftTopPoint.ScrPos = Pos->ScrPos;
		CHANGEMOUSESTATE;
	}
	else //nVisibleCurve > 1 || pDataVector->size() > 1
	{
		auto iter = InvalidCurveSet.find(DataListIter->Address);
		if (end(InvalidCurveSet) != iter) //如果已经按非实时曲线添加过点，则此时不能只通过Pos这一个点来更新位置范围信息
		{
			//曲线次数需要更新，虽然在绘制实时曲线时，AddMainData2会更新曲线次数（它更新效率会高些）
			//但本曲线既然被添加到了InvalidCurveSet，说明之前按非实时曲线绘制过，那么
			//次数显然得重新更新，因为AddMainData2没有做完全的判断（非实时曲线时，AddMainData2并不更新曲线次数）
			UpdatePower(DataListIter);
			UpdateOneRange(DataListIter); //这两个函数的调用次序非常重要，不可调换
			InvalidCurveSet.erase(iter);

			//更新所有曲线位置及范围，如果InvalidCurveSet里面还有其它曲线，则这个更新
			//并不是最后结果，因为还有一些曲线的位置及范围信息没有计算出来
			//但这里的调用仍然必须，至少它体现了目前所有刷新过的曲线的位置及范围
			//没刷新的不考虑也是对的，这与显示在画布里面的情况相一致
			UpdateTotalRange();
		}
		else
		{
			if (pDataVector->size() > 1)
				UpdateOneRange(DataListIter, Pos);

			CHANGERANGE(m_MinTime, m_MaxTime, Pos->Time, LeftTopPoint.ScrPos.x, RightBottomPoint.ScrPos.x, Pos->ScrPos.x);
			CHANGERANGE(m_MinValue, m_MaxValue, Pos->Value, RightBottomPoint.ScrPos.y, LeftTopPoint.ScrPos.y, Pos->ScrPos.y);
		}
	} //if nVisibleCurve > 1 || pDataVector->size() > 1

	if (DataListIter->Zx > nZLength * HSTEP) //曲线在Z轴后面，看不见
		return;

	auto bCanLocalDraw = TRUE; //是否可以即时刷新
	if (AutoRefresh) //检测是否应该要自动刷新了
		if (AutoRefresh & 0x80000000) //需要刷新了
		{
			AutoRefresh &= ~0x80000000; //刷新已经做过了，复位
			bCanLocalDraw = FALSE; //不允许即时刷新，因为可能要绘制不止一个点
		}
		else
		{
			if (AutoRefresh & 0x7FFF0000) //以数量作为间隔
				if ((AutoRefresh & 0xFFFF) >= ((AutoRefresh & 0x7FFF0000) >> 16) - 1) //此时低两字节用于累计记数
				{
					AutoRefresh &= 0x7FFF0000; //复位累计记数器
					AutoRefresh |= 0x80000001; //没有问题，必须初始化为1
				}
				else
					++AutoRefresh;

			return; //还不能刷新
		}

	if (SysState & 0x10000000 && RefreshLimitedOrFixedCoor())
		return;

	if (VisibleState & 0x20 && pDataVector->size() > 1)
	{
		auto stop_refresh = FALSE;
		if (Pos == begin(*pDataVector))
			stop_refresh = !IsPointVisible(DataListIter, next(Pos), FALSE, FALSE);
		else if (Pos == prev(end(*pDataVector)))
			stop_refresh = !IsPointVisible(DataListIter, prev(Pos), FALSE, FALSE);

		if (stop_refresh)
			return;
	}

	auto size = MakePointVisible(DataListIter, Pos, VisibleState); //计算移动量，注意，可能会让size溢出，但这也没办法
	if (size.cx || size.cy)
	{
		if (VisibleState & 2) //保持纵坐标不变
			size.cy = 0;

		if (VisibleState & 4) //保持横坐标不变
			size.cx = 0;

		if (MoveCurve(size) || !IsPointVisible(DataListIter, Pos, TRUE, FALSE)) //非部分在画布中，无需绘制（注意：MakePointVisible不考虑部分在画布中的问题）
		{
			UpdateRect(hFrceDC, PreviewRectMask);
			return;
		}
	}

	auto LegendIter = DataListIter->LegendIter;

	//以下情况需要重绘本页：
	//1：正在绘制基数样条曲线，所以绘制基数样条曲线效率是很差的
	//2：点添加在曲线中间，但又要强行显示
	//3：当前正在绘制的曲线不是最上层
	//4：前面已经指示不允许即时刷新（bCanLocalDraw参数）
	if (!bCanLocalDraw || NullLegendIter != LegendIter && 3 == LegendIter->CurveMode ||
		Pos > begin(*pDataVector) && Pos < prev(end(*pDataVector)) ||
		!CanCurveBeDrawnAlone(DataListIter))
		UpdateRect(hFrceDC, CanvasRectMask);
	else //能即时绘制
	{
		//考虑到效率问题，不直接调用UpdateRect函数，因为这个函数至少要把DataListIter曲线重新绘制一次
		if (m_ShowMode & 3)
			CHANGE_MAP_MODE(hFrceDC, m_ShowMode);

		if (Pos > begin(*pDataVector))
			--Pos; //运行到这里，Pos一定是向量的尾结点，注意前面的判断：if (Pos > begin(*pDataVector) && Pos < prev(end(*pDataVector)))

		DrawCurve(hFrceDC, hScreenRgn, DataListIter, Pos); //这里是唯一使用DrawCurve函数的最后一个参数的机会

		//刷新区域
		RECT rect;
		rect.left = Pos->ScrPos.x;
		rect.top = Pos->ScrPos.y;
		if (pDataVector->size() > 1)
			++Pos;
		rect.right = Pos->ScrPos.x;
		rect.bottom = Pos->ScrPos.y;

		//不存在NormalizeRect这个API，只有CRect类有，所有这里要自己实现矩形的规格化
		NormalizeRect(rect);

		auto nInflate = 1;
		if (NullLegendIter != DataListIter->LegendIter)
			nInflate += DataListIter->LegendIter->LineWidth;
		else //宽度当成1
			++nInflate;
		InflateRect(&rect, nInflate, nInflate);

		//如果进行了填充，则要扩展矩形到坐标轴
		if (NullLegendIter != LegendIter && 255 != LegendIter->BrushStyle &&
			(LegendIter->BrushStyle < 127 || LegendIter->BrushStyle > 127 && (size_t) (LegendIter->BrushStyle - 128) < BitBmps.size()))
		{
			UINT Mask = DataListIter->FillDirection;
			if (Mask & 1) //向下填充
				rect.bottom = CanvasRect[1].bottom;
			if (Mask & 2) //向右填充
				rect.right = CanvasRect[1].right;
			if (Mask & 2) //向上填充
				rect.top = CanvasRect[1].top;
			if (Mask & 2) //向左填充
				rect.left = CanvasRect[1].left;
		}
		//刷新区域计算完毕

		if (m_ShowMode & 3)
		{
			CHANGE_MAP_MODE(hFrceDC, 0);
			MOVERECT(rect, m_ShowMode);
		}

		InvalidateControl(&rect, FALSE); //刷新上面的绘制到屏幕
		UpdateRect(hFrceDC, PreviewRectMask); //更新全局位置窗口（严格来说，需要更新的条件有：所有曲线组成的范围改变；更新区域与全局位置窗口有重叠）
		//但是由于目前判断“所有曲线组成的范围改变”有难度（至少还需要增加额外的存储），所以干脆直接调用UpdateRect函数
		//至于“更新区域与全局位置窗口有重叠”这个判断，可以用下面的语句：IntersectRect(&rect, &rect, &PreviewRect)
	} //能即时绘制
}

short CST_CurveCtrl::AddMainData(long Address, LPCTSTR pTime, float Value, short State, short VisibleState, BOOL bAddTrail)
{
	ASSERT(pTime);
	if (IsBadStringPtr(pTime, -1)) //空指针也能正确判断
		return 0;

	if (m_ShowMode & 0x80)
	{
		LPTSTR pEnd = nullptr;
		auto Time = _tcstod(pTime, &pEnd);
		if (HUGE_VAL == Time || -HUGE_VAL == Time)
			return 0;
		if (nullptr == pEnd || pTime == pEnd) //根本无法解析
			return 0;

		return AddMainData2(Address, Time, Value, State, VisibleState, bAddTrail);
	}
	else
	{
		HCOOR_TYPE Time;
		USES_CONVERSION;
		auto hr = VarDateFromStr((LPOLESTR) T2COLE(pTime), LANG_USER_DEFAULT, 0, &Time);
		return SUCCEEDED(hr) ? AddMainData2(Address, Time, Value, State, VisibleState, bAddTrail) : 0;
	}
}

vector<InfiniteCurveData>::iterator CST_CurveCtrl::FindInfiniteCurve(long Address)
{
	auto iter = find(begin(InfiniteCurveArr), end(InfiniteCurveArr), Address);
	if (iter == end(InfiniteCurveArr))
		return NullInfiniteCurveIter;

	return iter;
}

//无限曲线
//0-水平，Value有效
//1-垂直，Time有效
//如果同一个点想既有水平曲线又有垂直曲线，则以相同的Time Value添加两条曲线即可
BOOL CST_CurveCtrl::AddInfiniteCurve(long Address, HCOOR_TYPE Time, float Value, short State, BOOL bUpdate)
{
	BYTE Direction = State & 0xFF;
	BYTE FillDirection = (State >> 8) & 0xFF;
	if (Direction > 1 || 1 == Direction && ISHVALUEINVALID(Time) || FillDirection > 0xF)
		return FALSE;

	auto iter = FindInfiniteCurve(Address);
	if (NullInfiniteCurveIter == iter)
	{
		InfiniteCurveData NoUse;
		NoUse.Address = Address;
		NoUse.LegendIter = FindLegend(Address, TRUE); //查找图例赋给LegendIter

		InfiniteCurveArr.push_back(NoUse);
		iter = prev(end(InfiniteCurveArr));
	}

	iter->Time = Time;
	iter->Value = Value;
	//这里的State与普通曲线不一样，注意区别，这里只能取0和1，分别代表水平和垂直无限曲线
	//其实也就是相当于判定Time和Value哪一个值有效（不能同时有效）
	iter->State = Direction;

	//DataListHead成员
	iter->FillDirection = FillDirection;

	CalcOriginDatumPoint(*iter, 0 == Direction ? 2 : 1);
	if (bUpdate)
		UpdateRect(hFrceDC, CanvasRectMask);

	return TRUE;
}

BOOL CST_CurveCtrl::DelInfiniteCurve(long Address, BOOL bAll, BOOL bUpdate)
{
	auto re = TRUE;

	if (bAll)
	{
		bUpdate = bUpdate && !InfiniteCurveArr.empty();
		free_container(InfiniteCurveArr);
	}
	else
	{
		auto iter = FindInfiniteCurve(Address);
		if (NullInfiniteCurveIter != iter)
			InfiniteCurveArr.erase(iter);
		else
			re = FALSE;
	}

	if (re && bUpdate)
		UpdateRect(hFrceDC, CanvasRectMask);

	return re;
}

//State的取值暂时如下：
//低字节当成一个整体来看（0-255）：
//0－普通点，意思仅仅是非其它状态
//1－断点，即这个不与后面的点相连
//2－隐藏该点（前一点将和后一点直接连接，相当于没有这个点）
//第2字节按位算：
//第1位－节点显示与图例相反（图例如果显示，则不显示，图例如果不显示，则显示）
//……以后还有待扩展

//VisibleState从低位起：
//1－是否马上绘制添加的点
//2－保持纵坐标不变（在第1位为1的情况下有效）
//3－保持横坐标不变（在第1位为1的情况下有效）
//4－在纵坐标上做最少的移动（在第1位为1的情况下有效）
//5－在横坐标上做最少的移动（在第1位为1的情况下有效）
//6－只有当上一点在画布中可见时，才自动移动曲线（在第1位为1的情况下有效）

//返回：0－失败，1－成功（曲线已经存在），2－成功（新添加了一条曲线）
short CST_CurveCtrl::AddMainData2(long Address, HCOOR_TYPE Time, float Value, short State, short VisibleState, BOOL bAddTrail)
{
	if (!IsMainDataStateValidate(State) || ISHVALUEINVALID(Time))
		return 0;

	short re = 1;
	auto DataListIter = FindMainData(Address);
	if (NullDataListIter == DataListIter)
	{
		if (!(SysState & 0x1000)) //确定基准值
		{
			SysState |= 0x1000;

			BenchmarkData.Time = Time;
			BenchmarkData.Value = Value;
			CalcOriginDatumPoint(OriginPoint);

			for (auto iter = begin(CommentDataArr); iter < end(CommentDataArr); ++iter)
				CalcOriginDatumPoint(*iter);
			for (auto iter = begin(InfiniteCurveArr); iter < end(InfiniteCurveArr); ++iter)
				CalcOriginDatumPoint(*iter);
		}

		DataListHead<MainData> NoUse;
		NoUse.Address = Address;
		NoUse.LegendIter = FindLegend(Address, TRUE); //查找图例赋给LegendIter
		NoUse.FillDirection = 1; //向下填充
		NoUse.Power = 1;
		NoUse.Zx = NoUse.Zy = 0;
		NoUse.SelectedIndex = -1; //未选中点

		if (ISCURVESHOWN((&NoUse))) //新添加的曲线一定看得见
			++nVisibleCurve;

		MainDataListArr.push_back(NoUse);
		DataListIter = prev(end(MainDataListArr));

		FIRE_CurveStateChange(Address, 1); //曲线被添加
		re = 2;
	}

	auto pDataVector = DataListIter->pDataVector;
	//已经考虑了曲线中的点超过MaxDotNum两倍以上的情况，因为有可能中途调用SeMaxLength函数
	if (MaxLength > 0 && pDataVector->size() >= (size_t) MaxLength)
	{
		auto CutNum = (long) (pDataVector->size() - (size_t) MaxLength + 1);
		auto LeftNum = CutNum % CutLength + MaxLength - 1;
		CutNum = CutNum - CutNum % CutLength;
		if (LeftNum >= MaxLength)
			CutNum += CutLength;
		if (CutNum)
		{
			FIRE_CurveStateChange(Address, 3); //曲线被裁剪
			DelRange2(Address, 0, CutNum, FALSE, FALSE);
		}
	}

	MainData NoUse;
	NoUse.Time = Time;
	NoUse.Value = Value;
	CalcOriginDatumPoint(NoUse, 3, 0, 0, DataListIter);
	if (2 == re) //曲线第一个点，为LeftTopPoint和RightBottomPoint赋值
		DataListIter->LeftTopPoint = DataListIter->RightBottomPoint = NoUse;
	NoUse.AllState = State;

	vector<MainData>::iterator Pos = NullDataIter;
	if (pDataVector->empty()) //该曲线的第一个点
	{
		if (1 == nVisibleCurve)
			VisibleState = 1; //第一条曲线的第一个点，此时传入的VisibleState无效，这里覆盖它，恒定写为1
		bAddTrail = TRUE;
	}
	else if (!bAddTrail) //寻找插入点，二次曲线bAddTrail必为真，所以二次曲线不会运行这个判断下面的程序
	{
		Pos = find_if(begin(*pDataVector), end(*pDataVector), bind2nd(greater_equal<HCOOR_TYPE>(), Time));

		if (Pos == end(*pDataVector)) //添加到最后一个点
			bAddTrail = TRUE;
		else if (Pos->Time > Time)
			Pos = pDataVector->insert(Pos, NoUse); //在Pos位置上insert，实际上是插在Pos的前面
		else //添加的点已存在
		{
			if (Pos->Value != Value)
			{
				Pos->Value = Value; //这里应该要刷新
				Pos->ScrPos = NoUse.ScrPos;
				UpdateRect(hFrceDC, CanvasRectMask);
			}
			return re;
		}
	}
	else if (VisibleState & 1 && 1 == DataListIter->Power && Time < pDataVector->back().Time) //此时pDataVector向量肯定非空，所以可以直接调用back函数
		DataListIter->Power = 2; //2次曲线，注意如果不是绘制实时曲线，则需要更新幂次的曲线将被保存在InvalidCurveSet集合里面，等待绘制曲线时再更新

	if (bAddTrail)
	{
		pDataVector->push_back(NoUse);
		Pos = prev(end(*pDataVector));
	}

	if (VisibleState & 1)
	{
		if (2 == Pos->State) //绘制实时曲线时，不允许添加隐藏点
			Pos->State = 0;

		RefreshRTData(DataListIter, Pos, VisibleState);
	}
	else
	{
		InvalidCurveSet.insert(Address);
		SysState |= 0x200;
	}

	return re;
}

BOOL CST_CurveCtrl::SetAutoRefresh(short TimeInterval, short NumInterval)
{
	if (NumInterval) //高两字节中的低15位
	{
		if (NumInterval <= 1)
			return FALSE;

		AutoRefresh = (UINT) NumInterval << 16;
		++AutoRefresh; //没有问题，必须初始化为1
		KillTimer(AUTOREFRESH);
	}
	else if (TimeInterval) //低两字节
	{
		AutoRefresh = (USHORT) TimeInterval;
		SetTimer(AUTOREFRESH, AutoRefresh * 100, nullptr); //1/10秒转换成毫秒
	}
	else
	{
		AutoRefresh = 0;
		KillTimer(AUTOREFRESH);
	}

	return TRUE;
}

long CST_CurveCtrl::GetAutoRefresh()
{
	if (AutoRefresh & 0x7FFF0000)
		return AutoRefresh & 0x7FFF0000;
	else
		return AutoRefresh & 0xFFFF;
}

BOOL CST_CurveCtrl::SetFillDirection(long Address, short FillDirection, BOOL bUpdate)
{
	if (0 <= FillDirection && FillDirection <= 0xFF)
	{
		auto DataListIter = FindMainData(Address);
		if (NullDataListIter != DataListIter)
		{
			if ((BYTE) FillDirection != DataListIter->FillDirection)
			{
				DataListIter->FillDirection = (BYTE) FillDirection;
				if (bUpdate)
					UpdateRect(hFrceDC, CanvasRectMask);
				else
					SysState |= 0x200;
			}

			return TRUE;
		}
	}

	return FALSE;
}

short CST_CurveCtrl::GetFillDirection(long Address)
{
	auto DataListIter = FindMainData(Address);
	return NullDataListIter != DataListIter ? DataListIter->FillDirection : -1;
}

vector<DataListHead<MainData>>::iterator CST_CurveCtrl::FindMainData(long Address)
{
	auto DataListIter = find(begin(MainDataListArr), end(MainDataListArr), Address);
	return DataListIter < end(MainDataListArr) ? DataListIter : NullDataListIter;
}

void CST_CurveCtrl::ChangeSelectState(vector<DataListHead<MainData>>::iterator DataListIter)
{
	ASSERT(NullDataListIter != DataListIter && DataListIter < end(MainDataListArr));

	if (CurCurveIndex == distance(begin(MainDataListArr), DataListIter) && ISCURVEINPAGE(DataListIter, TRUE, FALSE)) //部分可见即可
	{
		FIRE_SelectedCurveChange(0x7fffffff);
		CurCurveIndex = -1; //从选中到未选中
	}
	else
	{
		if (SysState & 0x2000) //把CurCurveIndex曲线放在曲线数组的最后面，这样它将会绘制在最后，也就出现在最前
		{
			auto j = prev(end(MainDataListArr));
			if (DataListIter < j)
				swap(*DataListIter, *j); //交换两个DataListHead的内容，而不是指针
			CurCurveIndex = MainDataListArr.size() - 1;

			ASSERT(CurCurveIndex < MainDataListArr.size());
			DataListIter = next(begin(MainDataListArr), CurCurveIndex); //重新取一次迭代器，以防万一
		}
		else //按照DataListIter来移动曲线，但不改变CurCurveIndex的值，因为曲线的位置不允许改变
			CurCurveIndex = distance(begin(MainDataListArr), DataListIter);
		FIRE_SelectedCurveChange(DataListIter->Address);

		if (ReSetCurvePosition(1, TRUE, DataListIter) & 6)
			return;
	}

	UpdateRect(hFrceDC, CanvasRectMask);
}

size_t CST_CurveCtrl::GetLegendIndex(long y)
{
	auto step = 1;
	if ((m_ShowMode & 3) < 2)
		step += y - LegendRect[0].top;
	else
		step += LegendRect[0].bottom - y;
	auto nIndex = step / (fHeight + 1);
	if (!(step % (fHeight + 1)))
		--nIndex;

	return (size_t) nIndex;
}

BOOL CST_CurveCtrl::SelectLegendFromIndex(size_t nIndex) //从0开始的序号
{
	if (nIndex >= LegendArr.size() || !(SysState & 8))
		return FALSE;

	auto LegendIter = next(begin(LegendArr), nIndex);
	if (LegendIter->State)
	{
		for (auto i = begin(LegendIter->Addrs); i < end(LegendIter->Addrs); ++i)
		{
			auto DataListIter = FindMainData(*i);
			if (NullDataListIter != DataListIter)
			{
				ChangeSelectState(DataListIter);
				return TRUE;
			}
		}
		return FALSE;
	}
	else //曲线处于隐藏状态，则先将其显示出来，再选中它
	{
		ShowLegendFromIndex(nIndex);
		return SelectLegendFromIndex(nIndex);
	}
}

BOOL CST_CurveCtrl::SelectCurve(long Address, BOOL bSelect)
{
	auto DataListIter = FindMainData(Address);
	if (NullDataListIter != DataListIter)
		if (ISCURVESHOWN(DataListIter))
		{
			BOOL bState = CurCurveIndex == distance(begin(MainDataListArr), DataListIter) && ISCURVEINPAGE(DataListIter, TRUE, FALSE); //部分可见即可
			if (bState != bSelect)
				ChangeSelectState(DataListIter);

			return TRUE;
		}
		else //将隐藏的曲线显示出来
		{
			ShowCurve(Address, TRUE);
			return SelectCurve(Address, bSelect);
		}

	return FALSE;
}

void CST_CurveCtrl::EnableSelectCurve(BOOL bEnable)
{
	bEnable <<= 3;
	if ((SysState ^ bEnable) & 8) //第4位上有变化
	{
		SysState &= ~8;
		SysState |= bEnable;
	}

	if (!bEnable && -1 != CurCurveIndex) //取消已经选中的曲线
		ChangeSelectState(begin(MainDataListArr) + CurCurveIndex);
}

void CST_CurveCtrl::ShowLegendFromIndex(size_t nIndex)
{
	ASSERT(nIndex < LegendArr.size());
	if (nIndex >= LegendArr.size())
		return;

	auto LegendIter = next(begin(LegendArr), nIndex);
	LegendIter->State = !LegendIter->State;
	FIRE_LegendVisableChange((long) nIndex, LegendIter->State);
	auto OldVisibleCurve = nVisibleCurve;
	auto bChanged = FALSE;
	auto& Addrs = LegendIter->Addrs;
	UINT UpdateMask = LegendRectMask;

	for (auto i = begin(Addrs); i < end(Addrs); ++i)
	{
		auto DataListIter = FindMainData(*i);
		if (NullDataListIter != DataListIter)
		{
			if (LegendIter->State)
				++nVisibleCurve;
			else
			{
				--nVisibleCurve;
				if (CurCurveIndex == distance(begin(MainDataListArr), DataListIter))
				{
					FIRE_SelectedCurveChange(0x7fffffff);
					CurCurveIndex = -1;
				}
			}

			bChanged = TRUE;
		}
	}

	if (bChanged)
	{
		UpdateTotalRange();
		if (nVisibleCurve < OldVisibleCurve || !OldVisibleCurve) //曲线条数减少了，或者从无到有
			ReSetCurvePosition(2, TRUE);
		UpdateMask |= CanvasRectMask;

		auto c = OldVisibleCurve + nVisibleCurve; //OldVisibleCurve 和 nVisibleCurve 不可能同时为0
		if (c == OldVisibleCurve || c == nVisibleCurve) //从无到有或者从有到无
			CHANGEMOUSESTATE;
	}

	UpdateRect(hFrceDC, UpdateMask);
}

BOOL CST_CurveCtrl::ShowLegend(LPCTSTR pSign, BOOL bShow) //相当于在图例上点击鼠标右键
{
	ASSERT(pSign);
	if (!IsBadStringPtr(pSign, -1))
	{
		auto LegendIter = find(begin(LegendArr), end(LegendArr), pSign);
		if (LegendIter < end(LegendArr))
		{
			if (bShow != LegendIter->State)
				ShowLegendFromIndex(distance(begin(LegendArr), LegendIter));

			return TRUE;
		}
	}

	return FALSE;
}

BOOL CST_CurveCtrl::ShowCurve(long Address, BOOL bShow)
{
	auto LegendIter = FindLegend(Address);
	if (NullLegendIter != LegendIter)
	{
		if (bShow != LegendIter->State)
			ShowLegendFromIndex(distance(begin(LegendArr), LegendIter));

		return TRUE;
	}

	return FALSE;
}

short CST_CurveCtrl::DragCurve(short xStep, short yStep, BOOL bUpdate) {return MoveCurve(xStep, yStep, bUpdate);}

UINT CST_CurveCtrl::MoveCurve(SIZE size) 
{
	UINT re = 0;

	SysState |= 0x40000000; //第31位阻止DrawCurve函数移动曲线（在垂直方向上）
	while (size.cx || size.cy) //size表达的距离是long型的，由于MoveCurve只支持short型，所以要用循环来MoveCurve
	{
		short xStep = size.cx > 0x7FFF ? 0x7FFF : (size.cx < (short) 0x8000 ? 0x8000 : (short) size.cx);
		size.cx -= xStep;
		short yStep = size.cy > 0x7FFF ? 0x7FFF : (size.cy < (short) 0x8000 ? 0x8000 : (short) size.cy);
		size.cy -= yStep;
		re |= MoveCurve(xStep, yStep, !size.cx && !size.cy, FALSE); //只在最后一次刷新
		//不用检测移动的合法性，肯定合法，如果不合法，也是因为size溢出了，此时也无办法补救，还不如让错误表现出来
	}
	SysState &= ~0x40000000;

	return re;
}

//xStep大于0时曲线右移，yStep大于0时曲线上移
UINT CST_CurveCtrl::MoveCurve(short xStep, short yStep, BOOL bUpdate/* = TRUE*/, BOOL bCheckBound/* = TRUE*/) 
{
	UINT re = 0;
	if (nVisibleCurve > 0 && (xStep || yStep))
	{
		if (bCheckBound)
			re =  CheckPosition(xStep * HSTEP, yStep * VSTEP);
		else
		{
			if (xStep)
				re |= 1;
			if (yStep)
				re |= 2;
		}

		if (re)
		{
			if (re & 1)
			{
				OriginPoint.Time -= xStep * HCoorData.fCurStep;
				FIRE_BeginTimeChange(OriginPoint.Time);
			}
			else
				xStep = 0; //非常重要
			if (re & 2)
			{
				OriginPoint.Value -= yStep * VCoorData.fCurStep;
				FIRE_BeginValueChange(OriginPoint.Value);
			}
			else
				yStep = 0; //非常重要
			CalcOriginDatumPoint(OriginPoint, re, -xStep * HSTEP, yStep * VSTEP);

			UINT UpdateMask = CanvasRectMask;
			if (re & 1)
			{
				SYNBUDDYS(2, &OriginPoint.Time);
				UpdateMask |= HLabelRectMask;
			}
			if (re & 2)
				if (!SetLeftSpace())
					UpdateMask |= VLabelRectMask;
				else
					UpdateMask = 0;

			if (!bUpdate)
				SysState |= 0x200;
			else if (UpdateMask)
				UpdateRect(hFrceDC, UpdateMask);

			if (re & 1)
				ReportPageChanges;

			SysState &= ~0x8000; //吸附结束，m_CurActualPoint里面的值将变为无效
		} //if (re)
	} //if (nVisibleCurve > 0 && (xStep || yStep))

	return re;
}

BOOL CST_CurveCtrl::OnMouseWheelZoom(int zDelta) {return SysState & 0x400 && SetZoom(Zoom + zDelta);}
BOOL CST_CurveCtrl::OnMouseWheelHZoom(int zDelta) {return SysState & 0x10 && SetHZoom(HZoom + zDelta);} //水平缩放模式，可缩放
BOOL CST_CurveCtrl::OnMouseWheelHMove(int zDelta)
{
	if (m_MoveMode & 1) //水平移动模式，可水平移动
	{
		auto hd = m_ShowMode & 1 ? -1 : 1;
		return MoveCurve(hd * zDelta, 0); //需要在水平方向上检测移动的合法性
	}

	return FALSE;
}

BOOL CST_CurveCtrl::OnMouseWheelVMove(int zDelta)
{
	if (m_MoveMode & 2) //垂直移动模式，可垂直移动
	{
		auto vd = (m_ShowMode & 3) < 2 ? -1 : 1;
		return MoveCurve(0, vd * zDelta); //需要在垂直方向上检测移动的合法性
	}

	return FALSE;
}

static POINT spoint;
LRESULT CST_CurveCtrl::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	if (WM_MOUSEMOVE <= message && message <= WM_LBUTTONUP || WM_RBUTTONUP == message)
	{
		POINT ScrPos = {(short) (lParam & 0xFFFF), (short) (lParam >> 16)};

		if (m_MoveMode & 0x80 || IsCursorNotInCanvas(ScrPos))
			spoint = ScrPos;
		else if (WM_MOUSEMOVE == message)
			if (wParam & (MK_CONTROL | MK_SHIFT))
			{
				if (wParam & MK_CONTROL) //水平移动鼠标
					spoint.x = ScrPos.x;
				else //垂直移动鼠标
					spoint.y = ScrPos.y;

				if (memcmp(&ScrPos, &spoint, sizeof POINT))
				{
					SYNA_TO_TRUE_POINT;
				}
			}
			else //任意移动鼠标
				spoint = ScrPos;
		//WM_LBUTTONDOWN WM_LBUTTONUP WM_RBUTTONUP
		else if (SysState & 0x20 || !(SysState & 0x100)) //需要反同步
			spoint = ScrPos;
		else if (SysState & 0x40) //需要同步
		{
			SYNA_TO_TRUE_POINT;
		}

		SysState &= ~0x60; //spoint与当前真正的鼠标是同步的
	} //if (WM_MOUSEMOVE <= message && message <= WM_LBUTTONUP || WM_RBUTTONUP == message)
	else if ((WM_SYSKEYUP == message || WM_SYSKEYDOWN == message) && VK_MENU == wParam)
	{
		//如果允许水平缩放，则消化掉alt消息，这样就不会跳到菜单上去了
		//这里处理不会影响到GetAsyncKeyState调用的结果
		if (SysState & 0x10)
			return 0;

		//按下alt键之后，控件焦点未丢失，但却收不到鼠标移动消息，所以当下次
		//按下鼠标右键之后，一定要把spoint同步一下，否则会移动曲线（2010.11.27）

		//如果焦点不在控件之内，把鼠标移动到控件之内，按下alt键，此时也会出现完全相同的BUG，
		//由于控件没有焦点，收不到VK_MENU消息，所以采用另外一种方式来解决此问题，参看上面的反同步
		if (WM_SYSKEYUP == message)
			SysState |= 0x20;
	}

	switch (message)
	{
//	case VMOVE:
//		ReSetCurvePosition(4, TRUE);
//		break;
	case WM_VSCROLL: //由于没有滚动条，此消息不会收到
		SCROLLCURVE(2, SB_LINEUP, SB_LINEDOWN, 0, 1);
		break;
	case WM_HSCROLL: //由于没有滚动条，此消息不会收到
		SCROLLCURVE(1, SB_LINELEFT, SB_LINERIGHT, 1, 0);
		break;
	case WM_MOUSEWHEEL:
		if (nVisibleCurve > 0 && MouseWheelSpeed > 0)
		{
			auto zDelta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA * MouseWheelSpeed;
			BOOL re = 0;

			if (MK_SHIFT & wParam)
				re = (this->*OnMouseWheelFun[0])(zDelta);

			if (!re)
			{
				if (GetAsyncKeyState(VK_MENU) & 0x8000) //必须这样检测，因为WM_MOUSEWHEEL消息并不附带alt键信息
					re = (this->*OnMouseWheelFun[1])(zDelta);

				if (!re)
				{
					if (MK_CONTROL & wParam)
						re = (this->*OnMouseWheelFun[2])(zDelta);

					if (!re)
						re = (this->*OnMouseWheelFun[3])(zDelta);
				} //if (!re)
			} //if (!re)

			if (re) //可以操作，吸收消息
				return 0;
		} //if (nVisibleCurve > 0 && MouseWheelSpeed > 0)
		break;
	case WM_SETFOCUS: //if ((HWND) wParam != m_hWnd) 不需要判断，因为本控件没有其它子控件
		if (SysState & 0x400000)
		{
			SysState |= 0x100;
			REFRESHFOCUS(auto hDC = ::GetDC(m_hWnd), ::ReleaseDC(m_hWnd, hDC)); //马上要得到显示效果，所以获取屏幕DC
		}
		break;
	case WM_KILLFOCUS:
		if (SysState & 0x100)
		{
			SysState &= ~0x100;
			REFRESHFOCUS(auto hDC = ::GetDC(m_hWnd), ::ReleaseDC(m_hWnd, hDC)); //马上要得到显示效果，所以获取屏幕DC
		}
		break;
	case WM_SYSCOLORCHANGE:
		InitFrce(TRUE); //InitFrce需要在DrawBkg前面调用
		DrawBkg(TRUE);
		SysState |= 0x200;
		break;
	case WM_SIZE:
		ReSetUIPosition((int) (lParam & 0xFFFF), (int) (lParam >> 16));
		break;
	case WM_WINDOWPOSCHANGED:
		//当控件运行在IE中时，滚动窗口将出现刷新问题，这是微软公开的一个BUG，这句解决这个问题，但带来了窗口的闪烁
		//如果只在应用程序中使用则不存在这个问题。
		if (pWebBrowser)
			UpdateRect(hFrceDC, AllRectMask);
		break;
	case WM_SETCURSOR:
		if (MouseMoveMode & 0xFF)
		{
			HCURSOR hNewCursor;
			switch (MouseMoveMode & 0xFF)
			{
			case ZOOMIN:
				hNewCursor = hZoomIn;
				break;
			case ZOOMOUT:
				hNewCursor = hZoomOut;
				break;
			case MOVEMODE:
				if (m_MoveMode & 0x80)
					hNewCursor = hMove;
				else
					hNewCursor = nullptr; //隐藏鼠标
				break;
			case DRAGMODE:
				hNewCursor = hDrag;
				break;
			}

			SetCursor(hNewCursor);
			return TRUE;
		}
		break;
	case WM_DESTROY:
		if (m_gdiplusToken)
			Gdiplus::GdiplusShutdown(m_gdiplusToken);

		/*已经放弃添加此功能，因为二次开发者来实现此功能将更加的灵活多样
		if (pMoveBuddy[0])
			delete pMoveBuddy[0];
		if (pMoveBuddy[1])
			delete pMoveBuddy[1];
		*/
		CANCELBUDDYS;
		//	Uninstall the WH_GETMESSAGE hook function.
		if (InterlockedDecrement(&nRef) <= 0 && MsgHook)
		{
			VERIFY(::UnhookWindowsHookEx(MsgHook));
			MsgHook = nullptr;
		}
		if (pWebBrowser)
			pWebBrowser->Release();
		ClearLegend();
		ClearCurve();
		DeleteGDIObject();
		if (m_BE)
		{
			if (m_BE->pFileName)
				delete[] m_BE->pFileName;
			delete m_BE;
		}

		pFormatXCoordinate = nullptr;
		pFormatYCoordinate = nullptr;

		//插件、脚本相关
		//////////////////////////////////////////////////////////////////////////
		CloseLua;
		ClosePlugIn;
		//////////////////////////////////////////////////////////////////////////
		break;
	case WM_TIMER:
		switch (wParam)
		{
		case SHOWTOOLTIP:
			{
				KillTimer(SHOWTOOLTIP);
				auto hDC = ::GetDC(m_hWnd); //马上要得到显示效果，所以获取屏幕DC
				ShowToolTip(hDC);
				::ReleaseDC(m_hWnd, hDC);
			}
			break;
		case BATCHEXPORTBMP:
			if (m_BE)
			{
				while (++m_BE->nFileNum)
				{
					auto nNum = _sntprintf_s(StrBuff, _TRUNCATE, m_BE->cNumFormat, m_BE->nFileNum);
					if (nNum != m_BE->nWidth) //已经溢出了，比如pFileName为：c:\****.bmp，而此时nFileNum已经大于4位数了，比如10001
					{
						m_BE->nFileNum = 0; //结束定时导出图片
						break;
					}

					memcpy(m_BE->pStart, StrBuff, m_BE->nWidth * sizeof(TCHAR));
					if (_taccess(m_BE->pFileName, 0)) //文件不存在
						break;
				}

				if (m_BE->nFileNum)
				{
					::ExportImage(hFrceBmp, m_BE->pFileName);
					FIRE_BatchExportImageChange((long) m_BE->nFileNum);
				}
				else //结束定时导出图片，并释放m_BE，有两种情况下需要结束定时导出图片：1.文件名宽度溢出；2.文件名序号溢出，即UINT数据溢出
					BatchExportImage(0, 0); //结束定时导出图片，并且释放m_BE
			}
			break;
		case REPORTPAGE:
			KillTimer(REPORTPAGE);
			ReportPageInfo();
			break;
		case HIDEHELPTIP:
			EnableHelpTip(FALSE); //EnableHelpTip会杀掉HIDEHELPTIP定时器
			break;
		case HIDECOPYRIGHTINFO:
			break;
		case AUTOREFRESH:
			AutoRefresh |= 0x80000000; //下次可以自动刷新了
			break;
		} //switch (wParam)
		break;
	case WM_MOUSEMOVE:
		if (DRAGMODE == MouseMoveMode)
		{
			MOVEWITHMOUS(m_MoveMode & 4 && m_MoveMode & 3); //快速拖动模式，且可移动
			break;
		}

		if (IsCursorNotInCanvas(spoint))
		{
			if (MouseMoveMode & 0xFF) //当可见曲线条数变为0时，仍然保留缩放状态
				MouseMoveMode <<= 8;

			//WM_MOUSEMOVE消息发送频率很高，这个判断防止在没有曲线的时候不停的调用DrawAcrossLine函数，而该函数又什么都不做的返回
			if (-1 != LastMousePoint.x && !(m_MoveMode & 0x80))
			{
				POINT EmptyPoint = {-1};
				DrawAcrossLine(&EmptyPoint); //清除十字交叉线
			}
		}
		else
		{
			if (!MouseMoveMode)
				MouseMoveMode = MOVEMODE;
			else if (!(MouseMoveMode & 0xFF))
				MouseMoveMode >>= 8;

			if (m_MoveMode & 0x80)
				break;

			if (SysState & 0x4000 && !(wParam & (MK_CONTROL | MK_SHIFT))) //允许吸附
			{
				RECT SorptionRect;
				if (SysState & 0x8000) //已经吸附
				{
					GetSorptionRect(SorptionRect, LastMousePoint.x, LastMousePoint.y, SorptionRange);
					if (PtInRect(&SorptionRect, spoint)) //仍然在吸附范围之内
					{
						if (memcmp(&LastMousePoint, &spoint, sizeof POINT))
						{
							SysState |= 0x40; //spoint与当前真正的鼠标不同步
							spoint = LastMousePoint;
						}

						break; //跳过了后面的DrawAcrossLine调用
					}
					else
					{
						SysState &= ~0x8000; //吸附结束，m_CurActualPoint里面的值将变为无效
						FIRE_SorptionChange(m_CurActualAddress, -1, 0);
					}
				}

				//判断spoint这个点的周围有没有可以吸附的坐标点
				for (auto i = begin(MainDataListArr); i < end(MainDataListArr); ++i)
				{
					if (!ISCURVESHOWN(i) || i->Zx > nZLength * HSTEP)
						continue;

					auto nIndex = DoGetPointFromScreenPoint(i, spoint.x, spoint.y, SorptionRange);
					if (-1 != nIndex)
					{
						SysState |= 0x8000; //新的吸附，m_CurActualPoint里面的值将变为有效

						//恒写入m_CurActualPoint，这样在ShwoToolTip时可以提高效率，不显示坐标提示也几乎没什么效率损失
						m_CurActualAddress = i->Address;
						auto iter = next(begin(*i->pDataVector), nIndex);
						m_CurActualPoint.Time = iter->Time;
						m_CurActualPoint.Value = iter->Value;
						FIRE_SorptionChange(m_CurActualAddress, (long) nIndex, 1);

						spoint = iter->ScrPos;
						MOVEPOINT(spoint, m_ShowMode);
						POINT ScrPos;
						SYNA_TO_TRUE_POINT;
						break;
					}
				} //遍历所有可见曲线
			} //if (SysState & 0x4000 && !(wParam & (MK_CONTROL | MK_SHIFT)))

			DrawAcrossLine(&spoint); //移动十字交叉线
		}
		break;
	case WM_LBUTTONDOWN:
//		if (!(SysState & 0x100)) //2012.8.20 似乎由父类实现了（会收到WM_SETFOCUS消息）
//			SetFocus();
		if (!(SysState & 0x200000) && PtInRect(&PreviewHotspotRect, spoint))
			EnablePreview(!(SysState & 0x80000000));
		else if (SysState & 0x80000000 && PtInRect(&PreviewRect, spoint)) //点击在全局位置区域
		{
			if (nVisibleCurve <= 0)
				break;

			SIZE size;
			if (spoint.x - 10 < PreviewRect.left)
				size.cx = PreviewRect.left + 10;
			else if (spoint.x + 10 > PreviewRect.right)
				size.cx = PreviewRect.right - 10;
			else
				size.cx = spoint.x;
			size.cx -= 10;
			size.cx = (long) ((float) (RightBottomPoint.ScrPos.x - LeftTopPoint.ScrPos.x + CanvasRect[1].right - CanvasRect[1].left) / (4 * HSTEP - 20) *
				(PreviewPoint.x - size.cx));
			size.cx /= HSTEP;

			if (spoint.y - 5 < PreviewRect.top)
				size.cy = PreviewRect.top + 5;
			else if (spoint.y + 4 > PreviewRect.bottom)
				size.cy = PreviewRect.bottom - 4;
			else
				size.cy = spoint.y;
			size.cy -= 5;
			size.cy = (long) ((float) (RightBottomPoint.ScrPos.y - LeftTopPoint.ScrPos.y + CanvasRect[1].bottom - CanvasRect[1].top) / (3 * VSTEP - 9) *
				(size.cy - PreviewPoint.y));
			size.cy /= VSTEP;

			if (m_ShowMode & 1)
				size.cx = -size.cx;
			if (m_ShowMode & 2)
				size.cy = -size.cy;

			MoveCurve(size);
		} //if (SysState & 0x80000000 && PtInRect(&PreviewRect, spoint))
		else if (MOVEMODE == MouseMoveMode)
		{
			if (m_MoveMode & 3 && nVisibleCurve > 0 && PtInRect(CanvasRect, spoint))
			{
				MouseMoveMode = DRAGMODE;
				POINT EmptyPoint = {-1};
				DrawAcrossLine(&EmptyPoint); //清除十字交叉线，WM_LBUTTONDOWN的发送频率很少，所以调用DrawAcrossLine函数之前不用再判断是否有必要（包括是否显示为手形）
				BeginMovePoint = spoint;
				PostMessage(WM_SETCURSOR, (WPARAM) m_hWnd, HTCLIENT | WM_MOUSEMOVE);
//				SetCapture(); //2012.8.20 似乎已经由父类实现了
			}
		}
		break;
	case WM_LBUTTONUP:
		if (DRAGMODE == MouseMoveMode) //正在拖动曲线
		{
			MOVEWITHMOUS(!(m_MoveMode & 4) && m_MoveMode & 3) //慢速移动模式，且可移动

			if (PtInRect(CanvasRect, spoint))
				MouseMoveMode = MOVEMODE;
			else
				MouseMoveMode = 0;
			PostMessage(WM_SETCURSOR, (WPARAM) m_hWnd, HTCLIENT | WM_MOUSEMOVE);
			ReleaseCapture();
		}
		else if (nVisibleCurve > 0 && (ZOOMIN == MouseMoveMode || ZOOMOUT == MouseMoveMode) && PtInRect(CanvasRect, spoint)) //定点缩放
		{
			if (DoFixedZoom(spoint))
				DrawAcrossLine(&spoint); //恢复十字交叉线
		}
		else if (!(SysState & 0x200000) && PtInRect(LegendRect, spoint)) //通过在图例上点击鼠标来更改曲线的显示状态
			SelectLegendFromIndex(GetLegendIndex(spoint.y));
		break;
//	case WM_RBUTTONDOWN: //2012.8.20 按照标准行为，这个事件不设置焦点
//		if (!(SysState & 0x100))
//			SetFocus();
//		break;
	case WM_RBUTTONUP:
		if (SysState & 0x80000000 && PtInRect(&PreviewRect, spoint))
			EnablePreview(FALSE);
		else if (DRAGMODE != MouseMoveMode)
			if (!(SysState & 0x200000) && PtInRect(LegendRect, spoint))
				ShowLegendFromIndex(GetLegendIndex(spoint.y));
			else if ((ZOOMIN == MouseMoveMode || ZOOMOUT == MouseMoveMode) && PtInRect(CanvasRect, spoint))
			{
				MouseMoveMode = MOVEMODE;
				PostMessage(WM_SETCURSOR, (WPARAM) m_hWnd, HTCLIENT | WM_MOUSEMOVE);

				FIRE_ZoomModeChange(0);
			}
		break;
	case BUDDYMSG:
		{
			UINT re = TRUE;
			BuddyState |= 1;
			switch(wParam)
			{
			case 0:
				if ((HWND) lParam == m_hWnd)
					re = FALSE;
				else if (hBuddyServer != (HWND) lParam)
				{
					if (hBuddyServer) //已经关联到别的联动服务器，所以先取消
						::SendMessage(hBuddyServer, BUDDYMSG, 1, (LPARAM) m_hWnd);

					hBuddyServer = (HWND) lParam;
				}
				break;
			case 1:
				if (hBuddyServer) //联动服务器主动取消
				{
					hBuddyServer = nullptr;
					CHLeftSpace(ActualLeftSpace); //更改自己的LeftSpace
				}
				else if (lParam) //联动客户机主动取消，服务器将更新LeftSpace
					REMOVEBUDDY((HWND) lParam);
				break;
			case 2: //设置当前页开始时间
				SetBeginTime2(*(HCOOR_TYPE*) lParam);
				break;
			case 3: //设置时间间隔
				SetTimeSpan(*(double*) lParam);
				break;
			case 4: //设置放大率
				SetZoom((short) lParam);
				break;
			case 5: //移动纵坐标
				CHLeftSpace((short) lParam);
				break;
			case 6: //查询最小LeftSpace
				re = ActualLeftSpace;
				break;
			case 7: //客户机请求重新确定LeftSpace
				if (pBuddys)
				{
					auto MaxLeftSpace = GetMaxLeftSpace(ActualLeftSpace);
					if (MaxLeftSpace != LeftSpace)
					{
						BROADCASTLEFTSPACE(MaxLeftSpace);
						CHLeftSpace(MaxLeftSpace);
					}

					re = MaxLeftSpace;
				}
				break;
			case 8: //设置水平放大率
				SetHZoom((short) lParam);
				break;
			} //switch(wParam)
			BuddyState &= ~1;
			return re;
		}
		break;
	}

	return COleControl::WindowProc(message, wParam, lParam);
}

void CST_CurveCtrl::ShowToolTip(HDC hDC)
{
	ActualPoint ap;
	if (SysState & 0x8000)
		ap = m_CurActualPoint;
	else if (3 == m_ReviseToolTip)
		return;
	else
	{
		auto point = LastMousePoint;
		if (m_ReviseToolTip && -1 != CurCurveIndex && (1 == m_ReviseToolTip || ISCURVEINPAGE(next(begin(MainDataListArr), CurCurveIndex), TRUE, FALSE)))
		{
			point.x -= MainDataListArr[CurCurveIndex].Zx;
			point.y += MainDataListArr[CurCurveIndex].Zy;
		}
		ap = CalcActualPoint(point);
	}
	SysState |= 0x800;

	FormatToolTipString(SysState & 0x8000 ? m_CurActualAddress : 0x7fffffff, ap.Time, ap.Value, 4);
	//由于可能安装有插件，所以可能会不需要显示（返回空字符串）
	auto pToolTip = StrBuff;
	if (_T('\n') == *pToolTip)
		++pToolTip;
	if (0 == *pToolTip)
		return;

	ToolTipRect.left = LastMousePoint.x;
	ToolTipRect.top = LastMousePoint.y;

	SelectObject(hDC, hFont);
	DrawText(hDC, pToolTip, -1, &ToolTipRect, DT_CALCRECT);
	ToolTipRect.right += 5;
	ToolTipRect.bottom += 5;

	//让tooltip自适应窗口
	if (ToolTipRect.right >= CanvasRect[0].right)
		ToolTipRect.right = (ToolTipRect.left << 1) - ToolTipRect.right;
	if (ToolTipRect.bottom >= CanvasRect[0].bottom)
		ToolTipRect.bottom = (ToolTipRect.top << 1) - ToolTipRect.bottom;
	NormalizeRect(ToolTipRect);

	InvalidRect = ToolTipRect;
	InvalidRect.right -= 5;
	InvalidRect.bottom -= 5;
	OffsetRect(&InvalidRect, 3, 3);

	SetTextColor(hDC, m_foreColor);
	::SetBkMode(hDC, TRANSPARENT);
	SelectClipRgn(hDC, hScreenRgn); //注意这里选入的区域并没有还原，所以hDC应该每次都是一个新的DC（通过GetDC获取，而不是一个内存兼容DC）
	BitBlt(hDC, ToolTipRect.left + 1, ToolTipRect.top + 1, ToolTipRect.right - ToolTipRect.left - 1, ToolTipRect.bottom - ToolTipRect.top - 1,
		hBackDC, ToolTipRect.left + 1, ToolTipRect.top + 1, SRCCOPY);
	DrawText(hDC, pToolTip, -1, &InvalidRect, 0);

	SelectObject(hDC, hAxisPen); //使用绘制坐标轴时的画笔
	SelectObject(hDC, GetStockObject(NULL_BRUSH));
	Rectangle(hDC, ToolTipRect.left, ToolTipRect.top, ToolTipRect.right + 1, ToolTipRect.bottom + 1);
}

void CST_CurveCtrl::DrawAcrossLine(const LPPOINT pPoint)
{
	ASSERT(pPoint);
	if (-1 == LastMousePoint.x && -1 == pPoint->x)
		return;

	if (!(m_MoveMode & 0x80))
	{
		auto hDC = ::GetDC(m_hWnd); //马上要得到显示效果，所以要马上获取DC
		if (LastMousePoint.x > -1 && (LastMousePoint.x != pPoint->x || LastMousePoint.y != pPoint->y)) //抹掉老的十字交叉线
		{
			static BOOL vista_or_grater = -1;
			if (-1 == vista_or_grater)
			{
#ifdef _USING_V110_SDK71_
				OSVERSIONINFOW v = {sizeof OSVERSIONINFOW};
				GetVersionEx(&v);
				vista_or_grater = v.dwMajorVersion >= 6;
#else
				vista_or_grater = IsWindowsVistaOrGreater();	
#endif // _USING_V110_SDK71_
			}

			if (vista_or_grater)
				BitBlt(hDC, CanvasRect->left, CanvasRect->top, CanvasRect->right - CanvasRect->left, CanvasRect->bottom - CanvasRect->top, hFrceDC, CanvasRect->left, CanvasRect->top, SRCCOPY);
			else
			{
				//下面的代码，在win7之前（vista下没试过）的系统下，运行良好，但在win7下，则显得效率及其低下，出现较为严重的闪烁，具体原因未明
				if (LastMousePoint.y != pPoint->y)
					BitBlt(hDC, CanvasRect->left, LastMousePoint.y, CanvasRect->right - CanvasRect->left, 1,
						hFrceDC, CanvasRect->left, LastMousePoint.y, SRCCOPY); //抹水平线
				if (LastMousePoint.x != pPoint->x)
					BitBlt(hDC, LastMousePoint.x, CanvasRect->top, 1, CanvasRect->bottom - CanvasRect->top,
						hFrceDC, LastMousePoint.x, CanvasRect->top, SRCCOPY); //抹垂直线

				if (SysState & 0x800)
				{
					BitBlt(hDC, ToolTipRect.left, ToolTipRect.top, ToolTipRect.right - ToolTipRect.left + 1, ToolTipRect.bottom - ToolTipRect.top + 1,
						hFrceDC, ToolTipRect.left, ToolTipRect.top, SRCCOPY); //抹坐标提示
					SysState &= ~0x800;
				}
			}
		}
		LastMousePoint.x = pPoint->x;
		LastMousePoint.y = pPoint->y;
		if (LastMousePoint.x > -1) //绘制新的十字交叉线
		{
			RepairAcrossLine(hDC, CanvasRect, FALSE);
			if ((3 != m_ReviseToolTip || SysState & 0x8000) && ToolTipDely > 0)
				SetTimer(SHOWTOOLTIP, ToolTipDely, nullptr);
		}
		else
			KillTimer(SHOWTOOLTIP);
		::ReleaseDC(m_hWnd, hDC);
	}
}

void CST_CurveCtrl::RepairAcrossLine(HDC hDC, LPCRECT pRect, BOOL bCheckBound/* = TRUE*/)
{
	ASSERT(pRect);
	SelectObject(hDC, hAxisPen); //使用绘制坐标轴时的画笔
	if (!bCheckBound || pRect->top <= LastMousePoint.y && LastMousePoint.y <= pRect->bottom)
	{
		auto xBegin = pRect->left, xEnd = pRect->right;
		if (SysState & 0x80000000)
			switch (m_ShowMode & 3)
			{
			case 0:
				if (LastMousePoint.y >= PreviewRect.top)
					xBegin = PreviewRect.right;
				break;
			case 1:
				if (LastMousePoint.y >= PreviewRect.top)
					xEnd = PreviewRect.left;
				break;
			case 2:
				if (LastMousePoint.y < PreviewRect.bottom)
					xBegin = PreviewRect.right;
				break;
			case 3:
				if (LastMousePoint.y < PreviewRect.bottom)
					xEnd = PreviewRect.left;
				break;
			}
		MoveToEx(hDC, xBegin, LastMousePoint.y, nullptr);
		LineTo(hDC, xEnd, LastMousePoint.y); //画水平线
	}
	if (!bCheckBound || pRect->left <= LastMousePoint.x && LastMousePoint.x <= pRect->right)
	{
		auto yBegin = pRect->top, yEnd = pRect->bottom;
		if (SysState & 0x80000000)
			switch (m_ShowMode & 3)
			{
			case 0:
				if (LastMousePoint.x < PreviewRect.right)
					yEnd = PreviewRect.top;
				break;
			case 1:
				if (LastMousePoint.x >= PreviewRect.left)
					yEnd = PreviewRect.top;
				break;
			case 2:
				if (LastMousePoint.x < PreviewRect.right)
					yBegin = PreviewRect.bottom;
				break;
			case 3:
				if (LastMousePoint.x >= PreviewRect.left)
					yBegin = PreviewRect.bottom;
				break;
			}
		MoveToEx(hDC, LastMousePoint.x, yBegin, nullptr);
		LineTo(hDC, LastMousePoint.x, yEnd); //画垂直线
	}

	if (bCheckBound && SysState & 0x800) //只有在OnDraw里面调用RepairAcrossLine函数时bCheckBound才为真，所以bCheckBound既代表要检测鼠标的位置，也代表要马上显示tooltip
	{
		IntersectRect(&InvalidRect, &ToolTipRect, pRect);
		if (!IsRectEmpty(&InvalidRect))
			ShowToolTip(hDC);
	}
}

BOOL CST_CurveCtrl::FirstPage(BOOL bLast, BOOL bUpdate)
{
	if (nVisibleCurve <= 0)
		return FALSE;

	auto NewBeginTime = GetNearFrontPos(m_MinTime, OriginPoint.Time);
	if (bLast)
	{
		auto TotalLen = RightBottomPoint.ScrPos.x - LeftTopPoint.ScrPos.x;
		auto PageLen = OnePageLength;
		if (PageLen > 0)
		{
			auto nPageNum = TotalLen / PageLen;
			if (!(TotalLen % PageLen))
				--nPageNum;
			NewBeginTime += nPageNum * (HCoorData.nScales - nZLength) * HCoorData.fCurStep;
		}
	}

	SetBeginTime2(NewBeginTime);
	ReSetCurvePosition(4, bUpdate);

	return TRUE;
}

short CST_CurveCtrl::GotoPage(short RelativePage, BOOL bUpdate)
{
	short JumpPages = 0;
	auto re = MoveCurve(-RelativePage * (HCoorData.nScales - nZLength), 0, bUpdate); //检测水平移动合法性
	if (re)
	{
		JumpPages += RelativePage;
		if (!ReSetCurvePosition(4, bUpdate))
			JumpPages += GotoPage(RelativePage > 0 ? 1 : -1, bUpdate);
	}

	return JumpPages;
}

//Flag从低位起：
//第一位：是否打印页序号；
//第二位：是否后台打印，此时获取默认打印机做默认打印，但打印方向为横向（一般打印默认竖向打印）
//第三位：是否强行垂直居中，如果不强行垂直居中，则在需要时垂直居中
//第四位：是否保持本页纵坐标，如果三、四位同时为1，则第四位有效
//第五位：意义与第三位一样，但只在打印第一页时有效，换句话说，第三位只在打印非第一页时有效
//第六位：意义与第四位一样，但只在打印第一页时有效，换句话说，第四位只在打印非第一页时有效
//第七位：如果为1，则按位图方式打印前景（在绘制平滑曲线时，如果打印机不支持，可以采用这种方式，
//		这种方式优点是解决了平滑曲线的打印问题，缺点是画面粗糙）

//返回值，0-成功；-1-成功，但无数据可打印，可能是没有找到曲线或者曲线隐藏；
//1-打印失败；2-参数无效；3-用户取消打印；4-打印区域不存在；5-不存在默认打印机；6-打印机不支持按位图打印
short CST_CurveCtrl::PrintCurve(long Address,
							   HCOOR_TYPE BTime,
							   HCOOR_TYPE ETime,
							   short Mask,
							   short LeftMargin,
							   short TopMargin,
							   short RightMargin,
							   short BottomMargin,
							   LPCTSTR pTitle,
							   LPCTSTR pFootNote,
							   short Flag,
							   BOOL bAll)
{
	//边距都是指打印机象素
	if (LeftMargin < 0 || TopMargin < 0 || RightMargin < 0 || BottomMargin < 0)
		return 2; //参数无效

	vector<DataListHead<MainData>>::iterator DataListIter = NullDataListIter;
	if (!bAll)
	{
		DataListIter = FindMainData(Address);
		if (NullDataListIter == DataListIter || !ISCURVESHOWN(DataListIter) || DataListIter->Zx > nZLength * HSTEP)
			return -1; //无数据
	}

	auto MinTime = !(Mask & 1) || BTime <= m_MinTime ? m_MinTime : BTime;
	MinTime = GetNearFrontPos(MinTime, OriginPoint.Time);
	auto MaxTime = !(Mask & 2) ||  ETime >= m_MaxTime ? m_MaxTime : ETime;

	int nPageNum;
	GETPAGENUM(MinTime, MaxTime);
	if (!nPageNum)
		return -1; //无数据

	short re = 3; //用户取消打印
	CPrintDialog SetupDlg(!(Flag & 2), PD_RETURNDC);
	if (Flag & 2)
		if (!SetupDlg.GetDefaults())
			return 5; //没有默认打印机
		else
		{
			auto pDevMode=(DEVMODE*)::GlobalLock(SetupDlg.m_pd.hDevMode);
			if (DMORIENT_LANDSCAPE != pDevMode->dmOrientation)
			{
				pDevMode->dmOrientation = DMORIENT_LANDSCAPE;
				DELETEDC(SetupDlg.m_pd.hDC);
				SetupDlg.m_pd.hDC = SetupDlg.CreatePrinterDC();
			}
			::GlobalUnlock(SetupDlg.m_pd.hDevMode);
		}
	else
	{
		SetupDlg.m_pd.hDevMode = ::GlobalAlloc(GHND, sizeof(DEVMODE));
		auto pDevMode = (DEVMODE*)::GlobalLock(SetupDlg.m_pd.hDevMode);
		pDevMode->dmSize = sizeof(DEVMODE);
		pDevMode->dmOrientation = /*DMORIENT_PORTRAIT*/DMORIENT_LANDSCAPE; //默认横向打印
		pDevMode->dmPaperSize = DMPAPER_A4; //默认打印A4纸
		pDevMode->dmFields = DM_ORIENTATION | DM_PAPERSIZE;
		::GlobalUnlock(SetupDlg.m_pd.hDevMode);
	}

	if (Flag & 2 || SetupDlg.DoModal() == IDOK)
	{
		auto hPrintDC = SetupDlg.m_pd.hDC;

		auto RC = GetDeviceCaps(hPrintDC, RASTERCAPS);
		if (Flag & 0x40 && !(RC & RC_BITMAP64 && RC & RC_STRETCHDIB)) //按位图打印前景
			re = 6; //不支持位图打印
		else
		{
			auto RateX = (float) GetDeviceCaps(hPrintDC, LOGPIXELSX) / GetDeviceCaps(hFrceDC, LOGPIXELSX);
			auto RateY = (float) GetDeviceCaps(hPrintDC, LOGPIXELSY) / GetDeviceCaps(hFrceDC, LOGPIXELSY);
			auto ViewWidth = GetDeviceCaps(hPrintDC, PHYSICALWIDTH) -
				2 * GetDeviceCaps(hPrintDC, PHYSICALOFFSETX) - (int) (RateX * (LeftMargin + RightMargin));
			auto ViewHeight = GetDeviceCaps(hPrintDC, PHYSICALHEIGHT) -
				2 * GetDeviceCaps(hPrintDC, PHYSICALOFFSETY) - (int) (RateY * (TopMargin + BottomMargin));
			auto PrintWinWidth = (int) (ViewWidth / RateX);
			auto PrintWinHeight = (int) (ViewHeight / RateY);

			if (PrintWinWidth > 0 && PrintWinHeight > 0)
			{
				int cx, cy;
				GetControlSize(&cx, &cy);
				SetRedraw(FALSE);
				if (!(Flag & 0x40)) //按位图打印时，控件中不认为是打印
					SysState |= 1;
				ReSetUIPosition(PrintWinWidth, PrintWinHeight);

				GETPAGENUM(MinTime, MaxTime);
				WORD PageFrom = 1, PageTo = nPageNum;
				CPrintDialog PrintDlg(FALSE, PD_ALLPAGES | PD_DISABLEPRINTTOFILE);
				if (!(Flag & 2))
				{
					PrintDlg.m_pd.nMinPage = PrintDlg.m_pd.nFromPage = PageFrom;
					PrintDlg.m_pd.nMaxPage = PrintDlg.m_pd.nToPage = PageTo;
				}

				if (Flag & 2 || PrintDlg.DoModal() == IDOK)
				{
					if (!(Flag & 2))
					{
						if (PrintDlg.PrintAll()) //打印全部页
							PageTo |= 0x8000;
						else if (PrintDlg.PrintRange()) //打印一部分页
						{
							PageFrom = PrintDlg.GetFromPage();
							PageTo = PrintDlg.GetToPage();
						}
						else if (PrintDlg.PrintSelection())
							PageFrom = PageTo = 0; //打印当前页

						DELETEDC(PrintDlg.m_pd.hDC);
						if (PrintDlg.m_pd.hDevMode)
							::GlobalFree(PrintDlg.m_pd.hDevMode);
						if (PrintDlg.m_pd.hDevNames)
							::GlobalFree(PrintDlg.m_pd.hDevNames);
					}

					auto OrgX = (int) (RateX * LeftMargin), OrgY = (int) (RateY * TopMargin);

					::SetBkMode(hPrintDC, TRANSPARENT);
					if (!(m_ShowMode & 3) || Flag & 0x40) //在这种情况下，一次行映射，后面不再映射坐标
						CHANGE_PRINT_MAP_MODE(hPrintDC, ViewWidth, ViewHeight, OrgX, OrgY, 0);

					re = !DoPrintCurve(hPrintDC,
									DataListIter,
									MinTime,
									IsBadStringPtr(pTitle, -1) ? CurveTitle : pTitle, //不可读时，读取自己的标题来打印
									IsBadStringPtr(pFootNote, -1) ? FootNote : pFootNote, //不可读时，读取自己的脚注来打印
									ViewWidth,
									ViewHeight,
									OrgX,
									OrgY,
									PageFrom,
									PageTo,
									Flag);
				} //if (Flag & 2 || dlg.DoModal() == IDOK)

				ReSetUIPosition(cx, cy);
				if (!(Flag & 0x40)) //按位图打印时，控件中不认为是打印
					SysState &= ~1;
				SetRedraw();
				UpdateRect(hFrceDC, AllRectMask);
			} //if (PrintWinWidth > 0 && PrintWinHeight > 0)
			else
				re = 4; //打印区域不存在
		} //支持位图打印

		DELETEDC(hPrintDC);
	} //if (Flag & 2 || dlg.DoModal() == IDOK)
	::GlobalFree(SetupDlg.m_pd.hDevMode);
	if (SetupDlg.m_pd.hDevNames)
		::GlobalFree(SetupDlg.m_pd.hDevNames);

	return re;
}

//真正的曲线打印函数，nIndexInMainDataList为在MainDataList向量中的位置，从0开始，如果为-1，则打印全部曲线
BOOL CST_CurveCtrl::DoPrintCurve(HDC hPrintDC,
								 vector<DataListHead<MainData>>::iterator DataListIter,
								 HCOOR_TYPE BeginTime,
								 LPCTSTR pTitle,
								 LPCTSTR pFootNote,
								 int ViewWidth,
								 int ViewHeight,
								 int OrgX,
								 int OrgY,
								 WORD PageFrom, //开始页
								 WORD PageTo,   //结束页，如果两者都为0，则打印当前页，最高位为1时打印全部页
								 short Flag)
{
	DOCINFO df;
	memset(&df, 0, sizeof(DOCINFO));
	df.cbSize = sizeof(DOCINFO);
	df.lpszDocName = _T("ST_Curve Print");
	if (StartDoc(hPrintDC, &df) <= 0)
		return FALSE;

	HFONT hFont = nullptr, hGeneralFont, hHeadFont, hTitleFont = nullptr;
	auto hFontSave = (HFONT) GetCurrentObject(hPrintDC, OBJ_FONT);
	LOGFONT f;
	GetObject(this->hFont, sizeof(LOGFONT), &f);

	if (!(Flag & 0x40))
		hFont = CreateFontIndirect(&f); //正文字体
	f.lfHeight = -14; //五号字
	hGeneralFont = CreateFontIndirect(&f); //这个字体是打印时才有的，专门用于打印脚注
	f.lfWeight = 700;
	f.lfHeight = -19; //四号黑体
	hHeadFont = CreateFontIndirect(&f); //这个字体是打印时才有的，专门用于打印外标题
	if (!(Flag & 0x40))
	{
		GetObject(this->hTitleFont, sizeof(LOGFONT), &f);
		hTitleFont = CreateFontIndirect(&f);
	}

	BOOL bPrintAllPage = PageTo & 0x8000;
	PageTo &= 0x7FFF;

	WORD TotalPage = PageTo - PageFrom + 1;
	auto OldBeginValue = OriginPoint.Value; //保存当前页纵坐标开始值
	HCOOR_TYPE OldBeginTime;
	if (PageFrom > 0)
	{
		OldBeginTime = OriginPoint.Time; //保存当前页开始时间
		auto NewBeginTime = GETPAGESTARTTIME(BeginTime, PageFrom); //打印开始时间
		SetBeginTime2(NewBeginTime);
	}

	CString FootNote;
	FootNote.LoadString(IDS_FOOTNOTE);

	RECT TitleRect = {0, -40, WinWidth, 0}, FootNoteRect = {0, WinHeight, WinWidth, WinHeight + 25}, PrintRect = {0};
	HRGN hPrintRgn = nullptr;

	auto hDC = ::GetDC(m_hWnd);

	auto OldLeftSpace = LeftSpace;
	LPBITMAPINFO lpbi = nullptr;
	auto RC = GetDeviceCaps(hPrintDC, RASTERCAPS);
	if (!(Flag & 0x40) && RC & RC_BITMAP64 && RC & RC_STRETCHDIB) //按位图打印时，无需再打印背景
		lpbi = GetDIBFromDDB(hDC, hBackBmp);
	auto dwPaletteSize = !lpbi || lpbi->bmiHeader.biBitCount > 8 ? 0 : sizeof(RGBQUAD) * ((1 << lpbi->bmiHeader.biBitCount) - 1);

	auto titleColor = m_titleColor;
	if (IsColorsSimilar(titleColor, 0xffffff)) //打印纸是白色的，如果与白色相似，则取反
		titleColor = ~titleColor & 0xffffff;
	auto footNoteColor = m_footNoteColor;
	if (IsColorsSimilar(footNoteColor, 0xffffff)) //打印纸是白色的，如果与白色相似，则取反
		footNoteColor = ~footNoteColor & 0xffffff;

	auto re = TRUE;
	auto nPrintedPage = 0; //已经打印的页数
	auto bNeedCHPage = FALSE;
	UINT OpStyle = 4 | (Flag >> 4);
	for (auto i = 0; i < TotalPage || bPrintAllPage; ++i) //当打印全部页的时候，一直打印到不能翻页为止
	{
		//ReSetCurvePosition函数会适当的移动曲线，可能造成页数的增减，所以下面对GotoPage返回值的判断是非常必要的，比如说，在
		//打印前的页数判断时，得到的结果为10页，但在打印过程中，因为进行了翻页，同时对ReSetCurvePosition函数时行了调用，可能
		//最终只能打印出9页来，此时GotoPage将返回0，接着当然应该结束打印了，否则将进入死循环。当然，也可能最终可以打印出11页
		//来，上面的i < TotalPage || bPrintAllPage条件判断正是为解决这个问题而设计的，也就是说当打印全部页的时候，打印结束条
		//件是无法继续翻页，而不是已打印到TotalPage页，因为页数有可能增减。
		while (bNeedCHPage || !ReSetCurvePosition(OpStyle, !!(Flag & 0x40), DataListIter)) //由于按位图打印时，不认为是打印，所以需要刷新
		{
			auto PageStep = GotoPage(1, !!(Flag & 0x40)); //由于按位图打印时，不认为是打印，所以需要刷新
			if (!PageStep)
				goto PRINTOVER;

			i += PageStep - 1;
			if (!bPrintAllPage && i >= TotalPage) //这里的判断是非常有必要的，因为GotoPage有可能一次翻多页
				goto PRINTOVER;

			bNeedCHPage = FALSE;
		}
		bNeedCHPage = TRUE;
		if (1 == ++nPrintedPage)
			OpStyle = 4 | (Flag >> 2);

		if (StartPage(hPrintDC) <= 0)
		{
			re = FALSE;
			break;
		}

		if (m_ShowMode & 3 && !(Flag & 0x40)) //先映射为正常模式，便于打印标题及脚注，还有背景，按位图打印时也不用映射
			CHANGE_PRINT_MAP_MODE(hPrintDC, ViewWidth, ViewHeight, OrgX, OrgY, 0);

		//打印时，标题和脚注如果要打印的话，是打印在控件整个窗口之外的，所以在UpdateRect函数里面，在打印时，是不可能执行DrawCurveTitle与DrawFootNote函数的
		//另外，背景也不会在UpdateRect函数里面去打印，而是通过下面的StretchDIBits函数
		if (!i && pTitle && *pTitle || pFootNote && *pFootNote || Flag & 1)
		{
			if (pTitle && *pTitle)
			{
				SetTextColor(hPrintDC, titleColor);
				SelectObject(hPrintDC, hHeadFont);
				DrawText(hPrintDC, pTitle, -1, &TitleRect, DT_VCENTER | DT_CENTER | DT_SINGLELINE);
				pTitle = nullptr;
			}

			if (pFootNote && *pFootNote || Flag & 1)
			{
				auto len = 0;
				if (pFootNote)
					len = _sntprintf_s(StrBuff, _TRUNCATE, _T("%s"), pFootNote);
				if (Flag & 1 && -1 != len && len < StrBuffLen - 1)
					_sntprintf_s(StrBuff + len, StrBuffLen - len, _TRUNCATE, FootNote, i + 1, TotalPage);

				SetTextColor(hPrintDC, footNoteColor);
				SelectObject(hPrintDC, hGeneralFont);
				DrawText(hPrintDC, StrBuff, -1, &FootNoteRect, DT_VCENTER | DT_LEFT | DT_SINGLELINE);
			}
		}

		if (Flag & 0x40) //按位图打印前景
		{
			auto lpbi = GetDIBFromDDB(hDC, hFrceBmp);
			if (lpbi)
			{
				auto dwPaletteSize = lpbi->bmiHeader.biBitCount > 8 ? 0 : sizeof(RGBQUAD) * ((1 << lpbi->bmiHeader.biBitCount) - 1);
				StretchDIBits(hPrintDC, 0, 0, WinWidth, WinHeight, 0, 0, WinWidth, WinHeight,
					(LPBYTE) lpbi + sizeof(BITMAPINFO) + dwPaletteSize, (LPBITMAPINFO) lpbi, DIB_RGB_COLORS, SRCCOPY);
				LocalFree((HLOCAL) lpbi);
			}

			EndPage(hPrintDC);
			continue; //按位图打印已经结束，非常简洁
		}
		else if (lpbi)
		{
			//背景位图发生了改变（上面的网格移位了，位图的任何其它属性不会变，比如大小，调色板信息等），dwPaletteSize无需重新计算
			if (SysState & 0x180000 && OldLeftSpace != LeftSpace)
			{
				OldLeftSpace = LeftSpace;

				LocalFree((HLOCAL) lpbi);
				lpbi = GetDIBFromDDB(hDC, hBackBmp);
			}

			if (lpbi)
				StretchDIBits(hPrintDC, 0, 0, WinWidth, WinHeight, 0, 0, WinWidth, WinHeight,
					(LPBYTE) lpbi + sizeof(BITMAPINFO) + dwPaletteSize, (LPBITMAPINFO) lpbi, DIB_RGB_COLORS, SRCCOPY);
		}

		if (m_ShowMode & 3) //映射为需要的模式
			CHANGE_PRINT_MAP_MODE(hPrintDC, ViewWidth, ViewHeight, OrgX, OrgY, m_ShowMode);

		//背景已绘制，下面只需要绘制前景
		SelectObject(hPrintDC, hFont); //打印时的字体与hFont不通用
		UpdateRect(hPrintDC, PrintRectMask); //绘制前景（除了曲线、标题和脚注）

		if (!EqualRect(&PrintRect, CanvasRect + 1)) //只有在画布改变后才需要重新创建区域
		{
			InvalidRect = PrintRect = CanvasRect[1];
			LPtoDP(hPrintDC, (LPPOINT) &InvalidRect, 2); //因为调用了LPtoDP函数，不可使用CanvasRect[0]矩形
			//调用LPtoDP后，矩形可能出现非规格化状态（比如lift大于right等），但HRGN不在乎，只要确定矩形大小相同，不管如何表达矩形，得到的区域是相等的
			DELETEOBJECT(hPrintRgn);
			hPrintRgn = CreateRectRgnIndirect(&InvalidRect);
		}
		DrawCurve(hPrintDC, hPrintRgn, DataListIter);
		//绘制曲线，没有放在UpdateRect绘制是因为打印时需要特殊的区域hPrintrgn（在UpdateRect中无法访问该变量）
		//同样，下面两个函数的调用也没有放在UpdateRect里面，因为有一些打印参数在UpdateRect里面无法获得
		if (!(SysState & 0x200000)) //全屏时不打印内部标题和脚注
		{
			//2012.8.5
			//CHVIEWORG这个宏调用有什么作用？不调用后果是什么？
//			CHVIEWORG(hDC, ViewWidth, ViewHeight, m_ShowMode);
			if (*CurveTitle)
			{
				SetTextColor(hPrintDC, m_titleColor);
				SelectObject(hPrintDC, hTitleFont);
				DrawCurveTitle(hPrintDC);
			}

			SetTextColor(hPrintDC, m_footNoteColor);
			SelectObject(hPrintDC, hFont);
			DrawFootNote(hPrintDC);
		}

		EndPage(hPrintDC);
	} //for (auto i = 0; i < TotalPage || bPrintAllPage; ++i)

PRINTOVER:
	if (lpbi)
		LocalFree((HLOCAL) lpbi);

	::ReleaseDC(m_hWnd, hDC);

	SelectObject(hPrintDC, hFontSave);
	DELETEOBJECT(hFont);
	DELETEOBJECT(hGeneralFont);
	DELETEOBJECT(hHeadFont);
	DELETEOBJECT(hTitleFont);
	DELETEOBJECT(hPrintRgn);
	EndDoc(hPrintDC);

	if (PageFrom > 0)
		SetBeginTime2(OldBeginTime); //恢复当前页的开始时间
	SetBeginValue(OldBeginValue); //恢复当前页的纵坐标开始值

	return nPrintedPage + 1; //返回已打印的页数加1，也就是说，如果打印的页数为0的话，会返回1，代表成功
}

void CST_CurveCtrl::ReSetUIPosition(int cx, int cy)
{
	auto YOff = 0;

	if (WinWidth != cx || WinHeight != cy || !hFrceDC)
	{
		WinWidth = cx;
		WinHeight = cy;

		InitFrce(); //前面的!hFrceDC判断是必须的，主要是为了解决以空矩形创建控件时，没有及时创建前景DC的BUG
	}

	//即使是窗口大小完全没有改变，也必须得执行以下代码，因为显示模式可能改了
	CHANGE_MAP_MODE(hFrceDC, 0);
	CHANGE_MAP_MODE(hBackDC, 0);

	if (SysState & 0x200000)
	{
		auto CanvasBottom = WinHeight % VSTEP;
		if (CanvasBottom < 2)
			CanvasBottom += VSTEP;
		CanvasRect[1].top = CanvasBottom / 2 - 1;
		CanvasBottom -= CanvasBottom / 2;
		CanvasBottom = WinHeight - CanvasBottom;
		YOff = CanvasRect[1].bottom - CanvasBottom - 1; //减一必须，因为下面有语句：OffsetRect(CanvasRect + 1, 0, 1);
		CanvasRect[1].bottom = CanvasBottom;
	}
	else
	{
		FootNoteRect.right = CurveTitleRect.right = WinWidth;
		VAxisRect.bottom = WinHeight - BottomSpace - 5;
		HAxisRect.top = VAxisRect.bottom;
		HAxisRect.right = WinWidth - RightSpace + 1;
		HAxisRect.bottom = WinHeight - BottomSpace + 5 + 1;
		VLabelRect.bottom = VAxisRect.bottom - 1 + fHeight - fHeight / 2;
		HLabelRect.top = HAxisRect.bottom + 1;
		HLabelRect.right = WinWidth;
		HLabelRect.bottom = HLabelRect.top + BottomSpaceLine * (fHeight + 1);

		LegendMarkRect.left = WinWidth - RightSpace;
		LegendMarkRect.right = WinWidth;

		FootNoteRect.top = HLabelRect.bottom + 2;
		FootNoteRect.bottom = WinHeight;

		LegendRect[1].left = WinWidth - RightSpace;
		LegendRect[1].right = WinWidth;
		LegendRect[0] = LegendRect[1];
		MOVERECT(LegendRect[0], m_ShowMode);

		TimeRect.left = WinWidth - RightSpace + 15;
		TimeRect.top = WinHeight - BottomSpace - fHeight / 2;
		TimeRect.right = WinWidth;
		TimeRect.bottom = TimeRect.top + fHeight;

		auto CanvasBottom = VAxisRect.bottom - 1;
		YOff = CanvasRect[1].bottom - CanvasBottom - 1; //减一必须，因为下面有语句：OffsetRect(CanvasRect + 1, 0, 1);
		CanvasRect[1].bottom = CanvasBottom;
		CanvasRect[1].top = (CanvasRect[1].bottom - (TopSpace + 10) - 1) % VSTEP;
		CanvasRect[1].top += TopSpace + 10;
	} //if (SysState & 0x200000)

	OffsetRect(CanvasRect + 1, 0, 1);
	CanvasRect[0] = CanvasRect[1];
	MOVERECT(CanvasRect[0], m_ShowMode);

//	VCoorData.nScales = (USHORT) ((WinHeight - BottomSpace - 5 - 1 - TopSpace - 10 - 1) / VSTEP);
	VCoorData.nScales = (USHORT) ((CanvasRect[1].bottom - CanvasRect[1].top) / VSTEP);
	VCoorData.reserve(1 + VCoorData.nScales / (m_vInterval + 1));

	if (YOff)
		CalcOriginDatumPoint(OriginPoint, 0, 0, YOff);

	SetLeftSpace(TRUE);
}

int CST_CurveCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (COleControl::OnCreate(lpCreateStruct) == -1)
		return -1;

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, nullptr);

	SetTimer(HIDEHELPTIP, HIDEDELAY, nullptr);
//	OnActivateInPlace(TRUE, nullptr); //激活控件
	if (1 == InterlockedIncrement(&nRef)) //第一次运行
	{
		MsgHook = ::SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc, nullptr, GetCurrentThreadId());
		ASSERT(MsgHook);
	}

	auto pClientSite = GetClientSite();
	if (pClientSite)
	{
		IServiceProvider *isp, *isp2 = nullptr;
		auto hr = pClientSite->QueryInterface(IID_IServiceProvider, reinterpret_cast<void **>(&isp));
		if (SUCCEEDED(hr))
		{
			hr = isp->QueryService(SID_STopLevelBrowser, IID_IServiceProvider, reinterpret_cast<void **>(&isp2));
			if (SUCCEEDED(hr))
				isp2->QueryService(SID_SWebBrowserApp, IID_IWebBrowser2, reinterpret_cast<void **>(&pWebBrowser));
		}

		if (isp)
			isp->Release();
		if (isp2)
			isp2->Release();
	}

	hAxisPen = CreatePen(PS_SOLID, 1, m_axisColor);
	iSetFont((HFONT) GetStockObject(DEFAULT_GUI_FONT));

	return 0;
}

BOOL CST_CurveCtrl::OnSetExtent(LPSIZEL lpSizeL) 
{
	if (!AmbientUserMode())
	{
		//下面的代码其实只需要运行一次，但如何只运行一次，还没想到办法，除非使用静态变量
		//也许应该有个只触发一次的事件，而这些代码应该放在这个事件的响应函数里面
		//////////////////////////////////////////////////////////////////////////
		if (!hAxisPen) //在ide中新拖一个控件时，需要如下代码
			hAxisPen = CreatePen(PS_SOLID, 1, m_axisColor);
		SysState &= ~0x80040000; //设计模式下不显示帮助和全局位置窗口
		if (!hFont)
			iSetFont((HFONT) GetStockObject(DEFAULT_GUI_FONT));
		//////////////////////////////////////////////////////////////////////////

		//方法一：
		//用下面这行程序可以少连接一个lib文件（不需要AtlHiMetricToPixel函数），但兼容性可能会差一些
//		ReSetUIPosition((int) (lpSizeL->cx * 96 / 2540), (int) (lpSizeL->cy * 96 / 2540));

		SIZEL PixelSizel;
		//方法二：
		//使用AtlHiMetricToPixel函数
		ATL::AtlHiMetricToPixel(lpSizeL, &PixelSizel);
		//方法三：
		//把AtlHiMetricToPixel函数从atlwin.h里面取出来（这样就不用再包涵atlwin.h，以解决在vc6下面
		//包涵atlwin.h可能带来的编译错误或者警告）
//		AtlHiMetricToPixel(lpSizeL, &PixelSizel);
		ReSetUIPosition(PixelSizel.cx, PixelSizel.cy);
	}

	return COleControl::OnSetExtent(lpSizeL);
}

void CST_CurveCtrl::OnResetState()
{
	// TODO: 在此添加专用代码和/或调用基类
	ResetVersion(MAKELONG(_wVerMinor, _wVerMajor));
	ResetStockProps();

	m_foreColor = DEFAULT_foreColor;
	m_backColor = DEFAULT_backColor;
	m_axisColor = DEFAULT_axisColor;
	m_gridColor = DEFAULT_gridColor;
	m_titleColor = DEFAULT_titleColor;
	m_footNoteColor = DEFAULT_footNoteColor;
	m_pageChangeMSG = DEFAULT_pageChangeMSG;
}

void CST_CurveCtrl::Serialize(CArchive& ar)
{
	SerializeVersion(ar, MAKELONG(_wVerMinor, _wVerMajor));
	SerializeExtent(ar);
	SerializeStockProps(ar);

	if (ar.IsLoading())
	{
		ar >> m_foreColor;
		ar >> m_backColor;
		ar >> m_axisColor;
		ar >> m_gridColor;
		ar >> m_titleColor;
		ar >> m_footNoteColor;
		ar >> m_pageChangeMSG;
		if (!AmbientUserMode())
		{
			OnForeColorChanged();
			OnBackColorChanged();
			OnAxisColorChanged();
			OnGridColorChanged();
		}
	}
	else
	{
		ar << m_foreColor;
		ar << m_backColor;
		ar << m_axisColor;
		ar << m_gridColor;
		ar << m_titleColor;
		ar << m_footNoteColor;
		ar << m_pageChangeMSG;
	}
}

HCOOR_TYPE CST_CurveCtrl::GetNearFrontPos(HCOOR_TYPE DateTime, HCOOR_TYPE BenchmarkTime)
{
	auto TimeDiff = DateTime - BenchmarkTime;
	auto xStep = (int) (TimeDiff / HCoorData.fCurStep);
	if (xStep > 0)
		return BenchmarkTime + xStep * HCoorData.fCurStep;
	else if (!xStep && TimeDiff >= .0)
		return BenchmarkTime;
	else
		return GetNearFrontPos(DateTime, BenchmarkTime + (xStep - 1) * HCoorData.fCurStep);
}

/*
检测是否可设置开始值为fBeginValue，返回值
0－非空，1－空页，2－所有曲线都在画布上面，3－所有曲线都在画布下面
*/
UINT CST_CurveCtrl::CheckVPosition(float fBeginValue) //这样的判断可能让所有曲线都移出画布，因为没有考虑到Z坐标的影响，DrawCurve函数会解决这个问题
{
	if (nVisibleCurve <= 0)
		return 1;

	if (m_MaxValue < fBeginValue)
		return 3;
	else if (m_MinValue > fBeginValue + VCoorData.nScales * VCoorData.fCurStep)
		return 2;
	else
		return 0;
}

/*
检测是否可设置开始时间为fBeginTime，返回值
0－非空，1－空页，2－所有曲线都在画布右面，3－所有曲线都在画布左面
*/
UINT CST_CurveCtrl::CheckHPosition(HCOOR_TYPE fBeginTime) //这样的判断可能让所有曲线都移出画布，因为没有考虑到Z坐标的影响，DrawCurve函数会解决这个问题
{
	if (nVisibleCurve <= 0)
		return 1;

	if (m_MaxTime < fBeginTime)
		return 3;
	else if (m_MinTime > fBeginTime + HCoorData.nScales * HCoorData.fCurStep)
		return 2;
	else
		return 0;
}

UINT CST_CurveCtrl::CheckPosition(short xStep, short yStep)
{
	UINT Mask = 0; //先假设不能移动
	//由于有了Z轴，所以不得不损失效率，不再使用所有曲线所占区域，而是用每一条曲线所占区域来判断
	for (auto i = begin(MainDataListArr); i < end(MainDataListArr); ++i)
	{
		auto left = CanvasRect[1].left + i->Zx, bottom = CanvasRect[1].bottom - i->Zy;
		if (xStep && !(Mask & 1) && CanvasRect[1].right >= i->LeftTopPoint.ScrPos.x + xStep && i->RightBottomPoint.ScrPos.x + xStep >= left)
			Mask |= 1;
		if (yStep && !(Mask & 2) && bottom >= i->LeftTopPoint.ScrPos.y - yStep && i->RightBottomPoint.ScrPos.y - yStep >= CanvasRect[1].top)
			Mask |= 2;

		if ((!xStep || Mask & 1) && (!yStep || Mask & 2))
			break;
	}

	return Mask;
}

/*OpStyle从低位开始
第1位：1－强行垂直居中
第2位：1－保持本页纵坐标
第3位：1－保持本页横坐标位置，不管是否设置该位，本函数都会尽量保持横坐标，只有在无法保持的时候，设置与否才会有区别
返回值
第1位：1－本页非空
第2位：1－水平方向上移动过曲线
第3位：1－垂直方向上移动过曲线
*/
UINT CST_CurveCtrl::ReSetCurvePosition(UINT OpStyle, BOOL bUpdate, vector<DataListHead<MainData>>::iterator DataListIter/*= NullDataListIter*/)
{
	if (nVisibleCurve <= 0)
		return 0;

	UINT re = 0;
	vector<DataListHead<MainData>>::iterator ThisCurveIter = NullDataListIter;
	if (NullDataListIter != DataListIter)
	{
		if (!ISCURVESHOWN(DataListIter))
			ShowCurve(DataListIter->Address, TRUE);

		if (ISCURVEINPAGE(DataListIter, 2 & OpStyle, !(2 & OpStyle))) //可以直接选中DataListIter曲线
			ThisCurveIter = DataListIter;
		else if (!(4 & OpStyle)) //可以移动横坐标
		{
			SetBeginTime2(GetNearFrontPos(DataListIter->LeftTopPoint.Time, OriginPoint.Time));
			re |= 2; //水平方向上移动过曲线
			if (ISCURVEINPAGE(DataListIter, 2 & OpStyle, !(2 & OpStyle))) //可以直接选中DataListIter曲线
				ThisCurveIter = DataListIter;
		}
	}
	else if (-1 != CurCurveIndex && ISCURVEINPAGE(next(begin(MainDataListArr), CurCurveIndex), TRUE, FALSE)) //有选中的曲线，且当前页里面就有显示，按它为基准移动曲线
		ThisCurveIter = next(begin(MainDataListArr), CurCurveIndex);
	else
	{
		auto bFound = FALSE;
		for (auto i = begin(MainDataListArr); !bFound && i < end(MainDataListArr); ++i)
		{
			ThisCurveIter = i;
			if (ISCURVEVISIBLE(ThisCurveIter, 2 & OpStyle, !(2 & OpStyle))) //在当前页中有显示的第一条曲线，就选择它
				bFound = TRUE;
		}

		if (!bFound) //还没有找到曲线
			if (!(4 & OpStyle)) //在允许移动横坐标的情况下，再次查找曲线
			{
				for (auto i = begin(MainDataListArr); !bFound && i < end(MainDataListArr); ++i)
				{
					ThisCurveIter = i;
					if (ISCURVESHOWN(ThisCurveIter))
					{
						if (!ISCURVEINPAGE(ThisCurveIter, 2 & OpStyle, !(2 & OpStyle)))
						{
							SetBeginTime2(GetNearFrontPos(ThisCurveIter->LeftTopPoint.Time, OriginPoint.Time));
							re |= 2;
						}
						bFound = TRUE;
					}
				}

				ASSERT(bFound); //一定可以找到，因为前面有DataListNum >= 0的判断
			}
			else
				ThisCurveIter = NullDataListIter; //找不到可作为移动依据的曲线
	}

	if (NullDataListIter != ThisCurveIter)
	{
		re |= 1;
		if (!(2 & OpStyle))
		{
			long MidValue = 0;
			auto Num = 0;
			auto pDataVector = ThisCurveIter->pDataVector;
			auto i = GetFirstVisiblePos(ThisCurveIter, FALSE, TRUE); //在水平轴上可见即可
			if (NullDataIter != i)
				for (; i < end(*pDataVector); ++i) //计算当前页的平均值
					if (IsPointVisible(ThisCurveIter, i, FALSE, TRUE)) //点的水平坐标只需要在画布中即可
					{
						MidValue += i->ScrPos.y; //所有点（包括隐藏点）都参与了平均值的计算
						++Num;
					}
					else if (1 == ThisCurveIter->Power) //1次曲线当找到画布之外的点的时候就马上退出
						break;

			if (Num)
			{
				MidValue /= Num;
				if (1 & OpStyle || CanvasRect[1].top > MidValue || MidValue > CanvasRect[1].bottom)
				{
					auto yStep = (short) ((MidValue - (CanvasRect[1].bottom + CanvasRect[1].top) / 2) / VSTEP);
					if (yStep)
					{
						MoveCurve(0, yStep, bUpdate, FALSE); //不用检测移动合法性
						re |= 4;
					}
				}
			} //if (Num)
		} //if (!(2 & OpStyle))

		ReportPageChanges;
	} //if (NullDataListIter != ThisCurveIter)

	return re;
}

//删除点，返回值按位算，从低位开始：3－是否删除了CurCurveIter，4－是否删除了DataListIter整条曲线
UINT CST_CurveCtrl::DoDelMainData(vector<DataListHead<MainData>>::iterator& DataListIter, vector<MainData>::iterator BeginPos, vector<MainData>::iterator EndPos, BOOL bUpdate)
{
	ASSERT(NullDataListIter != DataListIter && DataListIter < end(MainDataListArr));
	auto pDataVector = DataListIter->pDataVector;
	ASSERT(NullDataIter != BeginPos && NullDataIter != EndPos && BeginPos < EndPos && EndPos <= end(*pDataVector));

	UINT re = 0;
	pDataVector->erase(BeginPos, EndPos);
	if (pDataVector->empty())
	{
		if (ISCURVESHOWN(DataListIter))
			--nVisibleCurve;
		delete pDataVector;
		if (CurCurveIndex == distance(begin(MainDataListArr), DataListIter)) //当前选中的曲线被删除，所以删除过后，它将无效
		{
			FIRE_SelectedCurveChange(0x7fffffff);
			CurCurveIndex = -1;
			re |= 4;
		}
		FIRE_CurveStateChange(DataListIter->Address, 2); //曲线被删除
		DataListIter = MainDataListArr.erase(DataListIter);
		re |= 8;
	}
	else
	{
		if (2 == DataListIter->Power) //删除点后，可能从2次曲线变为1次曲线，仅此而已
			UpdatePower(DataListIter);
		UpdateOneRange(DataListIter); //更新MinTime, MaxTime, MinValue, MaxValue
	}

	if (nVisibleCurve <= 0)
		CHANGEMOUSESTATE;

	UpdateTotalRange(); //在这里调用该函数是正确的，不可插入到InvalidCurveSet集合，因为有可能整条曲线都被删除了
	if (!(ReSetCurvePosition(0, bUpdate) & 6))
		if (bUpdate)
			UpdateRect(hFrceDC, CanvasRectMask);
		else
			SysState |= 0x200;

	return re;
}

//Mask代表BTime和ETime的有效性，从低为起，第1位代表BTime的有效性，第2位代表ETime的有效性
//Mask还出现在ExportImageFromTime和PriveCurve函数，他们的意义与这里完全一样
void CST_CurveCtrl::DelRange(long Address, HCOOR_TYPE BTime, HCOOR_TYPE ETime, short Mask, BOOL bAll, BOOL bUpdate)
	{DELRANGE(;, k = j, Mask & 1 &&  BTime > j->Time, !(Mask & 2), k->Time <= ETime, TRUE, (!(Mask & 1) || BTime <= j->Time) && (!(Mask & 2) || j->Time <= ETime));}
void CST_CurveCtrl::DelRange2(long Address, long nBegin, long nCount, BOOL bAll, BOOL bUpdate)
{
	ASSERT(nBegin >= 0 && (nCount > 0 || -1 == nCount));
	if (nBegin >= 0 && (nCount > 0 || -1 == nCount))
		DELRANGE(advance(j, nBegin), //CON1取得开始删除位置
			k = j + nCount, //CON2取得结束删除位置
			0, //C1直接构造一个假条件
			-1 == nCount || (size_t) (nBegin + nCount) >= pDataVector->size(), //C2
			0, //C3直接构造一个假条件
			FALSE, FALSE);
}

void CST_CurveCtrl::ClearCurve()
{
	for (auto i = begin(MainDataListArr); i < end(MainDataListArr); ++i)
		delete i->pDataVector;
	free_container(MainDataListArr);
}

BOOL CST_CurveCtrl::SetMaxLength(long MaxLength, long CutLength)
{
	if (-1 == MaxLength || CutLength > 0 && MaxLength > CutLength)
	{
		this->MaxLength = MaxLength;
		this->CutLength = CutLength;
		return TRUE;
	}
	else
		return FALSE;
}
long CST_CurveCtrl::GetMaxLength() {return MaxLength;}

long CST_CurveCtrl::GetCutLength() {return CutLength;}

void CST_CurveCtrl::TrimCoor() 
{
	//不可用SetBeginTime2和SetBeginValue代替，因为该函数有可能设置不成功（比如在只有一个点的时候）
	//而下面几行可以保证成功，因为加了ReSetCurvePosition函数调用
	UINT Mask = 0;

	auto OldBeginTime = OriginPoint.Time;
	if (pTrimXCoordinate)
		OriginPoint.Time = pTrimXCoordinate(OldBeginTime);
	else
		OriginPoint.Time = TrimDateTime(OldBeginTime);
	if (OldBeginTime != OriginPoint.Time)
		Mask |= 1;

	float OldBeginValue = OriginPoint.Value;
	if (pTrimYCoordinate)
		OriginPoint.Value = pTrimYCoordinate(OldBeginValue);
	else
		OriginPoint.Value = TrimValue(OldBeginValue);
	if (OldBeginValue != OriginPoint.Value)
		Mask |= 2;

	if (Mask)
	{
		CalcOriginDatumPoint(OriginPoint, Mask);

		UINT UpdateMask = 0;
		if (Mask & 1)
		{
			if (!(2 & ReSetCurvePosition(2, TRUE)))
			{
				UpdateMask |= HLabelRectMask;
				UpdateMask |= CanvasRectMask;
			}
			SYNBUDDYS(2, &OriginPoint.Time);
		}

		if (Mask & 2)
			if (!(4 & ReSetCurvePosition(4, TRUE)))
			{
				UpdateMask |= VLabelRectMask;
				UpdateMask |= CanvasRectMask;
			}

		if (UpdateMask)
			UpdateRect(hFrceDC, UpdateMask);
	}
}

BOOL CST_CurveCtrl::CanCurveBeDrawnAlone(vector<DataListHead<MainData>>::iterator DataListIter)
{
	ASSERT(NullDataListIter != DataListIter);
	if (NullDataListIter == DataListIter)
		return TRUE;

	for (auto i = DataListIter + 1; i < end(MainDataListArr); ++i)
		if (NullDataIter != GetFirstVisiblePos(i, TRUE, FALSE)) //i曲线完全不可见（部分可见即可见）
			return FALSE;

	return TRUE;
}

void CST_CurveCtrl::MoveSelectNodeForward()
{
	ASSERT(CurCurveIndex < MainDataListArr.size());
	if (CurCurveIndex < MainDataListArr.size())
	{
		auto MainDataIter = next(begin(MainDataListArr), CurCurveIndex);
		auto LegendIter = MainDataIter->LegendIter;
		if (NullLegendIter != LegendIter &&
			LegendIter->NodeMode && LegendIter->NodeModeEx & 4) //在不显示选中点时，不移动选中点
		{
			size_t OldSelectedIndex = MainDataIter->SelectedIndex, NewSelectedIndex;
			auto TotalNode = MainDataIter->pDataVector->size();

			if (-1 == OldSelectedIndex) //让最后一个点成为选中点
				MainDataIter->SelectedIndex = NewSelectedIndex = TotalNode - 1;
			else //允许在有选中点的情况下，通过移动选中点变为没有选中点，设计即是如此
				MainDataIter->SelectedIndex = NewSelectedIndex = OldSelectedIndex - 1;

			if (NewSelectedIndex != OldSelectedIndex) //刷新选中点
				UpdateSelectedNode(MainDataIter, OldSelectedIndex);
		}
	}
}

void CST_CurveCtrl::MoveSelectNodeBackward()
{
	ASSERT(CurCurveIndex < MainDataListArr.size());
	if (CurCurveIndex < MainDataListArr.size())
	{
		auto MainDataIter = next(begin(MainDataListArr), CurCurveIndex);
		auto LegendIter = MainDataIter->LegendIter;
		if (NullLegendIter != LegendIter &&
			LegendIter->NodeMode && LegendIter->NodeModeEx & 4) //在不显示选中点时，不移动选中点
		{
			size_t OldSelectedIndex = MainDataIter->SelectedIndex, NewSelectedIndex;
			auto TotalNode = MainDataIter->pDataVector->size();

			if (-1 == OldSelectedIndex) //让第一个点成为选中点
				MainDataIter->SelectedIndex = NewSelectedIndex = 0;
			else
			{
				NewSelectedIndex = OldSelectedIndex + 1;
				if (NewSelectedIndex >= TotalNode) //允许在有选中点的情况下，通过移动选中点变为没有选中点，设计即是如此
					NewSelectedIndex = -1;

				MainDataIter->SelectedIndex = NewSelectedIndex;
			}

			if (NewSelectedIndex != OldSelectedIndex) //刷新选中点
				UpdateSelectedNode(MainDataIter, OldSelectedIndex);
		}
	}
}

//调用这个函数之前要保证所有可能的错误都排除了，就像UpdateSelectedNode所做的工作一样
UINT CST_CurveCtrl::DrawSelectedNode(HDC hDC, HRGN hRen, vector<DataListHead<MainData>>::iterator DataListIter, UINT PenWidth, size_t OldSelectedNode)
{
	auto NewSelectedNode = DataListIter->SelectedIndex;

	auto pDataVector = DataListIter->pDataVector;
	ASSERT(pDataVector);
	ASSERT(-1 == OldSelectedNode || OldSelectedNode < pDataVector->size());
	ASSERT(-1 == NewSelectedNode || NewSelectedNode < pDataVector->size());

	UINT re = 0;
	if ((-1 == OldSelectedNode || OldSelectedNode < pDataVector->size()) &&
		(-1 == NewSelectedNode || NewSelectedNode < pDataVector->size()))
	{
		if (hRen) //在DrawCurve里面调用时，不需要选入区域，因为已经选入了
			SelectClipRgn(hDC, hRen);
		auto OldBkColor = GetBkColor(hDC); //保存老的BkColor，这是必须的，因为在DrawCurve里面会调用本函数，必须要能够还原它

		auto LegendIter = DataListIter->LegendIter;
		if (-1 != OldSelectedNode && OldSelectedNode != NewSelectedNode) //抹掉老的选中点
		{
			auto OldNodeColor = LegendIter->PenColor;
			if (2 == LegendIter->NodeMode) //用曲线颜色的反色显示节点
				OldNodeColor = ~OldNodeColor & 0xFFFFFF;

			if (OldSelectedNode == pDataVector->size() - 1)
			{
				if (LegendIter->NodeModeEx & 2)
					OldNodeColor = LegendIter->EndNodeColor;
			}
			else if (!OldSelectedNode)
				if (LegendIter->NodeModeEx & 1)
					OldNodeColor = LegendIter->BeginNodeColor;

			SetBkColor(hDC, OldNodeColor);
			DrawNode((*pDataVector)[OldSelectedNode].ScrPos, PenWidth, LegendIter->NodeMode);
			re |= 1;
		}

		if (-1 != NewSelectedNode) //绘制新的选中点
		{
			SetBkColor(hDC, LegendIter->SelectedNodeColor);
			DrawNode((*pDataVector)[NewSelectedNode].ScrPos, PenWidth, LegendIter->NodeMode);
			re |= 2;
		}

		SetBkColor(hDC, OldBkColor);
		if (hRen)
			SelectClipRgn(hDC, nullptr);
	} //if

	return re;
}

void CST_CurveCtrl::UpdateSelectedNode(vector<DataListHead<MainData>>::iterator DataListIter, size_t OldSelectedNode)
{
	ASSERT(NullDataListIter != DataListIter);
	if (NullDataListIter == DataListIter)
		return;

	auto NewSelectedNode = DataListIter->SelectedIndex;
	ASSERT(OldSelectedNode != NewSelectedNode);

	if (OldSelectedNode == NewSelectedNode)
		return;

	//想要绘制出来选中节点需要的图例支持
	auto LegendIter = DataListIter->LegendIter;
	if (NullLegendIter == LegendIter)
		return;

	UINT NodeMode = LegendIter->NodeMode;
	if (!NodeMode) //节点必须要处于显示状态
		return;

	if (!(LegendIter->NodeModeEx & 4)) //不显示选中点
		return;
	//结束

	auto pDataVector = DataListIter->pDataVector;
	if (0 <= NewSelectedNode && NewSelectedNode < pDataVector->size())
	{
		auto size = MakePointVisible(DataListIter, next(begin(*pDataVector), NewSelectedNode));
		if (MoveCurve(size))
			return;
	}

	if (CanCurveBeDrawnAlone(DataListIter))
	{
		UINT PenWidth = LegendIter->LineWidth;
		if (CurCurveIndex == distance(begin(MainDataListArr), DataListIter))
			PenWidth *= 2;

		//刷新区域
		RECT rectOld, rectNew;

		//考虑到效率问题，本函数没有直接调用UpdateRect函数，因为这个函数至少要把DataListIter曲线重新绘制一次
		if (m_ShowMode & 3)
			CHANGE_MAP_MODE(hFrceDC, m_ShowMode);

		auto re = DrawSelectedNode(hFrceDC, hScreenRgn, DataListIter, PenWidth, OldSelectedNode);
		if (re)
		{
			if (re & 1)
			{
				MakeNodeRect(rectOld, (*pDataVector)[OldSelectedNode].ScrPos, PenWidth, NodeMode);
				//不存在NormalizeRect这个API，只有CRect类有，所有这里要自己实现矩形的规格化
				NormalizeRect(rectOld);
			}

			if (re & 2)
			{
				MakeNodeRect(rectNew, (*pDataVector)[NewSelectedNode].ScrPos, PenWidth, NodeMode);
				//不存在NormalizeRect这个API，只有CRect类有，所有这里要自己实现矩形的规格化
				NormalizeRect(rectNew);
			}
		} //if (re)

		if (m_ShowMode & 3)
		{
			CHANGE_MAP_MODE(hFrceDC, 0);
			if (re & 1)
			{
				InflateRect(&rectOld, 1, 1);
				MOVERECT(rectOld, m_ShowMode);
			}
			if (re & 2)
			{
				InflateRect(&rectNew, 1, 1);
				MOVERECT(rectNew, m_ShowMode);
			}
		}

		if (re & 1)
			InvalidateControl(&rectOld, FALSE); //刷新上面的绘制到屏幕
		if (re & 2)
			InvalidateControl(&rectNew, FALSE); //刷新上面的绘制到屏幕

		if (re & 1 && IntersectRect(&rectOld, &rectOld, &PreviewRect) ||
			re & 2 && IntersectRect(&rectNew, &rectNew, &PreviewRect))
			UpdateRect(hFrceDC, PreviewRectMask); //更新全局位置窗口
	} //if (CanCurveBeDrawnAlone(DataListIter))
	else
		UpdateRect(hFrceDC, CanvasRectMask);
}

BOOL CST_CurveCtrl::PreTranslateMessage(MSG* pMsg) 
{
	if (SysState & 0x40 && (VK_CONTROL == pMsg->wParam || VK_SHIFT == pMsg->wParam) &&
		(WM_KEYUP == pMsg->message || WM_KEYDOWN == pMsg->message))
	{
		auto re = COleControl::PreTranslateMessage(pMsg);

		POINT ScrPos;
		SYNA_TO_TRUE_POINT;
		SysState &= ~0x40;

		return re;
	}

	auto re = FALSE;
	if (WM_KEYDOWN == pMsg->message)
		switch (pMsg->wParam)
		{
		case VK_F4: //显示帮助
			re = ShortcutKey & 1;
			if (re)
				EnableHelpTip(!(SysState & 0x40000));
			break;
		case VK_F5: //刷新键
			re = ShortcutKey & 2;
			if (re)
				ReSetCurvePosition(1, TRUE);
			break;
		case VK_F6: //显示隐藏全局位置预览窗口
			re = ShortcutKey & 4;
			if (re)
				EnablePreview(!(SysState & 0x80000000));
			break;
		case VK_F7: //全屏
			re = ShortcutKey & 8;
			if (re)
				EnableFullScreen(!(SysState & 0x200000));
			break;
		case 0xBD: //缩放键
		case 0xBB:
		case VK_ADD:
		case VK_SUBTRACT:
			if (SysState & 0x400 && nVisibleCurve > 0)
				if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
					re = SetZoom(Zoom + (0xBD == pMsg->wParam || VK_SUBTRACT == pMsg->wParam ? -1 : 1));
				else if (DRAGMODE != MouseMoveMode)
				{
					DoSetFixedZoomMode(0xBD == pMsg->wParam || VK_SUBTRACT == pMsg->wParam ? ZOOMOUT : ZOOMIN, pMsg->pt);
					re = TRUE;
				}
			break;
		default:
			if (0x31 <= pMsg->wParam && pMsg->wParam <= 0x39 || VK_NUMPAD1 <= pMsg->wParam && pMsg->wParam <= VK_NUMPAD9) //数字键
			{
				size_t index = 0x31 <= pMsg->wParam && pMsg->wParam <= 0x39 ? pMsg->wParam - 0x31 : pMsg->wParam - VK_NUMPAD1;
				re = ShortcutKey & (1 << (index + 7));
				if (re)
					re = SelectLegendFromIndex(index);
			}
			else if (VK_PRIOR <= pMsg->wParam && pMsg->wParam <= VK_DOWN) //方向键
			{
				switch (pMsg->wParam)
				{
				case VK_PRIOR: //翻页键
					re = ShortcutKey & 0x10;
					if (re)
						GotoPage(-1, TRUE);
					break;
				case VK_NEXT: //翻页键
					re = ShortcutKey & 0x10;
					if (re)
						GotoPage(1, TRUE);
					break;
				case VK_END: //翻页键
					re = ShortcutKey & 0x10;
					if (re)
						FirstPage(TRUE, TRUE);
					break;
				case VK_HOME: //翻页键
					re = ShortcutKey & 0x10;
					if (re)
						FirstPage(FALSE, TRUE);
					break;
				case VK_LEFT: //方向键
					re = ShortcutKey & 0x40;
					if (re)
						if (-1 != CurCurveIndex && GetAsyncKeyState(VK_CONTROL) & 0x8000)
							//在ctrl键按下的情况下，如果当前有选中曲线，则移动曲线中的选中点，如果曲线中还没有选中点，则从最后一个点开始
							//并且设置当前选中点为最后一个点
MOVE_LEFT:
							if (m_ShowMode & 1)
								MoveSelectNodeBackward();
							else
								MoveSelectNodeForward();
						else
							MoveCurve(m_ShowMode & 1 ? 1 : -1, 0);
					break;
				case VK_UP: //方向键
					re = ShortcutKey & 0x20;
					if (re)
						if (-1 != CurCurveIndex && GetAsyncKeyState(VK_CONTROL) & 0x8000) //同VK_LEFT
							goto MOVE_LEFT;
						else
							MoveCurve(0, m_ShowMode & 2 ? -1 : 1);
					break;
				case VK_RIGHT: //方向键
					re = ShortcutKey & 0x40;
					if (re)
						if (-1 != CurCurveIndex && GetAsyncKeyState(VK_CONTROL) & 0x8000)
							//在ctrl键按下的情况下，如果当前有选中曲线，则移动曲线中的选中点，如果曲线中还没有选中点，则从第一个点开始
							//并且设置当前选中点为第一个点
MOVE_RIGHT:
							if (m_ShowMode & 1)
								MoveSelectNodeForward();
							else
								MoveSelectNodeBackward();
						else
							MoveCurve(m_ShowMode & 1 ? -1 : 1, 0);
					break;
				case VK_DOWN: //方向键
					re = ShortcutKey & 0x20;
					if (re)
						if (-1 != CurCurveIndex && GetAsyncKeyState(VK_CONTROL) & 0x8000) //同VK_RIGHT
							goto MOVE_RIGHT;
						else
							MoveCurve(0, m_ShowMode & 2 ? 1 : -1);
					break;
				} //switch
			} //方向键
			break;
		}

	return re || COleControl::PreTranslateMessage(pMsg);
}

BOOL CST_CurveCtrl::SetShowMode(short ShowMode)
{
	short HMode = ShowMode & 0x80;
	ShowMode &= ~0x80;
	if (0 > ShowMode || ShowMode > 0xF)
		return FALSE;

	UINT UpdateMask = 0;
	if ((HMode ^ m_ShowMode) & 0x80) //第8位有变化
	{
		m_ShowMode &= ~0x80;
		m_ShowMode |= HMode;

		UpdateMask |= TimeRectMask | HLabelRectMask;
		for (auto i = begin(MainDataListArr); i < end(MainDataListArr); ++i)
		{
			if (NullLegendIter != i->LegendIter && i->LegendIter->Lable & 1) //显示X坐标，这里需要更新
			{
				UpdateMask |= CanvasRectMask;
				break;
			}
		}
	}

	if ((m_ShowMode & 0xF) != ShowMode)
	{
		m_ShowMode &= ~0xF;
		m_ShowMode |= (BYTE) ShowMode;
		ReSetUIPosition(WinWidth, WinHeight);
		UpdateMask |= AllRectMask;
	}

	if (UpdateMask)
		UpdateRect(hFrceDC, UpdateMask);

	return TRUE;
}
short CST_CurveCtrl::GetShowMode() {return m_ShowMode;}

BOOL CST_CurveCtrl::GetOneTimeRange(long Address, HCOOR_TYPE* pMinTime, HCOOR_TYPE* pMaxTime)
	{FILL2VALUE(auto DataListIter = FindMainData(Address); if (NullDataListIter != DataListIter), pMinTime, pMaxTime, DataListIter->LeftTopPoint.Time, DataListIter->RightBottomPoint.Time);}
BOOL CST_CurveCtrl::GetTimeRange(HCOOR_TYPE* pMinTime, HCOOR_TYPE* pMaxTime) {FILL2VALUE(if (m_MaxTime >= m_MinTime), pMinTime, pMaxTime, m_MinTime, m_MaxTime);}
BOOL CST_CurveCtrl::GetOneValueRange(long Address, float* pMinValue, float* pMaxValue)
	{FILL2VALUE(auto DataListIter = FindMainData(Address); if (NullDataListIter != DataListIter), pMinValue, pMaxValue, DataListIter->RightBottomPoint.Value, DataListIter->LeftTopPoint.Value);}
BOOL CST_CurveCtrl::GetValueRange(float* pMinValue, float* pMaxValue) {FILL2VALUE(if (m_MaxValue >= m_MinValue), pMinValue, pMaxValue, m_MinValue, m_MaxValue);}

BOOL CST_CurveCtrl::GetOneFirstPos(long Address, HCOOR_TYPE* pTime, float* pValue, BOOL bLast)
{
	auto DataListIter = FindMainData(Address);
	if (NullDataListIter != DataListIter)
	{
		auto pDataVector = DataListIter->pDataVector;
		if (!IsBadWritePtr(pTime, sizeof(HCOOR_TYPE)))
			*pTime = bLast ? pDataVector->back().Time : pDataVector->front().Time;
		if (!IsBadWritePtr(pValue, sizeof(float)))
			*pValue = bLast ? pDataVector->back().Value : pDataVector->front().Value;

		return TRUE;
	}

	return FALSE;
}

BOOL CST_CurveCtrl::SetMoveMode(short MoveMode)
{
	UINT Hbit = MoveMode & 0x80;
	MoveMode &= ~0x80;
	if (0 <= MoveMode && MoveMode <= 7)
	{
		m_MoveMode &= 0x80;
		m_MoveMode |= (BYTE) MoveMode;
		Hbit ^= m_MoveMode;
		if (Hbit & 0x80) //最高位有变化
		{
			if (m_MoveMode & 0x80) //需要显示十字架
			{
				m_MoveMode &= ~0x80;
				POINT point;
				GetCursorPos(&point);
				ScreenToClient(&point);
				if (PtInRect(CanvasRect, point))
					DrawAcrossLine(&point);
			}
			else //需要隐藏十字架，改为显示普通鼠标
			{
				POINT EmptyPoint = {-1};
				DrawAcrossLine(&EmptyPoint);
				m_MoveMode |= 0x80;
				KillTimer(SHOWTOOLTIP);
			}

			PostMessage(WM_SETCURSOR, (WPARAM) m_hWnd, HTCLIENT | WM_MOUSEMOVE);
		}
		return TRUE;
	}
	else
		return FALSE;
}
short CST_CurveCtrl::GetMoveMode() {return m_MoveMode;}

long CST_CurveCtrl::TrimCurve2(long Address, short State, HCOOR_TYPE BTime, HCOOR_TYPE ETime, short Mask, short nStep, BOOL bAll)
{
	ASSERT(IsMainDataStateValidate(State) && nStep > 0);
	if (!IsMainDataStateValidate(State) || nStep <= 0)
		return 0;

	TRIMCURVE(if (Mask & 1) for (; j < end(*pDataVector) && j->Time < BTime; ++j), (!(Mask & 2) || j->Time <= ETime), TRUE,
		(!(Mask & 1) || BTime <= j->Time) && (!(Mask & 2) || j->Time <= ETime));
}

long CST_CurveCtrl::TrimCurve(long Address, short State, long nBegin, long nCount, short nStep, BOOL bAll)
{
	ASSERT(IsMainDataStateValidate(State) && nBegin >= 0 && (-1 == nCount || nCount > 0) && nStep > 0);
	if (!IsMainDataStateValidate(State) || nBegin < 0 || -1 != nCount && nCount <= 0 || nStep <= 0)
		return 0;

	TRIMCURVE(j += nBegin, //CON1取得开始修整位置
		(-1 == nCount || k <= nCount), FALSE, FALSE);
}

BOOL CST_CurveCtrl::GetSelectedCurve(long* pAddress)
{
	if (-1 != CurCurveIndex)
	{
		if (!IsBadWritePtr(pAddress, sizeof(long)))
			*pAddress = MainDataListArr[CurCurveIndex];

		return TRUE;
	}

	return FALSE;
}

vector<MainData>::iterator CST_CurveCtrl::GetFirstVisiblePos(long Address)
{
	auto DataListIter = FindMainData(Address);
	return NullDataListIter != DataListIter ? GetFirstVisiblePos(DataListIter, TRUE, FALSE) : NullDataIter;
}

//位置表达方式为：第1位，曲线在画布左边，第2位，曲线在画布右边，第3位，曲线在画布上边，第4位，曲线在画布下边
vector<MainData>::iterator CST_CurveCtrl::GetFirstVisiblePos(vector<DataListHead<MainData>>::iterator DataListIter,
															 BOOL bPart, BOOL bXOnly, UINT* pPosition /*= nullptr*/, vector<MainData>::iterator DataIter /*= NullDataIter*/)
{
	ASSERT(NullDataListIter != DataListIter);

	auto pLTpoint = &DataListIter->LeftTopPoint.ScrPos;
	auto pRBpoint = &DataListIter->RightBottomPoint.ScrPos;
	//判断整条DataListIter曲线与画布的相交性，不相交直接返回失败
	UINT Position = 0;
	if (pRBpoint->x < CanvasRect[1].left + DataListIter->Zx)
		Position |= 1; //画布左侧
	else if (pLTpoint->x > CanvasRect[1].right)
		Position |= 2; //画布右侧
	if (!Position && !bXOnly) //如果已经在画布的左侧或者右则，不用再继续
		if (pRBpoint->y < CanvasRect[1].top)
			Position |= 4; //画布上侧
		else if (pLTpoint->y > CanvasRect[1].bottom - DataListIter->Zy)
			Position |= 8; //画布下侧
	if (pPosition)
		*pPosition = Position;

	if (!Position)
	{
		auto pDataVector = DataListIter->pDataVector;
		auto i = NullDataIter != DataIter ? DataIter : begin(*pDataVector);
		for (; i < end(*pDataVector); ++i)
			if (2 == i->State && i > begin(*pDataVector) && i < prev(end(*pDataVector))) //无论如何，首尾两个点必须要绘制
				continue;
			else if (1 == DataListIter->Power && i->ScrPos.x > CanvasRect[1].right) //已经不可能再找到了，这是一步很重要的优化
				break;
			else if (IsPointVisible(DataListIter, i, bPart, bXOnly, 1)) //只向后检测
				return i;
	}

	return NullDataIter;
}

BOOL CST_CurveCtrl::IsSelected(long Address) {return -1 != CurCurveIndex && MainDataListArr[CurCurveIndex] == Address;}
BOOL CST_CurveCtrl::IsLegendVisible(LPCTSTR pSign)
{
	auto LegendIter = FindLegend(pSign);
	return NullLegendIter != LegendIter && LegendIter->State;
}
BOOL CST_CurveCtrl::IsCurveVisible(long Address)
{
	auto DataListIter = FindMainData(Address);
	return NullDataListIter != DataListIter && ISCURVESHOWN(DataListIter);
}
BOOL CST_CurveCtrl::IsCurveInCanvas(long Address) {return NullDataIter != GetFirstVisiblePos(Address);} //部分可见即可

BOOL CST_CurveCtrl::VCenterCurve(long Address, BOOL bUpdate)
{
	auto DataListIter = FindMainData(Address);
	if (NullDataListIter != DataListIter)
	{
		if (!ISCURVESHOWN(DataListIter)) //将隐藏的曲线显示出来
			ShowCurve(Address, TRUE);

		return ReSetCurvePosition(5, bUpdate, DataListIter);
	}

	return FALSE;
}

long CST_CurveCtrl::GetScaleNums() {return (HCoorData.nScales << 16) + VCoorData.nScales;}

void CST_CurveCtrl::GetViableTimeRange(HCOOR_TYPE* pMinTime, HCOOR_TYPE* pMaxTime) {DOFILL2VALUE(pMinTime, pMaxTime, MINTIME, MAXTIME);}

BOOL CST_CurveCtrl::GotoCurve(long Address)
{
	auto DataListIter = FindMainData(Address);
	if (NullDataListIter != DataListIter)
	{
		if (!ISCURVESHOWN(DataListIter)) //将隐藏的曲线显示出来
			ShowCurve(Address, TRUE);

		SetBeginTime2(GetNearFrontPos(DataListIter->LeftTopPoint.Time, OriginPoint.Time));
		return TRUE;
	}

	return FALSE;
}

HCOOR_TYPE CST_CurveCtrl::GetTimeData(short nCurveIndex, long nIndex)
{
	if (0 <= nCurveIndex && (size_t) nCurveIndex < MainDataListArr.size())
	{
		auto pDataVector = MainDataListArr[nCurveIndex].pDataVector;

		if (0 <= nIndex && (size_t) nIndex < pDataVector->size())
			return (*pDataVector)[nIndex].Time;
	}

	return MAXTIME;
}

BSTR CST_CurveCtrl::GetTimeData2(short nCurveIndex, long nIndex)
{
	BSTR bstr = nullptr;
	auto Time = GetTimeData(nCurveIndex, nIndex);

	if (ISHVALUEVALID(Time))
	{
		auto hr = VarBstrFromDate(Time, LANG_USER_DEFAULT, 0, &bstr);
		ASSERT(SUCCEEDED(hr));
	}

	return bstr;
}

float CST_CurveCtrl::GetValueData(short nCurveIndex, long nIndex)
{
	if (0 <= nCurveIndex && (size_t) nCurveIndex < MainDataListArr.size())
	{
		auto pDataVector = MainDataListArr[nCurveIndex].pDataVector;

		if (0 <= nIndex && (size_t) nIndex < pDataVector->size())
			return (*pDataVector)[nIndex].Value;
	}

	return .0f;
}

short CST_CurveCtrl::GetState(short nCurveIndex, long nIndex)
{
	if (0 <= nCurveIndex && (size_t) nCurveIndex < MainDataListArr.size())
	{
		auto pDataVector = MainDataListArr[nCurveIndex].pDataVector;

		if (0 <= nIndex && (size_t) nIndex < pDataVector->size())
			return (*pDataVector)[nIndex].AllState;
	}

	return 0;
}

BOOL CST_CurveCtrl::GetPosData(short nCurveIndex, long nIndex, long* px, long* py)
{
	if (0 <= nCurveIndex && (size_t) nCurveIndex < MainDataListArr.size())
	{
		auto pDataVector = MainDataListArr[nCurveIndex].pDataVector;

		if (0 <= nIndex && (size_t) nIndex < pDataVector->size())
		{
			DOFILL2VALUE(px, py, (*pDataVector)[nIndex].ScrPos.x, (*pDataVector)[nIndex].ScrPos.y);
			return TRUE;
		}
	}

	return FALSE;
}

BOOL CST_CurveCtrl::InsertMainData(short nCurveIndex, long nIndex, LPCTSTR pTime, float Value, short State, short Position, short Mask)
{
	ASSERT(pTime);
	if (IsBadStringPtr(pTime, -1)) //空指针也能正确判断
		return FALSE;

	if (m_ShowMode & 0x80)
	{
		LPTSTR pEnd = nullptr;
		auto Time = _tcstod(pTime, &pEnd);
		if (HUGE_VAL == Time || -HUGE_VAL == Time)
			return FALSE;
		if (nullptr == pEnd || pTime == pEnd) //根本无法解析
			return FALSE;

		return InsertMainData2(nCurveIndex, nIndex, Time, Value, State, Position, Mask);
	}
	else
	{
		HCOOR_TYPE Time;
		USES_CONVERSION;
		auto hr = VarDateFromStr((LPOLESTR) T2COLE(pTime), LANG_USER_DEFAULT, 0, &Time);
		return SUCCEEDED(hr) && InsertMainData2(nCurveIndex, nIndex, Time, Value, State, Position, Mask);
	}
}

//Position：-1－添加到nIndex前面； 0－更改nIndex点； 1－添加到nIndex后面；
//如果Position等于0，则Mask有意义，按位算，从低位起，1－Time有效，2－Value有效，3－State有效
BOOL CST_CurveCtrl::InsertMainData2(short nCurveIndex, long nIndex, HCOOR_TYPE Time, float Value, short State, short Position, short Mask)
{
	if (0 > nCurveIndex || (size_t) nCurveIndex >= MainDataListArr.size() || -1 > Position || Position > 1 || ISHVALUEINVALID(Time))
		return FALSE;

	auto DataListIter = next(begin(MainDataListArr), nCurveIndex);
	auto pDataVector = DataListIter->pDataVector;

	if (0 > nIndex && (size_t) nIndex >= pDataVector->size())
		return FALSE;

	auto InsertPos = next(begin(*pDataVector), nIndex);

	if (-1 == Position || 1 == Position)
	{
		if (!IsMainDataStateValidate(State))
			return FALSE;

		MainData NoUse;
		NoUse.Time = Time;
		NoUse.Value = Value;
		NoUse.AllState = State;
		CalcOriginDatumPoint(NoUse, 3, 0, 0, DataListIter);

		if (-1 == Position) //添加到前面
			pDataVector->insert(InsertPos, NoUse);
		else // if (1 == Position) //添加到后面
		{
			++InsertPos; //当insert的第一个参数等于向量的end()时，insert执行仍然会成功，因为它是在第一个参数指定的位置的前面添加
			pDataVector->insert(InsertPos, NoUse);
		}
	}
	else if (Mask & 7) //更改当前点
	{
		if (Mask & 4)
		{
			if (!IsMainDataStateValidate(State))
				return FALSE;

			InsertPos->AllState = State;
		}

		if (Mask & 3) //坐标有更新
		{
			if (Mask & 2)
				InsertPos->Value = Value;
			if (Mask & 1)
				InsertPos->Time = Time;

			CalcOriginDatumPoint(*InsertPos, Mask & 3, 0, 0, DataListIter);
		}
	}
	else
		return FALSE; //什么也没修改

	InvalidCurveSet.insert(DataListIter->Address); //插入或者更改点后，曲线的次数可能会从1到2，也可能反之，这不同于删除点，删除点时只可能从2次变为1次
	SysState |= 0x200; //由于没有bUpdate参数，所以需要Refresh函数才能让操作生效
	return TRUE;
}

BOOL CST_CurveCtrl::DelPoint(short nCurveIndex, long nIndex)
{
	if (0 <= nCurveIndex && (size_t) nCurveIndex < MainDataListArr.size())
	{
		auto DataListIter = next(begin(MainDataListArr), nCurveIndex);
		auto pDataVector = DataListIter->pDataVector;

		if (0 <= nIndex && (size_t) nIndex < pDataVector->size())
		{
			auto pos = next(begin(*pDataVector), nIndex);
			DoDelMainData(DataListIter, pos, pos + 1, FALSE);
			return TRUE;
		}
	}

	return FALSE;
}

BOOL CST_CurveCtrl::CanContinueEnum(long Address, short nCurveIndex, long nIndex)
{
	if (nIndex >= 0)
		if (0 <= nCurveIndex && (size_t) nCurveIndex < MainDataListArr.size())
		{
			auto DataListIter = next(begin(MainDataListArr), nCurveIndex);
			if (Address == DataListIter->Address)
			{
				auto pDataVector = DataListIter->pDataVector;
				return (size_t) nIndex < pDataVector->size();
			}
		}

	return FALSE;
}

short CST_CurveCtrl::GetCurveCount() {return (short) MainDataListArr.size();}
short CST_CurveCtrl::GetCurveIndex(long Address)
{
	auto MainDataIter = find(begin(MainDataListArr), end(MainDataListArr), Address);
	return MainDataIter < end(MainDataListArr) ? (short) distance(begin(MainDataListArr), MainDataIter) : -1;
}

BOOL CST_CurveCtrl::SetCurveIndex(long Address, short nIndex)
{
	if (0 > nIndex || (size_t) nIndex >= MainDataListArr.size())
		return FALSE;

	auto i = find(begin(MainDataListArr), end(MainDataListArr), Address);
	if (i < end(MainDataListArr))
	{
		auto j = next(begin(MainDataListArr), nIndex);
		if (i != j)
		{
			swap(*i, *j); //交换两个DataListHead的内容，而不是指针
			if (CurCurveIndex == distance(begin(MainDataListArr), i))
				CurCurveIndex += j - i;
			else if (CurCurveIndex == distance(begin(MainDataListArr), j))
				CurCurveIndex -= j - i;
			UpdateRect(hFrceDC, CanvasRectMask);
		}

		return TRUE;
	}

	return FALSE;
}

long CST_CurveCtrl::GetCurve(short nIndex) 
{
	ASSERT(0 <= nIndex && (size_t) nIndex < MainDataListArr.size());
	if (0 <= nIndex && (size_t) nIndex < MainDataListArr.size())
		return MainDataListArr[nIndex];

	return -1;
}

short CST_CurveCtrl::GetLegendCount() {return (short) LegendArr.size();}
BOOL CST_CurveCtrl::GetLegend2(short nIndex, OLE_COLOR* pPenColor, short* pPenStyle, short* pLineWidth, OLE_COLOR* pBrushColor, short* pBrushStyle, short* pCurveMode, short* pNodeMode)
{
	ASSERT(0 <= nIndex && (size_t) nIndex < LegendArr.size());
	if (0 <= nIndex && (size_t) nIndex < LegendArr.size())
	{
		auto LegendIter = next(begin(LegendArr), nIndex);
		DOFILL4VALUE(pPenColor, pPenStyle, pLineWidth, pBrushColor,
			LegendIter->PenColor, LegendIter->PenStyle, LegendIter->LineWidth, LegendIter->BrushColor);
		DOFILL3VALUE(pBrushStyle, pCurveMode, pNodeMode,
			LegendIter->BrushStyle, LegendIter->CurveMode, LegendIter->NodeMode);

		return TRUE;
	}

	return FALSE;
}

short CST_CurveCtrl::GetLegendIdCount(short nIndex) 
{
	ASSERT(0 <= nIndex && (size_t) nIndex < LegendArr.size());
	if (0 <= nIndex && (size_t) nIndex < LegendArr.size())
		return (short) LegendArr[nIndex].Addrs.size();

	return -1;
}

long CST_CurveCtrl::GetLegendId(short nLegendIndex, short nAddressIndex) 
{
	ASSERT(0 <= nLegendIndex && (size_t) nLegendIndex < LegendArr.size());
	if (0 <= nLegendIndex && (size_t) nLegendIndex < LegendArr.size())
	{
		auto LegendIter = next(begin(LegendArr), nLegendIndex);
		ASSERT(0 <= nAddressIndex && (size_t) nAddressIndex < LegendIter->Addrs.size());
		if (0 <= nAddressIndex && (size_t) nAddressIndex < LegendIter->Addrs.size())
			return LegendIter->Addrs[nAddressIndex];
	}

	return -1;
}

/*
hBuddy==0
如果控件为联动服务器，则取消联动服务器，并关闭与所有相关的联动客户机的连接
如果控件为联动客户机，则取消与联动服务器的连接

hBuddy!=0
如果State==0，则hBuddy看成联动客户机，并将其添加到当前控件
同时，本控件（调用SetBuddy方法的控件）将变成服务器
如果State==1，则hBuddy看成联动客户机，并将其从当前控件的联动客户机向量里删除

注：删除联动客户机有两种方法，一种是对联动服务器调用SetBuddy（此时hBuddy为联动客户机，State==1）；
一种是对联动客户机调用SetBuddy（此时hBuddy==0，State忽略）

删除联动服务器只能是对联动服务器调用SetBuddy（此时hBuddy==0，State忽略）这一种方法
*/
BOOL CST_CurveCtrl::SetBuddy(OLE_HANDLE hBuddy, short State) //设置联动服务器，hBuddy为联动客户机
{
	if (!hBuddy) //取消联动服务器
	{
		CANCELBUDDYS;
		return TRUE;
	}

	auto re = TRUE;
	auto hThisBuddy = Format64bitHandle(HWND, hBuddy);
	switch (State)
	{
	case 0:
		if (hBuddyServer) //已经是联动客户机，则取消，因为这次操作，将把本控件变成联动服务器
		{
			::SendMessage(hBuddyServer, BUDDYMSG, 1, (LPARAM) m_hWnd);
			hBuddyServer = 0;
		}

		if (!pBuddys)
			pBuddys = new vector<HWND>;
		else if (find(begin(*pBuddys), end(*pBuddys), hThisBuddy) < end(*pBuddys)) //重复添加
			return TRUE;

		re = (BOOL) ::SendMessage(hThisBuddy, BUDDYMSG, 0, (LPARAM) m_hWnd); //通知联动客户机，把自己的窗口句柄发给它
		if (re)
		{
			pBuddys->push_back(hThisBuddy);
			::SendMessage(hThisBuddy, BUDDYMSG, 2, (LPARAM) &OriginPoint.Time); //同步时间
			::SendMessage(hThisBuddy, BUDDYMSG, 3, (LPARAM) &HCoorData.fStep); //同步时间间隔
			::SendMessage(hThisBuddy, BUDDYMSG, 4, (LPARAM) Zoom); //同步放大率
			::SendMessage(hThisBuddy, BUDDYMSG, 8, (LPARAM) HZoom); //同步水平放大率
			auto ThisLeftSpace = (short) ::SendMessage(hThisBuddy, BUDDYMSG, 6, 0); //询问新加入的联动客户机的LeftSpace
			if (LeftSpace < ThisLeftSpace) //新加入的联动客户机的LeftSpace最大
			{
				BROADCASTLEFTSPACE(ThisLeftSpace);
				CHLeftSpace(ThisLeftSpace);
			}
			else if (LeftSpace > ThisLeftSpace) //新加入的联动客户机的LeftSpace非最大
				::SendMessage(hThisBuddy, BUDDYMSG, 5, (LPARAM) LeftSpace);
		}
		break;
	case 1:
		REMOVEBUDDY(hThisBuddy);
		break;
	default:
		re = FALSE;
		break;
	}

	return re;
}

//返回：-1－－非联动服务器也非联动客户机；0－－联动客户机；>0－－联动服务器
short CST_CurveCtrl::GetBuddyCount() 
{
	short re = -1;
	if (hBuddyServer)
		re = 0;
	else if (pBuddys)
		re = (short) pBuddys->size();

	return re;
}

OLE_HANDLE CST_CurveCtrl::GetBuddy(short nIndex) 
{
	if (pBuddys && (size_t) nIndex < pBuddys->size())
		return SplitHandle((*pBuddys)[nIndex]);

	return NULL;
}

void CST_CurveCtrl::EnableZoom(BOOL bEnable)
{
	bEnable <<= 10;
	if ((SysState ^ bEnable) & 0x400) //第11位上有变化
	{
		SysState &= ~0x400;
		SysState |= bEnable;
	}
}

void CST_CurveCtrl::EnableHZoom(BOOL bEnable)
{
	bEnable <<= 4;
	if ((SysState ^ bEnable) & 0x10) //第5位上有变化
	{
		SysState &= ~0x10;
		SysState |= bEnable;
	}
}

void CST_CurveCtrl::SetCurveTitle(LPCTSTR pCurveTitle) 
{
	if (!IsBadStringPtr(pCurveTitle, -1))
	{
		_tcsncpy_s(CurveTitle, pCurveTitle, _TRUNCATE);
		UpdateRect(hFrceDC, TitleRectMask); //CurveTitleRect包括UnitRect和LegendMarkRect
	}
}

BSTR CST_CurveCtrl::GetCurveTitle() 
{
	CComBSTR strResult = CurveTitle;
	return strResult.Copy();
}

void CST_CurveCtrl::SetFootNote(LPCTSTR pFootNote) 
{
	if (!IsBadStringPtr(pFootNote, -1))
	{
		_tcsncpy_s(FootNote, pFootNote, _TRUNCATE);
		UpdateRect(hFrceDC, FootNoteRectMask);
	}
}

BSTR CST_CurveCtrl::GetFootNote() 
{
	CComBSTR strResult = FootNote;
	return strResult.Copy();
}

BOOL CST_CurveCtrl::SetGridMode(short GridMode)
{
	if (0 <= GridMode && GridMode <= 0xF)
	{
		if (GridMode != ((SysState >> 19) & 3) + ((SysState >> 21) & 4) + ((SysState & 4) << 1)) //从第24位置移动到第3位置
		{
			SysState &= ~0x980004; //第3 20 21 24位
			SysState |= (UINT) (GridMode & 3) << 19;
			SysState |= (UINT) (GridMode & 4) << 21; //从第3位置移动到第24位置
			SysState |= (UINT) (GridMode & 8) >> 1; //从第4位置移动到第3位置
			DrawBkg();
			UpdateRect(hFrceDC, CanvasRectMask);
		}

		return TRUE;
	}
	else
		return FALSE;
}
short CST_CurveCtrl::GetGridMode() {return ((SysState >> 19) & 3) + ((SysState >> 21) & 4) + ((SysState << 1) & 8);}

void CST_CurveCtrl::EnableAdjustZOrder(BOOL bEnable)
{
	SysState &= ~0x2000;
	if (bEnable)
	{
		bEnable <<= 13; //第14位上有变化
		SysState |= bEnable;
	}
}

void CST_CurveCtrl::EnableAutoTrimCoor(BOOL bEnable)
{
	SysState &= ~0x20000;
	if (bEnable)
	{
		TrimCoor();
		bEnable <<= 17; //第18位上有变化
		SysState |= bEnable;
	}
}

//接口里面的BOOL类型，不管是传入控件还是传出控件，都会被处理，只留下0和1
void CST_CurveCtrl::EnableHelpTip(BOOL bEnable)
{
	bEnable <<= 18;
	if ((SysState ^ bEnable) & 0x40000) //第19位上有变化
	{
		SysState &= ~0x40000;
		if (bEnable)
			SysState |= bEnable;

		DrawBkg();
		UpdateRect(hFrceDC, AllRectMask);
	}
	KillTimer(HIDEHELPTIP);
}

void CST_CurveCtrl::SetBenchmark(HCOOR_TYPE Time, float Value) 
{
	SysState |= 0x1000;

	UINT Mask = 0;
	if (Time != BenchmarkData.Time)
	{
		BenchmarkData.Time = Time;
		Mask |= 1;
	}
	if (Value != BenchmarkData.Value)
	{
		BenchmarkData.Value = Value;
		Mask |= 2;
	}

	if (Mask)
		CalcOriginDatumPoint(OriginPoint, 0x10 | Mask); //不需要重新计算所有画布坐标
}
void CST_CurveCtrl::GetBenchmark(HCOOR_TYPE* pTime, float* pValue) {DOFILL2VALUE(pTime, pValue, BenchmarkData.Time, BenchmarkData.Value);}

void CST_CurveCtrl::SetVisibleCoorRange(HCOOR_TYPE MinTime, HCOOR_TYPE MaxTime, float MinValue, float MaxValue, short Mask)
{
	if (!Mask)
		return;

	if (Mask & 1)
	{
		HCoorData.fMinVisibleValue = MinTime;
		HCoorData.RangeMask |= 1;
	}
	if (Mask & 2)
	{
		HCoorData.fMaxVisibleValue = MaxTime;
		HCoorData.RangeMask |= 2;
	}

	if (Mask & 4)
	{
		VCoorData.fMinVisibleValue = MinValue;
		VCoorData.RangeMask |= 1;
	}
	if (Mask & 8)
	{
		VCoorData.fMaxVisibleValue = MaxValue;
		VCoorData.RangeMask |= 2;
	}

	if (Mask & 0x100)
		HCoorData.RangeMask &= ~1;
	if (Mask & 0x200)
		HCoorData.RangeMask &= ~2;

	if (Mask & 0x400)
		VCoorData.RangeMask &= ~1;
	if (Mask & 0x800)
		VCoorData.RangeMask &= ~2;

	UINT UpdateMask = 0;
	if (Mask & 0x303)
		UpdateMask |= HLabelRectMask;
	if (Mask & 0xC0C)
		UpdateMask |= VLabelRectMask;

	if (Mask)
		UpdateRect(hFrceDC, UpdateMask);
}

void CST_CurveCtrl::GetVisibleCoorRange(HCOOR_TYPE* pMinTime, HCOOR_TYPE* pMaxTime, float* pMinValue, float* pMaxValue)
{
	DOFILL4VALUE(pMinTime, pMaxTime, pMinValue, pMaxValue,
		HCoorData.fMinVisibleValue, HCoorData.fMaxVisibleValue, VCoorData.fMinVisibleValue, VCoorData.fMaxVisibleValue);
}

short CST_CurveCtrl::GetPower(long Address) 
{
	auto DataListIter = FindMainData(Address);
	return NullDataListIter != DataListIter ? DataListIter->Power : -1;
}

BOOL CST_CurveCtrl::ChangeId(long Address, long NewAddr)
{
	auto DataListIter = FindMainData(Address);
	if (NullDataListIter != DataListIter && !IsCurve(NewAddr))
	{
		DataListIter->Address = NewAddr;
		DataListIter->LegendIter = FindLegend(NewAddr, TRUE);
		UpdateRect(hFrceDC, CanvasRectMask);

		return TRUE;
	}

	return FALSE;
}

BOOL CST_CurveCtrl::CloneCurve(long Address, long NewAddr)
{
	auto DataListIter = FindMainData(Address);
	if (NullDataListIter != DataListIter)
	{
		auto NewDataListIter = FindMainData(NewAddr);
		if (NullDataListIter == NewDataListIter)
		{
			DataListHead<MainData> NoUse;
			NoUse.Address = NewAddr;
			NoUse.LeftTopPoint = DataListIter->LeftTopPoint;
			NoUse.RightBottomPoint = DataListIter->RightBottomPoint;
			NoUse.LegendIter = FindLegend(NewAddr, TRUE); //查找图例赋给LegendIter;
			NoUse.FillDirection = DataListIter->FillDirection;
			NoUse.Power = DataListIter->Power;
			NoUse.Zx = DataListIter->Zy;
			NoUse.Zy = DataListIter->Zy;

			if (ISCURVESHOWN((&NoUse)))
				++nVisibleCurve;

			auto SrcIndex = distance(begin(MainDataListArr), DataListIter); //考虑到MainDataListArr可能重新分配内存，先记下序号
			MainDataListArr.push_back(NoUse);
			//恢复DataListIter（其实只取其pDataVector成员），这种办法保证万无一失
			auto pDataVector = next(begin(MainDataListArr), SrcIndex)->pDataVector;
			NewDataListIter = prev(end(MainDataListArr));
			NewDataListIter->pDataVector->assign(begin(*pDataVector), end(*pDataVector)); //拷贝数据，vector将自动预先分配end - begin个空间

			UpdateRect(hFrceDC, CanvasRectMask);

			return TRUE;
		}
	}

	return FALSE;
}

//真正的求合集
void CST_CurveCtrl::DoUniteCurve(vector<DataListHead<MainData>>::iterator DesDataListIter, vector<MainData>::iterator InsertIter,
								 vector<DataListHead<MainData>>::iterator DataListIter, long nBegin, long nCount)
{
	UNITECURVE(if ((size_t) nBegin >= pDataVector->size()) i = end(*pDataVector); else i += nBegin, //CON1
		auto n = 0, (-1 == nCount || ++n <= nCount), //CON2 C1
		-1 != nCount && (size_t) (nBegin + nCount) < pDataVector->size(), i += nCount, //C2 CON3
		FALSE, FALSE); //C3(无意义)
}

void CST_CurveCtrl::DoUniteCurve(vector<DataListHead<MainData>>::iterator DesDataListIter, vector<MainData>::iterator InsertIter,
								 vector<DataListHead<MainData>>::iterator DataListIter, HCOOR_TYPE BTime, HCOOR_TYPE ETime, short Mask)
{
	UNITECURVE(if (Mask & 1) for (; i < end(*pDataVector) && i->Time < BTime; ++i), 0, (!(Mask & 2) || i->Time <= ETime), //CON1 CON2(无意义) C1
		Mask & 2, for (; i < end(*pDataVector) && i->Time <= ETime; ++i), //C2 CON3
		TRUE, (!(Mask & 1) || BTime <= i->Time) && (!(Mask & 2) || i->Time <= ETime)); //C3
}

//求合集，如果nInsertPos等于-1，内部实现是取出Address中符合条件的点，依次以bAddTrail为假调用AddMainData2函数。
//如果nInsertPos大于等于0，直接将Address曲线中所有符合条件的点全部插入到DesAddr的nInsertPos前面的位置，
//添加次序按点在Address中次序为准。范围是Address曲线的范围。
BOOL CST_CurveCtrl::UniteCurve(long DesAddr, long nInsertPos, long Address, long nBegin, long nCount)
{
	ASSERT(nInsertPos >= -1 && nBegin >= 0 && (-1 == nCount || nCount > 0));

	if (nInsertPos < -1 || nBegin < 0 || -1 != nCount && nCount <= 0)
		return FALSE;

	PREPAREUNITECURVE(nInsertPos > -1,
		InsertIter = (size_t) nInsertPos >= pDesDataVector->size() ? end(*pDesDataVector) : next(begin(*pDesDataVector), nInsertPos),
		DoUniteCurve(DesDataListIter, InsertIter, DataListIter, nBegin, nCount));
}

BOOL CST_CurveCtrl::UniteCurve2(long DesAddr, long nInsertPos, long Address, HCOOR_TYPE BTime, HCOOR_TYPE ETime, short Mask)
{
	ASSERT(nInsertPos >= -1);

	if (nInsertPos < -1)
		return FALSE;

	PREPAREUNITECURVE(nInsertPos > -1,
		InsertIter = (size_t) nInsertPos >= pDesDataVector->size() ? end(*pDesDataVector) : next(begin(*pDesDataVector), nInsertPos),
		DoUniteCurve(DesDataListIter, InsertIter, DataListIter, BTime, ETime, Mask));
}

BOOL CST_CurveCtrl::UniteCurve3(long DesAddr, HCOOR_TYPE fInsertPos, long Address, long nBegin, long nCount)
{
	ASSERT(nBegin >= 0 && (-1 == nCount || nCount > 0));

	if (nBegin < 0 || -1 != nCount && nCount <= 0)
		return FALSE;

	PREPAREUNITECURVE(TRUE,
		for (InsertIter = begin(*pDesDataVector); InsertIter < end(*pDesDataVector) && InsertIter->Time < fInsertPos; ++InsertIter),
		DoUniteCurve(DesDataListIter, InsertIter, DataListIter, nBegin, nCount));
}

BOOL CST_CurveCtrl::UniteCurve4(long DesAddr, HCOOR_TYPE fInsertPos, long Address, HCOOR_TYPE BTime, HCOOR_TYPE ETime, short Mask)
{
	PREPAREUNITECURVE(TRUE,
		for (InsertIter = begin(*pDesDataVector); InsertIter < end(*pDesDataVector) && InsertIter->Time < fInsertPos; ++InsertIter),
		DoUniteCurve(DesDataListIter, InsertIter, DataListIter, BTime, ETime, Mask));
}

//Operator的高字节代表TimeSpan操作类型，低字节代表ValueStep操作类型，他们的具体意义一样：
//'+'－加，'*'－乘，如果横坐标显示为时间，则操作类型只能是加，因为乘无意义
BOOL CST_CurveCtrl::OffSetCurve(long Address, double TimeSpan, float ValueStep, short Operator)
{
	auto DataListIter = FindMainData(Address);
	if (NullDataListIter != DataListIter)
	{
		auto pDataVector = DataListIter->pDataVector;

		UINT Operator1 = Operator >> 8, Operator2 = Operator & 0xFF;
		if ('+' != Operator1 && '*' != Operator1)
			Operator1 = 0;
		if ('+' != Operator2 && '*' != Operator2)
			Operator2 = 0;
		UINT ChMask = 0;
		int XOff, YOff;
		if (Operator1 && (m_ShowMode & 8 || '+' == Operator1) && .0 != TimeSpan)
		{
			XOff = DataListIter->LeftTopPoint.ScrPos.x;
			OFFSETVALUE(Operator1, DataListIter->LeftTopPoint.Time, TimeSpan); //更新曲线范围
			ChMask |= 1;
		}
		if (Operator2 && .0f != ValueStep)
		{
			YOff = DataListIter->LeftTopPoint.ScrPos.y;
			OFFSETVALUE(Operator2, DataListIter->LeftTopPoint.Value, ValueStep); //更新曲线范围
			ChMask |= 2;
		}
		if (!ChMask)
			return TRUE;

		CalcOriginDatumPoint(DataListIter->LeftTopPoint, ChMask, 0, 0, DataListIter);
		if (ChMask & 1)
			XOff -= DataListIter->LeftTopPoint.ScrPos.x;
		if (ChMask & 2)
			YOff -= DataListIter->LeftTopPoint.ScrPos.y; //计算偏移量

		OFFSETCURVE(DataListIter->RightBottomPoint); //更新曲线范围，并且应用偏移

		for (auto DataIter = begin(*pDataVector); DataIter < end(*pDataVector); ++DataIter)
		{
			OFFSETCURVE(*DataIter);
		}

		UpdateTotalRange();
		if (!(ReSetCurvePosition(0, TRUE) & 6))
			UpdateRect(hFrceDC, CanvasRectMask);

		return TRUE;
	}

	return FALSE;
}

long CST_CurveCtrl::ArithmeticOperate(long DesAddr, long Address, short Operator)
{
	if ('+' != Operator && '-' != Operator && '*' != Operator && '/' != Operator)
		return 0;

	long re = 0;
	auto DesDataListIter = FindMainData(DesAddr);
	if (NullDataListIter != DesDataListIter)
	{
		auto DataListIter = FindMainData(Address);
		if (NullDataListIter != DataListIter &&
			DesDataListIter->RightBottomPoint.Time >= DataListIter->LeftTopPoint.Time && DataListIter->RightBottomPoint.Time >= DesDataListIter->LeftTopPoint.Time)
		{
			auto pDesDataVector = DesDataListIter->pDataVector, pDataVector = DataListIter->pDataVector;
			auto i = begin(*pDesDataVector), j = begin(*pDataVector);
			while (i < end(*pDesDataVector) && j < end(*pDataVector))
				if (i->Time < j->Time)
					++i;
				else if (i->Time > j->Time)
					++j;
				else
				{
					switch (Operator)
					{
					case '+':
						i->Value += j->Value;
						break;
					case '-':
						i->Value -= j->Value;
						break;
					case '*':
						i->Value *= j->Value;
						break;
					default: //'/'
						i->Value /= j->Value;
						break;
					}
					CalcOriginDatumPoint(*i, 2, 0, 0, DesDataListIter);

					++re;
					++i, ++j;
				}

			if (re)
			{
				UpdateOneRange(DesDataListIter); //不同于OffSetCurve，调用ArithmeticOperate后，DesAddr曲线的范围必须要重新计算（但幂次不会改变）
				UpdateTotalRange();

				if (!(ReSetCurvePosition(0, TRUE) & 6))
					UpdateRect(hFrceDC, CanvasRectMask);
			}
		}
	}

	return re;
}

void CST_CurveCtrl::ClearTempBuff() {free_container(points);}

BOOL CST_CurveCtrl::PreMallocMem(long Address, long size)
{
	auto DataListIter = FindMainData(Address);
	if (NullDataListIter != DataListIter)
	{
		auto pDataVector = DataListIter->pDataVector;
		if (size <= 0) //清理多余缓存
		{
			if (pDataVector->capacity() > pDataVector->size())
			{
				DataListIter->pDataVector = new vector<MainData>(*pDataVector); //不可对*pDataVector调用std::move以提高效率，会起不到压缩缓存的目的
				delete pDataVector;
			}

			return TRUE;
		}
		else if ((size_t) size > pDataVector->capacity()) //分配指定的缓存
		{
			pDataVector->reserve(size);
			return TRUE;
		}
	}

	return FALSE;
}

long CST_CurveCtrl::GetMemSize(long Address)
{
	auto DataListIter = FindMainData(Address);
	return NullDataListIter != DataListIter ? (long) DataListIter->pDataVector->capacity() : -1;
}

BOOL CST_CurveCtrl::GetMemInfo(long* pTempBuffSize, long* pAllBuffSize, float* pUseRate, long* pAddress)
{
	if (MainDataListArr.empty())
		return FALSE;

	size_t TotalUsed = 0; //使用量
	size_t TotalAlloc = 0; //分配量
	float MinUseRate = 1.0; //最小利用率
	long Address; //最小利用率下的曲线地址

	for (auto i = begin(MainDataListArr); i < end(MainDataListArr); ++i)
	{
		auto pDataVector = i->pDataVector;

		TotalUsed += pDataVector->size();
		TotalAlloc += pDataVector->capacity();

		if (pAddress) //优化一下，浮点数操作比较慢
		{
			auto ThisUseRate = (float) pDataVector->size() / pDataVector->capacity();
			if (ThisUseRate < MinUseRate)
			{
				MinUseRate = ThisUseRate;
				Address = i->Address;
			}
		}
	} //for

	DOFILL4VALUE(pTempBuffSize, pAllBuffSize, pUseRate, pAddress,
		(long) points.capacity(), (long) TotalAlloc, (float) TotalUsed / TotalAlloc, Address);

	return TRUE;
}

BOOL CST_CurveCtrl::IsCurve(long Address) {return NullDataListIter != FindMainData(Address);}

void CST_CurveCtrl::SetSorptionRange(short Range)
{
	if (Range <= 0) //取消吸附效应
	{
		SorptionRange = -1;
		SysState &= ~0xC000;
	}
	else //开启吸附效应
	{
		SorptionRange = Range;
		SysState |= 0x4000;
	}
}

short CST_CurveCtrl::GetSorptionRange() {return SorptionRange;}

BOOL CST_CurveCtrl::IsLegend(LPCTSTR pSign) {return NullLegendIter != FindLegend(pSign);}

short CST_CurveCtrl::AddLegendHelper(long Address, LPCTSTR pSign, OLE_COLOR PenColor, short PenStyle, short LineWidth, BOOL bUpdate)
{
	if (IsLegend(pSign))
		return AddLegend(Address, pSign, PenColor, PenStyle, LineWidth, 0, 0, 0, 0, 0xF, bUpdate);
	else
		return AddLegend(Address, pSign, PenColor, PenStyle, LineWidth, 0, 255, 0, 1, 0xFF, bUpdate);
}

BOOL CST_CurveCtrl::GetActualPoint(long x, long y, HCOOR_TYPE* pTime, float* pValue)
{
	POINT point = {x, y};
	auto ap = CalcActualPoint(point);
	DOFILL2VALUE(pTime, pValue, ap.Time, ap.Value);

	return PtInRect(CanvasRect, point);
}

size_t CST_CurveCtrl::DoGetPointFromScreenPoint(vector<DataListHead<MainData>>::iterator DataListIter, long x, long y, short MaxRange)
{
	auto i = GetFirstVisiblePos(DataListIter, FALSE, FALSE); //调用这个函数是因为当曲线所有点都在画布之外的时候，会得到优化
	if (NullDataIter == i)
		return -1;

	RECT rect;
	GetSorptionRect(rect, x, y, MaxRange);
	MOVERECT(rect, m_ShowMode);

	auto pDataVector = DataListIter->pDataVector;
	for (; i < end(*pDataVector); ++i)
		if (2 != i->State && PtInRect(&rect, i->ScrPos))
			return distance(begin(*pDataVector), i);
		else if (1 == DataListIter->Power && i->ScrPos.x >= rect.right)
			break;

	return -1;
}

long CST_CurveCtrl::GetPointFromScreenPoint(long Address, long x, long y, short MaxRange)
{
	POINT point = {x, y};
	if (!PtInRect(CanvasRect, point))
		return -1;

	auto DataListIter = FindMainData(Address);
	return NullDataListIter == DataListIter ? -1 : (long) DoGetPointFromScreenPoint(DataListIter, x, y, MaxRange);
}

BOOL CST_CurveCtrl::GetPixelPoint(HCOOR_TYPE Time, float Value, long* px, long* py)
{
	MainData NoUse;
	NoUse.Time = Time;
	NoUse.Value = Value;
	CalcOriginDatumPoint(NoUse, 3);
	DOFILL2VALUE(px, py, NoUse.ScrPos.x, NoUse.ScrPos.y);

	return PtInRect(CanvasRect, NoUse.ScrPos);
}

void CST_CurveCtrl::EnableFullScreen(BOOL bEnable)
{
	bEnable <<= 21;
	if ((SysState ^ bEnable) & 0x200000) //第22位上有变化
	{
		SysState &= ~0x200000;
		SysState |= bEnable;
		LeftSpace = 0; //迫使SetLeftSpace重新计算LeftSpace，这是必须的
		ReSetUIPosition(WinWidth, WinHeight);
	}
}

HCOOR_TYPE CST_CurveCtrl::GetEndTime() {return OriginPoint.Time + HCoorData.fCurStep * HCoorData.nScales;}
BSTR CST_CurveCtrl::GetEndTime2()
{
	BSTR bstr = nullptr;
	auto EndTime = GetEndTime();

	if (ISHVALUEVALID(EndTime))
	{
		auto hr = VarBstrFromDate(EndTime, LANG_USER_DEFAULT, 0, &bstr);
		ASSERT(SUCCEEDED(hr));
	}

	return bstr;
}

float CST_CurveCtrl::GetEndValue() {return OriginPoint.Value + VCoorData.fCurStep * VCoorData.nScales;}

void CST_CurveCtrl::SetZLength(short ZLength)
{
	if (ZLength >= 0 && nZLength != ZLength)
	{
		nZLength = ZLength;
		DrawBkg();
		UpdateRect(hFrceDC, CanvasRectMask);
	}
}
short CST_CurveCtrl::GetZLength() {return nZLength;}

BOOL CST_CurveCtrl::SetCanvasBkBitmap(short nIndex)
{
	if (-1 == nIndex || 0 <= nIndex && (size_t) nIndex < BitBmps.size())
	{
		if (nCanvasBkBitmap != nIndex)
		{
			nCanvasBkBitmap = nIndex;
			DrawBkg();
			UpdateRect(hFrceDC, CanvasRectMask);
		}

		return TRUE;
	}

	return FALSE;
}
short CST_CurveCtrl::GetCanvasBkBitmap() {return nCanvasBkBitmap;}

BOOL CST_CurveCtrl::SetCanvasBkMode(short CanvasBkMode)
{
	if (-1 < CanvasBkMode && CanvasBkMode < 3)
	{
		if (m_CanvasBkMode != (BYTE) CanvasBkMode)
		{
			m_CanvasBkMode = (BYTE) CanvasBkMode;
			DrawBkg();
			UpdateRect(hFrceDC, CanvasRectMask);
		}

		return TRUE;
	}
	else
		return FALSE;
}
short CST_CurveCtrl::GetCanvasBkMode() {return m_CanvasBkMode;}

void CST_CurveCtrl::SetLeftBkColor(OLE_COLOR Color)
{
	if (Color & 0x40000000)
		LeftBkColor = Color & 0x40FFFFFF;
	else
	{
		CHECKCOLOR(Color);
		if (LeftBkColor != Color)
		{
			LeftBkColor = Color;
			DrawBkg();
			UpdateRect(hFrceDC, CanvasRectMask);
		}
	}
}
OLE_COLOR CST_CurveCtrl::GetLeftBkColor() {return LeftBkColor;}

void CST_CurveCtrl::SetBottomBkColor(OLE_COLOR Color)
{
	if (Color & 0x40000000)
		BottomBkColor = Color & 0x40FFFFFF;
	else
	{
		CHECKCOLOR(Color);
		if (BottomBkColor != Color)
		{
			BottomBkColor = Color;
			DrawBkg();
			UpdateRect(hFrceDC, CanvasRectMask);
		}
	}
}
OLE_COLOR CST_CurveCtrl::GetBottomBkColor() {return BottomBkColor;}

BOOL CST_CurveCtrl::SetZOffset(long Address, short nOffset, BOOL bUpdate)
{
	if (nOffset >= 0)
	{
		auto DataListIter = FindMainData(Address);
		if (NullDataListIter != DataListIter)
		{
			auto OldZx = DataListIter->Zx, OldZy = DataListIter->Zy;

			auto x = (double) ((int) nOffset * nOffset) / (HSTEP * HSTEP + VSTEP * VSTEP);
			x = sqrt(x);
			DataListIter->Zx = (short) (HSTEP * x);
			DataListIter->Zy = (short) (VSTEP * x);

			if (OldZx != DataListIter->Zx || OldZy != DataListIter->Zy)
			{
				OldZx -= DataListIter->Zx;
				OldZy -= DataListIter->Zy;
				auto pDataVector = DataListIter->pDataVector;
				for (auto i = begin(*pDataVector); i < end(*pDataVector); ++i) //偏移曲线
				{
					if (OldZx)
						i->ScrPos.x -= OldZx;
					if (OldZy)
						i->ScrPos.y += OldZy;
				}

				if (OldZx)
				{
					DataListIter->LeftTopPoint.ScrPos.x -= OldZx;
					DataListIter->RightBottomPoint.ScrPos.x -= OldZx;
				}
				if (OldZy)
				{
					DataListIter->LeftTopPoint.ScrPos.y += OldZy;
					DataListIter->RightBottomPoint.ScrPos.y += OldZy;
				}

				UpdateTotalRange(TRUE);
				if (!(ReSetCurvePosition(0, bUpdate) & 6) && bUpdate)
					UpdateRect(hFrceDC, CanvasRectMask);
				else
					SysState |= 0x200;
			}

			return TRUE;
		}
	}

	return FALSE;
}

long CST_CurveCtrl::GetZOffset(long Address)
{
	auto DataListIter = FindMainData(Address);
	if (NullDataListIter == DataListIter)
		return -1;

	return ((long) DataListIter->Zx << 16) + DataListIter->Zy;
}

void CST_CurveCtrl::EnableFocusState(BOOL bEnable)
{
	bEnable <<= 22;
	if ((SysState ^ bEnable) & 0x400000) //第23位上有变化
	{
		SysState &= ~0x400000;
		SysState |= bEnable;

		if (m_hWnd == ::GetFocus())
			if (bEnable)
				PostMessage(WM_SETFOCUS);
			else //不可发送WM_KILLFOCUS消息
			{
				SysState &= ~0x100;
				REFRESHFOCUS(auto hDC = ::GetDC(m_hWnd), ::ReleaseDC(m_hWnd, hDC)); //马上要得到显示效果，所以获取屏幕DC
			}
	}
}

BOOL CST_CurveCtrl::SetReviseToolTip(short Type)
{
	if (0 <= Type && Type <= 3)
	{
		m_ReviseToolTip = (BYTE) Type;
		return TRUE;
	}
	return FALSE;
}
short CST_CurveCtrl::GetReviseToolTip() {return m_ReviseToolTip;}

void CST_CurveCtrl::LimitOnePage(BOOL bLimit)
{
	bLimit <<= 28;
	if ((SysState ^ bLimit) & 0x10000000) //第29位上有变化
	{
		SysState &= ~0x10000000;
		SysState |= bLimit;

		if (bLimit)
		{
			SysState &= ~0xf000000;
			RefreshLimitedOrFixedCoor();
		}
	}
}

BOOL CST_CurveCtrl::SetLimitOnePageMode(short Mode)
{
	if (0 <= Mode && Mode <= 16)
	{
		LimitOnePageMode = Mode;
		return TRUE;
	}

	return FALSE;
}
short CST_CurveCtrl::GetLimitOnePageMode() {return LimitOnePageMode;}

BOOL CST_CurveCtrl::FixCoor(HCOOR_TYPE MinTime, HCOOR_TYPE MaxTime, float MinValue, float MaxValue, short Mask)
{
	Mask &= 0xf;
	if (3 == (Mask & 3) && MinTime >= MaxTime)
		return FALSE;
	if (0xc == (Mask & 0xc) && MinValue >= MaxValue)
		return FALSE;

	SysState &= ~0xf000000;
	if (Mask)
	{
		FixedBeginTime = MinTime;
		FixedEndTime = MaxTime;
		FixedBeginValue = MinValue;
		FixedEndValue = MaxValue;

		SysState &= ~0x10000000; //取消限制一页
		SysState |= (UINT) Mask << 24;
		RefreshLimitedOrFixedCoor();
	}
	return TRUE;
}

short CST_CurveCtrl::GetFixCoor(HCOOR_TYPE* pMinTime, HCOOR_TYPE* pMaxTime, float* pMinValue, float* pMaxValue)
{
	DOFILL4VALUE(pMinTime, pMaxTime, pMinValue, pMaxValue,
		FixedBeginTime, FixedEndTime, FixedBeginValue, FixedEndValue);
	return (SysState >> 24) & 0xf;
}

BOOL CST_CurveCtrl::RefreshLimitedOrFixedCoor()
{
	if (SysState & 0x10000000)
	{
		//在只有一个点的时候（即m_MinTime等于m_MaxTime，m_MinValue等于m_MaxValue），也调用UpdateFixedValues，并不会出错
		//在设置坐标间隔的时候，由于间隔为0，所以直接失败掉
		if (nVisibleCurve > 0)
			if (0 == LimitOnePageMode)
			{
				if (UpdateFixedValues(m_MinTime, m_MaxTime, m_MinValue, m_MaxValue, 0xf))
					return TRUE;
			}
			else
			{
				UINT re = UpdateFixedValues(m_MinTime, m_MaxTime, m_MinValue, m_MaxValue, 5);

				while (RightBottomPoint.ScrPos.x > CanvasRect[1].right && //出了画布右边了
					SetTimeSpan(HCoorData.fStep * (LimitOnePageMode + 1)))
					re |= 0x10; //低四位是UpdateFixedValues返回值，保留，下同
				while (LeftTopPoint.ScrPos.y < CanvasRect[1].top && //出了画布上边上
					SetValueStep(VCoorData.fStep * (LimitOnePageMode + 1)))
					re |= 0x10;

				return re;
			}
	}
	else
	{
		UINT Mask = (SysState & 0xf000000) >> 24;
		if (Mask && UpdateFixedValues(FixedBeginTime, FixedEndTime, FixedBeginValue, FixedEndValue, Mask))
			return TRUE;
	}

	return FALSE;
}

void CST_CurveCtrl::EnablePreview(BOOL bEnable)
{
	bEnable <<= 31;
	if ((SysState ^ bEnable) & 0x80000000) //第32位上有变化
	{
		SysState &= ~0x80000000;
		SysState |= bEnable;

		if (bEnable) //全局位置窗口处于隐藏状态，开启
			UpdateRect(hFrceDC, PreviewRectMask);
		else //全局位置窗口处于显示状态，关闭
			UpdateRect(hFrceDC, CanvasRectMask);

		POINT point;
		GetCursorPos(&point);
		ScreenToClient(&point);
		PostMessage(WM_MOUSEMOVE, 0, point.x + (point.y << 16)); //修改鼠标样子，不可用WM_SETCURSOR消息替代，因为要处理MouseMoveMode变量
	}
}

void CST_CurveCtrl::SetWaterMark(LPCTSTR pWaterMark)
{
	if (!IsBadStringPtr(pWaterMark, -1))
	{
		_tcsncpy_s(WaterMark, pWaterMark, _TRUNCATE);
		DrawBkg();
		UpdateRect(hFrceDC, AllRectMask);
	}
}

long CST_CurveCtrl::GetSysState()
{
	return
		((SysState &      0x400) >> 10) + //占1位
		((SysState &     0x2000) >> 11) + //占2位（前面一位以前是EnablePageChangeEvent，现在不再使用，但当成保留位，以兼容老代码）
		((SysState &    0x60000) >> 14) + //占2位
		((SysState &   0x600000) >> 16) + //占2位
		((SysState & 0x80000000) >> 24) + //占1位
		((SysState &       0x10) << 4 ) + //占1位
		((SysState &        0x8) << 6 );  //占1位
	//不能按原始位置传出SysState，这样很容易被破解
}

void CST_CurveCtrl::SetTension(float Tension) {this->Tension = Tension;}
float CST_CurveCtrl::GetTension() {return Tension;}

OLE_HANDLE CST_CurveCtrl::GetFont() {return SplitHandle(hFont);}

BOOL CST_CurveCtrl::SetXYFormat(LPCTSTR pSign, short Format)
{
	if (0 <= Format && Format <= 0xF)
	{
		auto LegendIter = FindLegend(pSign);
		if (NullLegendIter != LegendIter)
		{
			if (LegendIter->Lable != Format)
			{
				LegendIter->Lable = Format;
				UpdateRect(hFrceDC, CanvasRectMask);
			}

			return TRUE;
		}
	}

	return FALSE;
}

short CST_CurveCtrl::GetXYFormat(LPCTSTR pSign)
{
	auto LegendIter = FindLegend(pSign);
	if (NullLegendIter != LegendIter)
		return GetXYFormat2((short) distance(begin(LegendArr), LegendIter));

	return -1;
}

short CST_CurveCtrl::GetXYFormat2(short nIndex)
{
	ASSERT(0 <= nIndex && (size_t) nIndex < LegendArr.size());
	if (0 <= nIndex && (size_t) nIndex < LegendArr.size())
	{
		auto LegendIter = next(begin(LegendArr), nIndex);
		return LegendIter->Lable;
	}

	return -1;
}

extern const TCHAR* luaFormatXCoordinate(long Address, HCOOR_TYPE DateTime, UINT Action);
extern const TCHAR* luaFormatYCoordinate(long Address, float Value, UINT Action);
extern HCOOR_TYPE luaTrimXCoordinate(HCOOR_TYPE DateTime);
extern float luaTrimYCoordinate(float Value);
extern double luaCalcTimeSpan(double TimeSpan, short Zoom, short HZoom);
extern float luaCalcValueStep(float ValueStep, short Zoom);
long CST_CurveCtrl::FreePlugIn(UINT& UpdateMask, BOOL bUpdate) //释放插件（只会释放由插件dll加载的接口）
{
	ClosePlugIn; //卸载dll以释放内存

	RemovePlugInOrScript(!=, bUpdate);
}

long CST_CurveCtrl::LoadPlugIn(LPCTSTR pFileName, short Type, long Mask)
{
	if (1 != Type) //目前只支持1类插件
	{
		return Mask;
	}

	UINT UpdateMask = 0;
	if (!(Mask & PlugInType1Mask)) //取消插件
		return FreePlugIn(UpdateMask, TRUE);
	else if (!IsBadStringPtr(pFileName, -1))
	{
		auto hModule = LoadLibrary(pFileName);
		if (hModule)
		{
			FreePlugIn(UpdateMask, FALSE);
			hPlugIn = hModule;

			if (Mask & 1)
			{
				auto pFormatXCoordinate = (FormatXCoordinate*) GetProcAddress(hModule, "FormatXCoordinate");
				if (pFormatXCoordinate)
				{
					this->pFormatXCoordinate = pFormatXCoordinate;
					UpdateMask |= HLabelRectMask;
					Mask &= ~1;
				}
			}

			if (Mask & 2)
			{
				auto pFormatYCoordinate = (FormatYCoordinate*) GetProcAddress(hModule, "FormatYCoordinate");
				if (pFormatYCoordinate)
				{
					this->pFormatYCoordinate = pFormatYCoordinate;
					UpdateMask |= VLabelRectMask;
					Mask &= ~2;
				}
			}

			if (Mask & 4)
			{
				auto pTrimXCoordinate = (TrimXCoordinate*) GetProcAddress(hModule, "TrimXCoordinate");
				if (pTrimXCoordinate)
				{
					this->pTrimXCoordinate = pTrimXCoordinate;
					Mask &= ~4;
				}
			}

			if (Mask & 8)
			{
				auto pTrimYCoordinate = (TrimYCoordinate*) GetProcAddress(hModule, "TrimYCoordinate");
				if (pTrimYCoordinate)
				{
					this->pTrimYCoordinate = pTrimYCoordinate;
					Mask &= ~8;
				}
			}

			if (Mask & 0x10)
			{
				auto pCalcTimeSpan = (CalcTimeSpan*) GetProcAddress(hModule, "CalcTimeSpan");
				if (pCalcTimeSpan)
				{
					this->pCalcTimeSpan = pCalcTimeSpan;
					UpdateMask |= HLabelRectMask;
					Mask &= ~0x10;
				}
			}

			if (Mask & 0x20)
			{
				auto pCalcValueStep = (CalcValueStep*) GetProcAddress(hModule, "CalcValueStep");
				if (pCalcValueStep)
				{
					this->pCalcValueStep = pCalcValueStep;
					UpdateMask |= VLabelRectMask;
					Mask &= ~0x20;
				}
			}

			UpdateForPlugInOrScript;
		}
	} //if (!IsBadStringPtr(pFileName, -1))

	return Mask;
}

//////////////////////////////////////////////////////////////////////////
//以下所有内容与Lua脚本相关
#define LuaBuffLen	128
static TCHAR LuaBuff[LuaBuffLen];
static const TCHAR* luaFormatXCoordinate(long Address, HCOOR_TYPE DateTime, UINT Action) //序号1
{
	LuaBuff[0] = 0;
	if (g_L)
	{
		lua_settop(g_L, 0); //容错
		lua_getglobal(g_L, "FormatXCoordinate");
		if (lua_isfunction(g_L, -1))
		{
			lua_pushinteger(g_L, Address);
			lua_pushnumber(g_L, DateTime);
			lua_pushinteger(g_L, Action);
			lua_call(g_L, 3, 1);
			if (1 != Action && 3 != Action)
			{
				const char* pstr = lua_tostring(g_L, -1);
				if (pstr)
				{
#ifdef _UNICODE
					mbstowcs_s(nullptr, LuaBuff, pstr, _TRUNCATE);
#else
					strncpy_s(LuaBuff, pstr, LuaBuffLen - 1);
#endif
				} //if (pstr)
			} //if (1 != Action && 3 != Action)
		} //if (lua_isfunction(g_L, -1))
		lua_settop(g_L, 0);
	} //if (g_L)

	return LuaBuff;
}

static const TCHAR* luaFormatYCoordinate(long Address, float Value, UINT Action) //序号2
{
	LuaBuff[0] = 0;
	if (g_L)
	{
		lua_settop(g_L, 0); //容错
		lua_getglobal(g_L, "FormatYCoordinate");
		if (lua_isfunction(g_L, -1))
		{
			lua_pushinteger(g_L, Address);
			lua_pushnumber(g_L, Value);
			lua_pushinteger(g_L, Action);
			lua_call(g_L, 3, 1);
			if (1 != Action && 3 != Action)
			{
				auto pstr = lua_tostring(g_L, -1);
				if (pstr)
				{
#ifdef _UNICODE
					mbstowcs_s(nullptr, LuaBuff, pstr, _TRUNCATE);
#else
					strncpy_s(LuaBuff, pstr, LuaBuffLen - 1);
#endif
				} //if (pstr)
			} //if (1 != Action && 3 != Action)
		} //if (lua_isfunction(g_L, -1))
		lua_settop(g_L, 0);
	} //if (g_L)

	return LuaBuff;
}

static HCOOR_TYPE luaTrimXCoordinate(HCOOR_TYPE DateTime) //序号3
{
	auto re = FALSE;
	HCOOR_TYPE NewDateTime;

	if (g_L)
	{
		lua_settop(g_L, 0); //容错
		lua_getglobal(g_L, "TrimXCoordinate");
		if (lua_isfunction(g_L, -1))
		{
			lua_pushnumber(g_L, DateTime);
			lua_call(g_L, 1, 1);

			NewDateTime = lua_tonumber(g_L, -1);
			re = TRUE;
		}
		lua_settop(g_L, 0);
	}

	return re ? NewDateTime : TrimDateTime(DateTime);
}

static float luaTrimYCoordinate(float Value) //序号4
{
	auto re = FALSE;
	float NewValue;

	if (g_L)
	{
		lua_settop(g_L, 0); //容错
		lua_getglobal(g_L, "TrimYCoordinate");
		if (lua_isfunction(g_L, -1))
		{
			lua_pushnumber(g_L, Value);
			lua_call(g_L, 1, 1);

			NewValue = (float) lua_tonumber(g_L, -1);
			re = TRUE;
		}
		lua_settop(g_L, 0);
	}

	return re ? NewValue : TrimValue(Value);
}

static double luaCalcTimeSpan(double TimeSpan, short Zoom, short HZoom) //序号5
{
	auto re = FALSE;
	double NewTimeSpan;

	if (g_L)
	{
		lua_settop(g_L, 0); //容错
		lua_getglobal(g_L, "CalcTimeSpan");
		if (lua_isfunction(g_L, -1))
		{
			lua_pushnumber(g_L, TimeSpan);
			lua_pushinteger(g_L, Zoom);
			lua_pushinteger(g_L, HZoom);
			lua_call(g_L, 3, 1);

			NewTimeSpan = lua_tonumber(g_L, -1);
			re = TRUE;
		}
	}

	return re ? NewTimeSpan : GETSTEP(TimeSpan, Zoom + HZoom);
}

static float luaCalcValueStep(float ValueStep, short Zoom) //序号6
{
	auto re = FALSE;
	float NewValueStep;

	if (g_L)
	{
		lua_settop(g_L, 0); //容错
		lua_getglobal(g_L, "CalcValueStep");
		if (lua_isfunction(g_L, -1))
		{
			lua_pushnumber(g_L, ValueStep);
			lua_pushinteger(g_L, Zoom);
			lua_call(g_L, 2, 1);

			NewValueStep = (float) lua_tonumber(g_L, -1);
			re = TRUE;
		}
	}

	return re ? NewValueStep : GETSTEP(ValueStep, Zoom);
}

long CST_CurveCtrl::FreeLuaScript() //释放Lua脚本（只会释放由Lua脚本加载的接口）
{
	CloseLua; //关闭Lua以释放内存

	UINT UpdateMask = 0;
	RemovePlugInOrScript(==, TRUE);
}

long CST_CurveCtrl::LoadLuaScript(LPCTSTR pFileName, short Type, long Mask)
{
	if (1 != Type) //目前只支持1类插件
	{
		return Mask;
	}

	if (!(Mask & PlugInType1Mask)) //取消插件
		return FreeLuaScript();
	else if (!IsBadStringPtr(pFileName, -1))
	{
		if (!g_L)
		{
			g_L = luaL_newstate();
			if (g_L)
				luaL_openlibs(g_L);
		}

		if (g_L)
		{
			const char* pName;
#ifdef _UNICODE
			char FileName[_MAX_PATH];
			wcstombs_s(nullptr, FileName, pFileName, _TRUNCATE);
			pName = FileName;
#else
			pName = pFileName;
#endif
			luaL_dofile(g_L, pName);

			UINT UpdateMask = 0;
			if (Mask & 1)
			{
				lua_getglobal(g_L, "FormatXCoordinate");
				if (lua_isfunction(g_L, -1))
				{
					pFormatXCoordinate = luaFormatXCoordinate;
					UpdateMask |= HLabelRectMask;
					Mask &= ~1;
				}
				lua_pop(g_L, -1);
			}

			if (Mask & 2)
			{
				lua_getglobal(g_L, "FormatYCoordinate");
				if (lua_isfunction(g_L, -1))
				{
					pFormatYCoordinate = luaFormatYCoordinate;
					UpdateMask |= VLabelRectMask;
					Mask &= ~2;
				}
				lua_pop(g_L, -1);
			}

			if (Mask & 4)
			{
				lua_getglobal(g_L, "TrimXCoordinate");
				if (lua_isfunction(g_L, -1))
				{
					pTrimXCoordinate = luaTrimXCoordinate;
					Mask &= ~4;
				}
				lua_pop(g_L, -1);
			}

			if (Mask & 8)
			{
				lua_getglobal(g_L, "TrimYCoordinate");
				if (lua_isfunction(g_L, -1))
				{
					pTrimYCoordinate = luaTrimYCoordinate;
					Mask &= ~8;
				}
				lua_pop(g_L, -1);
			}

			if (Mask & 0x10)
			{
				lua_getglobal(g_L, "CalcTimeSpan");
				if (lua_isfunction(g_L, -1))
				{
					pCalcTimeSpan = luaCalcTimeSpan;
					UpdateMask |= HLabelRectMask;
					Mask &= ~0x10;
				}
				lua_pop(g_L, -1);
			}

			if (Mask & 0x20)
			{
				lua_getglobal(g_L, "CalcValueStep");
				if (lua_isfunction(g_L, -1))
				{
					pCalcValueStep = luaCalcValueStep;
					UpdateMask |= VLabelRectMask;
					Mask &= ~0x20;
				}
				lua_pop(g_L, -1);
			}

			UpdateForPlugInOrScript;
		} //if (g_L)
	} //else if (!IsBadStringPtr(pFileName, -1))

	return Mask;
}

void CST_CurveCtrl::SetShortcutKeyMask(long ShortcutKey)
{
	ShortcutKey &= ALL_SHORTCUT_KEY; //目前只使用了前16位
	this->ShortcutKey = (USHORT) ShortcutKey;
}
long CST_CurveCtrl::GetShortcutKeyMask() {return ShortcutKey;}

OLE_HANDLE CST_CurveCtrl::GetFrceHDC() {return SplitHandle(hFrceDC);}

BOOL CST_CurveCtrl::SetBottomSpace(short Space)
{
	if (0 > Space || Space > 127)
		return FALSE;

	if (BottomSpaceLine != Space)
	{
		BottomSpaceLine = (BYTE) Space;
		BottomSpace = 5 + (BottomSpaceLine + 1) * (2 +  fHeight); //下空白
		ReSetUIPosition(WinWidth, WinHeight);
	}

	return TRUE;
}
short CST_CurveCtrl::GetBottomSpace() {return BottomSpaceLine;}

short CST_CurveCtrl::AddComment(HCOOR_TYPE Time, float Value, short Position, short nBkBitmap, short Width, short Height, OLE_COLOR TransColor,
							   LPCTSTR pComment, OLE_COLOR TextColor, short XOffSet, short YOffSet, BOOL bUpdate)
{
	CommentData NoUse;
	memset(&NoUse, 0, sizeof(CommentData));
	NoUse.State = 1;
	CommentDataArr.push_back(NoUse);

	auto Mask = SetComment((long) CommentDataArr.size() - 1, Time, Value, Position, nBkBitmap, Width, Height, TransColor,
		pComment, TextColor, XOffSet, YOffSet, 0x7FF, FALSE); //自己刷新

	if (Mask) //添加失败
		CommentDataArr.pop_back();
	else if (bUpdate)
		UpdateRect(hFrceDC, CanvasRectMask);

	return Mask;
}

BOOL CST_CurveCtrl::DelComment(long nIndex, BOOL bAll, BOOL bUpdate)
{
	if (bAll)
		free_container(CommentDataArr);
	else
	{
		if (0 > nIndex || (size_t) nIndex >= CommentDataArr.size())
			return FALSE;

		CommentDataArr.erase(next(begin(CommentDataArr), nIndex));
	}

	if (bUpdate)
		UpdateRect(hFrceDC, CanvasRectMask);

	return TRUE;
}

long CST_CurveCtrl::GetCommentNum() {return (long) CommentDataArr.size();}

BOOL CST_CurveCtrl::GetComment(long nIndex, HCOOR_TYPE* pTime, float* pValue, short* pPosition, short* pBkBitmap, short* pWidth, short* pHeight, OLE_COLOR* pTransColor,
							   BSTR* pComment, OLE_COLOR* pTextColor, short* pXOffSet, short* pYOffSet)
{
	if (0 > nIndex || (size_t) nIndex >= CommentDataArr.size())
		return FALSE;

	DOFILL8VALUE(pTime, pValue, pPosition, pBkBitmap, pWidth, pHeight, pTransColor, pTextColor,
		CommentDataArr[nIndex].Time, CommentDataArr[nIndex].Value, (short) CommentDataArr[nIndex].Position, CommentDataArr[nIndex].nBkBitmap,
		CommentDataArr[nIndex].Width, CommentDataArr[nIndex].Height, CommentDataArr[nIndex].TransColor, CommentDataArr[nIndex].TextColor);
	DOFILL2VALUE(pXOffSet, pYOffSet, CommentDataArr[nIndex].XOffSet, CommentDataArr[nIndex].YOffset);

	if (!IsBadWritePtr(pComment, sizeof(BSTR)))
		*pComment = CComBSTR(CommentDataArr[nIndex].Comment).Copy();

	return TRUE;
}

short CST_CurveCtrl::SetComment(long nIndex, HCOOR_TYPE Time, float Value, short Position, short nBkBitmap, short Width, short Height, OLE_COLOR TransColor,
								LPCTSTR pComment, OLE_COLOR TextColor, short XOffSet, short YOffSet, short Mask, BOOL bUpdate)
{
	if (0 > nIndex || (size_t) nIndex >= CommentDataArr.size())
		return Mask;

	short OldMask = Mask & 0x7FF;

	if (Mask & 1)
		CommentDataArr[nIndex].Time = Time;

	if (Mask & 2)
		CommentDataArr[nIndex].Value = Value;

	CalcOriginDatumPoint(CommentDataArr[nIndex], Mask & 3);

	if (Mask & 4 && 0 <= Position && Position <= 4)
	{
		CommentDataArr[nIndex].Position = (BYTE) Position;
		Mask &= ~4;
	}

	if (Mask & 8 && (-1 == nBkBitmap || 0 <= nBkBitmap && (size_t) nBkBitmap < BitBmps.size()))
	{
		CommentDataArr[nIndex].nBkBitmap = nBkBitmap;
		Mask &= ~8;
	}

	if (Mask & 16 && Width >= 0)
	{
		CommentDataArr[nIndex].Width = Width;
		Mask &= ~16;
	}

	if (Mask & 32 && Height >= 0)
	{
		CommentDataArr[nIndex].Height = Height;
		Mask &= ~32;
	}

	if (Mask & 64)
		CommentDataArr[nIndex].TransColor = TransColor & 0x80FFFFFF;

	if (Mask & 128)
		if (!IsBadStringPtr(pComment, -1))
			_tcsncpy_s(CommentDataArr[nIndex].Comment, pComment, _TRUNCATE);
		else
			*CommentDataArr[nIndex].Comment = 0;

	if (Mask & 256)
		CommentDataArr[nIndex].TextColor = TextColor;

	if (Mask & 512 && -128 <= XOffSet && XOffSet <= 127)
	{
		CommentDataArr[nIndex].XOffSet = (char) XOffSet;
		Mask &= ~512;
	}

	if (Mask & 1024 && -128 <= YOffSet && YOffSet <= 127)
	{
		CommentDataArr[nIndex].YOffset = (char) YOffSet;
		Mask &= ~1024;
	}

	Mask &= 0x63C; //11000111100 为0这些位置上的设置肯定会成功

	if (bUpdate && OldMask != (Mask & 0x7FF))
		UpdateRect(hFrceDC, CanvasRectMask);

	return Mask;
}

BOOL CST_CurveCtrl::SwapCommentIndex(long nIndex, long nOldIndex, BOOL bUpdate)
{
	if (0 > nIndex || (size_t) nIndex >= CommentDataArr.size() ||
		0 > nOldIndex || (size_t) nOldIndex >= CommentDataArr.size())
		return FALSE;

	swap(*next(begin(CommentDataArr), nIndex), *next(begin(CommentDataArr), nOldIndex));

	if (bUpdate)
		UpdateRect(hFrceDC, CanvasRectMask);

	return TRUE;
}

BOOL CST_CurveCtrl::ShowComment(long nIndex, BOOL bShow, BOOL bUpdate)
{
	if (0 > nIndex || (size_t) nIndex >= CommentDataArr.size())
		return FALSE;

	if (CommentDataArr[nIndex].State != bShow)
	{
		CommentDataArr[nIndex].State = bShow;

		if (bUpdate)
			UpdateRect(hFrceDC, CanvasRectMask);
	}

	return TRUE;
}

BOOL CST_CurveCtrl::IsCommentVisiable(long nIndex)
{
	if (0 > nIndex || (size_t) nIndex >= CommentDataArr.size())
		return FALSE;

	return CommentDataArr[nIndex].State;
}

void CST_CurveCtrl::SetEventMask(long Event) {EventState = Event & ALL_EVENT_MASK;}
long CST_CurveCtrl::GetEventMask() {return EventState;}

void CST_CurveCtrl::DoSetFixedZoomMode(WORD ZoomMode, POINT& point)
{
	ASSERT(nVisibleCurve > 0);

	ScreenToClient(&point);
	auto NewZoomMode = ZoomMode;
	if (PtInRect(CanvasRect, point))
	{
		if (0 == ZoomMode || MouseMoveMode == ZoomMode)
		{
			NewZoomMode = 0;
			MouseMoveMode = MOVEMODE;
		}
		else
			MouseMoveMode = ZoomMode;

		PostMessage(WM_SETCURSOR, (WPARAM) m_hWnd, HTCLIENT | WM_MOUSEMOVE);
	}
	else if (0 == ZoomMode || (MouseMoveMode >> 8) == ZoomMode)
	{
		NewZoomMode = 0;
		MouseMoveMode = MOVEMODE << 8;
	}
	else
		MouseMoveMode = ZoomMode << 8;

	FIRE_ZoomModeChange(0 == NewZoomMode ? 0 : (ZOOMOUT == NewZoomMode ? '-' : '+'));
}

void CST_CurveCtrl::DoSetFixedZoomMode(WORD ZoomMode)
{
	ASSERT(nVisibleCurve <= 0);

	auto NewZoomMode = ZoomMode;
	if (0 == ZoomMode || (MouseMoveMode >> 8) == ZoomMode)
		MouseMoveMode = NewZoomMode = 0;
	else
		MouseMoveMode = ZoomMode << 8;

	FIRE_ZoomModeChange(0 == NewZoomMode ? 0 : (ZOOMOUT == NewZoomMode ? '-' : '+'));
}

BOOL CST_CurveCtrl::SetFixedZoomMode(short ZoomMode) //通过接口的调用，不受SysState属性的影响
{
	if (DRAGMODE == MouseMoveMode || '+' != ZoomMode && '-' != ZoomMode && 0 != ZoomMode)
		return FALSE;

	if (0 != ZoomMode)
		ZoomMode = '-' == ZoomMode ? ZOOMOUT : ZOOMIN;

	if (nVisibleCurve > 0)
	{
		POINT point;
		GetCursorPos(&point);
		DoSetFixedZoomMode(ZoomMode, point);
	}
	else
		DoSetFixedZoomMode(ZoomMode);

	return TRUE;
}

short CST_CurveCtrl::GetFixedZoomMode()
{
	WORD MoveMode = MouseMoveMode & 0xFF;
	if (!MoveMode)
		MoveMode = MouseMoveMode >> 8; //当缩放状态时，鼠标移动到非画布区，缩放状态会写入高8位，此时仍然当成是缩放状态

	if (ZOOMOUT == MoveMode)
		return '-';
	else if (ZOOMIN == MoveMode)
		return '+';

	return 0;
}

BOOL CST_CurveCtrl::DoFixedZoom(const POINT& point)
{
	if (ZOOMIN == MouseMoveMode && 32767 == Zoom || ZOOMOUT == MouseMoveMode && -32768 == Zoom)
		return FALSE; //越界

	auto ap = CalcActualPoint(point);

	SetRedraw(FALSE);
	SysState |= 0x20000000;
	auto re = SetZoom(ZOOMIN == MouseMoveMode ? Zoom + 1 : Zoom - 1);
	if (re)
	{
		auto ap2 = CalcActualPoint(point);

		SetBeginTime2(OriginPoint.Time + ap.Time - ap2.Time);
		SetBeginValue(OriginPoint.Value + ap.Value - ap2.Value);

		if (SysState & 0x20000)
			TrimCoor();
	}
	SysState &= ~0x20000000;
	SetRedraw();

	if (re)
		UpdateRect(hFrceDC, MostRectMask);

	return re;
}

BOOL CST_CurveCtrl::FixedZoom(short ZoomMode, short x, short y, BOOL bHoldMode)
{
	if (0 == ZoomMode) //本函数不是用来取消缩放状态的
		return FALSE;
	else if (ZoomMode == GetFixedZoomMode()) //已经是想要的缩放状态了，也当成合法，只是不再取消缩放状态
		bHoldMode = TRUE;
	else if (!SetFixedZoomMode(ZoomMode)) //参数错误，或者当前状态不允许置缩放状态
		return FALSE;

	POINT point = {x, y}; //客户坐标
	auto re = PtInRect(CanvasRect, point) && DoFixedZoom(point);

	if (!bHoldMode)
		SetFixedZoomMode(ZoomMode); //取消缩放状态

	return re;
}

BOOL CST_CurveCtrl::SetCommentPosition(short Position)
{
	if (0 != Position && 1 != Position)
		return FALSE;

	if (CommentPosition != (BYTE) Position)
	{
		CommentPosition = (BYTE) Position;
		UpdateRect(hFrceDC, CanvasRectMask);
	}

	return TRUE;
}
short CST_CurveCtrl::GetCommentPosition() {return CommentPosition;}

void CST_CurveCtrl::SetToolTipDelay(short Delay) {ToolTipDely = Delay;}
short CST_CurveCtrl::GetToolTipDelay() {return ToolTipDely;}

void CST_CurveCtrl::OnRegister1Changed()
{
	if (!AmbientUserMode()) //设计模式下不允许更改这个值
	{
		m_register1 = 0;
		MessageBeep(-1);
	}

	SetModifiedFlag(FALSE);
}

BOOL CST_CurveCtrl::SetGraduationSize(long size)
{
	auto this_HSTEP = (USHORT) (size >> 16);
	auto this_VSTEP = (USHORT) (size & 0xFFFF);

	if (0 == this_HSTEP || 0 == this_HSTEP)
		return FALSE;

	UINT mask = 0;
	if (this_HSTEP != HSTEP)
	{
		mask |= 0xD;
		HSTEP = this_HSTEP;
	}
	if (this_VSTEP != VSTEP)
	{
		mask |= 0xE;
		VSTEP = this_VSTEP;
	}
	CalcOriginDatumPoint(OriginPoint, mask); //不触发坐标间隔事件
	ReSetUIPosition(WinWidth, WinHeight);

	return TRUE;
}
long CST_CurveCtrl::GetGraduationSize() {return (HSTEP << 16) + VSTEP;}

void CST_CurveCtrl::SetMouseWheelMode(short Mode)
{
	MouseWheelMode = (BYTE) Mode;

	short mask = 3;
	for (auto i = 0; i < 4; ++i, mask <<= 2)
		switch ((Mode & mask) >> 2 * i)
		{
		case 1:
			OnMouseWheelFun[i] = &CST_CurveCtrl::OnMouseWheelZoom;
			break;
		case 2:
			OnMouseWheelFun[i] = &CST_CurveCtrl::OnMouseWheelHZoom;
			break;
		case 3:
			OnMouseWheelFun[i] = &CST_CurveCtrl::OnMouseWheelHMove;
			break;
		default:
			OnMouseWheelFun[i] = &CST_CurveCtrl::OnMouseWheelVMove;
			break;
		}
}
short CST_CurveCtrl::GetMouseWheelMode() {return MouseWheelMode;}

BOOL CST_CurveCtrl::SetMouseWheelSpeed(short Speed)
{
	if (0 <= Speed && Speed <= 255)
	{
		MouseWheelSpeed = (BYTE) Speed;
		return TRUE;
	}

	return FALSE;
}
short CST_CurveCtrl::GetMouseWheelSpeed() {return MouseWheelSpeed;}

BOOL CST_CurveCtrl::SetHLegend(LPCTSTR pHLegend)
{
	ASSERT(pHLegend);
	if (IsBadStringPtr(pHLegend, -1)) //空指针也能正确判断
		return FALSE;

	HLegend.clear();
	if (*pHLegend)
	{
		auto pNow = pHLegend;
		auto pPrev = pNow;
		while(*pNow)
		{
			if (_T('\n') == *pNow)
			{
				HLegend.push_back(CString(pPrev, (int) distance(pPrev, pNow)));
				pPrev = pNow + 1;
			}

			++pNow;
		}
		HLegend.push_back(CString(pPrev));

		while (!HLegend.empty())
			if (HLegend.back().IsEmpty())
				HLegend.pop_back();
			else
				break;
	}

	return TRUE;
}

BSTR CST_CurveCtrl::GetHLegend()
{
	CComBSTR strResult;
	for (auto iter = begin(HLegend); iter < end(HLegend); ++iter)
	{
		strResult += *iter;
		strResult += _T("\n");
	}
	return strResult.Copy();
}
