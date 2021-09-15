// ST_CurveCtrl.cpp : CST_CurveCtrl ActiveX �ؼ����ʵ�֡�

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
#pragma comment(lib, "..\\..\\lua_5.4.3\\" LUA_LIB_NAME "x64.lib")
#else
#pragma comment(lib, "..\\..\\lua_5.4.3\\" LUA_LIB_NAME ".lib")
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static lua_State* g_L;

static COLORREF CustClr[16];
static long	nRef; //���ؼ������е�ʵ��
static TCHAR Time[16]; //ʱ����ʾ��
static TCHAR Legend[16]; //ͼ����ʾ��
static TCHAR OverFlow[32];
static UINT OverFlowLen;
/*
static TCHAR YMD[32]; //�ꡢ�¡��մ�ӡ�ַ���
static TCHAR HMS[32]; //ʱ���֡����ӡ�ַ���
static TCHAR Time2[16]; //ʱ����ʾ��2
static TCHAR ValueStr[16]; //ֵ��ʾ��
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
	GetObject(hBitmap, sizeof(BITMAP), &Bitmap); //��ȡλͼ��Ϣ

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
	AFX_MANAGE_STATE(AfxGetStaticModuleState()); //���������MFC�Ķ���������������д���
	if (!hBitmap)
		return FALSE;

	TCHAR FileName[MAX_PATH];
	*FileName = 0;
	auto bWarn = TRUE;
	if (!IsBadStringPtr(pFileName, -1) && *pFileName) //�ɶ�����Ϊ��
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
	else //��̨���
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
		if (bNoType) //���û��ָ����ʽ����bmp����
			_tcscat_s(FileName, _T(".bmp"));
	}

	auto re = TRUE;
	ATL::CImage Image;
	Image.Attach(hBitmap);
	auto hr = Image.Save(FileName);
	if (FAILED(hr))
	{
		re = FALSE;
		if (bWarn) //��Ϊ���ؼ��������ӿڻ���ø÷��������������˴�����ʾ
			AfxMessageBox(IDS_WRITEFILEFAIL);
	}
	/*
	CFile Output;
	if (Output.Open(FileName, CFile::modeCreate | CFile::modeWrite | CFile::shareDenyRead | CFile::shareDenyWrite, nullptr))
	{
		LPBITMAPINFO lpbi = GetDIBFromDDB(hWnd, hBitmap);
		DWORD dwPaletteSize = lpbi->bmiHeader.biBitCount > 8 ? 0 : sizeof(RGBQUAD) * ((1 << lpbi->bmiHeader.biBitCount) - 1);
		//����λͼ�ļ�ͷ
		BITMAPFILEHEADER bmfHdr;
		bmfHdr.bfType = 0x4D42; //"BM"
		bmfHdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + (DWORD) sizeof(BITMAPINFO) + dwPaletteSize;
		bmfHdr.bfSize = bmfHdr.bfOffBits + lpbi->bmiHeader.biSizeImage;
		bmfHdr.bfReserved1 = 0;
		bmfHdr.bfReserved2 = 0;

		Output.Write((LPBYTE) &bmfHdr, sizeof(BITMAPFILEHEADER)); //д��λͼ�ļ�ͷ
		Output.Write((LPBYTE) lpbi, sizeof(BITMAPINFO) + dwPaletteSize + lpbi->bmiHeader.biSizeImage); //д��λͼ�ļ���������
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
	Image.Detach(); //��Ҫ������CImage���ͷŵ�hBitmap���ڱ��ؼ���֮����û�����⣬����ΪhBitmap��ѡ����dc�������ͷŲ���
	//���������ǿ����ⲿ����dll�����õģ����ʹ���ߴ���һ��δ��ѡ��dc��hBitmap����������ɱ��������غ�ʹ������Ҳ����
	//ʹ��hBitmap�����ˣ���Ϊ���Ѿ��������ͷ�

	return re;
}

IMPLEMENT_DYNCREATE(CST_CurveCtrl, COleControl)

// ��Ϣӳ��
BEGIN_MESSAGE_MAP(CST_CurveCtrl, COleControl)
	ON_WM_CREATE()
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()

// ����ӳ��
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



// �¼�ӳ��

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



// ����ҳ

// TODO: ����Ҫ��Ӹ�������ҳ�����ס���Ӽ���!
BEGIN_PROPPAGEIDS(CST_CurveCtrl, 1)
	PROPPAGEID(CST_CurvePropPage::guid)
END_PROPPAGEIDS(CST_CurveCtrl)



// ��ʼ���๤���� guid

IMPLEMENT_OLECREATE_EX(CST_CurveCtrl, "STCURVE.STCurveCtrl.1",
	0x315e7f0e, 0x6f9c, 0x41a3, 0xa6, 0x69, 0xa7, 0xe9, 0x62, 0x6d, 0x7c, 0xa0)



// ����� ID �Ͱ汾

IMPLEMENT_OLETYPELIB(CST_CurveCtrl, _tlid, _wVerMajor, _wVerMinor)



// �ӿ� ID

const IID IID_DST_Curve = { 0xb8f65d5c, 0xca0b, 0x494f, { 0x8b, 0x39, 0x6c, 0xb7, 0xe1, 0xa, 0x2d, 0xd4 } };
const IID IID_DST_CurveEvents = { 0x890ba0f6, 0x1786, 0x4e90, { 0x93, 0xe9, 0xf3, 0xc5, 0x24, 0xe1, 0xd0, 0xdc } };


// �ؼ�������Ϣ

static const DWORD _dwST_CurveOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CST_CurveCtrl, IDS_ST_CURVE, _dwST_CurveOleMisc)



// CST_CurveCtrl::CST_CurveCtrlFactory::UpdateRegistry -
// ��ӻ��Ƴ� CST_CurveCtrl ��ϵͳע�����

BOOL CST_CurveCtrl::CST_CurveCtrlFactory::UpdateRegistry(BOOL bRegister)
{
	// TODO: ��֤���Ŀؼ��Ƿ���ϵ�Ԫģ���̴߳������
	// �йظ�����Ϣ����ο� MFC ����˵�� 64��
	// ������Ŀؼ������ϵ�Ԫģ�͹�����
	// �����޸����´��룬��������������
	// afxRegApartmentThreading ��Ϊ 0��

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

static HCOOR_TYPE TrimDateTime(HCOOR_TYPE DateTime) //����ʱ��
{
	TCHAR StrBuff[StrBuffLen];
	_stprintf_s(StrBuff, _T("%.0f")/*HPrecisionExp*/, DateTime + .5);
	return _tcstod(StrBuff, nullptr);
}

static float TrimValue(float Value) //����ֵ
{
	TCHAR StrBuff[StrBuffLen];
	_stprintf_s(StrBuff , _T("%.0f")/*VPrecisionExp*/, Value + .5f);
	return (float) _tcstod(StrBuff, nullptr);
}

// CST_CurveCtrl::CST_CurveCtrl - ���캯��

CST_CurveCtrl::CST_CurveCtrl()
{
	InitializeIIDs(&IID_DST_Curve, &IID_DST_CurveEvents);

	auto pApp = AfxGetApp();
	hZoomIn = pApp->LoadCursor(IDC_ZOOMIN);
//	hZoomIn = pApp->LoadStandardCursor(IDC_SIZENS);
	hZoomOut = pApp->LoadCursor(IDC_ZOOMOUT);
	hMove = pApp->LoadCursor(IDC_MOVE);
	hDrag = pApp->LoadCursor(IDC_DRAG);
	MouseMoveMode = 0; //����ģʽ
	Zoom = 0; //������
	HZoom = 0; //������
	pWebBrowser = 0;
	Tension = .5f; //���ƻ�����������ʱ��ϵ��
	SysState = 0x805C2C08; //����ѡ�����ߡ��������š�����������ʾ���Զ���������ZOrder����ʾ��������ʾ�ݺ�������ʾ����״̬����ʾȫ��λ�ô���
	EventState = 0; //Ĭ�ϲ������κ��¼�
	m_iMSGRecWnd = nullptr;
	m_MSGRecWnd = m_register1 = 0;
	hAxisPen = nullptr;
	hScreenRgn = nullptr;
	hFont = nullptr;
	hTitleFont = nullptr;
	nBkBitmap = -1;
	hFrceBmp = hBackBmp = nullptr;
	hFrceDC = hBackDC = hTempDC = nullptr;
	fWidth = fHeight = 0; //���︳0�Ǳ���ģ������ǵ�һ�ε���SetFont����

	m_gdiplusToken = 0;

	nZLength = 0; //Z�᳤�ȣ���λΪ�̶���
	LeftBkColor = RGB(130, 130, 130);
	BottomBkColor = RGB(200, 200, 200);
	nCanvasBkBitmap = -1; //��������λͼ

	hBuddyServer = nullptr;
	pBuddys = nullptr;
	BuddyState = 0;
	/*�Ѿ�������Ӵ˹��ܣ���Ϊ���ο�������ʵ�ִ˹��ܽ����ӵ�������
	memset(pMoveBuddy, 0, sizeof(pMoveBuddy));
	*/

	SorptionRange = -1; //��Ч��������Χ
	LastMousePoint.x = -1; //����Ļ���棬����û����һ���������
	CurveTitle[0] = 0;
	FootNote[0] = 0;
	WaterMark[0] = 0;
	m_BE = nullptr;

	m_ShowMode = 0; //�ӵ�λ�𣬵�һλ��1��X����ʾ���ұߣ�0��X����ʾ����ߣ��ڶ�λ��1��Y����ʾ���ϱߣ�0��Y����ʾ���±�
	m_MoveMode = 7; //�����ƶ�ģʽ������ˮƽ�ʹ�ֱ�������ƶ�
	m_CanvasBkMode = m_BkMode = 0; //ƽ��λͼ
	m_ReviseToolTip = 0; //ʼ�հ�Z�������0��У��������ʾ

	MaxLength = -1; //��������������
	CutLength = 0;
	nVisibleCurve = 0;

	m_hInterval = 3; m_vInterval = 0;
	m_LegendSpace = 30;
	LeftSpace = 0;
	BottomSpaceLine = 2; //�¿հ�����
	CommentPosition = 1; //��߲�
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

	if (!*Time) //��һ�Σ�װ��ȫ�ֱ���
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

	CurCurveIndex = -1; //��ǰû��ѡ�е�����
	points.reserve(512); //Ĭ�Ϸ���512���ռ�

	//��1����
	pFormatXCoordinate = nullptr;
	pFormatYCoordinate = nullptr;
	pTrimXCoordinate = nullptr;
	pTrimYCoordinate = nullptr;
	pCalcTimeSpan = nullptr;
	pCalcValueStep = nullptr;
	hPlugIn = nullptr;

	ShortcutKey = ALL_SHORTCUT_KEY; //���п�ݼ�������
	LimitOnePageMode = 0;
	AutoRefresh = 0;
	ToolTipDely = SHOWDELAY;

	VSTEP = 21; //��������Ļ����
	HSTEP = 21; //��������Ļ����

	//0-��ֱ�ƶ��� 1-���ţ�2-ˮƽ���ţ� 3-ˮƽ�ƶ�
	//�ӵ�λ��ÿ��λΪһ�飬�����ǣ�shift alt ctrl�Ͳ�����
	SetMouseWheelMode(1 + (2 << 2) + (3 << 4) + (0 << 6));
	MouseWheelSpeed = 1;
}

// CST_CurveCtrl::~CST_CurveCtrl - ��������

CST_CurveCtrl::~CST_CurveCtrl()
{
	DeleteGDIObject();
}

// CST_CurveCtrl::OnDraw - ��ͼ����

void CST_CurveCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid) //��ӡʱ������ñ�����
{
	if (!hFrceDC) //������ڽ���� AcitveX Control Test Container �г��������
		return;

	if (SysState & 0x200) //���¶�����Ҫˢ��
	{
		SysState &= ~0x200;
		UpdateRect(hFrceDC, AllRectMask); //û�м�¼��Щ��Ҫˢ�£�����ֻ��ȫ��ˢ��
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
		InflateRect(&tr, step, 0); //����ʱ����������䱣֤��ӳ���������Ȼ���ھ��У�������ʾ������һ����
	}
//	Rectangle(hDC, tr.left, tr.top, tr.right, tr.bottom);
	DrawText(hDC, CurveTitle, -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

static BOOL IsColorsSimilar(COLORREF lc, COLORREF rc) //ÿ����ɫ�����Ĳ�ľ���ֵ��������С��60��Ϊ����
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

	return sum < 60; //С��60��Ϊ����
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
		tr.left -= step; //����ʱ����������䱣֤��ӳ���������Ȼ���ھ��ң�������ʾ������һ����
	}
	else
		tr = FootNoteRect;
//	Rectangle(hDC, tr.left, tr.top, tr.right, tr.bottom);
	DrawText(hDC, pCPInfo, -1, &tr, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
	SetTextColor(hDC, OldColor);
}

void CST_CurveCtrl::DrawVAxis(HDC hDC)
{
	auto pPoint = new POINT[(VCoorData.nScales + 1) * 2 + 5]; //��5���������������
	auto pNum = new DWORD[VCoorData.nScales + 1 + 2]; //��2���������������

	//��/��
	pPoint[0].x = LeftSpace - 5;
	pPoint[0].y = TopSpace + 10;
	pPoint[1].x = LeftSpace;
	pPoint[1].y = TopSpace;
	pNum[0] = 2;

	//��\��
	pPoint[2].x = LeftSpace + 5;
	pPoint[2].y = TopSpace + 10;
	pPoint[3] = pPoint[1];
	//��|��
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

	auto pPoint = new POINT[(HCoorData.nScales + 1) * 2 + 5]; //��5���������������
	auto pNum = new DWORD[HCoorData.nScales + 1 + 2]; //��2���������������

	//��\��
	pPoint[0].x = WinWidth - RightSpace - 10;
	pPoint[0].y = iy - 5;
	pPoint[1].x = WinWidth - RightSpace;
	pPoint[1].y = iy;
	pNum[0] = 2;

	//��/��
	pPoint[2].x = WinWidth - RightSpace - 10;
	pPoint[2].y = iy + 5;
	pPoint[3] = pPoint[1];
	//��-��
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

	if (pFormatYCoordinate) //���
		pFormatYCoordinate(0x7fffffff, .0f, 1);

	while (iy > TopSpace + 10)
	{
		if (!(n % (m_vInterval + 1))  && nIndex < VCoorData.nPolyText &&
			(!(VCoorData.RangeMask & 1) || fy >= VCoorData.fMinVisibleValue) && (!(VCoorData.RangeMask & 2) || fy <= VCoorData.fMaxVisibleValue))
		{
			if (pFormatYCoordinate) //���
			{
				//���ʹ�ò������������򱾿ؼ�������������ʾΪֵ����ʱ�䣬����ԭ���ɿؼ����ƵĻ��վɣ�����������ʾ��Χ��
				auto pStr = pFormatYCoordinate(0x7fffffff, fy, 2);
				ASSERT(pStr);

				if (!pStr)
					pStr = _T("");

				//���ڱ���Ҫдһ��������������ֻ�ܸ���PolyTextLen - 1���ַ�
				//��_sntprintf_s����_tcsncpy_s���Լ���һ��_tcslen����
				VCoorData.pPolyTexts[nIndex].n = _sntprintf_s(VCoorData.pTexts[nIndex], _TRUNCATE, _T("%s"), pStr);
			}
			else
				//��Ȼ��n��Ա��pPolyTexts�������û�н���������_sntprintf_s����Ҫдһ������������Ҳ���°汾crt��ȫ�Է���Ŀ���
				//���������ԭ��vc6�汾����Ŀؼ����ܹ�����ʾһ���ַ�
				VCoorData.pPolyTexts[nIndex].n = _sntprintf_s(VCoorData.pTexts[nIndex], _TRUNCATE, VPrecisionExp, fy);

			adjust_poly_len(VCoorData);

			if (m_ShowMode & 1)
				//��� �Ҷ��룬����ת��Ϊխ�ַ������󳤶�
				//����ַ���ȻҲ����ͬ�������������ʾ��Ȼ�������
				if (pFormatYCoordinate || VARIABLE_PITCH == FontPitch)
					VCoorData.pPolyTexts[nIndex].x = VLabelEnd;
				else //�ȿ�
				{
					VCoorData.pPolyTexts[nIndex].x = VLabelBegin;
					VCoorData.pPolyTexts[nIndex].x += fWidth * VCoorData.pPolyTexts[nIndex].n;
				}
			else
				//��� ����룬����ת��Ϊխ�ַ������󳤶�
				//����ַ���ȻҲ����ͬ�������������ʾ��Ȼ�������
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

	if (pFormatYCoordinate) //���
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
	if (BottomSpaceLine <= 0) //����Ӧ��Ҫ���һ��
		return;

	auto ix = LeftSpace + 5 + 1;
	auto dx = OriginPoint.Time;
	auto hd = m_ShowMode & 1 ? -1 : 1;
	auto iy = HLabelRect.top + (m_ShowMode & 2 ? fHeight : 0);
	auto n = 0, nIndex = 0, row = 1;
	auto bOverflow = FALSE, bDrawLabel = TRUE;

	if (pFormatXCoordinate) //���
		pFormatXCoordinate(0x7fffffff, .0, 1);

	while (ix < WinWidth - RightSpace - 10)
	{
		if (!(n % (m_hInterval + 1)) && nIndex < HCoorData.nPolyText &&
			(!(HCoorData.RangeMask & 1) || dx >= HCoorData.fMinVisibleValue) && (!(HCoorData.RangeMask & 2) || dx <= HCoorData.fMaxVisibleValue))
			if (pFormatXCoordinate) //���
			{
				//���ʹ�ò������������򱾿ؼ�������������ʾΪֵ����ʱ�䣬����ԭ���ɿؼ����ƵĻ��վɣ�����������ʾ��Χ��
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
				adjust_poly_len2(HCoorData.pPolyTexts[nIndex].n, (UINT) len); //��������ΪĿ�Ļ�����

				auto n = HCoorData.pPolyTexts[nIndex].n;
				//���ǵ�������������˫�ֽ��ַ�����������Ҫ��ת��Ϊխ�ַ��������ַ�������
				//�ǲ��������£���Ϊû��˫�ֽ��ַ������Բ���Ҫ�˲���
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
			} //���
			else if (m_ShowMode & 0x80) //��ʾΪֵ
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
					HCoorData.pPolyTexts[nIndex].x = ix - HCoorData.pPolyTexts[nIndex].n * fWidth / 2 * hd; //�������ʱ���ַ�������û�к��֣�����к��֣���ƫ�ƾ���λ��
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
			else //���
			{
				bOverflow = TRUE;
				HCoorData.pPolyTexts[nIndex].n = _stprintf_s(HCoorData.pTexts[nIndex], _T("%s"), OverFlow);
				HCoorData.pPolyTexts[nIndex].x = ix - OverFlowLen * fWidth / 2 * hd; //�����������������ʾ����Ϊ����������ģ��ڱ���Ĺ��캯������
				HCoorData.pPolyTexts[nIndex].y = iy;

				HCoorData.pPolyTexts[nIndex + 1].n = 0;
				nIndex += 2;
			}

		dx += HCoorData.fCurStep;
		ix += HSTEP;

		++n;
	} //while (ix < WinWidth - RightSpace - 10)

	if (pFormatXCoordinate) //���
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
			if (i->BrushStyle < 127) //hatch brush��ʽ���ο�CreateHatchBrush��������ɫΪBrushColor
			{
				if (3 != i->CurveMode || i->BrushStyle <= HS_DIAGCROSS) //�߼���GDI��֧�ֵ���ʽ��Hatch��ˢ��������
					hBrush = CreateHatchBrush(i->BrushStyle, i->BrushColor);
			}
			else if (127 < i->BrushStyle && i->BrushStyle < 255) //pattern brush��ʽ���ο�CreatePatternBrush������(BrushStyle - 128)��Ϊλͼ��BitBmps��������
			{
				if ((size_t) (i->BrushStyle - 128) < BitBmps.size())
					hBrush = CreatePatternBrush(BitBmps[i->BrushStyle - 128]);
			}
			else //�������Solid��䶼ִ������Ĵ���
				hBrush = CreateSolidBrush(*i);
//				SetBkColor(hDC, *i);
//				ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rect, nullptr, 0, nullptr);

			if (3 == i->CurveMode && HS_DIAGCROSS < i->BrushStyle && i->BrushStyle < 127) //�߼���GDI��֧�ֵ���ʽ��Hatch��ˢ��������
			{
				Gdiplus::Graphics graphics(hDC);
				Gdiplus::HatchBrush brush((Gdiplus::HatchStyle) i->BrushStyle,
					Gdiplus::Color(GetRValue(i->BrushColor), GetGValue(i->BrushColor), GetBValue(i->BrushColor)),
					Gdiplus::Color(GetRValue(m_backColor), GetGValue(m_backColor), GetBValue(m_backColor)));
				graphics.FillRectangle(&brush, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
			}
		}
		SelectObject(hDC, hBrush ? hBrush : GetStockObject(NULL_BRUSH)); //API��û��SelectStockObject����
		Rectangle(hDC, rect.left, rect.top, rect.right, rect.bottom);

		DELETEOBJECT(hBrush);
		DELETEOBJECT(hPen);

		TextOut(hDC, rect.left + fHeight + 1 + (m_ShowMode & 1 ? i->SignWidth : 0), iy + yStep, *i, (int) _tcslen(*i));
		iy += fHeight + 1;
	} //ÿ��ͼ��
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
		if (!(SysState & 0x200000)) //����ȫ��״̬ʱ��CanvasRect[1].right�Ѿ���ǰ����������
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

			if (bScaleChanged || bCanvasWidthCh) //ˢ����������
				RefreshLimitedOrFixedCoor();

			//������ĸı䣬���ܻ��������������ĸı䣨����ͼ���ͱ��⣩���Ժ���ע��
			UpdateRect(hFrceDC, bCanvasWidthCh ? AllRectMask : MostRectMask);

			//ҳ����Ϣ���ܸı�
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
	if (!(SysState & 0x200000)) //ȫ��ʱLeftSpace������
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
	if (NullDataIter == DataIter) //��ǰ����û�ж�LeftTopPoint��RightBottomPoint������λ����һ��BUG����ɾ�����������˵�ʱ�򣬻����
		DataListIter->LeftTopPoint = DataListIter->RightBottomPoint = pDataVector->front();
	for (auto i = NullDataIter == DataIter ? begin(*pDataVector) : DataIter; i < end(*pDataVector); ++i)
	{
		if (2 == DataListIter->Power) //�������ߣ���С���ʱ�䲻һ������ͷ
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

void CST_CurveCtrl::UpdateTotalRange(BOOL bRectOnly/* = FALSE*/) //����m_MinTime��m_MaxTimeֵ��m_MinValue��m_MaxValueֵ
{
	if (!bRectOnly)
	{
		m_MinTime = MAXTIME; //��ʾ��ǰû���������ݣ������޷�ȷ����Сʱ��
		m_MaxTime = MINTIME; //��ʾ��ǰû���������ݣ������޷�ȷ�����ʱ��
		m_MinValue = 1.0f; //��ʾ��ǰû���������ݣ������޷�ȷ����Сֵ
		m_MaxValue = -1.0f; //��ʾ��ǰû���������ݣ������޷�ȷ�����ֵ
	}

	auto bFirstData = TRUE;
	for (auto i = begin(MainDataListArr); i < end(MainDataListArr); ++i)
		if (ISCURVESHOWN(i)) //û��ͼ��ʱҲ��ʾ���ߣ�ֻ����������
			if (bFirstData) //������i == begin(MainDataListArr)���жϣ���Ϊ������ֻͳ�Ʒ����ص�����
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
	if ((SysState & 0x980000) > 0x800000) //����ʵ�߷��
	{
		hPen = CreatePen(PS_SOLID, 1, m_gridColor);
		SelectObject(hDC, hPen);

		if (SysState & 0x100000) //��ֱ������
			DRAWSOLIDGRIP(ix, XBegin, CanvasRect[1].right, HSTEP, ix, CanvasRect[1].top, ix, YEnd);
		if (SysState & 0x80000) //ˮƽ������
			DRAWSOLIDGRIP2(iy, CanvasRect[1].top, YEnd, VSTEP, XBegin, iy, CanvasRect[1].right, iy);
	}
	else
	{
		if (SysState & 0x100000) //��ֱ������
			DRAWGRID(ix, XBegin, CanvasRect[1].right, HSTEP, iy, CanvasRect[1].top, YEnd);
		if (SysState & 0x80000) //ˮƽ������
			DRAWGRID2(iy, CanvasRect[1].top, YEnd, VSTEP, ix, XBegin, CanvasRect[1].right);
	}

	if (nZLength && (!(LeftBkColor & 0x40000000) || !(BottomBkColor & 0x40000000))) //������άЧ��
	{
		//�������ߺ��±���ɫ
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

		AbortPath(hDC); //��ʡ�ڴ�

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
	} //������άЧ��

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
	if (!BkMode) //ƽ��
	{
		auto hBrush = CreatePatternBrush(hBitmap);
		FillRect(hDC, &rect, hBrush);
		DELETEOBJECT(hBrush);
	}
	else if (1 == BkMode || 2 == BkMode) //���С�����
	{
		ASSERT(hTempDC);
		SelectObject(hTempDC, hBitmap);

		BITMAP Bitmap;
		::GetObject(hBitmap, sizeof(BITMAP), &Bitmap);

		if (1 == BkMode)
		{
			auto Left = (rect.right - rect.left - Bitmap.bmWidth) / 2;
			if (Left < 0) //λͼ���ھ���
				Left = -Left;
			else
			{
				rect.left += Left;
				rect.right = rect.left + Bitmap.bmWidth;
				Left = 0;
			}

			auto Top = (rect.bottom - rect.top - Bitmap.bmHeight) / 2;
			if (Top < 0) //λͼ���ھ���
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

		if (m_BkMode & 0x80) //�ü���������
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
		ExtTextOut(hBackDC, 0, 0, ETO_OPAQUE, &rect, nullptr, 0, nullptr); //�������Ч�����

	if (SysState & 0x180000 || nCanvasBkBitmap >= 0 || nZLength && (!(LeftBkColor & 0x40000000) || !(BottomBkColor & 0x40000000)))
		DrawGrid(hBackDC);

	if (WaterMark[0]) //��ʾˮӡ
	{
		SetOneHalfColor(hBackDC);
		CFont font;
		font.CreatePointFont(400, _T("")); //API��û��������CreatePointFont�ĺ���������ֻ��ʹ��CFont��
		SelectObject(hBackDC, font);
		DrawText(hBackDC, WaterMark, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}

	if (SysState & 0x40000) //��ʾ�򵥵İ�����Ϣ�ڱ�������
	{
		SetOneHalfColor(hBackDC);
		SelectObject(hBackDC, hFont);

		RECT rect = {CanvasRect[1].left, (WinHeight - 13 * (fHeight + 1)) / 2, CanvasRect[1].right, WinHeight};
		DrawText(hBackDC,
			_T("F4����ʾ/���ر�����\n")
			_T("F7ȫ��/ȡ��ȫ��\n")
			_T("��ס�������϶�����\n")
			_T("�����������ƶ�����\n")
			_T("1-9���ּ������ѡ������\n")
			_T("-+��������������(���ȡ��)\n")
			_T("ctrl�������������ƶ�����\n")
			_T("shift�������ִ�ԭ����������\n")
			_T("alt�������ִ�ԭ��ˮƽ��������\n")
			_T("ctrl���������ֻ����ˮƽ�������˶�\n")
			_T("shift���������ֻ���ڴ�ֱ�������˶�\n")
			_T("F5������ǰѡ�е������ڴ�ֱ�����Ͼ���\n")
			_T("��ͼ���ϵ������Ҽ�����/��ʾ����\n")
			_T("��ͼ���ϵ��������ѡ��/ȡ��ѡ������\n")
			_T("F6/���ԭ�㴦С��������ʾ/����ȫ��λ��Ԥ������\n")
			_T("��ȫ��λ��Ԥ���������棬�һ�����ȫ��λ��Ԥ������")
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
		auto n = _stprintf_s(TempBuf, VPrecisionExp, Value); //TempBuf����ռ�����㹻
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
	//StrBuff����ռ�����㹻
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

	if (n < StrBuffLen - 1 - 1) //һ����������һ���س���
	{
		ASSERT(pY);

		StrBuff[n] = _T('\n');
		_tcsncpy_s(StrBuff + n + 1, StrBuffLen - n - 1, pY, StrBuffLen - 1 - n - 1); //����������ڷ��ص��ַ�����֮��
	}

	//�лس��Ļ���ֻ���ػس���֮ǰ���ַ�����
	//û��д��س�������Ϊ�ռ䲻���⣩�Ļ�����ʱ����ǰ���Ѿ�д�����Ч�ַ�������Ӱ��ʹ�ø÷���ֵ�ĵط�
	return n;
}

//ÿ�����������һ�Σ��õ���BeginPos��ʼ��һ�������ĵ㣬�����points����
//�����2�����ߣ����������Ϊ��ȡֵ����EndPos�������˶ϵ�
//�����1�����ߣ����������Ϊ��ȡֵ����EndPos�������˶ϵ㣻�����˳��������ұ߿�ĵ�
//����ֵ��λ�㣬�ӵ�λ��1-ȡ�������2-�������׵㣻3-������ĩ�㣻4-������ѡ�е�
//bDrawNodeΪFALSEʱ������ֵֻ�е�1λ��Ч��bDrawNodeΪTRUEʱ������ֵֻ�е�2��3��4λ��Ч
//EndDrawPos���ػ��ƽ����㣨�Ѿ������ƣ�
UINT CST_CurveCtrl::GetPoints(vector<DataListHead<MainData>>::iterator DataListIter,
							  vector<MainData>::iterator BeginPos, vector<MainData>::iterator EndPos, vector<MainData>::iterator& EndDrawPos,
							  UINT CurveMode, BOOL bClosedAndFilled, BOOL bDrawNode, HDC hDC, UINT NodeMode, UINT PenWidth)
{
	ASSERT(NullDataListIter != DataListIter && NullDataIter != BeginPos && NullDataIter != EndPos);

	UINT re = bDrawNode ? 0 : 1;
	UINT Pos = 0; //1-��2-�ϣ�4-�ң�8-��
	UINT LastPos = 0; //��һ�ε�Posֵ
	UINT OutTimes = 0; //������������
	UINT FlexTimes = 0; //�յ������points���������������յ㽫��ɾ���������ж��ٸ�
	vector<MainData>::iterator LastOutPos = NullDataIter;

	auto pDataVector = DataListIter->pDataVector;
	for (auto i = BeginPos; i < EndPos; ++i)
	{
		if (2 == i->State && i > begin(*pDataVector) && i < prev(end(*pDataVector))) //������Σ���β���������Ҫ����
			continue;
		else if (1 == i->State && i > BeginPos) //�ϵ�
		{
			re &= ~1;
			break;
		}
		auto ppoint = &i->ScrPos;

		//////////////////////////////////////////////////////////////////////////
		UINT thisPos = 0;

		if (!bClosedAndFilled || i < prev(end(*pDataVector))) //���������ߣ����һ������Ҫ���ƣ������ڲ��ڻ���֮��
		{
			if (ppoint->x < CanvasRect[1].left + DataListIter->Zx)
				thisPos |= 1; //1-��
			else if (ppoint->x > CanvasRect[1].right)
				thisPos |= 4; //4-��

			if (ppoint->y < CanvasRect[1].top)
				thisPos |= 2; //2-��
			else if (ppoint->y > CanvasRect[1].bottom - DataListIter->Zy)
				thisPos |= 8; //8-��
		}

		//һ���� thisPos & Pos �ж��ȳ���Ϊ�ٵ����
		//ע�������жϷǳ���Ҫ���п��� thisPos �� Pos ��ͬһ�࣬���� LastPos ����ͬһ��
		if (thisPos & Pos && thisPos & LastPos) //�Ż���2.0.1.9
		{
			++OutTimes; //����֮��ĵ������������ģ�
			LastPos = thisPos; //��Ҫ��Ҫ��֤ LastPos ʼ��Ϊ thisPos ����һ����
			continue;
		}
		//�����ܵ��˻����ڲ��������ܵ��˻�����һ�ࣨ�����ڲࣩ

		if (1 == DataListIter->Power &&
			thisPos && LastPos && //�����㶼�ڻ���֮��
			!IsPointVisible(DataListIter, i, TRUE, FALSE, 2)) //���ɼ���ֻ��ǰ��⣬����ֻ����һ��д�������ǵ����ص�����⣬����д����������BUG��
			break; //һ�������Ż������������㶼�ڻ���֮�⣬���Ҳ���ͬ�࣬��ʱ��������ֿɼ��������һ���ǿ��Խ�����

		if (OutTimes > 1) //��������Ļ�����㣬���ܳ��ֹյ㣨�յ㼴Ϊ�˱���������۶�����Ҫ���ƵĻ�����㣬ֻ�ж������߲Ż��йյ㣩
		{
			if (thisPos & LastPos) //���ֹյ㣬�յ��������һ����ͬһ��
			{
				//��Ȼ�����˹յ㣬��ӹյ㿪ʼ��һ�������������ϵĵ��ڻ���֮��ͬһ�࣬�����´�һ���л������е� OutTimes > 1 ����ж�����
				//������в�������˵�� points �����������Ĺյ㣬������������Ҫ�ģ����Բ��õ��� FlexTimes �޷�����
				if (1 == ++FlexTimes)
					LastOutPos = EndDrawPos; //��� points �����������յ㣬���������������Ļ��ƽ�����
			}
			else //�ٹյ�
				FlexTimes = 0;

			OutTimes = 0; //��Ҫ����ֹ��������ӹյ��ʱ�򣬽��뱾 if ����ڲ�
			LastPos = Pos = 0; //��Ҫ����֤�յ�һ���ᱻ��ӵ� points
			i -= 2; //��ǰһ���㣨�յ㣩���¿�ʼ�������1��Ϊ���кͽ�����Ҫִ�е� ++i ���

			continue;
		}

		OutTimes = thisPos ? 1 : 0;
		LastPos = Pos = thisPos;
		//////////////////////////////////////////////////////////////////////////

		if (!bDrawNode)
		{
			EndDrawPos = i; //ÿ��ѭ�������޸ģ��Լ��ٸ��Ӷ�

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
		else if (!Pos) //�ڻ���֮��
		{
			if (distance(begin(*pDataVector), i) == DataListIter->SelectedIndex)
				re |= 8; //ѡ�е�

			if (i == prev(end(*pDataVector)))
				re |= 4; //ĩ��
			else if (i == begin(*pDataVector))
				re |= 2; //�׵�

			BOOL bDrawThisNode = NodeMode;
			auto ThisNodeMode = NodeMode;
			if (i->StateEx & 1) //����ʾ�ڵ㣬����ͼ��ָʾ��Ҫ��ʾ
				ThisNodeMode = bDrawThisNode = FALSE;

			if (!bDrawThisNode) //��Ȼͼ�����ߵ��״ָ̬ʾ����ʾ�ڵ㣬����ĳЩ�������Ȼ����ʾ���������ӿ�ȣ�����������������ʾ
			{
				bDrawThisNode = 1 == pDataVector->size(); //��������ֻ��һ����
				if (!bDrawThisNode && 1 == i->State)
				{
					bDrawThisNode = i == prev(end(*pDataVector)); //���һ����
					if (!bDrawThisNode)
					{
						vector<MainData>::iterator tj = i;
						bDrawThisNode = 1 == (++tj)->State; //������
					}
				}
			} //if (!bDrawThisNode)

			//SetViewportOrgEx��ExtTextOut������
			if (bDrawThisNode)
			{
				RECT rect;
				DrawNodeRect(rect, *ppoint, PenWidth, ThisNodeMode);

				auto LegendIter = DataListIter->LegendIter; //���������Ϊ�˺������д���㣬���Ҽ���һ��Ѱַ
				if (NullLegendIter != LegendIter && LegendIter->Lable & 3) //�������㣬�ӵ�λ��1-�Ƿ���ʾXֵ��2-�Ƿ���ʾYֵ��3-�Ƿ����ص�λ��4-�Ƿ���ʾ����
				{
					//Ŀǰ��ʱ�Ȱ�������ʾ�ĸ�ʽ��ӡ�ַ�����������ɾ����ʹ�õ�
					//��������һЩ�˷ѣ���Ϊĳ����������ǲ���ʾ��
					auto nIndex = FormatToolTipString(DataListIter->Address, i->Time, i->Value, 5);
					ASSERT(0 <= nIndex && nIndex < StrBuffLen);

					//unit test
					//_tcscpy_s(StrBuff, _T("\n")); //���ٶ���������ӣ������ܵõ�һ�����ַ�������ΪFormatToolTipStringΪ��ӻس���
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
					auto pEnter = StrBuff + nIndex; //pEnter����ط�������\nҲ������\0�ַ���ֻ�����������

					UINT Lable = LegendIter->Lable;
					if (0 == nIndex) //�س���������ǰ�棬���ڲ���ʾXֵ
						Lable &= ~1;
					if (!*pEnter || !pEnter[1]) //�س�����������棬���ڲ���ʾYֵ
						Lable &= ~2;

					if (!(Lable & 3)) //XY������ʾ
						continue;

					if (!(Lable & 1)) //����ʾXֵ
						pStrBuff = pEnter + 1;
					else if (!(Lable & 2)) //����ʾYֵ��ע�⣬������XYֵ������ʾ��
						*pEnter = 0;
					else if (Lable & 8) //��ʾΪ����
						*pEnter = _T(' ');

					if (Lable & 4) //���ص�λ���ڲ��״̬�£��޷����ص�λ
					{
						if (!pFormatYCoordinate && Lable & 2 && *Unit) //����Y��λ
						{
							auto pUnit = _tcsstr(pEnter + 1, Unit);
							if (pUnit)
								*(pUnit - 1) = 0; //��λǰ����һ�ո�
						}

						if (!pFormatXCoordinate && Lable & 1 && m_ShowMode & 0x80 && *HUnit) //����X��λ
						{
							auto pUnit = _tcsstr(pStrBuff, HUnit);
							if (pUnit && pUnit < pEnter) //y��������������һ��x���굥λ���������������
							{
								--pUnit;
								while(pStrBuff <= pUnit && isspace(*pUnit)) //�ų��ո�
									--pUnit;
								++pUnit;

								_tcscpy_s(pUnit, StrBuffLen - 1 - distance(pStrBuff, pUnit), pEnter);
								pEnter = pUnit;
							}
						}
					} //if (Lable & 4)

					//����rect���濼�ǹ�PenWidth�����أ�����Ҫ�ų��������أ�Ϊ���û�����������tooltip��ȫ�غϣ�
					//��������ƫ�Ʒ�ʽ��ֻ�ʺ���PenWidth����1�������������1ʱ����ԭ�ɵ���1�����
					if (PenWidth > 1)
						MakeNodeRect(rect, *ppoint, 1, ThisNodeMode);
					auto x = rect.right + 1, y = rect.bottom + 1;
					if (m_ShowMode & 1)
						x -= 5;
					if (m_ShowMode & 2)
						y -= 5;

					//������������ʱ��������ܷ���һ�����ַ�����������һ�д���
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
		points.erase(prev(end(points), FlexTimes), end(points)); //ɾ�����ùյ�

		EndDrawPos = LastOutPos; //�޸Ľ�����
	}

	return re;
}

//Mask�����������Ч�ԣ��ӵ��������ǣ���4λ����MinTime��MaxTime��MinValue��MaxValue
//����ֵ�ӵ�λ�������ǣ�SetBeginTime2��SetTimeSpan��SetBeginValue��SetValueStep������ִ�н������TRUE����1��������0����Щ�����ɷ���FALSE��TRUE��2��
UINT CST_CurveCtrl::UpdateFixedValues(HCOOR_TYPE MinTime, HCOOR_TYPE MaxTime, float MinValue, float MaxValue, UINT Mask)
{
	UINT re = 0;

	SetRedraw(FALSE);
	SysState |= 0x20000000;
	BOOL bResult;
	if (Mask & 2 && HCoorData.nScales > 0) //MaxTime��Ч
		if (Mask & 1) //MinTime��Ч
		{
			auto TimeSpan = (MaxTime - MinTime) / HCoorData.nScales;
			TimeSpan = pCalcTimeSpan ? pCalcTimeSpan(TimeSpan, -Zoom, -HZoom) : GETSTEP(TimeSpan, -(Zoom + HZoom));
			bResult = SetTimeSpan(TimeSpan);
			if (TRUE == bResult) //����bResult���ܻ����FALSE��TRUE��2������TRUE == bResult����жϲ��ɸ�ΪbResult����ͬ
				re |= 2;
		}
		else
		{
			MinTime = MaxTime - HCoorData.nScales * HCoorData.fCurStep;
			Mask |= 1;
		}
	if (Mask & 1) //MinTime��Ч
	{
		bResult = SetBeginTime2(MinTime);
		if (TRUE == bResult)
			re |= 1;
	}

	if (Mask & 8 && VCoorData.nScales > 0) //MaxValue��Ч
		if (Mask & 4) //MinValue��Ч
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
	if (Mask & 4) //MinValue��Ч
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

void CST_CurveCtrl::DrawComment(HDC hDC) //����ע��
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

				if (!Width) //��Чʱʹ��λͼ�Ŀ��
					Width = bmp.bmWidth;
				if (!Height) //��Чʱʹ��λͼ�ĸ߶�
					Height = bmp.bmHeight;

				CreateCommentRect(iterComment->Position);

				SelectObject(hTempDC, BitBmps[iterComment->nBkBitmap].hBitBmp);
				if (iterComment->TransColor & 0x80000000) //����͸������
					BitBlt(hDC, PosX, PosY, bmp.bmWidth, bmp.bmHeight, hTempDC, 0, 0, SRCCOPY);
				else
					TransparentBlt(hDC, PosX, PosY, bmp.bmWidth, bmp.bmHeight, hTempDC, 0, 0, bmp.bmWidth, bmp.bmHeight, iterComment->TransColor);
			}
			else
				CreateCommentRect(iterComment->Position);

			auto pComment = iterComment->Comment;
			if (nullptr == pComment || _T('\0') == *pComment)
				continue;

			//��Ϊ��ӳ���ϵ������Ҫ����
			if (m_ShowMode & 1)
				PosX += Width - 2 * iterComment->XOffSet;
			if (m_ShowMode & 2)
				PosY += Height - 2 * iterComment->YOffset;

			//��ʼ��������
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
		if (i == begin(DataListArr) && NullDataListIter != DataListIter) //i == begin(DataListArr)����жϷǳ���Ҫ������ȥ������Ϊ������i������DataListIter
			i = prev(end(DataListArr)); //Ϊ���˳�forѭ��
		else
			DataListIter = i;

		//�жϿɼ���ֻ��ҪZx��Zy�е����һ����Լ��ʹ��Zx����������ִ��ƫ�Ƶ�ʱ�򣬾�Ҫ�ֱ�ʹ���ˣ���ȻĿǰZx��Zy����ȵģ�
		if (!ISCURVESHOWN(DataListIter) || DataListIter->Zx > nZLength * HSTEP) //���߲��ɼ���������ͼ�����غ���Z�������������
			continue;

		auto pDataVector = DataListIter->pDataVector;
		vector<MainData>::iterator j = NullDataIter;
		if (NullDataIter != BeginPos)
			j = BeginPos;
		else
		{
			UINT thisPosition;
			j = GetFirstVisiblePos(DataListIter, TRUE, FALSE, &thisPosition); //���ֿɼ�����
			if (!bInfiniteCurve)
				if (0xFF == CurvePosition)
					CurvePosition = thisPosition;
				else
					CurvePosition &= thisPosition;

			if (NullDataIter == j) //DataListIter������ȫ���ɼ�
				continue;
		}

		if (nZLength) //�ü�������Ҫ���ģ���ΪhDC�Ѿ���������ӳ�䣬������CanvasRect[1]�������
		{
			if (DataListIter->Zx)
				ExcludeClipRect(hDC, CanvasRect[1].left - 1, CanvasRect[1].top, CanvasRect[1].left + DataListIter->Zx, CanvasRect[1].bottom);
			if (DataListIter->Zy)
				ExcludeClipRect(hDC, CanvasRect[1].left, CanvasRect[1].bottom - DataListIter->Zy, CanvasRect[1].right, CanvasRect[1].bottom);
				//����CanvasRect[1].left������1����ν�ˣ���Ϊ����Ѿ��ص�һ���ˣ����������Դ�CanvasRect[1].left + DataListIter->Zx��ʼ��
		}

		UINT PenStyle;
		UINT PenWidth;
		COLORREF PenColor;
		auto LegendIter = DataListIter->LegendIter; //���������Ϊ�˺������д���㣬���Ҽ���һ��Ѱַ

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
				LockGdiplus; //�Ȳ���GDI+��Ч
			}

			if (255 != LegendIter->BrushStyle && DataListIter->FillDirection & 0xF) //���û��������û����䷽��û����䷽����Ϊ�ǲ���䣩��ʡȥ����ˢ��
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
					else if ((size_t) (LegendIter->BrushStyle - 128) < BitBmps.size()) //Pattern���ģʽ�£�BrushColor��Ч
					{
						pBitmap = new Gdiplus::Bitmap(BitBmps[LegendIter->BrushStyle - 128], (HPALETTE) nullptr);
						pBrush = new Gdiplus::TextureBrush(pBitmap);
					}

					if (pBrush)
					{
						hBrush = (HBRUSH) 1; //��ʹ��hBrush���������������nullptr������������ж�
						pPath = new Gdiplus::GraphicsPath();
					}
				} //if (3 == CurveMode)
				else
				{
					if (LegendIter->BrushStyle < 127)
						hBrush = CreateHatchBrush(LegendIter->BrushStyle, LegendIter->BrushColor);
					else if (127 == LegendIter->BrushStyle)
						hBrush = CreateSolidBrush(LegendIter->BrushColor);
					else if ((size_t) (LegendIter->BrushStyle - 128) < BitBmps.size()) //Pattern���ģʽ�£�BrushColor��Ч
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

		if (hBrush || hPen) //Ҫô��䣬Ҫô�ǿջ��ʣ��������߶��У�����ȫ��
		{
			COLORREF NodeColor = PenColor;
			if (2 == NodeMode) //��������ɫ�ķ�ɫ��ʾ�ڵ�
				NodeColor = ~NodeColor & 0xFFFFFF;
			SetBkColor(hDC, NodeColor);
			SetTextColor(hDC, NodeColor);

			//�ҵ����ƵĽ�����
			vector<MainData>::iterator EndPos = NullDataIter;
			if (NullDataIter != BeginPos)
			{
				ASSERT(BeginPos < end(*pDataVector));

				EndPos = BeginPos;
				++EndPos;
				if (EndPos < end(*pDataVector))
					++EndPos; //����ʵʱ���ߣ�ÿ�������������
			}
			else
				EndPos = end(*pDataVector);

			BOOL bClosedAndFilled = hBrush && IsCurveClosed2(DataListIter);
			//����Ƕ��η��������ߣ������ͷ��ʼ������ǰ���GetFirstVisiblePosҲ�Ǳز����ٵģ���Ϊ��Ҫ����Position�����Ҫ����
			if (bClosedAndFilled)
				j = begin(*pDataVector);

			for (; j < EndPos; ++j)
			{
				points.clear(); //��vc2010�������ϵİ汾�У���clear�����ͷŻ��棬��֮ǰ�İ汾��ģ�����Ҫ��erase

				vector<MainData>::iterator EndDrawPos = NullDataIter;
				BOOL bQuit = 1 & GetPoints(DataListIter, j, EndPos, EndDrawPos, CurveMode, bClosedAndFilled, FALSE, 0, 0, 0); //����������������ʹ��
				ASSERT(NullDataIter != EndDrawPos);

				auto nPoints = points.size(); //�ȱ�������������ΪFILLPATH��������ܻ����ӵ�
				if (hBrush && nPoints > 1) //���������Ҫ������
				{
					if (3 != CurveMode)
						SelectObject(hDC, GetStockObject(NULL_PEN));

					if (DataListIter->FillDirection & 0x30) //�������ֵ��׼����ɫ����Ҫ��FillValue����������������ֻ��Ҫ��һ��
					{
						UINT FillDirection = DataListIter->FillDirection;
						COLORREF TextColor;

						if (127 == LegendIter->BrushStyle) //Solid���ʱ���������ɫ�ķ�ɫ
						{
							TextColor = LegendIter->BrushColor;
							FillDirection |= 0x80; //������ĳ���ִ����
						}
						else
							TextColor = FillDirection & 0x40 ? LegendIter->PenColor : m_foreColor; //ȷ���ǲ���ǰ��ɫ���Ǳ���������ɫ

						if (FillDirection & 0x80) //��
							TextColor = ~TextColor & 0xFFFFFF;

						SetTextColor(hDC, TextColor);
					}

					//Windows 95/98/Me��һ�δ���Polygon�����ĵ�ĸ��������ƣ��Һÿؼ�ֻ������Windows 2000�����Ժ��ϵͳ
					//��ΪhBrush��Ϊ�գ�����LegendIterһ����һ����Ч�ĵ�����
					FILLPATH(1, EndDrawPos->ScrPos.x, CanvasRect[1].bottom, j->ScrPos.x, CanvasRect[1].bottom); //�������
					FILLPATH(2, CanvasRect[1].right, EndDrawPos->ScrPos.y, CanvasRect[1].right, j->ScrPos.y); //�������
					FILLPATH(4, EndDrawPos->ScrPos.x, CanvasRect[1].top, j->ScrPos.x, CanvasRect[1].top); //�������
					FILLPATH(8, CanvasRect[1].left, EndDrawPos->ScrPos.y, CanvasRect[1].left, j->ScrPos.y); //�������

					if (DataListIter->FillDirection & 0x30) //��ԭ��ɫ
						SetTextColor(hDC, NodeColor);
				}

				if (hPen && nPoints) //�ջ��ʲ����ٻ���
				{
					if (nPoints > 1) //����������Ҫ������
						if (3 == CurveMode) //���ֻ�������㣬Ҳ�û���ƽ�����ߣ���ΪhPenҪôδ������Ҫô��Ϊ����ĳ�����ߴ�����
						{
							UnlockGdiplus; //��GDI+��Ч������Ҫʹ����
							pGraphics->DrawCurve(pPen, (Gdiplus::Point*) &points.front(), (int) nPoints, Tension); //Gdiplus::Point�ṹ��POINT�ṹ��win32���Ǽ��ݵ�
							LockGdiplus; //�����������GDI+ʧЧ
						}
						else
						{
							/*
							Windows 95/98/Me��һ�δ���Polyline�����ĵ�ĸ��������ƣ��Һÿؼ�ֻ������Windows 2000�����Ժ��ϵͳ
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
					auto re = GetPoints(DataListIter, j, EndPos, NoUse, CurveMode, bClosedAndFilled, TRUE, hDC, NodeMode, PenWidth); //��Ҫʹ�ú�����������
					//���Ƶ�һ������һ�㣬�����ɫ����ͬ�Ļ�
					if (re & 0xE && NodeMode && NullLegendIter != LegendIter) //��������ĩ�㡢����ѡ�е�
					{
						if (LegendIter->NodeModeEx & 3) //��λ�㣬�ӵ�λ��1-�Ƿ���ʾͷ�ڵ㣻2-�Ƿ���ʾβ�ڵ㣻3-�Ƿ���ʾѡ�е㣻
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
				if (NullDataIter == j) //DataListIter������ȫ���ɼ�
					break;

				--j; //��������������ִ�е�++j���
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
			UnlockGdiplus; //��Ҫ�������ڴ�й©
			delete pGraphics;
		}

		if ((UINT_PTR) hPen > 1)
			DeleteObject(hPen);
		if ((UINT_PTR) hBrush > 1)
			DeleteObject(hBrush);

		if (nZLength && (DataListIter->Zx || DataListIter->Zy)) //�ָ��ü������Ա���һ��ִ��ExcludeClipRect
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
				UpdateOneRange(DataListIter); //�����������ĵ��ô���ǳ���Ҫ�����ɵ���
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

	//�����²����ע��
	if (!CommentPosition)
		DrawComment(hDC);

	if (!InfiniteCurveArr.empty())
	{
		//������������
		vector<DataListHead<MainData>> InfiniteCurveDataListArr;
		for (auto iter = begin(InfiniteCurveArr); iter < end(InfiniteCurveArr); ++iter)
		{
			DataListHead<MainData> NoUse;

			NoUse.Address = iter->Address;
			NoUse.LegendIter = iter->LegendIter;
			NoUse.SelectedIndex = -1; //δѡ�е�
			NoUse.FillDirection = iter->FillDirection;
			NoUse.Power = 0;

			NoUse.Zx = NoUse.Zy = 0;

			UINT PenWidth = 1;
			auto LegendIter = iter->LegendIter; //���������Ϊ�˺������д���㣬���Ҽ���һ��Ѱַ
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
			//���ɵ���UpdateOneRange�����Ǹ���ʵ��ֵ������ģ��������ߵ�ʵ��ֵ������ģ���ʹ��
			i->LeftTopPoint = i->pDataVector->front();
			i->RightBottomPoint = i->pDataVector->back();
		} //for (auto iter = begin(InfiniteCurveArr); iter < end(InfiniteCurveArr); ++iter)

		//������������
		DrawCurve(hDC, hRgn, InfiniteCurveDataListArr, CurvePosition, TRUE);

		//�ͷ���������
		for (auto iter = begin(InfiniteCurveDataListArr); iter < end(InfiniteCurveDataListArr); ++iter)
			delete iter->pDataVector;
	} //if (!InfiniteCurveArr.empty())

	//������ͨ����
	DrawCurve(hDC, hRgn, MainDataListArr, CurvePosition, FALSE, DataListIter, BeginPos);

	//�����ϲ����ע��
	if (CommentPosition)
		DrawComment(hDC);

//	ExtSelectClipRgn(hDC, nullptr, RGN_COPY);
	SelectClipRgn(hDC, nullptr);

	if (0xFF != CurvePosition)
	{
		//����Ĵ��봿��Ϊ���ԵĶž�������MoveCurve��������Ķ�ջ������ݹ���ò��̫�
		static UINT LastPosition; //�ϴ����е�����ʱ��CurvePositionֵ
		static short MoveDistance; //�ϴ��ƶ�ʱ���ƶ���
		if (CurvePosition) //��ҳ��û���κο���ʾ������
		{
			if (!(SysState & 0x40000000) || CurvePosition & 0xC) //��31λ��ֹ�����ڴ�ֱ�����ϵ��ƶ�
			{
				if (LastPosition == CurvePosition) //�ϴε�״̬�ͱ���һ�������Լ����ƶ�
				{
					if (MoveDistance <= 0x2000)
						MoveDistance <<= 1; //�����ƶ�
					else
						MoveDistance = 0x7fff;
				}
				else
				{
					LastPosition = CurvePosition;
					MoveDistance = 1;
				}

				//GetFirstVisiblePos�������ö�·�㷨������ˮƽ�ʹ�ֱ�ϵ�ƫ�����Ტ��
				//�����ˮƽ�ƶ��󣬴�ֱ��������Ȼƫ������ʱ��Ȼ������һ��DrawCurveʱ�����õ������else��֧��
				if (CurvePosition & 3) //����ȫ��ƫ���˻�����ˮƽ�ϣ�
					MoveCurve(CurvePosition & 1 ? MoveDistance : -MoveDistance, 0, FALSE, FALSE); //����ˢ�£��������ע��
				//��Ҫ����ƶ��ĺϷ��ԣ������ƶ����ɹ��������ѭ��������ƶ���������Ȼ�ڻ���֮�⣬�������ͬһ�������ƶ�
				//CurvePosition�����ܳ��ֵ�1��2λ���ߵ�3��4λͬʱΪ1�����忴GetFirstVisiblePos����
				else if (CurvePosition & 0xC) //����ȫ��ƫ���˻�������ֱ�ϣ�
					MoveCurve(0, CurvePosition & 4 ? -MoveDistance : MoveDistance, FALSE, FALSE); //����ˢ��
				//����Ϊʲô����ˢ�£����������ֻ��UpdateRect������ñ�����������ˢ��Ӧ����UpdateRect������
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
	//FillDirection����ĵ�4λ��ÿ��ֻ��һλΪ1���ο�FILLPATH�꼰DrawCurve����
	switch (FillDirection & 0xF)
	{
	case 1: //�������
		FILLVALUE(rect.top, y);

		rect.left = BeginPos->ScrPos.x;
		rect.right = EndPos->ScrPos.x;
		rect.bottom = CanvasRect[1].bottom - DataListIter->Zy;
		break;
	case 2: //�������
		FILLVALUE(rect.left, x);

		rect.top = BeginPos->ScrPos.y;
		rect.right = CanvasRect[1].right;
		rect.bottom = EndPos->ScrPos.y;
		break;
	case 4: //�������
		FILLVALUE(rect.bottom, y);

		rect.left = BeginPos->ScrPos.x;
		rect.right = EndPos->ScrPos.x;
		rect.top = CanvasRect[1].top;
		break;
	case 8: //�������
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
	else //StrBuff����ռ�����㹻
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

// CST_CurveCtrl::DoPropExchange - �־���֧��

void CST_CurveCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	// TODO: Ϊÿ���־õ��Զ������Ե��� PX_ ������
	PX_Color(pPX, _T("ForeColor"), m_foreColor, DEFAULT_foreColor);
	PX_Color(pPX, _T("BackColor"), m_backColor, DEFAULT_backColor);
	PX_Color(pPX, _T("AxisColor"), m_axisColor, DEFAULT_axisColor);
	PX_Color(pPX, _T("GridColor"), m_gridColor, DEFAULT_gridColor);
	PX_Color(pPX, _T("TitleColor"), m_titleColor, DEFAULT_titleColor);
	PX_Color(pPX, _T("FootNoteColor"), m_footNoteColor, DEFAULT_footNoteColor);
	PX_Long(pPX, _T("PageChangeMSG"), m_pageChangeMSG, DEFAULT_pageChangeMSG);
}

// CST_CurveCtrl::GetControlFlags -
// �Զ��� MFC �� ActiveX �ؼ�ʵ�ֵı�־��
//
DWORD CST_CurveCtrl::GetControlFlags()
{
	auto dwFlags = COleControl::GetControlFlags();

	// ��ǰδ�����ؼ��������
	// �ؼ���֤��������Ƶ�����
	// ���ι�����֮�⡣
	dwFlags &= ~clipPaintDC;

	// �ڻ�Ͳ��״̬֮�����ת��ʱ��
	// �������»��ƿؼ���
	dwFlags |= noFlickerActivate;

	// �ؼ�ͨ������ԭ�豸�������е�
	// ԭʼ GDI ���󣬿����Ż����� OnDraw ������
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

//�ر�ע�⣺����UpdateRect������ʱ�򣬶���Mask������Ҫô����PreviewRectMask��Ҫôˢ�¾��α���Ҫ����PreviewRectMaskָ��ľ���
//�ٸ����ӣ�AllRectMask��MostRectMask��MostRectMask��Щ�����Ӧ�ľ��ζ�������PreviewRectMask��Ӧ�ľ���
//����ΪʲôҪ����������Ϊ����PreviewRectMaskû�е���ERASEBKG�꣬����ˢ������InvalidRect�����潫������������ؼ��Ĵ���Ҳ�͵ò���ˢ��
//����Ϊʲô������ERASEBKG�꣬����Ϊ����DrawPreviewViewǰ����Ҫ������Ļ��ӳ��
//ע�⣬��Ȼ����������ֻ����ĳһ�����ߣ����������߻ᱻ����
void CST_CurveCtrl::UpdateRect(HDC hDC, UINT Mask, vector<DataListHead<MainData>>::iterator DataListIter/*= NullDataListIter*/, BOOL bUpdate/*= TRUE*/)
{
	if (SysState & 0x20000000) //ˢ���Ѿ���ֹ����
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

	if (SysState & 0x200000 && MostRectMask == Mask) //ȫ��ʱMostRectMask������AllRectMask����
		Mask = AllRectMask;

	//ˢ����
	SetRectEmpty(&InvalidRect);
	if (!(SysState & 1) && //�Ǵ�ӡ��ִ�еĳ���
		(!(SysState & 0x200000) || Mask & CanvasRectMask)) //Mask & CanvasRectMask����жϷǳ���Ҫ��������ܳ���ˢ�˱�����ǰ��ȴû�еõ����ƣ���ȫ����ʱ��ᷢ����
		if (AllRectMask == Mask) //���еľ��β������ľ��Σ��������ڿؼ�����������
		{
			InvalidRect.right = WinWidth;
			InvalidRect.bottom = WinHeight;
			BitBlt(hDC, 0, 0, WinWidth, WinHeight, hBackDC, 0, 0 , SRCCOPY);
		}
		else if (MostRectMask == Mask) //����������£�TotalRect�ļ�����д���ģ��������޸�MostRectMask��һ��Ҫ�޸�����
		{
			InvalidRect.top = VAxisRect.top;
			InvalidRect.right = min(LegendMarkRect.left, LegendRect[1].left);
			InvalidRect.bottom = WinHeight;
			MOVERECT(InvalidRect, m_ShowMode);
			BitBlt(hDC, InvalidRect.left, InvalidRect.top, InvalidRect.right - InvalidRect.left, InvalidRect.bottom - InvalidRect.top,
				hBackDC, InvalidRect.left, InvalidRect.top , SRCCOPY);

			//��Ҫ����Ϊ������̶�ֵ���ܻ�д��LegendRect�ڲ�ȥ�����û������Ĵ��룬��Ļ�Ͽ��ܻ�����������
			InvalidRect.left = min(LegendMarkRect.left, LegendRect[1].left);
			InvalidRect.top = HLabelRect.top;
			InvalidRect.right = WinWidth;
			InvalidRect.bottom = WinHeight;
			MOVERECT(InvalidRect, m_ShowMode);
			BitBlt(hDC, InvalidRect.left, InvalidRect.top, InvalidRect.right - InvalidRect.left, InvalidRect.bottom - InvalidRect.top,
				hBackDC, InvalidRect.left, InvalidRect.top , SRCCOPY);

			//����������InvalidRect������BitBlt�Ĳ�����
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

	if (!(SysState & 1) && m_ShowMode & 3) //ӳ�����꣬��ӡ��ʱ����ڱ�ӳ������
		CHANGE_MAP_MODE(hDC, m_ShowMode);

	if (!(SysState & 0x200000))
	{
		SetTextColor(hDC, m_foreColor); //��ӡ��ˢ�¶�Ҫִ�еĳ���
		UPDATERECT(VAxisRectMask, DrawVAxis);
//		Rectangle(hDC, VAxisRect.left, VAxisRect.top, VAxisRect.right, VAxisRect.bottom);
		UPDATERECT(HAxisRectMask, DrawHAxis);
//		Rectangle(hDC, HAxisRect.left, HAxisRect.top, HAxisRect.right, HAxisRect.bottom);
	}
	if (Mask & CanvasRectMask)
	{
		DrawCurve(hDC, hScreenRgn, DataListIter); //���������ܻ����TextColor����
		if (SysState & 0x80000000) //����ȫ��λ�ô���ʱ��Ҫ��������ӳ��Ϊ��ʼ��״̬����δӳ��״̬��
		{
			if (m_ShowMode & 3) //�ָ�����
				CHANGE_MAP_MODE(hDC, 0);
			DrawPreviewView(hDC); //��ӡʱ���������ȫ��λ�ô���
			if (m_ShowMode & 3) //ӳ������
				CHANGE_MAP_MODE(hDC, m_ShowMode);
		}
	}
	if (!(SysState & 0x200000))
	{
		SetTextColor(hDC, m_foreColor); //��ӡ��ˢ�¶�Ҫִ�еĳ���
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
		if (!(SysState & 1)) //�Ǵ�ӡ��ִ�еĳ���
		{
			//2012.8.5
			//CHVIEWORG����������ʲô���ã������ú����ʲô��
//			CHVIEWORG(hDC, WinWidth, WinHeight, m_ShowMode); //��ӡʱ���������ӳ�䣬������΢�е��λ
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
	if (!(SysState & 1)) //�Ǵ�ӡ��ִ�еĳ��򣬴�ӡ��ʱ����ڱ𴦵���SelectClipRgnѡ���ӡ���򣬲�����DrawCurve����
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
	else //ȫ��λ�ô�����Ҫ����ɫ�����
		UpdateRect(hFrceDC, PreviewRectMask);

	SetModifiedFlag();
}

void CST_CurveCtrl::OnAxisColorChanged() 
{
	CHECKCOLOR(m_axisColor);
	DELETEOBJECT(hAxisPen);
	hAxisPen = CreatePen(PS_SOLID, 1, m_axisColor);

	//����������䣬���ܼ�����һ��UpdateRect��������ã�����ԭ��ο�UpdateRect����˵��
	UpdateRect(hFrceDC, VAxisRectMask | HAxisRectMask);
	UpdateRect(hFrceDC, PreviewRectMask);
	RECT rect = {VAxisRect.left, VAxisRect.bottom, HAxisRect.left, HAxisRect.bottom};
	InvalidateControl(&rect, FALSE);
	//����ԭ��������ͺ����궼��5��������VAxisRectҲ������VAxisRect�����أ���DrawHAxis������DrawVAxis����ȥ�������ǣ���������ֻ��Ҫˢ��������򼴿�

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
	UpdateRect(hFrceDC, TitleRectMask); //CurveTitleRect����UnitRect��LegendMarkRect

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
		BOOL bKeep = LegendSpace < 0; //��LegendSpaceС�����ʱ�����ͼ��������ʾ��ȫ���򲻸���ͼ����ȣ���LegendSpace������ʱ��������Сͼ����Ȳ�Ӧ��
		if (LegendSpace <= 0) //��ʱ������С��m_LegendSpaceֵ
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
			RightSpace = fHeight + 1 + 1 + m_LegendSpace; //�ҿհ�
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
	if (IsBadStringPtr(pFileName, -1) || !*pFileName) //���ɶ���Ϊ���ַ���
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
		AddBitmap(Image.Detach(), 2 + bShared); //λͼ�ɿؼ��Լ�����
		return TRUE;
	}
	else
		return FALSE;
}

void CST_CurveCtrl::AddBitmapHandle(OLE_HANDLE hBitmap, BOOL bShared)
{
	if (hBitmap)
		AddBitmap(Format64bitHandle(HBITMAP, hBitmap), bShared); //λͼ����紴��
}

BOOL CST_CurveCtrl::AddBitmapHandle2(OLE_HANDLE hInstance, LPCTSTR pszResourceName, BOOL bShared)
{
	if (hInstance)
	{
		auto hBitmap = LoadImage(Format64bitHandle(HINSTANCE, hInstance), pszResourceName, IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
		if (hBitmap)
		{
			AddBitmap((HBITMAP) hBitmap, 2 + bShared); //λͼ�ɿؼ��Լ�����
			return TRUE;
		}
	}

	return FALSE;
}
BOOL CST_CurveCtrl::AddBitmapHandle3(OLE_HANDLE hInstance, long nIDResource, BOOL bShared) //ͨ����ԴID��ӱ���λͼ��hInstanceΪ��Դ����Դ
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
	if ((HMode ^ m_BkMode) & 0x80 || m_BkMode != (BYTE) BkMode) //��8λ�б仯
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
	auto i = find(begin(BitBmps), end(BitBmps), Format64bitHandle(HBITMAP, hBitmap)); //���δ�ҵ�������Ҫ�����⴦����������Ȼ��ȷ
	return RemoveBitmapHandle2((short) distance(begin(BitBmps), i), bDel);
}

BOOL CST_CurveCtrl::RemoveBitmapHandle2(short nIndex, BOOL bDel)
{
	if (0 <= nIndex && (size_t) nIndex < BitBmps.size())
	{
		if (bDel || BitBmps[nIndex].State & 2) //�ؼ����Լ�����ʱ������bDel�Ƿ�Ϊ�棬�����ͷ���Դ
			DeleteObject(BitBmps[nIndex]);
		BitBmps.erase(next(begin(BitBmps), nIndex));

		UINT UpdateMask = 0;
		for (auto LegendIter = begin(LegendArr); LegendIter < end(LegendArr); ++LegendIter)
			if (255 != LegendIter->BrushStyle && LegendIter->BrushStyle > 127) //������λͼ��ˢ
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
	if (0 <= nIndex && (size_t) nIndex < BitBmps.size() && BitBmps[nIndex].State & 1) //ֻ�й����λͼ���ܴ��ⲿ��ȡ�ؼ��ڲ���λͼ���
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
	return GetBitmapState((short) distance(begin(BitBmps), i)); //���δ�ҵ�������Ҫ�����⴦����������Ȼ��ȷ
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
			hDC = ::GetDC(m_hWnd); //�����п���hFrceDC��û�б�����
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

		TopSpace	= 2 + fHeight + 5; //�Ͽհ�
		RightSpace	= fHeight + 1 + m_LegendSpace; //�ҿհ�
		BottomSpace = 5 + (BottomSpaceLine + 1) * (2 +  fHeight); //�¿հ�

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
			//LegendRect[1].bottom - LegendRect[1].topһ����OldfHeight + 1�����㣬���Բ��ÿ���ȡ����ɵ���ʧ
			ReSetUIPosition(WinWidth, WinHeight);

			if (hFrceDC)
				hDC = hFrceDC;
			else
			{
				hDC = ::GetDC(m_hWnd); //�����п���hFrceDC��û�б�����
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
	if (!hFont) //Ϊ0ʱ�����ؼ���������ѡ������û�ѡ��
	{
		LOGFONT lf;
		GetObject(this->hFont, sizeof(LOGFONT), &lf); //�ܵ���SetFont������ʱ��this->hFont������Ϊ0

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
	if (!AmbientUserMode()) //���ģʽ�²�����������ֵ
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
			//���������������꾫�ȵ�ʱ����Ϊ���ȸ��ģ��϶����ƶ�������ģ���������������������ϵ�������п��ܲ����ƶ�������
			if (!SetLeftSpace())
			{
				UINT UpdateMask = VLabelRectMask;
				for (auto i = begin(MainDataListArr); i < end(MainDataListArr); ++i)
				{
					if (NullLegendIter != i->LegendIter && i->LegendIter->Lable & 2) //��ʾY���꣬������Ҫ����
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
					if (NullLegendIter != i->LegendIter && i->LegendIter->Lable & 1) //��ʾX���꣬������Ҫ����
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
	if (!IsBadStringPtr(pUnit, -1)) //pUnitΪ��ָ��ʱҲ����ȷ�ж�
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
					if (NullLegendIter != i->LegendIter && 2 == (i->LegendIter->Lable & 6)) //��ʾY���꣬����û�����ص�λ��������Ҫ����
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
	if (!IsBadStringPtr(pHUnit, -1)) //pHUnitΪ��ָ��ʱҲ����ȷ�ж�
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
						if (NullLegendIter != i->LegendIter && 1 == (i->LegendIter->Lable & 5)) //��ʾX���꣬����û�����ص�λ��������Ҫ����
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

//StyleȡֵΪ��
//1������ΪͼƬ��2������ΪANSI�ļ���3������ΪUnicode�ļ���4������ΪUnicode big endian�ļ���5������Ϊutf8�ļ���6������Ϊ�����ļ�
long CST_CurveCtrl::ExportImageFromPage(LPCTSTR pFileName, long Address, long nBegin, long nCount, BOOL bAll, short Style)
{
	if (nBegin < 1 || (-1 != nCount && nCount < 1))
		return 0;

	HCOOR_TYPE BTime = GetNearFrontPos(m_MinTime, OriginPoint.Time), ETime;
	//ETimeû�г�ʼ�����ڵ��Ե�ʱ�򣬿��ܻᵯ�����棬������ᣬ��Ϊ��Mask������û�г�ʼ����ֵ��һ���ò���
	//����BTime�����ں����ڼ���ETimeʱ���ܻ���Ҫ������ֱ�Ӽ������������ʹ��Mask����������Ч�ԣ�����Ч��
	short Mask = 3;
	if (nBegin > 1)
		BTime = GETPAGESTARTTIME(BTime, nBegin);
	if (-1 == nCount)
		Mask &= ~2; //ETime��Ч
	else
		ETime = GETPAGESTARTTIME(BTime, nCount + 1); //BTime�϶���Ч�����ڴ�BTime��ʼ�����Ǵ���ҳ��ʼ��������ֻ��Ҫ����nCountҳ

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
			return 0; //������

		return ExportMetaFileFromTime(pFileName, Address, ExportIter, MinTime, MaxTime, Style);
	}
	else
		return ExportImageFromTime(pFileName, Address, BTime, ETime, Mask, bAll, Style); //����ʹ��ETime�����ܳ��������ᵽ�ľ���
}

#define BUFF_A_W_LEN	(16 + 32 + 16 + 8)
//ShowMode����CST_CurveCtrl::m_ShowMode������ȷ��������ĵ�����ʽ��DATE����doubleֵ������ʱ�䣬����ֻ�������ڻ���ֻ����ʱ�䣩��ֻ�����λ����
static void WriteFile(HANDLE hFile, long Address, vector<MainData>::iterator MainDataIter, short Style, UINT ShowMode, char* pBuffA, wchar_t* pBuffW)
{
	DWORD WritedLen;
	if (6 == Style)
	{
		DWORD WriteLen = 0;
		WriteFile(hFile, &Address, 4, &WritedLen, nullptr);
		ASSERT(4 == WritedLen);
		WriteFile(hFile, MainDataIter._Ptr, 8 + 4 + 2, &WritedLen, nullptr);
		//��λ��ǰ��ϵͳ����Ч������д��State��������ֽڣ�Ŀǰ�����ںܳ�һ��ʱ��֮�ڶ���ֻʹ�õ����ֽڣ�
		ASSERT(8 + 4 + 2 == WritedLen);
	}
	else if (2 == Style || 5 == Style)
		WRITEFILE(sprintf_s, pBuffA, "", len += sprintf_s(pBuffA + len, BUFF_A_W_LEN - len, "%S,", bstr), ;)
	else
		WRITEFILE(swprintf_s, pBuffW, L, len += swprintf_s(pBuffW + len, BUFF_A_W_LEN - len, L"%s,", bstr), if (4 == Style) for (auto i = 0; i < len; ++i) pBuffW[i] = htons(pBuffW[i]);)
}

long CST_CurveCtrl::ExportMetaFile(LPCTSTR pFileName, long Address, long nBegin, long nCount, BOOL bAll, short Style)
{
	if (2 > Style || Style > 6 || nCount < 0 && -1 != nCount) //Styleֻ��ȡֵ2��6
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
	if (!IsBadStringPtr(pFileName, -1) && *pFileName) //�ɶ�����Ϊ��
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

			if (!bAll) //ֻ����һ������
				break;
			else
				++DataListIter;
		} //while (DataListIter < end(MainDataListArr))

		CloseHandle(hFile);
	} //if (INVALID_HANDLE_VALUE != hFile)

	return nTotalData; //�����ļ��ɹ�
}

long CST_CurveCtrl::ExportMetaFileFromTime(LPCTSTR pFileName, long Address, vector<DataListHead<MainData>>::iterator ExportIter, HCOOR_TYPE MinTime, HCOOR_TYPE MaxTime, short Style)
{
	TCHAR FileName[MAX_PATH];
	*FileName = 0;
	if (!IsBadStringPtr(pFileName, -1) && *pFileName) //�ɶ�����Ϊ��
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
			if (2 == DataListIter->Power) //�������ߵĵ���Ч���Ǻܵ͵�
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

			if (NullDataListIter != ExportIter) //ֻ����һ������
				break;
			else
				++DataListIter;
		} //while (DataListIter < end(MainDataListArr))

		CloseHandle(hFile);
	} //if (INVALID_HANDLE_VALUE != hFile)

	return nTotalData; //�����ļ��ɹ�
}

long CST_CurveCtrl::ExportImageFromTime(LPCTSTR pFileName, long Address, HCOOR_TYPE BTime, HCOOR_TYPE ETime, short Mask, BOOL bAll, short Style)
{
	if (!hFrceDC || Style < 1 || Style > 6)
		return 0;

	vector<DataListHead<MainData>>::iterator ExportIter = NullDataListIter;
	if (!bAll)
	{
		ExportIter = FindMainData(Address);
		if (NullDataListIter == ExportIter || (1 == Style && (!ISCURVESHOWN(ExportIter) || ExportIter->Zx > nZLength * HSTEP))) //����ΪͼԪ�ļ�ʱ���������߿ɼ����
			return 0;
	}

	auto MinTime = !(Mask & 1) || BTime <= m_MinTime ? m_MinTime : BTime;
	MinTime = GetNearFrontPos(MinTime, OriginPoint.Time);
	auto MaxTime = !(Mask & 2) || ETime >= m_MaxTime ? m_MaxTime : ETime;
	int nPageNum;
	GETPAGENUM(MinTime, MaxTime);
	if (!nPageNum)
		return 0; //������

	if (1 < Style) //����ΪͼԪ�ļ�
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

	//�����ҵ�'*'�󣬷���һ��Ҳ�����ҵõ�
//	be.nWidth = _tcsrchr(be.pFileName, _T('*')) - be.pStart + 1;
	be.nWidth = (UINT) ((find(reverse_iterator<TCHAR*>(be.pFileName + len), reverse_iterator<TCHAR*>(be.pFileName), _T('*')) + 1).base() - be.pStart + 1);
	_stprintf_s(be.cNumFormat, _T("%%0%uu"), be.nWidth);
	be.nFileNum = 0;

	SetRedraw(FALSE);
	SysState |= 2;

	BOOL bPrintAllPage = m_MaxTime == MaxTime;
	auto OldBeginValue = OriginPoint.Value; //���浱ǰҳ�����꿪ʼֵ
	auto OldBeginTime = OriginPoint.Time;  //���浱ǰҳ��ʼʱ��
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
			--i; //�к͵�forѭ�������++i
			if (!bPrintAllPage && i >= nPageNum) //������ж��Ƿǳ��б�Ҫ�ģ���ΪGotoPage�п���һ�η���ҳ
				goto EXPORTOVER;

			bNeedCHPage = FALSE;
		}
		bNeedCHPage = TRUE;

		UpdateRect(hFrceDC, AllRectMask, ExportIter, FALSE);
		while (++be.nFileNum)
		{
			auto nNum = _sntprintf_s(StrBuff, _TRUNCATE, be.cNumFormat, be.nFileNum);
			if (nNum != be.nWidth) //�Ѿ�����ˣ�����pFileNameΪ��c:\****.bmp������ʱnFileNum�Ѿ�����4λ���ˣ�����10000
				goto EXPORTOVER;

			memcpy(be.pStart, StrBuff, be.nWidth * sizeof(TCHAR));
			if (_taccess(be.pFileName, 0)) //�ļ�������
				break;
		}
		if (be.nFileNum)
			::ExportImage(hFrceBmp, be.pFileName);
		else //�ļ�����������Ҳ����UINT�������
			break;

		++nTotalPage;
	} //for (auto i = 0; i < nPageNum || bPrintAllPage; ++i)

EXPORTOVER:
	SetBeginTime2(OldBeginTime);  //�ָ���ǰҳ�Ŀ�ʼʱ��
	SetBeginValue(OldBeginValue); //�ָ���ǰҳ�������꿪ʼֵ

	SysState &= ~2;
	SetRedraw();
	UpdateRect(hFrceDC, AllRectMask);

	delete[] be.pFileName;
	return nTotalPage;
}

BOOL CST_CurveCtrl::BatchExportImage(LPCTSTR pFileName, long nSecond) //nSecond�ĵ�λΪ��
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

			FIRE_BatchExportImageChange(0); //0����������������
		}
	}
	else if (!m_BE) //���ֻ����һ����ʱ����ʱ����ͼƬ
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
			BatchExportImage(0, 0); //�ͷ�m_BE
			return FALSE;
		}

		//�����ҵ�'*'�󣬷���һ��Ҳ�����ҵõ�
//		m_BE->nWidth = _tcsrchr(m_BE->pStart, _T('*')) - m_BE->pStart + 1;
		m_BE->nWidth = (UINT) ((find(reverse_iterator<TCHAR*>(m_BE->pFileName + Len), reverse_iterator<TCHAR*>(m_BE->pFileName), _T('*')) + 1).base() - m_BE->pStart + 1);
		_stprintf_s(m_BE->cNumFormat, _T("%%0%uu"), m_BE->nWidth);
		m_BE->nFileNum = 0;

		SetTimer(BATCHEXPORTBMP, nSecond * 1000, nullptr);
	}

	return TRUE;
}

//StyleȡֵΪ��
//2�����ı��ļ����룻6�������ļ�����
//����ֵ��-1��ʾ������������û�ȡ������
//�����2�ֽڴ����ļ�������������������ı��ļ�һ��Ϊһ�����ݣ��������ļ�18�ֽ�Ϊһ�����ݣ� ����2�ֽڴ���ɹ���ӵ������������������޷���������
long CST_CurveCtrl::ImportFile(LPCTSTR pFileName, short Style, BOOL bAddTrail)
{
	bAddTrail = TRUE;

	TCHAR FileName[MAX_PATH];
	*FileName = 0;
	if (!IsBadStringPtr(pFileName, -1) && *pFileName) //�ɶ�����Ϊ��
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

		Style = 2 == Open_Dialog.m_ofn.nFilterIndex ? 2 : 6; //nFilterIndexΪ2ʱ�����ı��ļ��е��루nFilterIndex��1��ʼ��
	}
	else if (2 != Style && 6 != Style)
		return -1;

	auto hFile = CreateFile(FileName, FILE_GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (INVALID_HANDLE_VALUE == hFile)
		return 0;

#define BUFFLEN		(1024 * 1024)
#define BINBUFFLEN	(BUFFLEN / 18 * 18) //��ȡ�������ļ�ʱ����18��������������ȡ

	auto nTotalRow = 0, nImportRow = 0;
	DWORD len;
	if (6 == Style) //��ȡ�������ļ�
	{
		auto BinBuff = new BYTE[BINBUFFLEN];
		while(ReadFile(hFile, BinBuff, BINBUFFLEN, &len, nullptr) && len)
		{
			nTotalRow += len / 18; //������
			nImportRow += iAddMemMainData(BinBuff, len, bAddTrail); //��ӳɹ�������
		}
		delete[] BinBuff;
	}
	else //�ı��ļ����ȶ�ȡ�ı��ļ�ͷ����ȷ���ļ���ʽ��ansi��unicode��unicode big endian��utf8
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
		if (3 == Style || 4 == Style) //��ȡunicode�ļ�
		{
			UINT State = 0; //0��δ֪��ʽ��1����ͨʱ�䣻2��������
			auto Buff = new wchar_t[BUFFLEN];
			while (ReadFile(hFile, (char*) Buff + pos, BUFFLEN * sizeof(wchar_t) - pos, &len, nullptr) && len)
			{
				auto TotalLen = len + pos; //����������ֽ���
				BOOL bReadOver = TotalLen < BUFFLEN * sizeof(wchar_t); //�����Ƿ��Ѷ���
				len = TotalLen / 2; //����������ַ���
				if (4 == Style) //����unicode big endian�ļ�����Ҫ�ߵ��ֽڻ���
					for (UINT i = 0; i < len; ++i)
						Buff[i] = ntohs(Buff[i]);
				--len;
				auto pRowStart = Buff; //swscanf��ȡλ��
				UINT i = 0;
				for (; i < len; ++i)
					if (0xA000D == *(DWORD*) (Buff + i))
					{
						SCANDATA(swscanf_s, L"%d,%lf,%f,%hd", L"%d", L"%f,%hd");
						++i; //����iָ��0x000A
						pRowStart = Buff + i + 1; //����0x000A
					}
				TotalLen -= (UINT) distance(Buff, pRowStart) * sizeof(wchar_t); //ʣ�µ����ݵ��ֽ���
				if (TotalLen)
				{
					memcpy(Buff, pRowStart, TotalLen); //��ʣ�µ������ƶ�����������ǰ��
					len = TotalLen / 2; //����������ַ���
					if (len > 0)
						if (bReadOver && len > 0) //�����Ѿ���ȡ��ϣ����һ��û�лس�������Ҫ�����һ�н��ж�ȡ
						{
							Buff[len] = 0; //����Ҫ����������������Խ����ݿ�����������ǰ�棨����Ҫ��������ó�һ��λ�ó�������������ֹд��������Ŀ���
							SCANDATA(swscanf_s, L"%d,%lf,%f,%hd", L"%d", L"%f,%hd");
							break;
						}
						else if (4 == Style)
							for (i = 0; i < len; ++i)
								Buff[i] = htons(Buff[i]);
				}
				pos = TotalLen; //�´����ݿ�ʼд��λ��
			} //while
			delete[] Buff;
		}
		else //��ȡansi��utf8�ļ�
		{
			UINT State = 0; //0��δ֪��ʽ��1����ͨʱ�䣻2��������
			auto Buff = new char[BUFFLEN];
			while (ReadFile(hFile, Buff + pos, BUFFLEN - pos, &len, nullptr) && len)
			{
				len += pos; //��������������ֽ���
				--len;
				auto pRowStart = Buff; //sscanf��ȡλ��
				UINT i = 0;
				for (; i < len; ++i)
					if (0xA0D == *(WORD*) (Buff + i))
					{
						SCANDATA(sscanf_s, "%d,%lf,%f,%hd", "%d", "%f,%hd");
						++i; //����iָ��0x0A
						pRowStart = Buff + i + 1; //����0x0A
					}
				++len; //��������������ֽ���
				BOOL bReadOver = len < BUFFLEN; //�����Ƿ��ȡ���
				len -= (UINT) distance(Buff, pRowStart); //���������滹ʣ�µ��ֽ���
				if (len > 0)
				{
					memcpy(Buff, pRowStart, len); //��ʣ�µ������ƶ�����������ǰ��
					if (bReadOver) //�����Ѿ���ȡ��ϣ����һ��û�лس�������Ҫ�����һ�н��ж�ȡ
					{
						Buff[len] = 0; //����Ҫ����������������Խ����ݿ�����������ǰ�棬������ֹд��������Ŀ���
						SCANDATA(sscanf_s, "%d,%lf,%f,%hd", "%d", "%f,%hd");
						break;
					}
				}
				pos = len; //�´����ݿ�ʼд��λ��
			} //while
			delete[] Buff;
		} //��ȡansi��utf8�ļ�

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

long CST_CurveCtrl::AddMemMainData(OLE_HANDLE pMemMainData, long MemSize, BOOL bAddTrail) //MemSizeָ�����ܵ��ֽڳ���
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

		CalcOriginDatumPoint(OriginPoint, 0xF); //�������������¼�
		if (!SetLeftSpace()) //SetLeftSpace����ֻ���ڻ�����С��ʱ��ŵ���ReSetCurvePosition��������������Ҫ����
			UpdateRect(hFrceDC, VLabelRectMask | HLabelRectMask | CanvasRectMask);
		this->Zoom ^= Zoom;
		Zoom ^= this->Zoom;
		this->Zoom ^= Zoom;
		SYNBUDDYS(4, this->Zoom);
		FIRE_ZoomChange(this->Zoom);

		//2012.10.29 �о�������Բ��ӣ���Ϊ��SetValueStep��SetTimeSpan���棬���п��ܴ������߱��Ƴ���Ļ�Ŀ����ԣ������Ƕ�û�м�
//		if (Zoom < this->Zoom) //��ҳ�п��������߱��Ƴ���Ļ
//			ReSetCurvePosition(0, TRUE); //�������ƶ����ߣ���ReSetCurvePosition���������ȿ����ƶ�������
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

		CalcOriginDatumPoint(OriginPoint, 0xD); //�������������¼�
		UpdateRect(hFrceDC, HLabelRectMask | CanvasRectMask);
		HZoom ^= Zoom;
		Zoom ^= HZoom;
		HZoom ^= Zoom;
		SYNBUDDYS(8, HZoom);
		FIRE_HZoomChange(HZoom);

		if (Zoom < HZoom) //��ҳ�п��������߱��Ƴ���Ļ
			ReSetCurvePosition(0, TRUE); //�������ƶ����ߣ���ReSetCurvePosition���������ȿ����ƶ�������
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
	if (IsBadStringPtr(pBeginTime, -1)) //��ָ��Ҳ����ȷ�ж�
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
	if (!(SysState & 0x20000001)) //�ڴ�ӡ��ʱ��Ҳ����õ������������λ�Ǹ�1�����Ǵ�ӡ��־
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
	if (IsBadStringPtr(pFrom, -1) || IsBadStringPtr(pTo, -1)) //��ָ��Ҳ����ȷ�ж�
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

//��Address��ԭ����ͼ����ɾ����Ȼ����ӵ�LegendIter���棬����Ѿ���LegendIter���棬�����κβ���
void CST_CurveCtrl::DoMoveCurveToLegend(long Address, vector<LegendData>::iterator& LegendIter, BOOL bUpdate)
{
	if (NullLegendIter != LegendIter && 0x80000000 == FindLegend(LegendIter, Address)) //��ַ�����ڣ�����ӵ�����
	{
		auto pSign = LegendIter->pSign;

		DelLegend(Address, FALSE, bUpdate);

		LegendIter = FindLegend(pSign); //����DelLegend֮��LegendIter�����Ѿ�ʧЧ
		LegendIter->Addrs.push_back(Address);

		auto DataListIter = FindMainData(Address);
		if (NullDataListIter != DataListIter)
		{
			DataListIter->LegendIter = LegendIter;

			if (bUpdate)
			{
				if (nVisibleCurve > 0) //�и��£�ˢ������
					UpdateRect(hFrceDC, CanvasRectMask);
			}
			else
				SysState |= 0x200;
		}
	}
}

//Mask����Address, PenColor, PenStyle, LineWidth, BrushColor, BrushStyle, CurveMode, NodeMode����Ч�ԣ���λ�㣬��ǰ�����е�˳��

//��Ҫ��ӵ�ͼ����pSign���Ѵ���ʱ��
//�����Address��ַ��ӵ�ͼ���У�����������ڲ���Mask���λΪ1�Ļ�����������Mask��������Щֵ��������ͼ����

//��Ҫ��ӵ�ͼ����pSign��������ʱ��
//���Address�Ѵ��ڣ������Ȱ�Address�������ڵ�ͼ����ɾ��������pSignΪͼ���½�������Address��ӵ����У���ʱ���в�����������Ч
//Ҳ����˵��Mask�������0xFF

//BrushStyleȡֵ���£�
//255������䣻
//127��solid brush��ʽ���ο�CreateSolidBrush��������ɫΪBrushColor��
//0-126��hatch brush��ʽ��û����ô�����ʽ�������Ժ���չ�����Կؼ�û���жϲ�����ֵ�Ƿ���CreateHatchBrush������ʶ��ķ�Χ֮�ڣ����ڷ�Χ֮���ǲ������ģ����ο�CreateHatchBrush��������ɫΪBrushColor��
//128-254��pattern brush��ʽ���ο�CreatePatternBrush������(BrushStyle - 128)��Ϊλͼ��ţ�λͼ��AddBitmap�Ⱥ�����ӣ����忴����ĵ�����
//NodeModeȡֵ���£�
//0������ʾ�ڵ㣻1��������ɫ��ʾ�ڵ㣻2��������ɫ�ķ�ɫ��ʾ�ڵ�
short CST_CurveCtrl::AddLegend(long Address, LPCTSTR pSign, OLE_COLOR PenColor, short PenStyle, short LineWidth, OLE_COLOR BrushColor, short BrushStyle, short CurveMode, short NodeMode, short Mask, BOOL bUpdate)
{
	ASSERT(pSign);
	if (IsBadStringPtr(pSign, -1)) //��ָ��Ҳ����ȷ�ж�
		return Mask;

	size_t LegendIndex;
	auto LegendIter = FindLegend(pSign);
	if (NullLegendIter != LegendIter)
	{
		if (Mask & 1)
			DoMoveCurveToLegend(Address, LegendIter, FALSE);

		//PenColor, PenStyle, LineWidth, BrushColor, BrushStyle, CurveMode, NodeMode
		if (Mask & 2)
			LegendIter->PenColor = (COLORREF) (PenColor & 0xFFFFFF); //�������������ɫ����Ҫˢ��ͼ��
		if (Mask & 4)
			LegendIter->PenStyle = (BYTE) PenStyle;
		if (Mask & 8)
			if (0 <= LineWidth && LineWidth <= 255)
			{
				LegendIter->LineWidth = (BYTE) LineWidth;
				Mask &= ~8;
			}

		if (Mask & 16)
			LegendIter->BrushColor = (COLORREF) (BrushColor & 0xFFFFFF); //�������������ɫ
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
			LegendIndex = -1; //ͼ������Ҫˢ��

		Mask &= ~( 1 | 2 | 4 | 16 | 32); //�����ֵ����ʧ��
	}
	else if (0xFF == (Mask & 0xFF)) //����µ�ͼ��
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
		NoUse.Lable = 0; //����ʾ����

		//������DoMoveCurveToLegend���棬��Ϊ�������������//�м䲿�ֵĴ���
		DelLegend(Address, FALSE, bUpdate);

		//////////////////////////////////////////////////////////////////////////
		BOOL bUpdateLegend = !LegendArr.empty() && LegendArr.capacity() == LegendArr.size(); //vectorҪ���·����ڴ��ˣ�DataListHead.LegendIter��������ʧЧ

		LegendArr.push_back(NoUse);
		LegendIter = prev(end(LegendArr));
		LegendIter->Addrs.push_back(Address);

		if (bUpdateLegend)
			ReSetDataListLegend(begin(LegendArr), prev(end(LegendArr))); //���һ��ͼ��������ӵģ����ÿ���
		//////////////////////////////////////////////////////////////////////////

		auto DataListIter = FindMainData(Address);
		if (NullDataListIter != DataListIter) //���DataListHead����Ӧ��LegendIter
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
		if (-1 != LegendIndex) //���Ǹո���ӵĻ��߸��µ�ͼ��
			UpdateMask |= LegendRectMask;
		if (nVisibleCurve > 0) //�и��£�ˢ������
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

			if (nVisibleCurve > 0) //ע�����ﲻ��ֻ�����ܵ�Ӱ������ߣ���Ϊ�������ƻ�����֮��Ĳ�νṹ
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
				DataListIter->SelectedIndex = NewNodeIndex; //�޸������ѡ�нڵ㣬����һ������ʾ��������Ҫ��ͼ���Ƿ�֧�֣������UpdateSelectedNode���ݴ�
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

//��ɾ���������ͼ�������ʱ��ֻ�����ڴ�����Ҫ���·����ڴ��ʱ��ʱ�����ñ���������DataListHead��LegendIter��Ա
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
	ASSERT(NullLegendIter != LegendPos && LegendPos < end(LegendArr) && (!bAll || bDel)); //���bAllҪΪ�棬ֻ������bDelΪ��������
	COLORREF Color = 0x80000000;
	for (auto i = begin(LegendPos->Addrs); i < end(LegendPos->Addrs);)
	{
		auto tempIter = i++;
		if (bAll || Address == *tempIter)
		{
			if (bDel)
			{
				auto DataListIter = FindMainData(*tempIter);
				if (NullDataListIter != DataListIter) //ɾ��DataListHead����Ӧ��LegendIter
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
		else if (0x80000000 != FindLegend(tempIter, Address, TRUE, bAll)) //����FindLegend�����б�Ҫ���ú����������ӦDataListHead�ṹ��LegendIter��Ա
		{
			if (bAll || tempIter->Addrs.empty())
			{
				delete[] tempIter->pSign;
				i = LegendArr.erase(tempIter);
				ReSetDataListLegend(i, end(LegendArr));
				bDelLegend = TRUE;
			}

			re = TRUE;

			if (!bAll) //ɾ������
				break;
		}

		if (NullLegendIter != LegendPos) //pLegendDataΪ��ʱ��bAll�����Ƿ�ɾ������ͼ������������Ƿ�ɾ��pLegendData��һ��ͼ��
			break;
	}

	if (bDelLegend)
	{
		//����ͼ����ɾ��ʱ��������ȫ����ˢ��һ�Σ���ΪLegendRect�ڽ����������Ҫ�޸��ˣ�����ȫˢ�µĻ��������²�������Ϊͼ�������ˣ����Ӿ�û�£�
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
			return DoDelLegend(LegendIter, 0, TRUE, bUpdate); //����LegendPos��Ч������bDelAll��TRUE������˼��ɾ��pLegendData������������ͼ��
	}

	return FALSE;
}

BOOL CST_CurveCtrl::DelLegend(long Address, BOOL bAll, BOOL bUpdate)
{
	return DoDelLegend(NullLegendIter, Address, bAll, bUpdate); //����LegendPos��Ч������bDelAll���ͻ��˴����bAll��������˼��ɾ������ͼ�������Ϊ��Ļ���
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
		auto j = i++; //������ǰ꡼Ӽ�
		if (j->Time > i->Time)
		{
			DataListIter->Power = 2;
			return;
		}
	}

	DataListIter->Power = 1;
}

//������ָ����ɼ�ʱ��Ҫ���ƶ���

//VisibleState�ӵ�λ��
//1���Ƿ����ϻ�����ӵĵ㣨���жϸ�λ��
//2�����������겻�䣨���жϸ�λ��
//3�����ֺ����겻�䣨���жϸ�λ��
//4�����������������ٵ��ƶ�
//5���ں������������ٵ��ƶ�
//6��ֻ�е���һ���ڻ����пɼ�ʱ�����Զ��ƶ����ߣ����жϸ�λ��
SIZE CST_CurveCtrl::MakePointVisible(vector<DataListHead<MainData>>::iterator DataListIter, vector<MainData>::iterator DataIter, short VisibleState /*= 0*/)
{
	ASSERT(NullDataListIter != DataListIter);
	SIZE size = {0, 0};
	if (!IsPointVisible(DataListIter, DataIter, FALSE, FALSE)) //������Ѿ��ڻ����У�����Ҫ�ƶ�����
	{
		int Position;
		int step;

		//////////////////////////////////////////////////////////////////////////
		Position = CanvasRect[1].right < DataIter->ScrPos.x ? 1 : 0; //1��ʾ���ұ�
		if (!Position)
			Position = CanvasRect[1].left + DataListIter->Zx > DataIter->ScrPos.x ? 2 : 0; //2��ʾ�����

		if (Position) //��Ҫ�ƶ�
			if (VisibleState & 0x10) //�����ƶ�
				if (1 == Position) //���ұ�
				{
					step = CanvasRect[1].right - DataIter->ScrPos.x;
					size.cx = step / HSTEP;
					if (step % HSTEP)
						--size.cx;
				}
				else //if (2 == Position) //�����
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
		Position = CanvasRect[1].bottom - DataListIter->Zy < DataIter->ScrPos.y ? 1 : 0; //1��ʾ���±�
		if (!Position)
			Position = CanvasRect[1].top > DataIter->ScrPos.y ? 2 : 0; //2��ʾ���ϱ�

		if (Position) //��Ҫ�ƶ�
			if (VisibleState & 8) //�����ƶ�
				if (1 == Position) //���±�
				{
					step = DataIter->ScrPos.y - (CanvasRect[1].bottom - DataListIter->Zy);
					size.cy = step / VSTEP;
					if (step % VSTEP)
						++size.cy;
				}
				else //if (2 == Position) //���ϱ�
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

//ChMask��λ��
//1��ˮƽ�����¼���ScrPos.x
//2����ֱ�����¼���ScrPos.y
//3�������ı䣬����ı����ʲô������1��2λ����
//4��ֻ�ڵ�3λ��Чʱ��Ч�����Ϊ1����������Zoom���߿̶ȼ���ı�����ģ����Բ������������ı��¼�
//5��ֻ����ap�㣨��������OriginPoint�㣩�������޸Ļ���ʱ����
//��XOff����YOff��Ϊ0ʱ����ChMask��������������ֻ��0�ͷ�0֮�֣���0����OriginPointҲҪƽ��
void CST_CurveCtrl::CalcOriginDatumPoint(MainData& ap, UINT ChMask/* = 3*/, int XOff/* = 0*/, int YOff/* = 0*/, vector<DataListHead<MainData>>::iterator DataListIter/* = NullDataListIter*/)
{
	//ƽ�����������
	//XOff = LeftSpace(old) - LeftSpace(new)
	//YOff = WinHeight(old) - WinHeight(new)
	//YOff = BottomSpace(new) - BottomSpace(old)
	//�Լ��ƶ�����ʱ
	if (XOff || YOff) //ƽ������
	{
		ASSERT(OriginPoint == ap); //ƽ������ʱ��ֻ��Ҫ��OriginPoint����CalcOriginDatumPoint��������

		for (auto j = begin(MainDataListArr); j < end(MainDataListArr); ++j) //ÿ������
		{
			auto pDataVector = j->pDataVector;
			for (auto i = begin(*pDataVector); i < end(*pDataVector); ++i) //ÿ����
			{
				if (XOff)
					i->ScrPos.x -= XOff;
				if (YOff)
					i->ScrPos.y -= YOff;
			}

			//ÿ������ռ�ݵľ���
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

		for (auto k = begin(InfiniteCurveArr); k < end(InfiniteCurveArr); ++k) //ÿ����������
		{
			if (XOff && 1 == k->State)
				k->ScrPos.x -= XOff;
			if (YOff && 0 == k->State)
				k->ScrPos.y -= YOff;
		}

		for (auto i = begin(CommentDataArr); i < end(CommentDataArr); ++i) //ÿ��ע��
		{
			if (XOff)
				i->ScrPos.x -= XOff;
			if (YOff)
				i->ScrPos.y -= YOff;
		}

		if (ChMask) //��XOff��YOff��ȫΪ0��ʱ��ChMask�����Ƿ�Ҫ��OriginPoint����ƫ��
		{
			if (XOff)
				OriginPoint.ScrPos.x += XOff; //����ļӺ���Ҫ
			if (YOff)
				OriginPoint.ScrPos.y -= YOff;
		}

		if (nVisibleCurve > 0) //��������ռ�ݵľ���
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

	if (ChMask & 1) //ˮƽ���б仯
	{
		if (!(ChMask & 0x14) && OriginPoint == ap) //��3��5λ
			XOff = ap.ScrPos.x;

		BOOL bNegative = ap.Time < BenchmarkData.Time;
		ap.ScrPos.x = (long) ((ap.Time - BenchmarkData.Time) / HCoorData.fCurStep * HSTEP + (bNegative ? -.5 : .5));
	}
	if (ChMask & 2) //��ֱ���б仯
	{
		if (!(ChMask & 0x14) && OriginPoint == ap) //��3��5λ
			YOff = ap.ScrPos.y;

		BOOL bNegative = ap.Value < BenchmarkData.Value;
		ap.ScrPos.y = (long) ((ap.Value - BenchmarkData.Value) / VCoorData.fCurStep * VSTEP + (bNegative ? -.5f : .5f));
	}

	if (ChMask & 0x10)
	{
		ASSERT(OriginPoint == ap); //�޸Ļ���
		return;
	}

	if (ChMask & 4) //�����б仯����Ҫ���¼������е��ScrPosֵ
	{
		if (ChMask & 3)
		{
			ASSERT(OriginPoint == ap); //�����ı�ʱ��ֻ��Ҫ��OriginPoint����CalcOriginDatumPoint��������

			//�����¼�
			if (!(ChMask & 8)) //��Zoom����
			{
				if (ChMask & 1)
					FIRE_TimeSpanChange(HCoorData.fStep);
				if (ChMask & 2)
					FIRE_ValueStepChange(VCoorData.fStep);
			}

			ChMask &= 3;
			for (auto j = begin(MainDataListArr); j < end(MainDataListArr); ++j) //ÿ������
			{
				auto pDataVector = j->pDataVector;
				for (auto i = begin(*pDataVector); i < end(*pDataVector); ++i) //ÿ����
					CalcOriginDatumPoint(*i, ChMask, 0, 0, j);
				//ÿ������ռ�ݵľ���
				CalcOriginDatumPoint(j->LeftTopPoint, ChMask, 0, 0, j);
				CalcOriginDatumPoint(j->RightBottomPoint, ChMask, 0, 0, j);
			}

			for (auto k = begin(InfiniteCurveArr); k < end(InfiniteCurveArr); ++k) //ÿ����������
				CalcOriginDatumPoint(*k, ChMask & (0 == k->State ? 2 : 1));

			for (auto i = begin(CommentDataArr); i < end(CommentDataArr); ++i) //ÿ��ע��
				CalcOriginDatumPoint(*i, ChMask); //ע�ⲻ����Z��Ӱ��

			UpdateTotalRange(TRUE);
		}

		return;
	}

	if (OriginPoint == ap)
	{
		if (ChMask & 1) //ˮƽ���б仯
		{
			XOff -= ap.ScrPos.x;
			FIRE_BeginTimeChange(OriginPoint.Time); //�����¼�
		}
		if (ChMask & 2) //��ֱ���б仯
		{
			YOff -= ap.ScrPos.y;
			FIRE_BeginValueChange(OriginPoint.Value); //�����¼�
		}

		CalcOriginDatumPoint(ap, 0, -XOff, YOff); //ƽ������
	}
	else
	{
		if (ChMask & 1) //ˮƽ���б仯
			ap.ScrPos.x = CanvasRect[1].left + ap.ScrPos.x - OriginPoint.ScrPos.x;
		if (ChMask & 2) //��ֱ���б仯
			ap.ScrPos.y = CanvasRect[1].bottom - 1 - (ap.ScrPos.y - OriginPoint.ScrPos.y); //��һ�Ǳ���ģ���Ϊ��ReSetUIPosition��CanvasRect[1]ƫ����һ������

		if (NullDataListIter != DataListIter) //����жϺ���Ҫ�������ĳ�����ߵĵ�һ�����ʱ���ڼ���ҳ������ʱ��DataListIter����Ч
		{
			if (ChMask & 1) //ˮƽ���б仯
				ap.ScrPos.x += DataListIter->Zx;
			if (ChMask & 2) //��ֱ���б仯
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
	auto re = 0; //���ص���λ��Ч���ӵ�λ�������������ڻ������ң��£����Ϸ�
	if (p.y < CanvasRect[1].top)
		re |= 8; //��
	else if (p.y > CanvasRect[1].bottom)
		re |= 2; //��

	if (p.x < CanvasRect[1].left)
		re |= 1; //��
	else if (p.x > CanvasRect[1].right)
		re |= 4; //��

	return re;
}

int CST_CurveCtrl::IsLineOutdrop(POINT& p1, POINT& p2)
{
	auto re = 0, re1 = IsPointOutdrop(p1), re2 = IsPointOutdrop(p2); //���ص���λ��Ч���ӵ�λ�������������ڻ������ң��£����Ϸ�
	if (re1 & 8 && re2 & 8)
		re |= 8; //��
	else if (re1 & 2 && re2 & 2)
		re |= 2; //��

	if (re1 & 1 && re2 & 1)
		re |= 1; //��
	else if (re1 & 4 && re2 & 4)
		re |= 4; //��

	return re;
}
*/
BOOL CST_CurveCtrl::IsLineInCanvas(vector<LegendData>::iterator LegendIter, const POINT& p1, const POINT& p2)
{
//	if (IsLineOutdrop(p1, p2)) //�߶��ھ��ε�ĳһ�࣬�϶��޷�������ཻ
//		return FALSE;
	if (p1.y < CanvasRect[1].top && p2.y < CanvasRect[1].top ||
		p1.y > CanvasRect[1].bottom && p2.y > CanvasRect[1].bottom ||
		p1.x < CanvasRect[1].left && p2.x < CanvasRect[1].left ||
		p1.x > CanvasRect[1].right && p2.x > CanvasRect[1].right)
		return FALSE;
	else if (p1.x == p2.x || p1.y == p2.y) //ˮƽ�߶λ��ߴ�ֱ�߶Σ���ʱ�϶���������
		return TRUE;

	if (NullLegendIter == LegendIter || 1 != LegendIter->CurveMode && 2 != LegendIter->CurveMode) //ֻҪ���Ƿ������������ﴦ������������������
	{
		if (p1.y < CanvasRect[1].top) //����������б��ֱ��
		{
			auto x = p1.x + (p2.x - p1.x) * (CanvasRect[1].top - p1.y) / (p2.y - p1.y);
			if (CanvasRect[1].left <= x && x <= CanvasRect[1].right)
				return TRUE;
		}
		else if (p1.y > CanvasRect[1].bottom) //����������б��ֱ��
		{
			auto x = p1.x + (p2.x - p1.x) * (p1.y - CanvasRect[1].bottom) / (p1.y - p2.y);
			if (CanvasRect[1].left <= x && x <= CanvasRect[1].right)
				return TRUE;
		}
		else if (p1.x < CanvasRect[1].left) //����������б��ֱ��
		{
			auto y = p1.y + (p2.y - p1.y) * (CanvasRect[1].left - p1.x) / (p2.x - p1.x);
			if (CanvasRect[1].top <= y && y <= CanvasRect[1].bottom)
				return TRUE;
		}
		else //if (p1.x > CanvasRect[1].right) //����������б��ֱ��
		{
			auto y = p1.y + (p2.y - p1.y) * (p1.x - CanvasRect[1].right) / (p1.x - p2.x);
			if (CanvasRect[1].top <= y && y <= CanvasRect[1].bottom)
				return TRUE;
		}
		//����һ�ַ�����˼������ǣ�
		//���߶�p1p2�ͻ��������Խ����Ƿ��ཻ������ཻ�����߶�p1p2������ཻ�����ַ���ʹ����ʸ��������ʸ�����
		//ͨ����������߶�����һ��ԭʼ�߶���ʸ��������������
		//ʸ�������ʵ���ڵó�һ������������߶�1�����߶�2��˳ʱ�뷽������ʱ�뷽�򣬻����غϣ���ʱ���߶��ڷ����Ͽ�����Ȼ������180�ȣ���
		//���Ϸ������Խ����б�ľ��Σ�����Ч���Ѿ������ˣ������ǵ����ؼ�ʵ�ʵ����������������б�ģ������и��ŵ��㷨
		//ʸ�������������õ�����£�������10�Σ�������5�Σ��������£�������20�Σ�������10�Σ�ƽ��Ϊ��������15�Σ�������7.5��
		//�����ؼ�ʹ�õķ�����ÿ��ִ�е�������ȫ��ͬ��Ϊ�������ӣ�����4�Σ��ˣ���������2��
	}
	else //1���ȴ�ֱ��ˮƽ�ķ�����2����ˮƽ��ֱ�ķ���
	{
		POINT MidPoint;
		if (1 == LegendIter->CurveMode) //�ȴ�ֱ��ˮƽ�ķ���
		{
			MidPoint.x = p1.x;
			MidPoint.y = p2.y;
		}
		else //if (2 == LegendIter->CurveMode) //��ˮƽ��ֱ�ķ���
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
	if (bPart) //�ж�DataIter��������ǰһ��ͺ�һ�㣨����в��ҷ����طǶϵ�Ļ�����ɵ������Ƿ񣨲��֣��ɼ�
	{
//		if (IsPointVisible(DataListIter, DataIter, FALSE, FALSE))
		if (PtInRect(&rect, DataIter->ScrPos)) //�������ڻ����У�ֱ�ӷ�����
			return TRUE;

		auto pDataVector = DataListIter->pDataVector;
		ASSERT(begin(*pDataVector) <= DataIter && DataIter < end(*pDataVector));
		vector<MainData>::iterator DataIter2 = NullDataIter;
		if (2 & Mask && DataIter > begin(*pDataVector)) //����ǰ
		{
			DataIter2 = DataIter;
			--DataIter2;
			while (DataIter2 > begin(*pDataVector))
			{
				if (2 != DataIter2->State) //�������ص�
					break;
				--DataIter2;
			}

			if (PtInRect(&rect, DataIter2->ScrPos)) //�������ڻ����У�ֱ�ӷ�����
				return TRUE;

			//����DataIter2��DataIter�����뻭�����ཻ��
			if (IsLineInCanvas(DataListIter->LegendIter, DataIter2->ScrPos, DataIter->ScrPos))
				return TRUE;
		} //if (2 & Mask && DataIter > begin(*pDataVector))

		if (1 & Mask) //�����
		{
			DataIter2 = DataIter;
			++DataIter2;
			if (DataIter2 < end(*pDataVector))
			{
				while (DataIter2 < prev(end(*pDataVector)))
				{
					if (2 != DataIter2->State) //�������ص�
						break;
					++DataIter2;
				}

				if (PtInRect(&rect, DataIter2->ScrPos)) //�������ڻ����У�ֱ�ӷ�����
					return TRUE;

				//����DataIter��DataIter2�����뻭�����ཻ��
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

//VisibleState�ӵ�λ��
//1���Ƿ����ϻ�����ӵĵ㣨���жϸ�λ��
//2�����������겻��
//3�����ֺ����겻��
//4�����������������ٵ��ƶ�
//5���ں������������ٵ��ƶ�
//6��ֻ�е���һ���ڻ����пɼ�ʱ�����Զ��ƶ�����
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

	if (1 == nVisibleCurve && 1 == pDataVector->size()) //��һ���ɼ����ߵĵ�һ���㣬��ʼ������������ռ�ݵ�����
	{
		m_MinTime = m_MaxTime = Pos->Time;
		m_MinValue = m_MaxValue = Pos->Value;
		RightBottomPoint.ScrPos = LeftTopPoint.ScrPos = Pos->ScrPos;
		CHANGEMOUSESTATE;
	}
	else //nVisibleCurve > 1 || pDataVector->size() > 1
	{
		auto iter = InvalidCurveSet.find(DataListIter->Address);
		if (end(InvalidCurveSet) != iter) //����Ѿ�����ʵʱ������ӹ��㣬���ʱ����ֻͨ��Pos��һ����������λ�÷�Χ��Ϣ
		{
			//���ߴ�����Ҫ���£���Ȼ�ڻ���ʵʱ����ʱ��AddMainData2��������ߴ�����������Ч�ʻ��Щ��
			//�������߼�Ȼ����ӵ���InvalidCurveSet��˵��֮ǰ����ʵʱ���߻��ƹ�����ô
			//������Ȼ�����¸��£���ΪAddMainData2û������ȫ���жϣ���ʵʱ����ʱ��AddMainData2�����������ߴ�����
			UpdatePower(DataListIter);
			UpdateOneRange(DataListIter); //�����������ĵ��ô���ǳ���Ҫ�����ɵ���
			InvalidCurveSet.erase(iter);

			//������������λ�ü���Χ�����InvalidCurveSet���滹���������ߣ����������
			//���������������Ϊ����һЩ���ߵ�λ�ü���Χ��Ϣû�м������
			//������ĵ�����Ȼ���룬������������Ŀǰ����ˢ�¹������ߵ�λ�ü���Χ
			//ûˢ�µĲ�����Ҳ�ǶԵģ�������ʾ�ڻ�������������һ��
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

	if (DataListIter->Zx > nZLength * HSTEP) //������Z����棬������
		return;

	auto bCanLocalDraw = TRUE; //�Ƿ���Լ�ʱˢ��
	if (AutoRefresh) //����Ƿ�Ӧ��Ҫ�Զ�ˢ����
		if (AutoRefresh & 0x80000000) //��Ҫˢ����
		{
			AutoRefresh &= ~0x80000000; //ˢ���Ѿ������ˣ���λ
			bCanLocalDraw = FALSE; //������ʱˢ�£���Ϊ����Ҫ���Ʋ�ֹһ����
		}
		else
		{
			if (AutoRefresh & 0x7FFF0000) //��������Ϊ���
				if ((AutoRefresh & 0xFFFF) >= ((AutoRefresh & 0x7FFF0000) >> 16) - 1) //��ʱ�����ֽ������ۼƼ���
				{
					AutoRefresh &= 0x7FFF0000; //��λ�ۼƼ�����
					AutoRefresh |= 0x80000001; //û�����⣬�����ʼ��Ϊ1
				}
				else
					++AutoRefresh;

			return; //������ˢ��
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

	auto size = MakePointVisible(DataListIter, Pos, VisibleState); //�����ƶ�����ע�⣬���ܻ���size���������Ҳû�취
	if (size.cx || size.cy)
	{
		if (VisibleState & 2) //���������겻��
			size.cy = 0;

		if (VisibleState & 4) //���ֺ����겻��
			size.cx = 0;

		if (MoveCurve(size) || !IsPointVisible(DataListIter, Pos, TRUE, FALSE)) //�ǲ����ڻ����У�������ƣ�ע�⣺MakePointVisible�����ǲ����ڻ����е����⣩
		{
			UpdateRect(hFrceDC, PreviewRectMask);
			return;
		}
	}

	auto LegendIter = DataListIter->LegendIter;

	//���������Ҫ�ػ汾ҳ��
	//1�����ڻ��ƻ����������ߣ����Ի��ƻ�����������Ч���Ǻܲ��
	//2��������������м䣬����Ҫǿ����ʾ
	//3����ǰ���ڻ��Ƶ����߲������ϲ�
	//4��ǰ���Ѿ�ָʾ������ʱˢ�£�bCanLocalDraw������
	if (!bCanLocalDraw || NullLegendIter != LegendIter && 3 == LegendIter->CurveMode ||
		Pos > begin(*pDataVector) && Pos < prev(end(*pDataVector)) ||
		!CanCurveBeDrawnAlone(DataListIter))
		UpdateRect(hFrceDC, CanvasRectMask);
	else //�ܼ�ʱ����
	{
		//���ǵ�Ч�����⣬��ֱ�ӵ���UpdateRect��������Ϊ�����������Ҫ��DataListIter�������»���һ��
		if (m_ShowMode & 3)
			CHANGE_MAP_MODE(hFrceDC, m_ShowMode);

		if (Pos > begin(*pDataVector))
			--Pos; //���е����Posһ����������β��㣬ע��ǰ����жϣ�if (Pos > begin(*pDataVector) && Pos < prev(end(*pDataVector)))

		DrawCurve(hFrceDC, hScreenRgn, DataListIter, Pos); //������Ψһʹ��DrawCurve���������һ�������Ļ���

		//ˢ������
		RECT rect;
		rect.left = Pos->ScrPos.x;
		rect.top = Pos->ScrPos.y;
		if (pDataVector->size() > 1)
			++Pos;
		rect.right = Pos->ScrPos.x;
		rect.bottom = Pos->ScrPos.y;

		//������NormalizeRect���API��ֻ��CRect���У���������Ҫ�Լ�ʵ�־��εĹ��
		NormalizeRect(rect);

		auto nInflate = 1;
		if (NullLegendIter != DataListIter->LegendIter)
			nInflate += DataListIter->LegendIter->LineWidth;
		else //��ȵ���1
			++nInflate;
		InflateRect(&rect, nInflate, nInflate);

		//�����������䣬��Ҫ��չ���ε�������
		if (NullLegendIter != LegendIter && 255 != LegendIter->BrushStyle &&
			(LegendIter->BrushStyle < 127 || LegendIter->BrushStyle > 127 && (size_t) (LegendIter->BrushStyle - 128) < BitBmps.size()))
		{
			UINT Mask = DataListIter->FillDirection;
			if (Mask & 1) //�������
				rect.bottom = CanvasRect[1].bottom;
			if (Mask & 2) //�������
				rect.right = CanvasRect[1].right;
			if (Mask & 2) //�������
				rect.top = CanvasRect[1].top;
			if (Mask & 2) //�������
				rect.left = CanvasRect[1].left;
		}
		//ˢ������������

		if (m_ShowMode & 3)
		{
			CHANGE_MAP_MODE(hFrceDC, 0);
			MOVERECT(rect, m_ShowMode);
		}

		InvalidateControl(&rect, FALSE); //ˢ������Ļ��Ƶ���Ļ
		UpdateRect(hFrceDC, PreviewRectMask); //����ȫ��λ�ô��ڣ��ϸ���˵����Ҫ���µ������У�����������ɵķ�Χ�ı䣻����������ȫ��λ�ô������ص���
		//��������Ŀǰ�жϡ�����������ɵķ�Χ�ı䡱���Ѷȣ����ٻ���Ҫ���Ӷ���Ĵ洢�������Ըɴ�ֱ�ӵ���UpdateRect����
		//���ڡ�����������ȫ��λ�ô������ص�������жϣ��������������䣺IntersectRect(&rect, &rect, &PreviewRect)
	} //�ܼ�ʱ����
}

short CST_CurveCtrl::AddMainData(long Address, LPCTSTR pTime, float Value, short State, short VisibleState, BOOL bAddTrail)
{
	ASSERT(pTime);
	if (IsBadStringPtr(pTime, -1)) //��ָ��Ҳ����ȷ�ж�
		return 0;

	if (m_ShowMode & 0x80)
	{
		LPTSTR pEnd = nullptr;
		auto Time = _tcstod(pTime, &pEnd);
		if (HUGE_VAL == Time || -HUGE_VAL == Time)
			return 0;
		if (nullptr == pEnd || pTime == pEnd) //�����޷�����
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

//��������
//0-ˮƽ��Value��Ч
//1-��ֱ��Time��Ч
//���ͬһ���������ˮƽ�������д�ֱ���ߣ�������ͬ��Time Value����������߼���
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
		NoUse.LegendIter = FindLegend(Address, TRUE); //����ͼ������LegendIter

		InfiniteCurveArr.push_back(NoUse);
		iter = prev(end(InfiniteCurveArr));
	}

	iter->Time = Time;
	iter->Value = Value;
	//�����State����ͨ���߲�һ����ע����������ֻ��ȡ0��1���ֱ����ˮƽ�ʹ�ֱ��������
	//��ʵҲ�����൱���ж�Time��Value��һ��ֵ��Ч������ͬʱ��Ч��
	iter->State = Direction;

	//DataListHead��Ա
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

//State��ȡֵ��ʱ���£�
//���ֽڵ���һ������������0-255����
//0����ͨ�㣬��˼�����Ƿ�����״̬
//1���ϵ㣬������������ĵ�����
//2�����ظõ㣨ǰһ�㽫�ͺ�һ��ֱ�����ӣ��൱��û������㣩
//��2�ֽڰ�λ�㣺
//��1λ���ڵ���ʾ��ͼ���෴��ͼ�������ʾ������ʾ��ͼ���������ʾ������ʾ��
//�����Ժ��д���չ

//VisibleState�ӵ�λ��
//1���Ƿ����ϻ�����ӵĵ�
//2�����������겻�䣨�ڵ�1λΪ1���������Ч��
//3�����ֺ����겻�䣨�ڵ�1λΪ1���������Ч��
//4�����������������ٵ��ƶ����ڵ�1λΪ1���������Ч��
//5���ں������������ٵ��ƶ����ڵ�1λΪ1���������Ч��
//6��ֻ�е���һ���ڻ����пɼ�ʱ�����Զ��ƶ����ߣ��ڵ�1λΪ1���������Ч��

//���أ�0��ʧ�ܣ�1���ɹ��������Ѿ����ڣ���2���ɹ����������һ�����ߣ�
short CST_CurveCtrl::AddMainData2(long Address, HCOOR_TYPE Time, float Value, short State, short VisibleState, BOOL bAddTrail)
{
	if (!IsMainDataStateValidate(State) || ISHVALUEINVALID(Time))
		return 0;

	short re = 1;
	auto DataListIter = FindMainData(Address);
	if (NullDataListIter == DataListIter)
	{
		if (!(SysState & 0x1000)) //ȷ����׼ֵ
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
		NoUse.LegendIter = FindLegend(Address, TRUE); //����ͼ������LegendIter
		NoUse.FillDirection = 1; //�������
		NoUse.Power = 1;
		NoUse.Zx = NoUse.Zy = 0;
		NoUse.SelectedIndex = -1; //δѡ�е�

		if (ISCURVESHOWN((&NoUse))) //����ӵ�����һ�����ü�
			++nVisibleCurve;

		MainDataListArr.push_back(NoUse);
		DataListIter = prev(end(MainDataListArr));

		FIRE_CurveStateChange(Address, 1); //���߱����
		re = 2;
	}

	auto pDataVector = DataListIter->pDataVector;
	//�Ѿ������������еĵ㳬��MaxDotNum�������ϵ��������Ϊ�п�����;����SeMaxLength����
	if (MaxLength > 0 && pDataVector->size() >= (size_t) MaxLength)
	{
		auto CutNum = (long) (pDataVector->size() - (size_t) MaxLength + 1);
		auto LeftNum = CutNum % CutLength + MaxLength - 1;
		CutNum = CutNum - CutNum % CutLength;
		if (LeftNum >= MaxLength)
			CutNum += CutLength;
		if (CutNum)
		{
			FIRE_CurveStateChange(Address, 3); //���߱��ü�
			DelRange2(Address, 0, CutNum, FALSE, FALSE);
		}
	}

	MainData NoUse;
	NoUse.Time = Time;
	NoUse.Value = Value;
	CalcOriginDatumPoint(NoUse, 3, 0, 0, DataListIter);
	if (2 == re) //���ߵ�һ���㣬ΪLeftTopPoint��RightBottomPoint��ֵ
		DataListIter->LeftTopPoint = DataListIter->RightBottomPoint = NoUse;
	NoUse.AllState = State;

	vector<MainData>::iterator Pos = NullDataIter;
	if (pDataVector->empty()) //�����ߵĵ�һ����
	{
		if (1 == nVisibleCurve)
			VisibleState = 1; //��һ�����ߵĵ�һ���㣬��ʱ�����VisibleState��Ч�����︲�������㶨дΪ1
		bAddTrail = TRUE;
	}
	else if (!bAddTrail) //Ѱ�Ҳ���㣬��������bAddTrail��Ϊ�棬���Զ������߲�����������ж�����ĳ���
	{
		Pos = find_if(begin(*pDataVector), end(*pDataVector), bind2nd(greater_equal<HCOOR_TYPE>(), Time));

		if (Pos == end(*pDataVector)) //��ӵ����һ����
			bAddTrail = TRUE;
		else if (Pos->Time > Time)
			Pos = pDataVector->insert(Pos, NoUse); //��Posλ����insert��ʵ�����ǲ���Pos��ǰ��
		else //��ӵĵ��Ѵ���
		{
			if (Pos->Value != Value)
			{
				Pos->Value = Value; //����Ӧ��Ҫˢ��
				Pos->ScrPos = NoUse.ScrPos;
				UpdateRect(hFrceDC, CanvasRectMask);
			}
			return re;
		}
	}
	else if (VisibleState & 1 && 1 == DataListIter->Power && Time < pDataVector->back().Time) //��ʱpDataVector�����϶��ǿգ����Կ���ֱ�ӵ���back����
		DataListIter->Power = 2; //2�����ߣ�ע��������ǻ���ʵʱ���ߣ�����Ҫ�����ݴε����߽���������InvalidCurveSet�������棬�ȴ���������ʱ�ٸ���

	if (bAddTrail)
	{
		pDataVector->push_back(NoUse);
		Pos = prev(end(*pDataVector));
	}

	if (VisibleState & 1)
	{
		if (2 == Pos->State) //����ʵʱ����ʱ��������������ص�
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
	if (NumInterval) //�����ֽ��еĵ�15λ
	{
		if (NumInterval <= 1)
			return FALSE;

		AutoRefresh = (UINT) NumInterval << 16;
		++AutoRefresh; //û�����⣬�����ʼ��Ϊ1
		KillTimer(AUTOREFRESH);
	}
	else if (TimeInterval) //�����ֽ�
	{
		AutoRefresh = (USHORT) TimeInterval;
		SetTimer(AUTOREFRESH, AutoRefresh * 100, nullptr); //1/10��ת���ɺ���
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

	if (CurCurveIndex == distance(begin(MainDataListArr), DataListIter) && ISCURVEINPAGE(DataListIter, TRUE, FALSE)) //���ֿɼ�����
	{
		FIRE_SelectedCurveChange(0x7fffffff);
		CurCurveIndex = -1; //��ѡ�е�δѡ��
	}
	else
	{
		if (SysState & 0x2000) //��CurCurveIndex���߷����������������棬������������������Ҳ�ͳ�������ǰ
		{
			auto j = prev(end(MainDataListArr));
			if (DataListIter < j)
				swap(*DataListIter, *j); //��������DataListHead�����ݣ�������ָ��
			CurCurveIndex = MainDataListArr.size() - 1;

			ASSERT(CurCurveIndex < MainDataListArr.size());
			DataListIter = next(begin(MainDataListArr), CurCurveIndex); //����ȡһ�ε��������Է���һ
		}
		else //����DataListIter���ƶ����ߣ������ı�CurCurveIndex��ֵ����Ϊ���ߵ�λ�ò�����ı�
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

BOOL CST_CurveCtrl::SelectLegendFromIndex(size_t nIndex) //��0��ʼ�����
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
	else //���ߴ�������״̬�����Ƚ�����ʾ��������ѡ����
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
			BOOL bState = CurCurveIndex == distance(begin(MainDataListArr), DataListIter) && ISCURVEINPAGE(DataListIter, TRUE, FALSE); //���ֿɼ�����
			if (bState != bSelect)
				ChangeSelectState(DataListIter);

			return TRUE;
		}
		else //�����ص�������ʾ����
		{
			ShowCurve(Address, TRUE);
			return SelectCurve(Address, bSelect);
		}

	return FALSE;
}

void CST_CurveCtrl::EnableSelectCurve(BOOL bEnable)
{
	bEnable <<= 3;
	if ((SysState ^ bEnable) & 8) //��4λ���б仯
	{
		SysState &= ~8;
		SysState |= bEnable;
	}

	if (!bEnable && -1 != CurCurveIndex) //ȡ���Ѿ�ѡ�е�����
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
		if (nVisibleCurve < OldVisibleCurve || !OldVisibleCurve) //�������������ˣ����ߴ��޵���
			ReSetCurvePosition(2, TRUE);
		UpdateMask |= CanvasRectMask;

		auto c = OldVisibleCurve + nVisibleCurve; //OldVisibleCurve �� nVisibleCurve ������ͬʱΪ0
		if (c == OldVisibleCurve || c == nVisibleCurve) //���޵��л��ߴ��е���
			CHANGEMOUSESTATE;
	}

	UpdateRect(hFrceDC, UpdateMask);
}

BOOL CST_CurveCtrl::ShowLegend(LPCTSTR pSign, BOOL bShow) //�൱����ͼ���ϵ������Ҽ�
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

	SysState |= 0x40000000; //��31λ��ֹDrawCurve�����ƶ����ߣ��ڴ�ֱ�����ϣ�
	while (size.cx || size.cy) //size���ľ�����long�͵ģ�����MoveCurveֻ֧��short�ͣ�����Ҫ��ѭ����MoveCurve
	{
		short xStep = size.cx > 0x7FFF ? 0x7FFF : (size.cx < (short) 0x8000 ? 0x8000 : (short) size.cx);
		size.cx -= xStep;
		short yStep = size.cy > 0x7FFF ? 0x7FFF : (size.cy < (short) 0x8000 ? 0x8000 : (short) size.cy);
		size.cy -= yStep;
		re |= MoveCurve(xStep, yStep, !size.cx && !size.cy, FALSE); //ֻ�����һ��ˢ��
		//���ü���ƶ��ĺϷ��ԣ��϶��Ϸ���������Ϸ���Ҳ����Ϊsize����ˣ���ʱҲ�ް취���ȣ��������ô�����ֳ���
	}
	SysState &= ~0x40000000;

	return re;
}

//xStep����0ʱ�������ƣ�yStep����0ʱ��������
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
				xStep = 0; //�ǳ���Ҫ
			if (re & 2)
			{
				OriginPoint.Value -= yStep * VCoorData.fCurStep;
				FIRE_BeginValueChange(OriginPoint.Value);
			}
			else
				yStep = 0; //�ǳ���Ҫ
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

			SysState &= ~0x8000; //����������m_CurActualPoint�����ֵ����Ϊ��Ч
		} //if (re)
	} //if (nVisibleCurve > 0 && (xStep || yStep))

	return re;
}

BOOL CST_CurveCtrl::OnMouseWheelZoom(int zDelta) {return SysState & 0x400 && SetZoom(Zoom + zDelta);}
BOOL CST_CurveCtrl::OnMouseWheelHZoom(int zDelta) {return SysState & 0x10 && SetHZoom(HZoom + zDelta);} //ˮƽ����ģʽ��������
BOOL CST_CurveCtrl::OnMouseWheelHMove(int zDelta)
{
	if (m_MoveMode & 1) //ˮƽ�ƶ�ģʽ����ˮƽ�ƶ�
	{
		auto hd = m_ShowMode & 1 ? -1 : 1;
		return MoveCurve(hd * zDelta, 0); //��Ҫ��ˮƽ�����ϼ���ƶ��ĺϷ���
	}

	return FALSE;
}

BOOL CST_CurveCtrl::OnMouseWheelVMove(int zDelta)
{
	if (m_MoveMode & 2) //��ֱ�ƶ�ģʽ���ɴ�ֱ�ƶ�
	{
		auto vd = (m_ShowMode & 3) < 2 ? -1 : 1;
		return MoveCurve(0, vd * zDelta); //��Ҫ�ڴ�ֱ�����ϼ���ƶ��ĺϷ���
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
				if (wParam & MK_CONTROL) //ˮƽ�ƶ����
					spoint.x = ScrPos.x;
				else //��ֱ�ƶ����
					spoint.y = ScrPos.y;

				if (memcmp(&ScrPos, &spoint, sizeof POINT))
				{
					SYNA_TO_TRUE_POINT;
				}
			}
			else //�����ƶ����
				spoint = ScrPos;
		//WM_LBUTTONDOWN WM_LBUTTONUP WM_RBUTTONUP
		else if (SysState & 0x20 || !(SysState & 0x100)) //��Ҫ��ͬ��
			spoint = ScrPos;
		else if (SysState & 0x40) //��Ҫͬ��
		{
			SYNA_TO_TRUE_POINT;
		}

		SysState &= ~0x60; //spoint�뵱ǰ�����������ͬ����
	} //if (WM_MOUSEMOVE <= message && message <= WM_LBUTTONUP || WM_RBUTTONUP == message)
	else if ((WM_SYSKEYUP == message || WM_SYSKEYDOWN == message) && VK_MENU == wParam)
	{
		//�������ˮƽ���ţ���������alt��Ϣ�������Ͳ��������˵���ȥ��
		//���ﴦ����Ӱ�쵽GetAsyncKeyState���õĽ��
		if (SysState & 0x10)
			return 0;

		//����alt��֮�󣬿ؼ�����δ��ʧ����ȴ�ղ�������ƶ���Ϣ�����Ե��´�
		//��������Ҽ�֮��һ��Ҫ��spointͬ��һ�£�������ƶ����ߣ�2010.11.27��

		//������㲻�ڿؼ�֮�ڣ�������ƶ����ؼ�֮�ڣ�����alt������ʱҲ�������ȫ��ͬ��BUG��
		//���ڿؼ�û�н��㣬�ղ���VK_MENU��Ϣ�����Բ�������һ�ַ�ʽ����������⣬�ο�����ķ�ͬ��
		if (WM_SYSKEYUP == message)
			SysState |= 0x20;
	}

	switch (message)
	{
//	case VMOVE:
//		ReSetCurvePosition(4, TRUE);
//		break;
	case WM_VSCROLL: //����û�й�����������Ϣ�����յ�
		SCROLLCURVE(2, SB_LINEUP, SB_LINEDOWN, 0, 1);
		break;
	case WM_HSCROLL: //����û�й�����������Ϣ�����յ�
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
				if (GetAsyncKeyState(VK_MENU) & 0x8000) //����������⣬��ΪWM_MOUSEWHEEL��Ϣ��������alt����Ϣ
					re = (this->*OnMouseWheelFun[1])(zDelta);

				if (!re)
				{
					if (MK_CONTROL & wParam)
						re = (this->*OnMouseWheelFun[2])(zDelta);

					if (!re)
						re = (this->*OnMouseWheelFun[3])(zDelta);
				} //if (!re)
			} //if (!re)

			if (re) //���Բ�����������Ϣ
				return 0;
		} //if (nVisibleCurve > 0 && MouseWheelSpeed > 0)
		break;
	case WM_SETFOCUS: //if ((HWND) wParam != m_hWnd) ����Ҫ�жϣ���Ϊ���ؼ�û�������ӿؼ�
		if (SysState & 0x400000)
		{
			SysState |= 0x100;
			REFRESHFOCUS(auto hDC = ::GetDC(m_hWnd), ::ReleaseDC(m_hWnd, hDC)); //����Ҫ�õ���ʾЧ�������Ի�ȡ��ĻDC
		}
		break;
	case WM_KILLFOCUS:
		if (SysState & 0x100)
		{
			SysState &= ~0x100;
			REFRESHFOCUS(auto hDC = ::GetDC(m_hWnd), ::ReleaseDC(m_hWnd, hDC)); //����Ҫ�õ���ʾЧ�������Ի�ȡ��ĻDC
		}
		break;
	case WM_SYSCOLORCHANGE:
		InitFrce(TRUE); //InitFrce��Ҫ��DrawBkgǰ�����
		DrawBkg(TRUE);
		SysState |= 0x200;
		break;
	case WM_SIZE:
		ReSetUIPosition((int) (lParam & 0xFFFF), (int) (lParam >> 16));
		break;
	case WM_WINDOWPOSCHANGED:
		//���ؼ�������IE��ʱ���������ڽ�����ˢ�����⣬����΢������һ��BUG�������������⣬�������˴��ڵ���˸
		//���ֻ��Ӧ�ó�����ʹ���򲻴���������⡣
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
					hNewCursor = nullptr; //�������
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

		/*�Ѿ�������Ӵ˹��ܣ���Ϊ���ο�������ʵ�ִ˹��ܽ����ӵ�������
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

		//������ű����
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
				auto hDC = ::GetDC(m_hWnd); //����Ҫ�õ���ʾЧ�������Ի�ȡ��ĻDC
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
					if (nNum != m_BE->nWidth) //�Ѿ�����ˣ�����pFileNameΪ��c:\****.bmp������ʱnFileNum�Ѿ�����4λ���ˣ�����10001
					{
						m_BE->nFileNum = 0; //������ʱ����ͼƬ
						break;
					}

					memcpy(m_BE->pStart, StrBuff, m_BE->nWidth * sizeof(TCHAR));
					if (_taccess(m_BE->pFileName, 0)) //�ļ�������
						break;
				}

				if (m_BE->nFileNum)
				{
					::ExportImage(hFrceBmp, m_BE->pFileName);
					FIRE_BatchExportImageChange((long) m_BE->nFileNum);
				}
				else //������ʱ����ͼƬ�����ͷ�m_BE���������������Ҫ������ʱ����ͼƬ��1.�ļ�����������2.�ļ�������������UINT�������
					BatchExportImage(0, 0); //������ʱ����ͼƬ�������ͷ�m_BE
			}
			break;
		case REPORTPAGE:
			KillTimer(REPORTPAGE);
			ReportPageInfo();
			break;
		case HIDEHELPTIP:
			EnableHelpTip(FALSE); //EnableHelpTip��ɱ��HIDEHELPTIP��ʱ��
			break;
		case HIDECOPYRIGHTINFO:
			break;
		case AUTOREFRESH:
			AutoRefresh |= 0x80000000; //�´ο����Զ�ˢ����
			break;
		} //switch (wParam)
		break;
	case WM_MOUSEMOVE:
		if (DRAGMODE == MouseMoveMode)
		{
			MOVEWITHMOUS(m_MoveMode & 4 && m_MoveMode & 3); //�����϶�ģʽ���ҿ��ƶ�
			break;
		}

		if (IsCursorNotInCanvas(spoint))
		{
			if (MouseMoveMode & 0xFF) //���ɼ�����������Ϊ0ʱ����Ȼ��������״̬
				MouseMoveMode <<= 8;

			//WM_MOUSEMOVE��Ϣ����Ƶ�ʺܸߣ�����жϷ�ֹ��û�����ߵ�ʱ��ͣ�ĵ���DrawAcrossLine���������ú�����ʲô�������ķ���
			if (-1 != LastMousePoint.x && !(m_MoveMode & 0x80))
			{
				POINT EmptyPoint = {-1};
				DrawAcrossLine(&EmptyPoint); //���ʮ�ֽ�����
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

			if (SysState & 0x4000 && !(wParam & (MK_CONTROL | MK_SHIFT))) //��������
			{
				RECT SorptionRect;
				if (SysState & 0x8000) //�Ѿ�����
				{
					GetSorptionRect(SorptionRect, LastMousePoint.x, LastMousePoint.y, SorptionRange);
					if (PtInRect(&SorptionRect, spoint)) //��Ȼ��������Χ֮��
					{
						if (memcmp(&LastMousePoint, &spoint, sizeof POINT))
						{
							SysState |= 0x40; //spoint�뵱ǰ��������겻ͬ��
							spoint = LastMousePoint;
						}

						break; //�����˺����DrawAcrossLine����
					}
					else
					{
						SysState &= ~0x8000; //����������m_CurActualPoint�����ֵ����Ϊ��Ч
						FIRE_SorptionChange(m_CurActualAddress, -1, 0);
					}
				}

				//�ж�spoint��������Χ��û�п��������������
				for (auto i = begin(MainDataListArr); i < end(MainDataListArr); ++i)
				{
					if (!ISCURVESHOWN(i) || i->Zx > nZLength * HSTEP)
						continue;

					auto nIndex = DoGetPointFromScreenPoint(i, spoint.x, spoint.y, SorptionRange);
					if (-1 != nIndex)
					{
						SysState |= 0x8000; //�µ�������m_CurActualPoint�����ֵ����Ϊ��Ч

						//��д��m_CurActualPoint��������ShwoToolTipʱ�������Ч�ʣ�����ʾ������ʾҲ����ûʲôЧ����ʧ
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
				} //�������пɼ�����
			} //if (SysState & 0x4000 && !(wParam & (MK_CONTROL | MK_SHIFT)))

			DrawAcrossLine(&spoint); //�ƶ�ʮ�ֽ�����
		}
		break;
	case WM_LBUTTONDOWN:
//		if (!(SysState & 0x100)) //2012.8.20 �ƺ��ɸ���ʵ���ˣ����յ�WM_SETFOCUS��Ϣ��
//			SetFocus();
		if (!(SysState & 0x200000) && PtInRect(&PreviewHotspotRect, spoint))
			EnablePreview(!(SysState & 0x80000000));
		else if (SysState & 0x80000000 && PtInRect(&PreviewRect, spoint)) //�����ȫ��λ������
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
				DrawAcrossLine(&EmptyPoint); //���ʮ�ֽ����ߣ�WM_LBUTTONDOWN�ķ���Ƶ�ʺ��٣����Ե���DrawAcrossLine����֮ǰ�������ж��Ƿ��б�Ҫ�������Ƿ���ʾΪ���Σ�
				BeginMovePoint = spoint;
				PostMessage(WM_SETCURSOR, (WPARAM) m_hWnd, HTCLIENT | WM_MOUSEMOVE);
//				SetCapture(); //2012.8.20 �ƺ��Ѿ��ɸ���ʵ����
			}
		}
		break;
	case WM_LBUTTONUP:
		if (DRAGMODE == MouseMoveMode) //�����϶�����
		{
			MOVEWITHMOUS(!(m_MoveMode & 4) && m_MoveMode & 3) //�����ƶ�ģʽ���ҿ��ƶ�

			if (PtInRect(CanvasRect, spoint))
				MouseMoveMode = MOVEMODE;
			else
				MouseMoveMode = 0;
			PostMessage(WM_SETCURSOR, (WPARAM) m_hWnd, HTCLIENT | WM_MOUSEMOVE);
			ReleaseCapture();
		}
		else if (nVisibleCurve > 0 && (ZOOMIN == MouseMoveMode || ZOOMOUT == MouseMoveMode) && PtInRect(CanvasRect, spoint)) //��������
		{
			if (DoFixedZoom(spoint))
				DrawAcrossLine(&spoint); //�ָ�ʮ�ֽ�����
		}
		else if (!(SysState & 0x200000) && PtInRect(LegendRect, spoint)) //ͨ����ͼ���ϵ��������������ߵ���ʾ״̬
			SelectLegendFromIndex(GetLegendIndex(spoint.y));
		break;
//	case WM_RBUTTONDOWN: //2012.8.20 ���ձ�׼��Ϊ������¼������ý���
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
					if (hBuddyServer) //�Ѿ����������������������������ȡ��
						::SendMessage(hBuddyServer, BUDDYMSG, 1, (LPARAM) m_hWnd);

					hBuddyServer = (HWND) lParam;
				}
				break;
			case 1:
				if (hBuddyServer) //��������������ȡ��
				{
					hBuddyServer = nullptr;
					CHLeftSpace(ActualLeftSpace); //�����Լ���LeftSpace
				}
				else if (lParam) //�����ͻ�������ȡ����������������LeftSpace
					REMOVEBUDDY((HWND) lParam);
				break;
			case 2: //���õ�ǰҳ��ʼʱ��
				SetBeginTime2(*(HCOOR_TYPE*) lParam);
				break;
			case 3: //����ʱ����
				SetTimeSpan(*(double*) lParam);
				break;
			case 4: //���÷Ŵ���
				SetZoom((short) lParam);
				break;
			case 5: //�ƶ�������
				CHLeftSpace((short) lParam);
				break;
			case 6: //��ѯ��СLeftSpace
				re = ActualLeftSpace;
				break;
			case 7: //�ͻ�����������ȷ��LeftSpace
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
			case 8: //����ˮƽ�Ŵ���
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
	//���ڿ��ܰ�װ�в�������Կ��ܻ᲻��Ҫ��ʾ�����ؿ��ַ�����
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

	//��tooltip����Ӧ����
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
	SelectClipRgn(hDC, hScreenRgn); //ע������ѡ�������û�л�ԭ������hDCӦ��ÿ�ζ���һ���µ�DC��ͨ��GetDC��ȡ��������һ���ڴ����DC��
	BitBlt(hDC, ToolTipRect.left + 1, ToolTipRect.top + 1, ToolTipRect.right - ToolTipRect.left - 1, ToolTipRect.bottom - ToolTipRect.top - 1,
		hBackDC, ToolTipRect.left + 1, ToolTipRect.top + 1, SRCCOPY);
	DrawText(hDC, pToolTip, -1, &InvalidRect, 0);

	SelectObject(hDC, hAxisPen); //ʹ�û���������ʱ�Ļ���
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
		auto hDC = ::GetDC(m_hWnd); //����Ҫ�õ���ʾЧ��������Ҫ���ϻ�ȡDC
		if (LastMousePoint.x > -1 && (LastMousePoint.x != pPoint->x || LastMousePoint.y != pPoint->y)) //Ĩ���ϵ�ʮ�ֽ�����
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
				//����Ĵ��룬��win7֮ǰ��vista��û�Թ�����ϵͳ�£��������ã�����win7�£����Ե�Ч�ʼ�����£����ֽ�Ϊ���ص���˸������ԭ��δ��
				if (LastMousePoint.y != pPoint->y)
					BitBlt(hDC, CanvasRect->left, LastMousePoint.y, CanvasRect->right - CanvasRect->left, 1,
						hFrceDC, CanvasRect->left, LastMousePoint.y, SRCCOPY); //Ĩˮƽ��
				if (LastMousePoint.x != pPoint->x)
					BitBlt(hDC, LastMousePoint.x, CanvasRect->top, 1, CanvasRect->bottom - CanvasRect->top,
						hFrceDC, LastMousePoint.x, CanvasRect->top, SRCCOPY); //Ĩ��ֱ��

				if (SysState & 0x800)
				{
					BitBlt(hDC, ToolTipRect.left, ToolTipRect.top, ToolTipRect.right - ToolTipRect.left + 1, ToolTipRect.bottom - ToolTipRect.top + 1,
						hFrceDC, ToolTipRect.left, ToolTipRect.top, SRCCOPY); //Ĩ������ʾ
					SysState &= ~0x800;
				}
			}
		}
		LastMousePoint.x = pPoint->x;
		LastMousePoint.y = pPoint->y;
		if (LastMousePoint.x > -1) //�����µ�ʮ�ֽ�����
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
	SelectObject(hDC, hAxisPen); //ʹ�û���������ʱ�Ļ���
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
		LineTo(hDC, xEnd, LastMousePoint.y); //��ˮƽ��
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
		LineTo(hDC, LastMousePoint.x, yEnd); //����ֱ��
	}

	if (bCheckBound && SysState & 0x800) //ֻ����OnDraw�������RepairAcrossLine����ʱbCheckBound��Ϊ�棬����bCheckBound�ȴ���Ҫ�������λ�ã�Ҳ����Ҫ������ʾtooltip
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
	auto re = MoveCurve(-RelativePage * (HCoorData.nScales - nZLength), 0, bUpdate); //���ˮƽ�ƶ��Ϸ���
	if (re)
	{
		JumpPages += RelativePage;
		if (!ReSetCurvePosition(4, bUpdate))
			JumpPages += GotoPage(RelativePage > 0 ? 1 : -1, bUpdate);
	}

	return JumpPages;
}

//Flag�ӵ�λ��
//��һλ���Ƿ��ӡҳ��ţ�
//�ڶ�λ���Ƿ��̨��ӡ����ʱ��ȡĬ�ϴ�ӡ����Ĭ�ϴ�ӡ������ӡ����Ϊ����һ���ӡĬ�������ӡ��
//����λ���Ƿ�ǿ�д�ֱ���У������ǿ�д�ֱ���У�������Ҫʱ��ֱ����
//����λ���Ƿ񱣳ֱ�ҳ�����꣬���������λͬʱΪ1�������λ��Ч
//����λ�����������λһ������ֻ�ڴ�ӡ��һҳʱ��Ч�����仰˵������λֻ�ڴ�ӡ�ǵ�һҳʱ��Ч
//����λ�����������λһ������ֻ�ڴ�ӡ��һҳʱ��Ч�����仰˵������λֻ�ڴ�ӡ�ǵ�һҳʱ��Ч
//����λ�����Ϊ1����λͼ��ʽ��ӡǰ�����ڻ���ƽ������ʱ�������ӡ����֧�֣����Բ������ַ�ʽ��
//		���ַ�ʽ�ŵ��ǽ����ƽ�����ߵĴ�ӡ���⣬ȱ���ǻ���ֲڣ�

//����ֵ��0-�ɹ���-1-�ɹ����������ݿɴ�ӡ��������û���ҵ����߻����������أ�
//1-��ӡʧ�ܣ�2-������Ч��3-�û�ȡ����ӡ��4-��ӡ���򲻴��ڣ�5-������Ĭ�ϴ�ӡ����6-��ӡ����֧�ְ�λͼ��ӡ
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
	//�߾඼��ָ��ӡ������
	if (LeftMargin < 0 || TopMargin < 0 || RightMargin < 0 || BottomMargin < 0)
		return 2; //������Ч

	vector<DataListHead<MainData>>::iterator DataListIter = NullDataListIter;
	if (!bAll)
	{
		DataListIter = FindMainData(Address);
		if (NullDataListIter == DataListIter || !ISCURVESHOWN(DataListIter) || DataListIter->Zx > nZLength * HSTEP)
			return -1; //������
	}

	auto MinTime = !(Mask & 1) || BTime <= m_MinTime ? m_MinTime : BTime;
	MinTime = GetNearFrontPos(MinTime, OriginPoint.Time);
	auto MaxTime = !(Mask & 2) ||  ETime >= m_MaxTime ? m_MaxTime : ETime;

	int nPageNum;
	GETPAGENUM(MinTime, MaxTime);
	if (!nPageNum)
		return -1; //������

	short re = 3; //�û�ȡ����ӡ
	CPrintDialog SetupDlg(!(Flag & 2), PD_RETURNDC);
	if (Flag & 2)
		if (!SetupDlg.GetDefaults())
			return 5; //û��Ĭ�ϴ�ӡ��
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
		pDevMode->dmOrientation = /*DMORIENT_PORTRAIT*/DMORIENT_LANDSCAPE; //Ĭ�Ϻ����ӡ
		pDevMode->dmPaperSize = DMPAPER_A4; //Ĭ�ϴ�ӡA4ֽ
		pDevMode->dmFields = DM_ORIENTATION | DM_PAPERSIZE;
		::GlobalUnlock(SetupDlg.m_pd.hDevMode);
	}

	if (Flag & 2 || SetupDlg.DoModal() == IDOK)
	{
		auto hPrintDC = SetupDlg.m_pd.hDC;

		auto RC = GetDeviceCaps(hPrintDC, RASTERCAPS);
		if (Flag & 0x40 && !(RC & RC_BITMAP64 && RC & RC_STRETCHDIB)) //��λͼ��ӡǰ��
			re = 6; //��֧��λͼ��ӡ
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
				if (!(Flag & 0x40)) //��λͼ��ӡʱ���ؼ��в���Ϊ�Ǵ�ӡ
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
						if (PrintDlg.PrintAll()) //��ӡȫ��ҳ
							PageTo |= 0x8000;
						else if (PrintDlg.PrintRange()) //��ӡһ����ҳ
						{
							PageFrom = PrintDlg.GetFromPage();
							PageTo = PrintDlg.GetToPage();
						}
						else if (PrintDlg.PrintSelection())
							PageFrom = PageTo = 0; //��ӡ��ǰҳ

						DELETEDC(PrintDlg.m_pd.hDC);
						if (PrintDlg.m_pd.hDevMode)
							::GlobalFree(PrintDlg.m_pd.hDevMode);
						if (PrintDlg.m_pd.hDevNames)
							::GlobalFree(PrintDlg.m_pd.hDevNames);
					}

					auto OrgX = (int) (RateX * LeftMargin), OrgY = (int) (RateY * TopMargin);

					::SetBkMode(hPrintDC, TRANSPARENT);
					if (!(m_ShowMode & 3) || Flag & 0x40) //����������£�һ����ӳ�䣬���治��ӳ������
						CHANGE_PRINT_MAP_MODE(hPrintDC, ViewWidth, ViewHeight, OrgX, OrgY, 0);

					re = !DoPrintCurve(hPrintDC,
									DataListIter,
									MinTime,
									IsBadStringPtr(pTitle, -1) ? CurveTitle : pTitle, //���ɶ�ʱ����ȡ�Լ��ı�������ӡ
									IsBadStringPtr(pFootNote, -1) ? FootNote : pFootNote, //���ɶ�ʱ����ȡ�Լ��Ľ�ע����ӡ
									ViewWidth,
									ViewHeight,
									OrgX,
									OrgY,
									PageFrom,
									PageTo,
									Flag);
				} //if (Flag & 2 || dlg.DoModal() == IDOK)

				ReSetUIPosition(cx, cy);
				if (!(Flag & 0x40)) //��λͼ��ӡʱ���ؼ��в���Ϊ�Ǵ�ӡ
					SysState &= ~1;
				SetRedraw();
				UpdateRect(hFrceDC, AllRectMask);
			} //if (PrintWinWidth > 0 && PrintWinHeight > 0)
			else
				re = 4; //��ӡ���򲻴���
		} //֧��λͼ��ӡ

		DELETEDC(hPrintDC);
	} //if (Flag & 2 || dlg.DoModal() == IDOK)
	::GlobalFree(SetupDlg.m_pd.hDevMode);
	if (SetupDlg.m_pd.hDevNames)
		::GlobalFree(SetupDlg.m_pd.hDevNames);

	return re;
}

//���������ߴ�ӡ������nIndexInMainDataListΪ��MainDataList�����е�λ�ã���0��ʼ�����Ϊ-1�����ӡȫ������
BOOL CST_CurveCtrl::DoPrintCurve(HDC hPrintDC,
								 vector<DataListHead<MainData>>::iterator DataListIter,
								 HCOOR_TYPE BeginTime,
								 LPCTSTR pTitle,
								 LPCTSTR pFootNote,
								 int ViewWidth,
								 int ViewHeight,
								 int OrgX,
								 int OrgY,
								 WORD PageFrom, //��ʼҳ
								 WORD PageTo,   //����ҳ��������߶�Ϊ0�����ӡ��ǰҳ�����λΪ1ʱ��ӡȫ��ҳ
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
		hFont = CreateFontIndirect(&f); //��������
	f.lfHeight = -14; //�����
	hGeneralFont = CreateFontIndirect(&f); //��������Ǵ�ӡʱ���еģ�ר�����ڴ�ӡ��ע
	f.lfWeight = 700;
	f.lfHeight = -19; //�ĺź���
	hHeadFont = CreateFontIndirect(&f); //��������Ǵ�ӡʱ���еģ�ר�����ڴ�ӡ�����
	if (!(Flag & 0x40))
	{
		GetObject(this->hTitleFont, sizeof(LOGFONT), &f);
		hTitleFont = CreateFontIndirect(&f);
	}

	BOOL bPrintAllPage = PageTo & 0x8000;
	PageTo &= 0x7FFF;

	WORD TotalPage = PageTo - PageFrom + 1;
	auto OldBeginValue = OriginPoint.Value; //���浱ǰҳ�����꿪ʼֵ
	HCOOR_TYPE OldBeginTime;
	if (PageFrom > 0)
	{
		OldBeginTime = OriginPoint.Time; //���浱ǰҳ��ʼʱ��
		auto NewBeginTime = GETPAGESTARTTIME(BeginTime, PageFrom); //��ӡ��ʼʱ��
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
	if (!(Flag & 0x40) && RC & RC_BITMAP64 && RC & RC_STRETCHDIB) //��λͼ��ӡʱ�������ٴ�ӡ����
		lpbi = GetDIBFromDDB(hDC, hBackBmp);
	auto dwPaletteSize = !lpbi || lpbi->bmiHeader.biBitCount > 8 ? 0 : sizeof(RGBQUAD) * ((1 << lpbi->bmiHeader.biBitCount) - 1);

	auto titleColor = m_titleColor;
	if (IsColorsSimilar(titleColor, 0xffffff)) //��ӡֽ�ǰ�ɫ�ģ�������ɫ���ƣ���ȡ��
		titleColor = ~titleColor & 0xffffff;
	auto footNoteColor = m_footNoteColor;
	if (IsColorsSimilar(footNoteColor, 0xffffff)) //��ӡֽ�ǰ�ɫ�ģ�������ɫ���ƣ���ȡ��
		footNoteColor = ~footNoteColor & 0xffffff;

	auto re = TRUE;
	auto nPrintedPage = 0; //�Ѿ���ӡ��ҳ��
	auto bNeedCHPage = FALSE;
	UINT OpStyle = 4 | (Flag >> 4);
	for (auto i = 0; i < TotalPage || bPrintAllPage; ++i) //����ӡȫ��ҳ��ʱ��һֱ��ӡ�����ܷ�ҳΪֹ
	{
		//ReSetCurvePosition�������ʵ����ƶ����ߣ��������ҳ�������������������GotoPage����ֵ���ж��Ƿǳ���Ҫ�ģ�����˵����
		//��ӡǰ��ҳ���ж�ʱ���õ��Ľ��Ϊ10ҳ�����ڴ�ӡ�����У���Ϊ�����˷�ҳ��ͬʱ��ReSetCurvePosition����ʱ���˵��ã�����
		//����ֻ�ܴ�ӡ��9ҳ������ʱGotoPage������0�����ŵ�ȻӦ�ý�����ӡ�ˣ����򽫽�����ѭ������Ȼ��Ҳ�������տ��Դ�ӡ��11ҳ
		//���������i < TotalPage || bPrintAllPage�����ж�����Ϊ�������������Ƶģ�Ҳ����˵����ӡȫ��ҳ��ʱ�򣬴�ӡ������
		//�����޷�������ҳ���������Ѵ�ӡ��TotalPageҳ����Ϊҳ���п���������
		while (bNeedCHPage || !ReSetCurvePosition(OpStyle, !!(Flag & 0x40), DataListIter)) //���ڰ�λͼ��ӡʱ������Ϊ�Ǵ�ӡ��������Ҫˢ��
		{
			auto PageStep = GotoPage(1, !!(Flag & 0x40)); //���ڰ�λͼ��ӡʱ������Ϊ�Ǵ�ӡ��������Ҫˢ��
			if (!PageStep)
				goto PRINTOVER;

			i += PageStep - 1;
			if (!bPrintAllPage && i >= TotalPage) //������ж��Ƿǳ��б�Ҫ�ģ���ΪGotoPage�п���һ�η���ҳ
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

		if (m_ShowMode & 3 && !(Flag & 0x40)) //��ӳ��Ϊ����ģʽ�����ڴ�ӡ���⼰��ע�����б�������λͼ��ӡʱҲ����ӳ��
			CHANGE_PRINT_MAP_MODE(hPrintDC, ViewWidth, ViewHeight, OrgX, OrgY, 0);

		//��ӡʱ������ͽ�ע���Ҫ��ӡ�Ļ����Ǵ�ӡ�ڿؼ���������֮��ģ�������UpdateRect�������棬�ڴ�ӡʱ���ǲ�����ִ��DrawCurveTitle��DrawFootNote������
		//���⣬����Ҳ������UpdateRect��������ȥ��ӡ������ͨ�������StretchDIBits����
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

		if (Flag & 0x40) //��λͼ��ӡǰ��
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
			continue; //��λͼ��ӡ�Ѿ��������ǳ����
		}
		else if (lpbi)
		{
			//����λͼ�����˸ı䣨�����������λ�ˣ�λͼ���κ��������Բ���䣬�����С����ɫ����Ϣ�ȣ���dwPaletteSize�������¼���
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

		if (m_ShowMode & 3) //ӳ��Ϊ��Ҫ��ģʽ
			CHANGE_PRINT_MAP_MODE(hPrintDC, ViewWidth, ViewHeight, OrgX, OrgY, m_ShowMode);

		//�����ѻ��ƣ�����ֻ��Ҫ����ǰ��
		SelectObject(hPrintDC, hFont); //��ӡʱ��������hFont��ͨ��
		UpdateRect(hPrintDC, PrintRectMask); //����ǰ�����������ߡ�����ͽ�ע��

		if (!EqualRect(&PrintRect, CanvasRect + 1)) //ֻ���ڻ����ı�����Ҫ���´�������
		{
			InvalidRect = PrintRect = CanvasRect[1];
			LPtoDP(hPrintDC, (LPPOINT) &InvalidRect, 2); //��Ϊ������LPtoDP����������ʹ��CanvasRect[0]����
			//����LPtoDP�󣬾��ο��ܳ��ַǹ��״̬������lift����right�ȣ�����HRGN���ں���ֻҪȷ�����δ�С��ͬ��������α����Σ��õ�����������ȵ�
			DELETEOBJECT(hPrintRgn);
			hPrintRgn = CreateRectRgnIndirect(&InvalidRect);
		}
		DrawCurve(hPrintDC, hPrintRgn, DataListIter);
		//�������ߣ�û�з���UpdateRect��������Ϊ��ӡʱ��Ҫ���������hPrintrgn����UpdateRect���޷����ʸñ�����
		//ͬ�����������������ĵ���Ҳû�з���UpdateRect���棬��Ϊ��һЩ��ӡ������UpdateRect�����޷����
		if (!(SysState & 0x200000)) //ȫ��ʱ����ӡ�ڲ�����ͽ�ע
		{
			//2012.8.5
			//CHVIEWORG����������ʲô���ã������ú����ʲô��
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
		SetBeginTime2(OldBeginTime); //�ָ���ǰҳ�Ŀ�ʼʱ��
	SetBeginValue(OldBeginValue); //�ָ���ǰҳ�������꿪ʼֵ

	return nPrintedPage + 1; //�����Ѵ�ӡ��ҳ����1��Ҳ����˵�������ӡ��ҳ��Ϊ0�Ļ����᷵��1������ɹ�
}

void CST_CurveCtrl::ReSetUIPosition(int cx, int cy)
{
	auto YOff = 0;

	if (WinWidth != cx || WinHeight != cy || !hFrceDC)
	{
		WinWidth = cx;
		WinHeight = cy;

		InitFrce(); //ǰ���!hFrceDC�ж��Ǳ���ģ���Ҫ��Ϊ�˽���Կվ��δ����ؼ�ʱ��û�м�ʱ����ǰ��DC��BUG
	}

	//��ʹ�Ǵ��ڴ�С��ȫû�иı䣬Ҳ�����ִ�����´��룬��Ϊ��ʾģʽ���ܸ���
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
		YOff = CanvasRect[1].bottom - CanvasBottom - 1; //��һ���룬��Ϊ��������䣺OffsetRect(CanvasRect + 1, 0, 1);
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
		YOff = CanvasRect[1].bottom - CanvasBottom - 1; //��һ���룬��Ϊ��������䣺OffsetRect(CanvasRect + 1, 0, 1);
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
//	OnActivateInPlace(TRUE, nullptr); //����ؼ�
	if (1 == InterlockedIncrement(&nRef)) //��һ������
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
		//����Ĵ�����ʵֻ��Ҫ����һ�Σ������ֻ����һ�Σ���û�뵽�취������ʹ�þ�̬����
		//Ҳ��Ӧ���и�ֻ����һ�ε��¼�������Щ����Ӧ�÷�������¼�����Ӧ��������
		//////////////////////////////////////////////////////////////////////////
		if (!hAxisPen) //��ide������һ���ؼ�ʱ����Ҫ���´���
			hAxisPen = CreatePen(PS_SOLID, 1, m_axisColor);
		SysState &= ~0x80040000; //���ģʽ�²���ʾ������ȫ��λ�ô���
		if (!hFont)
			iSetFont((HFONT) GetStockObject(DEFAULT_GUI_FONT));
		//////////////////////////////////////////////////////////////////////////

		//����һ��
		//���������г������������һ��lib�ļ�������ҪAtlHiMetricToPixel���������������Կ��ܻ��һЩ
//		ReSetUIPosition((int) (lpSizeL->cx * 96 / 2540), (int) (lpSizeL->cy * 96 / 2540));

		SIZEL PixelSizel;
		//��������
		//ʹ��AtlHiMetricToPixel����
		ATL::AtlHiMetricToPixel(lpSizeL, &PixelSizel);
		//��������
		//��AtlHiMetricToPixel������atlwin.h����ȡ�����������Ͳ����ٰ���atlwin.h���Խ����vc6����
		//����atlwin.h���ܴ����ı��������߾��棩
//		AtlHiMetricToPixel(lpSizeL, &PixelSizel);
		ReSetUIPosition(PixelSizel.cx, PixelSizel.cy);
	}

	return COleControl::OnSetExtent(lpSizeL);
}

void CST_CurveCtrl::OnResetState()
{
	// TODO: �ڴ����ר�ô����/����û���
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
����Ƿ�����ÿ�ʼֵΪfBeginValue������ֵ
0���ǿգ�1����ҳ��2���������߶��ڻ������棬3���������߶��ڻ�������
*/
UINT CST_CurveCtrl::CheckVPosition(float fBeginValue) //�������жϿ������������߶��Ƴ���������Ϊû�п��ǵ�Z�����Ӱ�죬DrawCurve���������������
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
����Ƿ�����ÿ�ʼʱ��ΪfBeginTime������ֵ
0���ǿգ�1����ҳ��2���������߶��ڻ������棬3���������߶��ڻ�������
*/
UINT CST_CurveCtrl::CheckHPosition(HCOOR_TYPE fBeginTime) //�������жϿ������������߶��Ƴ���������Ϊû�п��ǵ�Z�����Ӱ�죬DrawCurve���������������
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
	UINT Mask = 0; //�ȼ��費���ƶ�
	//��������Z�ᣬ���Բ��ò���ʧЧ�ʣ�����ʹ������������ռ���򣬶�����ÿһ��������ռ�������ж�
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

/*OpStyle�ӵ�λ��ʼ
��1λ��1��ǿ�д�ֱ����
��2λ��1�����ֱ�ҳ������
��3λ��1�����ֱ�ҳ������λ�ã������Ƿ����ø�λ�����������ᾡ�����ֺ����ֻ꣬�����޷����ֵ�ʱ���������Ż�������
����ֵ
��1λ��1����ҳ�ǿ�
��2λ��1��ˮƽ�������ƶ�������
��3λ��1����ֱ�������ƶ�������
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

		if (ISCURVEINPAGE(DataListIter, 2 & OpStyle, !(2 & OpStyle))) //����ֱ��ѡ��DataListIter����
			ThisCurveIter = DataListIter;
		else if (!(4 & OpStyle)) //�����ƶ�������
		{
			SetBeginTime2(GetNearFrontPos(DataListIter->LeftTopPoint.Time, OriginPoint.Time));
			re |= 2; //ˮƽ�������ƶ�������
			if (ISCURVEINPAGE(DataListIter, 2 & OpStyle, !(2 & OpStyle))) //����ֱ��ѡ��DataListIter����
				ThisCurveIter = DataListIter;
		}
	}
	else if (-1 != CurCurveIndex && ISCURVEINPAGE(next(begin(MainDataListArr), CurCurveIndex), TRUE, FALSE)) //��ѡ�е����ߣ��ҵ�ǰҳ���������ʾ������Ϊ��׼�ƶ�����
		ThisCurveIter = next(begin(MainDataListArr), CurCurveIndex);
	else
	{
		auto bFound = FALSE;
		for (auto i = begin(MainDataListArr); !bFound && i < end(MainDataListArr); ++i)
		{
			ThisCurveIter = i;
			if (ISCURVEVISIBLE(ThisCurveIter, 2 & OpStyle, !(2 & OpStyle))) //�ڵ�ǰҳ������ʾ�ĵ�һ�����ߣ���ѡ����
				bFound = TRUE;
		}

		if (!bFound) //��û���ҵ�����
			if (!(4 & OpStyle)) //�������ƶ������������£��ٴβ�������
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

				ASSERT(bFound); //һ�������ҵ�����Ϊǰ����DataListNum >= 0���ж�
			}
			else
				ThisCurveIter = NullDataListIter; //�Ҳ�������Ϊ�ƶ����ݵ�����
	}

	if (NullDataListIter != ThisCurveIter)
	{
		re |= 1;
		if (!(2 & OpStyle))
		{
			long MidValue = 0;
			auto Num = 0;
			auto pDataVector = ThisCurveIter->pDataVector;
			auto i = GetFirstVisiblePos(ThisCurveIter, FALSE, TRUE); //��ˮƽ���Ͽɼ�����
			if (NullDataIter != i)
				for (; i < end(*pDataVector); ++i) //���㵱ǰҳ��ƽ��ֵ
					if (IsPointVisible(ThisCurveIter, i, FALSE, TRUE)) //���ˮƽ����ֻ��Ҫ�ڻ����м���
					{
						MidValue += i->ScrPos.y; //���е㣨�������ص㣩��������ƽ��ֵ�ļ���
						++Num;
					}
					else if (1 == ThisCurveIter->Power) //1�����ߵ��ҵ�����֮��ĵ��ʱ��������˳�
						break;

			if (Num)
			{
				MidValue /= Num;
				if (1 & OpStyle || CanvasRect[1].top > MidValue || MidValue > CanvasRect[1].bottom)
				{
					auto yStep = (short) ((MidValue - (CanvasRect[1].bottom + CanvasRect[1].top) / 2) / VSTEP);
					if (yStep)
					{
						MoveCurve(0, yStep, bUpdate, FALSE); //���ü���ƶ��Ϸ���
						re |= 4;
					}
				}
			} //if (Num)
		} //if (!(2 & OpStyle))

		ReportPageChanges;
	} //if (NullDataListIter != ThisCurveIter)

	return re;
}

//ɾ���㣬����ֵ��λ�㣬�ӵ�λ��ʼ��3���Ƿ�ɾ����CurCurveIter��4���Ƿ�ɾ����DataListIter��������
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
		if (CurCurveIndex == distance(begin(MainDataListArr), DataListIter)) //��ǰѡ�е����߱�ɾ��������ɾ������������Ч
		{
			FIRE_SelectedCurveChange(0x7fffffff);
			CurCurveIndex = -1;
			re |= 4;
		}
		FIRE_CurveStateChange(DataListIter->Address, 2); //���߱�ɾ��
		DataListIter = MainDataListArr.erase(DataListIter);
		re |= 8;
	}
	else
	{
		if (2 == DataListIter->Power) //ɾ����󣬿��ܴ�2�����߱�Ϊ1�����ߣ����˶���
			UpdatePower(DataListIter);
		UpdateOneRange(DataListIter); //����MinTime, MaxTime, MinValue, MaxValue
	}

	if (nVisibleCurve <= 0)
		CHANGEMOUSESTATE;

	UpdateTotalRange(); //��������øú�������ȷ�ģ����ɲ��뵽InvalidCurveSet���ϣ���Ϊ�п����������߶���ɾ����
	if (!(ReSetCurvePosition(0, bUpdate) & 6))
		if (bUpdate)
			UpdateRect(hFrceDC, CanvasRectMask);
		else
			SysState |= 0x200;

	return re;
}

//Mask����BTime��ETime����Ч�ԣ��ӵ�Ϊ�𣬵�1λ����BTime����Ч�ԣ���2λ����ETime����Ч��
//Mask��������ExportImageFromTime��PriveCurve���������ǵ�������������ȫһ��
void CST_CurveCtrl::DelRange(long Address, HCOOR_TYPE BTime, HCOOR_TYPE ETime, short Mask, BOOL bAll, BOOL bUpdate)
	{DELRANGE(;, k = j, Mask & 1 &&  BTime > j->Time, !(Mask & 2), k->Time <= ETime, TRUE, (!(Mask & 1) || BTime <= j->Time) && (!(Mask & 2) || j->Time <= ETime));}
void CST_CurveCtrl::DelRange2(long Address, long nBegin, long nCount, BOOL bAll, BOOL bUpdate)
{
	ASSERT(nBegin >= 0 && (nCount > 0 || -1 == nCount));
	if (nBegin >= 0 && (nCount > 0 || -1 == nCount))
		DELRANGE(advance(j, nBegin), //CON1ȡ�ÿ�ʼɾ��λ��
			k = j + nCount, //CON2ȡ�ý���ɾ��λ��
			0, //C1ֱ�ӹ���һ��������
			-1 == nCount || (size_t) (nBegin + nCount) >= pDataVector->size(), //C2
			0, //C3ֱ�ӹ���һ��������
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
	//������SetBeginTime2��SetBeginValue���棬��Ϊ�ú����п������ò��ɹ���������ֻ��һ�����ʱ��
	//�����漸�п��Ա�֤�ɹ�����Ϊ����ReSetCurvePosition��������
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
		if (NullDataIter != GetFirstVisiblePos(i, TRUE, FALSE)) //i������ȫ���ɼ������ֿɼ����ɼ���
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
			LegendIter->NodeMode && LegendIter->NodeModeEx & 4) //�ڲ���ʾѡ�е�ʱ�����ƶ�ѡ�е�
		{
			size_t OldSelectedIndex = MainDataIter->SelectedIndex, NewSelectedIndex;
			auto TotalNode = MainDataIter->pDataVector->size();

			if (-1 == OldSelectedIndex) //�����һ�����Ϊѡ�е�
				MainDataIter->SelectedIndex = NewSelectedIndex = TotalNode - 1;
			else //��������ѡ�е������£�ͨ���ƶ�ѡ�е��Ϊû��ѡ�е㣬��Ƽ������
				MainDataIter->SelectedIndex = NewSelectedIndex = OldSelectedIndex - 1;

			if (NewSelectedIndex != OldSelectedIndex) //ˢ��ѡ�е�
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
			LegendIter->NodeMode && LegendIter->NodeModeEx & 4) //�ڲ���ʾѡ�е�ʱ�����ƶ�ѡ�е�
		{
			size_t OldSelectedIndex = MainDataIter->SelectedIndex, NewSelectedIndex;
			auto TotalNode = MainDataIter->pDataVector->size();

			if (-1 == OldSelectedIndex) //�õ�һ�����Ϊѡ�е�
				MainDataIter->SelectedIndex = NewSelectedIndex = 0;
			else
			{
				NewSelectedIndex = OldSelectedIndex + 1;
				if (NewSelectedIndex >= TotalNode) //��������ѡ�е������£�ͨ���ƶ�ѡ�е��Ϊû��ѡ�е㣬��Ƽ������
					NewSelectedIndex = -1;

				MainDataIter->SelectedIndex = NewSelectedIndex;
			}

			if (NewSelectedIndex != OldSelectedIndex) //ˢ��ѡ�е�
				UpdateSelectedNode(MainDataIter, OldSelectedIndex);
		}
	}
}

//�����������֮ǰҪ��֤���п��ܵĴ����ų��ˣ�����UpdateSelectedNode�����Ĺ���һ��
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
		if (hRen) //��DrawCurve�������ʱ������Ҫѡ��������Ϊ�Ѿ�ѡ����
			SelectClipRgn(hDC, hRen);
		auto OldBkColor = GetBkColor(hDC); //�����ϵ�BkColor�����Ǳ���ģ���Ϊ��DrawCurve�������ñ�����������Ҫ�ܹ���ԭ��

		auto LegendIter = DataListIter->LegendIter;
		if (-1 != OldSelectedNode && OldSelectedNode != NewSelectedNode) //Ĩ���ϵ�ѡ�е�
		{
			auto OldNodeColor = LegendIter->PenColor;
			if (2 == LegendIter->NodeMode) //��������ɫ�ķ�ɫ��ʾ�ڵ�
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

		if (-1 != NewSelectedNode) //�����µ�ѡ�е�
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

	//��Ҫ���Ƴ���ѡ�нڵ���Ҫ��ͼ��֧��
	auto LegendIter = DataListIter->LegendIter;
	if (NullLegendIter == LegendIter)
		return;

	UINT NodeMode = LegendIter->NodeMode;
	if (!NodeMode) //�ڵ����Ҫ������ʾ״̬
		return;

	if (!(LegendIter->NodeModeEx & 4)) //����ʾѡ�е�
		return;
	//����

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

		//ˢ������
		RECT rectOld, rectNew;

		//���ǵ�Ч�����⣬������û��ֱ�ӵ���UpdateRect��������Ϊ�����������Ҫ��DataListIter�������»���һ��
		if (m_ShowMode & 3)
			CHANGE_MAP_MODE(hFrceDC, m_ShowMode);

		auto re = DrawSelectedNode(hFrceDC, hScreenRgn, DataListIter, PenWidth, OldSelectedNode);
		if (re)
		{
			if (re & 1)
			{
				MakeNodeRect(rectOld, (*pDataVector)[OldSelectedNode].ScrPos, PenWidth, NodeMode);
				//������NormalizeRect���API��ֻ��CRect���У���������Ҫ�Լ�ʵ�־��εĹ��
				NormalizeRect(rectOld);
			}

			if (re & 2)
			{
				MakeNodeRect(rectNew, (*pDataVector)[NewSelectedNode].ScrPos, PenWidth, NodeMode);
				//������NormalizeRect���API��ֻ��CRect���У���������Ҫ�Լ�ʵ�־��εĹ��
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
			InvalidateControl(&rectOld, FALSE); //ˢ������Ļ��Ƶ���Ļ
		if (re & 2)
			InvalidateControl(&rectNew, FALSE); //ˢ������Ļ��Ƶ���Ļ

		if (re & 1 && IntersectRect(&rectOld, &rectOld, &PreviewRect) ||
			re & 2 && IntersectRect(&rectNew, &rectNew, &PreviewRect))
			UpdateRect(hFrceDC, PreviewRectMask); //����ȫ��λ�ô���
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
		case VK_F4: //��ʾ����
			re = ShortcutKey & 1;
			if (re)
				EnableHelpTip(!(SysState & 0x40000));
			break;
		case VK_F5: //ˢ�¼�
			re = ShortcutKey & 2;
			if (re)
				ReSetCurvePosition(1, TRUE);
			break;
		case VK_F6: //��ʾ����ȫ��λ��Ԥ������
			re = ShortcutKey & 4;
			if (re)
				EnablePreview(!(SysState & 0x80000000));
			break;
		case VK_F7: //ȫ��
			re = ShortcutKey & 8;
			if (re)
				EnableFullScreen(!(SysState & 0x200000));
			break;
		case 0xBD: //���ż�
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
			if (0x31 <= pMsg->wParam && pMsg->wParam <= 0x39 || VK_NUMPAD1 <= pMsg->wParam && pMsg->wParam <= VK_NUMPAD9) //���ּ�
			{
				size_t index = 0x31 <= pMsg->wParam && pMsg->wParam <= 0x39 ? pMsg->wParam - 0x31 : pMsg->wParam - VK_NUMPAD1;
				re = ShortcutKey & (1 << (index + 7));
				if (re)
					re = SelectLegendFromIndex(index);
			}
			else if (VK_PRIOR <= pMsg->wParam && pMsg->wParam <= VK_DOWN) //�����
			{
				switch (pMsg->wParam)
				{
				case VK_PRIOR: //��ҳ��
					re = ShortcutKey & 0x10;
					if (re)
						GotoPage(-1, TRUE);
					break;
				case VK_NEXT: //��ҳ��
					re = ShortcutKey & 0x10;
					if (re)
						GotoPage(1, TRUE);
					break;
				case VK_END: //��ҳ��
					re = ShortcutKey & 0x10;
					if (re)
						FirstPage(TRUE, TRUE);
					break;
				case VK_HOME: //��ҳ��
					re = ShortcutKey & 0x10;
					if (re)
						FirstPage(FALSE, TRUE);
					break;
				case VK_LEFT: //�����
					re = ShortcutKey & 0x40;
					if (re)
						if (-1 != CurCurveIndex && GetAsyncKeyState(VK_CONTROL) & 0x8000)
							//��ctrl�����µ�����£������ǰ��ѡ�����ߣ����ƶ������е�ѡ�е㣬��������л�û��ѡ�е㣬������һ���㿪ʼ
							//�������õ�ǰѡ�е�Ϊ���һ����
MOVE_LEFT:
							if (m_ShowMode & 1)
								MoveSelectNodeBackward();
							else
								MoveSelectNodeForward();
						else
							MoveCurve(m_ShowMode & 1 ? 1 : -1, 0);
					break;
				case VK_UP: //�����
					re = ShortcutKey & 0x20;
					if (re)
						if (-1 != CurCurveIndex && GetAsyncKeyState(VK_CONTROL) & 0x8000) //ͬVK_LEFT
							goto MOVE_LEFT;
						else
							MoveCurve(0, m_ShowMode & 2 ? -1 : 1);
					break;
				case VK_RIGHT: //�����
					re = ShortcutKey & 0x40;
					if (re)
						if (-1 != CurCurveIndex && GetAsyncKeyState(VK_CONTROL) & 0x8000)
							//��ctrl�����µ�����£������ǰ��ѡ�����ߣ����ƶ������е�ѡ�е㣬��������л�û��ѡ�е㣬��ӵ�һ���㿪ʼ
							//�������õ�ǰѡ�е�Ϊ��һ����
MOVE_RIGHT:
							if (m_ShowMode & 1)
								MoveSelectNodeForward();
							else
								MoveSelectNodeBackward();
						else
							MoveCurve(m_ShowMode & 1 ? -1 : 1, 0);
					break;
				case VK_DOWN: //�����
					re = ShortcutKey & 0x20;
					if (re)
						if (-1 != CurCurveIndex && GetAsyncKeyState(VK_CONTROL) & 0x8000) //ͬVK_RIGHT
							goto MOVE_RIGHT;
						else
							MoveCurve(0, m_ShowMode & 2 ? 1 : -1);
					break;
				} //switch
			} //�����
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
	if ((HMode ^ m_ShowMode) & 0x80) //��8λ�б仯
	{
		m_ShowMode &= ~0x80;
		m_ShowMode |= HMode;

		UpdateMask |= TimeRectMask | HLabelRectMask;
		for (auto i = begin(MainDataListArr); i < end(MainDataListArr); ++i)
		{
			if (NullLegendIter != i->LegendIter && i->LegendIter->Lable & 1) //��ʾX���꣬������Ҫ����
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
		if (Hbit & 0x80) //���λ�б仯
		{
			if (m_MoveMode & 0x80) //��Ҫ��ʾʮ�ּ�
			{
				m_MoveMode &= ~0x80;
				POINT point;
				GetCursorPos(&point);
				ScreenToClient(&point);
				if (PtInRect(CanvasRect, point))
					DrawAcrossLine(&point);
			}
			else //��Ҫ����ʮ�ּܣ���Ϊ��ʾ��ͨ���
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

	TRIMCURVE(j += nBegin, //CON1ȡ�ÿ�ʼ����λ��
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

//λ�ñ�﷽ʽΪ����1λ�������ڻ�����ߣ���2λ�������ڻ����ұߣ���3λ�������ڻ����ϱߣ���4λ�������ڻ����±�
vector<MainData>::iterator CST_CurveCtrl::GetFirstVisiblePos(vector<DataListHead<MainData>>::iterator DataListIter,
															 BOOL bPart, BOOL bXOnly, UINT* pPosition /*= nullptr*/, vector<MainData>::iterator DataIter /*= NullDataIter*/)
{
	ASSERT(NullDataListIter != DataListIter);

	auto pLTpoint = &DataListIter->LeftTopPoint.ScrPos;
	auto pRBpoint = &DataListIter->RightBottomPoint.ScrPos;
	//�ж�����DataListIter�����뻭�����ཻ�ԣ����ֱཻ�ӷ���ʧ��
	UINT Position = 0;
	if (pRBpoint->x < CanvasRect[1].left + DataListIter->Zx)
		Position |= 1; //�������
	else if (pLTpoint->x > CanvasRect[1].right)
		Position |= 2; //�����Ҳ�
	if (!Position && !bXOnly) //����Ѿ��ڻ��������������򣬲����ټ���
		if (pRBpoint->y < CanvasRect[1].top)
			Position |= 4; //�����ϲ�
		else if (pLTpoint->y > CanvasRect[1].bottom - DataListIter->Zy)
			Position |= 8; //�����²�
	if (pPosition)
		*pPosition = Position;

	if (!Position)
	{
		auto pDataVector = DataListIter->pDataVector;
		auto i = NullDataIter != DataIter ? DataIter : begin(*pDataVector);
		for (; i < end(*pDataVector); ++i)
			if (2 == i->State && i > begin(*pDataVector) && i < prev(end(*pDataVector))) //������Σ���β���������Ҫ����
				continue;
			else if (1 == DataListIter->Power && i->ScrPos.x > CanvasRect[1].right) //�Ѿ����������ҵ��ˣ�����һ������Ҫ���Ż�
				break;
			else if (IsPointVisible(DataListIter, i, bPart, bXOnly, 1)) //ֻ�����
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
BOOL CST_CurveCtrl::IsCurveInCanvas(long Address) {return NullDataIter != GetFirstVisiblePos(Address);} //���ֿɼ�����

BOOL CST_CurveCtrl::VCenterCurve(long Address, BOOL bUpdate)
{
	auto DataListIter = FindMainData(Address);
	if (NullDataListIter != DataListIter)
	{
		if (!ISCURVESHOWN(DataListIter)) //�����ص�������ʾ����
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
		if (!ISCURVESHOWN(DataListIter)) //�����ص�������ʾ����
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
	if (IsBadStringPtr(pTime, -1)) //��ָ��Ҳ����ȷ�ж�
		return FALSE;

	if (m_ShowMode & 0x80)
	{
		LPTSTR pEnd = nullptr;
		auto Time = _tcstod(pTime, &pEnd);
		if (HUGE_VAL == Time || -HUGE_VAL == Time)
			return FALSE;
		if (nullptr == pEnd || pTime == pEnd) //�����޷�����
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

//Position��-1����ӵ�nIndexǰ�棻 0������nIndex�㣻 1����ӵ�nIndex���棻
//���Position����0����Mask�����壬��λ�㣬�ӵ�λ��1��Time��Ч��2��Value��Ч��3��State��Ч
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

		if (-1 == Position) //��ӵ�ǰ��
			pDataVector->insert(InsertPos, NoUse);
		else // if (1 == Position) //��ӵ�����
		{
			++InsertPos; //��insert�ĵ�һ����������������end()ʱ��insertִ����Ȼ��ɹ�����Ϊ�����ڵ�һ������ָ����λ�õ�ǰ�����
			pDataVector->insert(InsertPos, NoUse);
		}
	}
	else if (Mask & 7) //���ĵ�ǰ��
	{
		if (Mask & 4)
		{
			if (!IsMainDataStateValidate(State))
				return FALSE;

			InsertPos->AllState = State;
		}

		if (Mask & 3) //�����и���
		{
			if (Mask & 2)
				InsertPos->Value = Value;
			if (Mask & 1)
				InsertPos->Time = Time;

			CalcOriginDatumPoint(*InsertPos, Mask & 3, 0, 0, DataListIter);
		}
	}
	else
		return FALSE; //ʲôҲû�޸�

	InvalidCurveSet.insert(DataListIter->Address); //������߸��ĵ�����ߵĴ������ܻ��1��2��Ҳ���ܷ�֮���ⲻͬ��ɾ���㣬ɾ����ʱֻ���ܴ�2�α�Ϊ1��
	SysState |= 0x200; //����û��bUpdate������������ҪRefresh���������ò�����Ч
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
			swap(*i, *j); //��������DataListHead�����ݣ�������ָ��
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
����ؼ�Ϊ��������������ȡ�����������������ر���������ص������ͻ���������
����ؼ�Ϊ�����ͻ�������ȡ��������������������

hBuddy!=0
���State==0����hBuddy���������ͻ�������������ӵ���ǰ�ؼ�
ͬʱ�����ؼ�������SetBuddy�����Ŀؼ�������ɷ�����
���State==1����hBuddy���������ͻ�����������ӵ�ǰ�ؼ��������ͻ���������ɾ��

ע��ɾ�������ͻ��������ַ�����һ���Ƕ���������������SetBuddy����ʱhBuddyΪ�����ͻ�����State==1����
һ���Ƕ������ͻ�������SetBuddy����ʱhBuddy==0��State���ԣ�

ɾ������������ֻ���Ƕ���������������SetBuddy����ʱhBuddy==0��State���ԣ���һ�ַ���
*/
BOOL CST_CurveCtrl::SetBuddy(OLE_HANDLE hBuddy, short State) //����������������hBuddyΪ�����ͻ���
{
	if (!hBuddy) //ȡ������������
	{
		CANCELBUDDYS;
		return TRUE;
	}

	auto re = TRUE;
	auto hThisBuddy = Format64bitHandle(HWND, hBuddy);
	switch (State)
	{
	case 0:
		if (hBuddyServer) //�Ѿ��������ͻ�������ȡ������Ϊ��β��������ѱ��ؼ��������������
		{
			::SendMessage(hBuddyServer, BUDDYMSG, 1, (LPARAM) m_hWnd);
			hBuddyServer = 0;
		}

		if (!pBuddys)
			pBuddys = new vector<HWND>;
		else if (find(begin(*pBuddys), end(*pBuddys), hThisBuddy) < end(*pBuddys)) //�ظ����
			return TRUE;

		re = (BOOL) ::SendMessage(hThisBuddy, BUDDYMSG, 0, (LPARAM) m_hWnd); //֪ͨ�����ͻ��������Լ��Ĵ��ھ��������
		if (re)
		{
			pBuddys->push_back(hThisBuddy);
			::SendMessage(hThisBuddy, BUDDYMSG, 2, (LPARAM) &OriginPoint.Time); //ͬ��ʱ��
			::SendMessage(hThisBuddy, BUDDYMSG, 3, (LPARAM) &HCoorData.fStep); //ͬ��ʱ����
			::SendMessage(hThisBuddy, BUDDYMSG, 4, (LPARAM) Zoom); //ͬ���Ŵ���
			::SendMessage(hThisBuddy, BUDDYMSG, 8, (LPARAM) HZoom); //ͬ��ˮƽ�Ŵ���
			auto ThisLeftSpace = (short) ::SendMessage(hThisBuddy, BUDDYMSG, 6, 0); //ѯ���¼���������ͻ�����LeftSpace
			if (LeftSpace < ThisLeftSpace) //�¼���������ͻ�����LeftSpace���
			{
				BROADCASTLEFTSPACE(ThisLeftSpace);
				CHLeftSpace(ThisLeftSpace);
			}
			else if (LeftSpace > ThisLeftSpace) //�¼���������ͻ�����LeftSpace�����
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

//���أ�-1����������������Ҳ�������ͻ�����0���������ͻ�����>0��������������
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
	if ((SysState ^ bEnable) & 0x400) //��11λ���б仯
	{
		SysState &= ~0x400;
		SysState |= bEnable;
	}
}

void CST_CurveCtrl::EnableHZoom(BOOL bEnable)
{
	bEnable <<= 4;
	if ((SysState ^ bEnable) & 0x10) //��5λ���б仯
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
		UpdateRect(hFrceDC, TitleRectMask); //CurveTitleRect����UnitRect��LegendMarkRect
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
		if (GridMode != ((SysState >> 19) & 3) + ((SysState >> 21) & 4) + ((SysState & 4) << 1)) //�ӵ�24λ���ƶ�����3λ��
		{
			SysState &= ~0x980004; //��3 20 21 24λ
			SysState |= (UINT) (GridMode & 3) << 19;
			SysState |= (UINT) (GridMode & 4) << 21; //�ӵ�3λ���ƶ�����24λ��
			SysState |= (UINT) (GridMode & 8) >> 1; //�ӵ�4λ���ƶ�����3λ��
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
		bEnable <<= 13; //��14λ���б仯
		SysState |= bEnable;
	}
}

void CST_CurveCtrl::EnableAutoTrimCoor(BOOL bEnable)
{
	SysState &= ~0x20000;
	if (bEnable)
	{
		TrimCoor();
		bEnable <<= 17; //��18λ���б仯
		SysState |= bEnable;
	}
}

//�ӿ������BOOL���ͣ������Ǵ���ؼ����Ǵ����ؼ������ᱻ����ֻ����0��1
void CST_CurveCtrl::EnableHelpTip(BOOL bEnable)
{
	bEnable <<= 18;
	if ((SysState ^ bEnable) & 0x40000) //��19λ���б仯
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
		CalcOriginDatumPoint(OriginPoint, 0x10 | Mask); //����Ҫ���¼������л�������
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
			NoUse.LegendIter = FindLegend(NewAddr, TRUE); //����ͼ������LegendIter;
			NoUse.FillDirection = DataListIter->FillDirection;
			NoUse.Power = DataListIter->Power;
			NoUse.Zx = DataListIter->Zy;
			NoUse.Zy = DataListIter->Zy;

			if (ISCURVESHOWN((&NoUse)))
				++nVisibleCurve;

			auto SrcIndex = distance(begin(MainDataListArr), DataListIter); //���ǵ�MainDataListArr�������·����ڴ棬�ȼ������
			MainDataListArr.push_back(NoUse);
			//�ָ�DataListIter����ʵֻȡ��pDataVector��Ա�������ְ취��֤����һʧ
			auto pDataVector = next(begin(MainDataListArr), SrcIndex)->pDataVector;
			NewDataListIter = prev(end(MainDataListArr));
			NewDataListIter->pDataVector->assign(begin(*pDataVector), end(*pDataVector)); //�������ݣ�vector���Զ�Ԥ�ȷ���end - begin���ռ�

			UpdateRect(hFrceDC, CanvasRectMask);

			return TRUE;
		}
	}

	return FALSE;
}

//��������ϼ�
void CST_CurveCtrl::DoUniteCurve(vector<DataListHead<MainData>>::iterator DesDataListIter, vector<MainData>::iterator InsertIter,
								 vector<DataListHead<MainData>>::iterator DataListIter, long nBegin, long nCount)
{
	UNITECURVE(if ((size_t) nBegin >= pDataVector->size()) i = end(*pDataVector); else i += nBegin, //CON1
		auto n = 0, (-1 == nCount || ++n <= nCount), //CON2 C1
		-1 != nCount && (size_t) (nBegin + nCount) < pDataVector->size(), i += nCount, //C2 CON3
		FALSE, FALSE); //C3(������)
}

void CST_CurveCtrl::DoUniteCurve(vector<DataListHead<MainData>>::iterator DesDataListIter, vector<MainData>::iterator InsertIter,
								 vector<DataListHead<MainData>>::iterator DataListIter, HCOOR_TYPE BTime, HCOOR_TYPE ETime, short Mask)
{
	UNITECURVE(if (Mask & 1) for (; i < end(*pDataVector) && i->Time < BTime; ++i), 0, (!(Mask & 2) || i->Time <= ETime), //CON1 CON2(������) C1
		Mask & 2, for (; i < end(*pDataVector) && i->Time <= ETime; ++i), //C2 CON3
		TRUE, (!(Mask & 1) || BTime <= i->Time) && (!(Mask & 2) || i->Time <= ETime)); //C3
}

//��ϼ������nInsertPos����-1���ڲ�ʵ����ȡ��Address�з��������ĵ㣬������bAddTrailΪ�ٵ���AddMainData2������
//���nInsertPos���ڵ���0��ֱ�ӽ�Address���������з��������ĵ�ȫ�����뵽DesAddr��nInsertPosǰ���λ�ã�
//��Ӵ��򰴵���Address�д���Ϊ׼����Χ��Address���ߵķ�Χ��
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

//Operator�ĸ��ֽڴ���TimeSpan�������ͣ����ֽڴ���ValueStep�������ͣ����ǵľ�������һ����
//'+'���ӣ�'*'���ˣ������������ʾΪʱ�䣬���������ֻ���Ǽӣ���Ϊ��������
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
			OFFSETVALUE(Operator1, DataListIter->LeftTopPoint.Time, TimeSpan); //�������߷�Χ
			ChMask |= 1;
		}
		if (Operator2 && .0f != ValueStep)
		{
			YOff = DataListIter->LeftTopPoint.ScrPos.y;
			OFFSETVALUE(Operator2, DataListIter->LeftTopPoint.Value, ValueStep); //�������߷�Χ
			ChMask |= 2;
		}
		if (!ChMask)
			return TRUE;

		CalcOriginDatumPoint(DataListIter->LeftTopPoint, ChMask, 0, 0, DataListIter);
		if (ChMask & 1)
			XOff -= DataListIter->LeftTopPoint.ScrPos.x;
		if (ChMask & 2)
			YOff -= DataListIter->LeftTopPoint.ScrPos.y; //����ƫ����

		OFFSETCURVE(DataListIter->RightBottomPoint); //�������߷�Χ������Ӧ��ƫ��

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
				UpdateOneRange(DesDataListIter); //��ͬ��OffSetCurve������ArithmeticOperate��DesAddr���ߵķ�Χ����Ҫ���¼��㣨���ݴβ���ı䣩
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
		if (size <= 0) //������໺��
		{
			if (pDataVector->capacity() > pDataVector->size())
			{
				DataListIter->pDataVector = new vector<MainData>(*pDataVector); //���ɶ�*pDataVector����std::move�����Ч�ʣ����𲻵�ѹ�������Ŀ��
				delete pDataVector;
			}

			return TRUE;
		}
		else if ((size_t) size > pDataVector->capacity()) //����ָ���Ļ���
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

	size_t TotalUsed = 0; //ʹ����
	size_t TotalAlloc = 0; //������
	float MinUseRate = 1.0; //��С������
	long Address; //��С�������µ����ߵ�ַ

	for (auto i = begin(MainDataListArr); i < end(MainDataListArr); ++i)
	{
		auto pDataVector = i->pDataVector;

		TotalUsed += pDataVector->size();
		TotalAlloc += pDataVector->capacity();

		if (pAddress) //�Ż�һ�£������������Ƚ���
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
	if (Range <= 0) //ȡ������ЧӦ
	{
		SorptionRange = -1;
		SysState &= ~0xC000;
	}
	else //��������ЧӦ
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
	auto i = GetFirstVisiblePos(DataListIter, FALSE, FALSE); //���������������Ϊ���������е㶼�ڻ���֮���ʱ�򣬻�õ��Ż�
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
	if ((SysState ^ bEnable) & 0x200000) //��22λ���б仯
	{
		SysState &= ~0x200000;
		SysState |= bEnable;
		LeftSpace = 0; //��ʹSetLeftSpace���¼���LeftSpace�����Ǳ����
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
				for (auto i = begin(*pDataVector); i < end(*pDataVector); ++i) //ƫ������
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
	if ((SysState ^ bEnable) & 0x400000) //��23λ���б仯
	{
		SysState &= ~0x400000;
		SysState |= bEnable;

		if (m_hWnd == ::GetFocus())
			if (bEnable)
				PostMessage(WM_SETFOCUS);
			else //���ɷ���WM_KILLFOCUS��Ϣ
			{
				SysState &= ~0x100;
				REFRESHFOCUS(auto hDC = ::GetDC(m_hWnd), ::ReleaseDC(m_hWnd, hDC)); //����Ҫ�õ���ʾЧ�������Ի�ȡ��ĻDC
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
	if ((SysState ^ bLimit) & 0x10000000) //��29λ���б仯
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

		SysState &= ~0x10000000; //ȡ������һҳ
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
		//��ֻ��һ�����ʱ�򣨼�m_MinTime����m_MaxTime��m_MinValue����m_MaxValue����Ҳ����UpdateFixedValues�����������
		//��������������ʱ�����ڼ��Ϊ0������ֱ��ʧ�ܵ�
		if (nVisibleCurve > 0)
			if (0 == LimitOnePageMode)
			{
				if (UpdateFixedValues(m_MinTime, m_MaxTime, m_MinValue, m_MaxValue, 0xf))
					return TRUE;
			}
			else
			{
				UINT re = UpdateFixedValues(m_MinTime, m_MaxTime, m_MinValue, m_MaxValue, 5);

				while (RightBottomPoint.ScrPos.x > CanvasRect[1].right && //���˻����ұ���
					SetTimeSpan(HCoorData.fStep * (LimitOnePageMode + 1)))
					re |= 0x10; //����λ��UpdateFixedValues����ֵ����������ͬ
				while (LeftTopPoint.ScrPos.y < CanvasRect[1].top && //���˻����ϱ���
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
	if ((SysState ^ bEnable) & 0x80000000) //��32λ���б仯
	{
		SysState &= ~0x80000000;
		SysState |= bEnable;

		if (bEnable) //ȫ��λ�ô��ڴ�������״̬������
			UpdateRect(hFrceDC, PreviewRectMask);
		else //ȫ��λ�ô��ڴ�����ʾ״̬���ر�
			UpdateRect(hFrceDC, CanvasRectMask);

		POINT point;
		GetCursorPos(&point);
		ScreenToClient(&point);
		PostMessage(WM_MOUSEMOVE, 0, point.x + (point.y << 16)); //�޸�������ӣ�������WM_SETCURSOR��Ϣ�������ΪҪ����MouseMoveMode����
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
		((SysState &      0x400) >> 10) + //ռ1λ
		((SysState &     0x2000) >> 11) + //ռ2λ��ǰ��һλ��ǰ��EnablePageChangeEvent�����ڲ���ʹ�ã������ɱ���λ���Լ����ϴ��룩
		((SysState &    0x60000) >> 14) + //ռ2λ
		((SysState &   0x600000) >> 16) + //ռ2λ
		((SysState & 0x80000000) >> 24) + //ռ1λ
		((SysState &       0x10) << 4 ) + //ռ1λ
		((SysState &        0x8) << 6 );  //ռ1λ
	//���ܰ�ԭʼλ�ô���SysState�����������ױ��ƽ�
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
long CST_CurveCtrl::FreePlugIn(UINT& UpdateMask, BOOL bUpdate) //�ͷŲ����ֻ���ͷ��ɲ��dll���صĽӿڣ�
{
	ClosePlugIn; //ж��dll���ͷ��ڴ�

	RemovePlugInOrScript(!=, bUpdate);
}

long CST_CurveCtrl::LoadPlugIn(LPCTSTR pFileName, short Type, long Mask)
{
	if (1 != Type) //Ŀǰֻ֧��1����
	{
		return Mask;
	}

	UINT UpdateMask = 0;
	if (!(Mask & PlugInType1Mask)) //ȡ�����
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
//��������������Lua�ű����
#define LuaBuffLen	128
static TCHAR LuaBuff[LuaBuffLen];
static const TCHAR* luaFormatXCoordinate(long Address, HCOOR_TYPE DateTime, UINT Action) //���1
{
	LuaBuff[0] = 0;
	if (g_L)
	{
		lua_settop(g_L, 0); //�ݴ�
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

static const TCHAR* luaFormatYCoordinate(long Address, float Value, UINT Action) //���2
{
	LuaBuff[0] = 0;
	if (g_L)
	{
		lua_settop(g_L, 0); //�ݴ�
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

static HCOOR_TYPE luaTrimXCoordinate(HCOOR_TYPE DateTime) //���3
{
	auto re = FALSE;
	HCOOR_TYPE NewDateTime;

	if (g_L)
	{
		lua_settop(g_L, 0); //�ݴ�
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

static float luaTrimYCoordinate(float Value) //���4
{
	auto re = FALSE;
	float NewValue;

	if (g_L)
	{
		lua_settop(g_L, 0); //�ݴ�
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

static double luaCalcTimeSpan(double TimeSpan, short Zoom, short HZoom) //���5
{
	auto re = FALSE;
	double NewTimeSpan;

	if (g_L)
	{
		lua_settop(g_L, 0); //�ݴ�
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

static float luaCalcValueStep(float ValueStep, short Zoom) //���6
{
	auto re = FALSE;
	float NewValueStep;

	if (g_L)
	{
		lua_settop(g_L, 0); //�ݴ�
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

long CST_CurveCtrl::FreeLuaScript() //�ͷ�Lua�ű���ֻ���ͷ���Lua�ű����صĽӿڣ�
{
	CloseLua; //�ر�Lua���ͷ��ڴ�

	UINT UpdateMask = 0;
	RemovePlugInOrScript(==, TRUE);
}

long CST_CurveCtrl::LoadLuaScript(LPCTSTR pFileName, short Type, long Mask)
{
	if (1 != Type) //Ŀǰֻ֧��1����
	{
		return Mask;
	}

	if (!(Mask & PlugInType1Mask)) //ȡ�����
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
	ShortcutKey &= ALL_SHORTCUT_KEY; //Ŀǰֻʹ����ǰ16λ
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
		BottomSpace = 5 + (BottomSpaceLine + 1) * (2 +  fHeight); //�¿հ�
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
		pComment, TextColor, XOffSet, YOffSet, 0x7FF, FALSE); //�Լ�ˢ��

	if (Mask) //���ʧ��
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

	Mask &= 0x63C; //11000111100 Ϊ0��Щλ���ϵ����ÿ϶���ɹ�

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

BOOL CST_CurveCtrl::SetFixedZoomMode(short ZoomMode) //ͨ���ӿڵĵ��ã�����SysState���Ե�Ӱ��
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
		MoveMode = MouseMoveMode >> 8; //������״̬ʱ������ƶ����ǻ�����������״̬��д���8λ����ʱ��Ȼ����������״̬

	if (ZOOMOUT == MoveMode)
		return '-';
	else if (ZOOMIN == MoveMode)
		return '+';

	return 0;
}

BOOL CST_CurveCtrl::DoFixedZoom(const POINT& point)
{
	if (ZOOMIN == MouseMoveMode && 32767 == Zoom || ZOOMOUT == MouseMoveMode && -32768 == Zoom)
		return FALSE; //Խ��

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
	if (0 == ZoomMode) //��������������ȡ������״̬��
		return FALSE;
	else if (ZoomMode == GetFixedZoomMode()) //�Ѿ�����Ҫ������״̬�ˣ�Ҳ���ɺϷ���ֻ�ǲ���ȡ������״̬
		bHoldMode = TRUE;
	else if (!SetFixedZoomMode(ZoomMode)) //�������󣬻��ߵ�ǰ״̬������������״̬
		return FALSE;

	POINT point = {x, y}; //�ͻ�����
	auto re = PtInRect(CanvasRect, point) && DoFixedZoom(point);

	if (!bHoldMode)
		SetFixedZoomMode(ZoomMode); //ȡ������״̬

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
	if (!AmbientUserMode()) //���ģʽ�²�����������ֵ
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
	CalcOriginDatumPoint(OriginPoint, mask); //�������������¼�
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
	if (IsBadStringPtr(pHLegend, -1)) //��ָ��Ҳ����ȷ�ж�
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
