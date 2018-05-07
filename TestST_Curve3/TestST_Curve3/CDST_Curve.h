// CDST_Curve.h : 由 Microsoft Visual C++ 创建的 ActiveX 控件包装类的声明

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CDST_Curve

class CDST_Curve : public CWnd
{
protected:
	DECLARE_DYNCREATE(CDST_Curve)
public:
	CLSID const& GetClsid()
	{
		static CLSID const clsid
			= { 0x315E7F0E, 0x6F9C, 0x41A3, { 0xA6, 0x69, 0xA7, 0xE9, 0x62, 0x6D, 0x7C, 0xA0 } };
		return clsid;
	}
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle,
						const RECT& rect, CWnd* pParentWnd, UINT nID, 
						CCreateContext* pContext = NULL)
	{ 
		return CreateControl(GetClsid(), lpszWindowName, dwStyle, rect, pParentWnd, nID); 
	}

    BOOL Create(LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, 
				UINT nID, CFile* pPersist = NULL, BOOL bStorage = FALSE,
				BSTR bstrLicKey = NULL)
	{ 
		return CreateControl(GetClsid(), lpszWindowName, dwStyle, rect, pParentWnd, nID,
		pPersist, bStorage, bstrLicKey); 
	}

// 特性
public:

// 操作
public:

	BOOL SetVInterval(short VInterval)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0x9, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, VInterval);
		return result;
	}
	BOOL SetHInterval(short HInterval)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0xa, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, HInterval);
		return result;
	}
	short GetScaleInterval()
	{
		short result;
		InvokeHelper(0xb, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
		return result;
	}
	void EnableHelpTip(BOOL bEnable)
	{
		static BYTE parms[] = VTS_BOOL ;
		InvokeHelper(0xc, DISPATCH_METHOD, VT_EMPTY, NULL, parms, bEnable);
	}
	BOOL SetLegendSpace(short LegendSpace)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0xd, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, LegendSpace);
		return result;
	}
	short GetLegendSpace()
	{
		short result;
		InvokeHelper(0xe, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
		return result;
	}
	BOOL SetBeginValue(float fBeginValue)
	{
		BOOL result;
		static BYTE parms[] = VTS_R4 ;
		InvokeHelper(0xf, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, fBeginValue);
		return result;
	}
	float GetBeginValue()
	{
		float result;
		InvokeHelper(0x10, DISPATCH_METHOD, VT_R4, (void*)&result, NULL);
		return result;
	}
	BOOL SetBeginTime(LPCTSTR pBeginTime)
	{
		BOOL result;
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x11, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, pBeginTime);
		return result;
	}
	BOOL SetBeginTime2(DATE fBeginTime)
	{
		BOOL result;
		static BYTE parms[] = VTS_DATE ;
		InvokeHelper(0x12, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, fBeginTime);
		return result;
	}
	CString GetBeginTime()
	{
		CString result;
		InvokeHelper(0x13, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	DATE GetBeginTime2()
	{
		DATE result;
		InvokeHelper(0x14, DISPATCH_METHOD, VT_DATE, (void*)&result, NULL);
		return result;
	}
	BOOL SetTimeSpan(double TimeStep)
	{
		BOOL result;
		static BYTE parms[] = VTS_R8 ;
		InvokeHelper(0x15, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, TimeStep);
		return result;
	}
	double GetTimeSpan()
	{
		double result;
		InvokeHelper(0x16, DISPATCH_METHOD, VT_R8, (void*)&result, NULL);
		return result;
	}
	BOOL SetValueStep(float ValueStep)
	{
		BOOL result;
		static BYTE parms[] = VTS_R4 ;
		InvokeHelper(0x17, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, ValueStep);
		return result;
	}
	float GetValueStep()
	{
		float result;
		InvokeHelper(0x18, DISPATCH_METHOD, VT_R4, (void*)&result, NULL);
		return result;
	}
	BOOL SetVPrecision(short Precision)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0x19, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Precision);
		return result;
	}
	short GetVPrecision()
	{
		short result;
		InvokeHelper(0x1a, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
		return result;
	}
	BOOL SetUnit(LPCTSTR pUnit)
	{
		BOOL result;
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x1b, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, pUnit);
		return result;
	}
	CString GetUnit()
	{
		CString result;
		InvokeHelper(0x1c, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	void TrimCoor()
	{
		InvokeHelper(0x1d, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	short AddLegend(long Id, LPCTSTR pSign, long PenColor, short PenStyle, short LineWidth, long BrushColor, short BrushStyle, short CurveMode, short NodeMode, short Mask, BOOL bUpdate)
	{
		short result;
		static BYTE parms[] = VTS_I4 VTS_BSTR VTS_I4 VTS_I2 VTS_I2 VTS_I4 VTS_I2 VTS_I2 VTS_I2 VTS_I2 VTS_BOOL ;
		InvokeHelper(0x1e, DISPATCH_METHOD, VT_I2, (void*)&result, parms, Id, pSign, PenColor, PenStyle, LineWidth, BrushColor, BrushStyle, CurveMode, NodeMode, Mask, bUpdate);
		return result;
	}
	BOOL GetLegend(LPCTSTR pSign, long * pPenColor, short * pPenStyle, short * pLineWidth, long * pBrushColor, short * pBrushStyle, short * pCurveMode, short * pNodeMode)
	{
		BOOL result;
		static BYTE parms[] = VTS_BSTR VTS_PI4 VTS_PI2 VTS_PI2 VTS_PI4 VTS_PI2 VTS_PI2 VTS_PI2 ;
		InvokeHelper(0x1f, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, pSign, pPenColor, pPenStyle, pLineWidth, pBrushColor, pBrushStyle, pCurveMode, pNodeMode);
		return result;
	}
	CString QueryLegend(long Id)
	{
		CString result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x20, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, Id);
		return result;
	}
	short GetLegendCount()
	{
		short result;
		InvokeHelper(0x21, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
		return result;
	}
	BOOL GetLegend2(short nIndex, long * pPenColor, short * pPenStyle, short * pLineWidth, long * pBrushColor, short * pBrushStyle, short * pCurveMode, short * pNodeMode)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 VTS_PI4 VTS_PI2 VTS_PI2 VTS_PI4 VTS_PI2 VTS_PI2 VTS_PI2 ;
		InvokeHelper(0x22, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, nIndex, pPenColor, pPenStyle, pLineWidth, pBrushColor, pBrushStyle, pCurveMode, pNodeMode);
		return result;
	}
	short GetLegendIdCount(short nIndex)
	{
		short result;
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0x23, DISPATCH_METHOD, VT_I2, (void*)&result, parms, nIndex);
		return result;
	}
	long GetLegendId(short nIndex, short nIdIndex)
	{
		long result;
		static BYTE parms[] = VTS_I2 VTS_I2 ;
		InvokeHelper(0x24, DISPATCH_METHOD, VT_I4, (void*)&result, parms, nIndex, nIdIndex);
		return result;
	}
	BOOL DelLegend(long Id, BOOL bAll, BOOL bUpdate)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_BOOL VTS_BOOL ;
		InvokeHelper(0x25, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Id, bAll, bUpdate);
		return result;
	}
	BOOL DelLegend2(LPCTSTR pSign, BOOL bUpdate)
	{
		BOOL result;
		static BYTE parms[] = VTS_BSTR VTS_BOOL ;
		InvokeHelper(0x26, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, pSign, bUpdate);
		return result;
	}
	short AddMainData(long Id, LPCTSTR pTime, float Value, short State, short VisibleState, BOOL bAddTrail)
	{
		short result;
		static BYTE parms[] = VTS_I4 VTS_BSTR VTS_R4 VTS_I2 VTS_I2 VTS_BOOL ;
		InvokeHelper(0x27, DISPATCH_METHOD, VT_I2, (void*)&result, parms, Id, pTime, Value, State, VisibleState, bAddTrail);
		return result;
	}
	short AddMainData2(long Id, DATE Time, float Value, short State, short VisibleState, BOOL bAddTrail)
	{
		short result;
		static BYTE parms[] = VTS_I4 VTS_DATE VTS_R4 VTS_I2 VTS_I2 VTS_BOOL ;
		InvokeHelper(0x28, DISPATCH_METHOD, VT_I2, (void*)&result, parms, Id, Time, Value, State, VisibleState, bAddTrail);
		return result;
	}
	void SetVisibleCoorRange(DATE MinTime, DATE MaxTime, float MinValue, float MaxValue, short Mask)
	{
		static BYTE parms[] = VTS_DATE VTS_DATE VTS_R4 VTS_R4 VTS_I2 ;
		InvokeHelper(0x29, DISPATCH_METHOD, VT_EMPTY, NULL, parms, MinTime, MaxTime, MinValue, MaxValue, Mask);
	}
	void GetVisibleCoorRange(DATE * pMinTime, DATE * pMaxTime, float * pMinValue, float * pMaxValue)
	{
		static BYTE parms[] = VTS_PDATE VTS_PDATE VTS_PR4 VTS_PR4 ;
		InvokeHelper(0x2a, DISPATCH_METHOD, VT_EMPTY, NULL, parms, pMinTime, pMaxTime, pMinValue, pMaxValue);
	}
	void DelRange(long Id, DATE BTime, DATE ETime, short Mask, BOOL bAll, BOOL bUpdate)
	{
		static BYTE parms[] = VTS_I4 VTS_DATE VTS_DATE VTS_I2 VTS_BOOL VTS_BOOL ;
		InvokeHelper(0x2b, DISPATCH_METHOD, VT_EMPTY, NULL, parms, Id, BTime, ETime, Mask, bAll, bUpdate);
	}
	void DelRange2(long Id, long nBegin, long nCount, BOOL bAll, BOOL bUpdate)
	{
		static BYTE parms[] = VTS_I4 VTS_I4 VTS_I4 VTS_BOOL VTS_BOOL ;
		InvokeHelper(0x2c, DISPATCH_METHOD, VT_EMPTY, NULL, parms, Id, nBegin, nCount, bAll, bUpdate);
	}
	BOOL FirstPage(BOOL bLast, BOOL bUpdate)
	{
		BOOL result;
		static BYTE parms[] = VTS_BOOL VTS_BOOL ;
		InvokeHelper(0x2d, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, bLast, bUpdate);
		return result;
	}
	short GotoPage(short RelativePage, BOOL bUpdate)
	{
		short result;
		static BYTE parms[] = VTS_I2 VTS_BOOL ;
		InvokeHelper(0x2e, DISPATCH_METHOD, VT_I2, (void*)&result, parms, RelativePage, bUpdate);
		return result;
	}
	BOOL SetZoom(short Zoom)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0x2f, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Zoom);
		return result;
	}
	short GetZoom()
	{
		short result;
		InvokeHelper(0x30, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
		return result;
	}
	BOOL SetMaxLength(long MaxLength, long CutLength)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_I4 ;
		InvokeHelper(0x31, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, MaxLength, CutLength);
		return result;
	}
	long GetMaxLength()
	{
		long result;
		InvokeHelper(0x32, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	long GetCutLength()
	{
		long result;
		InvokeHelper(0x33, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	BOOL SetShowMode(short ShowMode)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0x34, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, ShowMode);
		return result;
	}
	short GetShowMode()
	{
		short result;
		InvokeHelper(0x35, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
		return result;
	}
	BOOL SetBkBitmap(short nIndex)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0x36, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, nIndex);
		return result;
	}
	short GetBkBitmap()
	{
		short result;
		InvokeHelper(0x37, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
		return result;
	}
	BOOL SetFillDirection(long Id, short FillDirection, BOOL bUpdate)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_I2 VTS_BOOL ;
		InvokeHelper(0x38, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Id, FillDirection, bUpdate);
		return result;
	}
	short GetFillDirection(long Id)
	{
		short result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x39, DISPATCH_METHOD, VT_I2, (void*)&result, parms, Id);
		return result;
	}
	BOOL SetMoveMode(short MoveMode)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0x3a, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, MoveMode);
		return result;
	}
	short GetMoveMode()
	{
		short result;
		InvokeHelper(0x3b, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
		return result;
	}
	void SetFont(long hFont)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x3c, DISPATCH_METHOD, VT_EMPTY, NULL, parms, hFont);
	}
	BOOL AddImageHandle(LPCTSTR pFileName, BOOL bShared)
	{
		BOOL result;
		static BYTE parms[] = VTS_BSTR VTS_BOOL ;
		InvokeHelper(0x3d, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, pFileName, bShared);
		return result;
	}
	void AddBitmapHandle(long hBitmap, BOOL bShared)
	{
		static BYTE parms[] = VTS_I4 VTS_BOOL ;
		InvokeHelper(0x3e, DISPATCH_METHOD, VT_EMPTY, NULL, parms, hBitmap, bShared);
	}
	BOOL AddBitmapHandle2(long hInstance, LPCTSTR pszResourceName, BOOL bShared)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_BSTR VTS_BOOL ;
		InvokeHelper(0x3f, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, hInstance, pszResourceName, bShared);
		return result;
	}
	BOOL AddBitmapHandle3(long hInstance, long nIDResource, BOOL bShared)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_I4 VTS_BOOL ;
		InvokeHelper(0x40, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, hInstance, nIDResource, bShared);
		return result;
	}
	long GetBitmapCount()
	{
		long result;
		InvokeHelper(0x41, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	BOOL SetBkMode(short BkMode)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0x42, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, BkMode);
		return result;
	}
	short GetBkMode()
	{
		short result;
		InvokeHelper(0x43, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
		return result;
	}
	BOOL ExportImage(LPCTSTR pFileName)
	{
		BOOL result;
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x44, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, pFileName);
		return result;
	}
	long ExportImageFromPage(LPCTSTR pFileName, long Id, long nBegin, long nCount, BOOL bAll, short Style)
	{
		long result;
		static BYTE parms[] = VTS_BSTR VTS_I4 VTS_I4 VTS_I4 VTS_BOOL VTS_I2 ;
		InvokeHelper(0x45, DISPATCH_METHOD, VT_I4, (void*)&result, parms, pFileName, Id, nBegin, nCount, bAll, Style);
		return result;
	}
	long ExportImageFromTime(LPCTSTR pFileName, long Id, DATE BTime, DATE ETime, short Mask, BOOL bAll, short Style)
	{
		long result;
		static BYTE parms[] = VTS_BSTR VTS_I4 VTS_DATE VTS_DATE VTS_I2 VTS_BOOL VTS_I2 ;
		InvokeHelper(0x46, DISPATCH_METHOD, VT_I4, (void*)&result, parms, pFileName, Id, BTime, ETime, Mask, bAll, Style);
		return result;
	}
	BOOL BatchExportImage(LPCTSTR pFileName, long nSecond)
	{
		BOOL result;
		static BYTE parms[] = VTS_BSTR VTS_I4 ;
		InvokeHelper(0x47, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, pFileName, nSecond);
		return result;
	}
	void EnableAutoTrimCoor(BOOL bEnable)
	{
		static BYTE parms[] = VTS_BOOL ;
		InvokeHelper(0x48, DISPATCH_METHOD, VT_EMPTY, NULL, parms, bEnable);
	}
	long ImportFile(LPCTSTR pFileName, short Style, BOOL bAddTrail)
	{
		long result;
		static BYTE parms[] = VTS_BSTR VTS_I2 VTS_BOOL ;
		InvokeHelper(0x49, DISPATCH_METHOD, VT_I4, (void*)&result, parms, pFileName, Style, bAddTrail);
		return result;
	}
	BOOL GetOneTimeRange(long Id, DATE * pMinTime, DATE * pMaxTime)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_PDATE VTS_PDATE ;
		InvokeHelper(0x4b, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Id, pMinTime, pMaxTime);
		return result;
	}
	BOOL GetOneValueRange(long Id, float * pMinValue, float * pMaxValue)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_PR4 VTS_PR4 ;
		InvokeHelper(0x4c, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Id, pMinValue, pMaxValue);
		return result;
	}
	BOOL GetOneFirstPos(long Id, DATE * pTime, float * pValue, BOOL bLast)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_PDATE VTS_PR4 VTS_BOOL ;
		InvokeHelper(0x4d, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Id, pTime, pValue, bLast);
		return result;
	}
	BOOL GetTimeRange(DATE * pMinTime, DATE * pMaxTime)
	{
		BOOL result;
		static BYTE parms[] = VTS_PDATE VTS_PDATE ;
		InvokeHelper(0x4e, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, pMinTime, pMaxTime);
		return result;
	}
	BOOL GetValueRange(float * pMinValue, float * pMaxValue)
	{
		BOOL result;
		static BYTE parms[] = VTS_PR4 VTS_PR4 ;
		InvokeHelper(0x4f, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, pMinValue, pMaxValue);
		return result;
	}
	void GetViableTimeRange(DATE * pMinTime, DATE * pMaxTime)
	{
		static BYTE parms[] = VTS_PDATE VTS_PDATE ;
		InvokeHelper(0x50, DISPATCH_METHOD, VT_EMPTY, NULL, parms, pMinTime, pMaxTime);
	}
	long AddMemMainData(long pMemMainData, long MemSize, BOOL bAddTrail)
	{
		long result;
		static BYTE parms[] = VTS_I4 VTS_I4 VTS_BOOL ;
		InvokeHelper(0x51, DISPATCH_METHOD, VT_I4, (void*)&result, parms, pMemMainData, MemSize, bAddTrail);
		return result;
	}
	BOOL ShowCurve(long Id, BOOL bShow)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_BOOL ;
		InvokeHelper(0x52, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Id, bShow);
		return result;
	}
	void SetFootNote(LPCTSTR pFootNote)
	{
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x54, DISPATCH_METHOD, VT_EMPTY, NULL, parms, pFootNote);
	}
	CString GetFootNote()
	{
		CString result;
		InvokeHelper(0x55, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	long TrimCurve(long Id, short State, long nBegin, long nCount, short nStep, BOOL bAll)
	{
		long result;
		static BYTE parms[] = VTS_I4 VTS_I2 VTS_I4 VTS_I4 VTS_I2 VTS_BOOL ;
		InvokeHelper(0x56, DISPATCH_METHOD, VT_I4, (void*)&result, parms, Id, State, nBegin, nCount, nStep, bAll);
		return result;
	}
	short PrintCurve(long Id, DATE BTime, DATE ETime, short Mask, short LeftMargin, short TopMargin, short RightMargin, short BottomMargin, LPCTSTR pTitle, LPCTSTR pFootNote, short Flag, BOOL bAll)
	{
		short result;
		static BYTE parms[] = VTS_I4 VTS_DATE VTS_DATE VTS_I2 VTS_I2 VTS_I2 VTS_I2 VTS_I2 VTS_BSTR VTS_BSTR VTS_I2 VTS_BOOL ;
		InvokeHelper(0x57, DISPATCH_METHOD, VT_I2, (void*)&result, parms, Id, BTime, ETime, Mask, LeftMargin, TopMargin, RightMargin, BottomMargin, pTitle, pFootNote, Flag, bAll);
		return result;
	}
	long GetEventMask()
	{
		long result;
		InvokeHelper(0x58, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	long GetScaleNums()
	{
		long result;
		InvokeHelper(0x59, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	long ReportPageInfo()
	{
		long result;
		InvokeHelper(0x5a, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	BOOL ShowLegend(LPCTSTR pSign, BOOL bShow)
	{
		BOOL result;
		static BYTE parms[] = VTS_BSTR VTS_BOOL ;
		InvokeHelper(0x5b, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, pSign, bShow);
		return result;
	}
	BOOL SelectCurve(long Id, BOOL bSelect)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_BOOL ;
		InvokeHelper(0x5c, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Id, bSelect);
		return result;
	}
	short DragCurve(short xStep, short yStep, BOOL bUpdate)
	{
		short result;
		static BYTE parms[] = VTS_I2 VTS_I2 VTS_BOOL ;
		InvokeHelper(0x5d, DISPATCH_METHOD, VT_I2, (void*)&result, parms, xStep, yStep, bUpdate);
		return result;
	}
	BOOL VCenterCurve(long Id, BOOL bUpdate)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_BOOL ;
		InvokeHelper(0x5e, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Id, bUpdate);
		return result;
	}
	BOOL GetSelectedCurve(long * pId)
	{
		BOOL result;
		static BYTE parms[] = VTS_PI4 ;
		InvokeHelper(0x5f, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, pId);
		return result;
	}
	void EnableAdjustZOrder(BOOL bEnable)
	{
		static BYTE parms[] = VTS_BOOL ;
		InvokeHelper(0x60, DISPATCH_METHOD, VT_EMPTY, NULL, parms, bEnable);
	}
	BOOL IsSelected(long Id)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x61, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Id);
		return result;
	}
	BOOL IsLegendVisible(LPCTSTR pSign)
	{
		BOOL result;
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x62, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, pSign);
		return result;
	}
	BOOL IsCurveVisible(long Id)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x63, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Id);
		return result;
	}
	BOOL IsCurveInCanvas(long Id)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x64, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Id);
		return result;
	}
	BOOL GotoCurve(long Id)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x65, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Id);
		return result;
	}
	void EnableZoom(BOOL bEnable)
	{
		static BYTE parms[] = VTS_BOOL ;
		InvokeHelper(0x66, DISPATCH_METHOD, VT_EMPTY, NULL, parms, bEnable);
	}
	long GetCurveLength(long Id)
	{
		long result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x67, DISPATCH_METHOD, VT_I4, (void*)&result, parms, Id);
		return result;
	}
	CString GetLuaVer()
	{
		CString result;
		InvokeHelper(0x68, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	DATE GetTimeData(short nCurveIndex, long nIndex)
	{
		DATE result;
		static BYTE parms[] = VTS_I2 VTS_I4 ;
		InvokeHelper(0x69, DISPATCH_METHOD, VT_DATE, (void*)&result, parms, nCurveIndex, nIndex);
		return result;
	}
	float GetValueData(short nCurveIndex, long nIndex)
	{
		float result;
		static BYTE parms[] = VTS_I2 VTS_I4 ;
		InvokeHelper(0x6a, DISPATCH_METHOD, VT_R4, (void*)&result, parms, nCurveIndex, nIndex);
		return result;
	}
	short GetState(short nCurveIndex, long nIndex)
	{
		short result;
		static BYTE parms[] = VTS_I2 VTS_I4 ;
		InvokeHelper(0x6b, DISPATCH_METHOD, VT_I2, (void*)&result, parms, nCurveIndex, nIndex);
		return result;
	}
	BOOL InsertMainData(short nCurveIndex, long nIndex, LPCTSTR pTime, float Value, short State, short Position, short Mask)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 VTS_I4 VTS_BSTR VTS_R4 VTS_I2 VTS_I2 VTS_I2 ;
		InvokeHelper(0x6c, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, nCurveIndex, nIndex, pTime, Value, State, Position, Mask);
		return result;
	}
	BOOL InsertMainData2(short nCurveIndex, long nIndex, DATE Time, float Value, short State, short Position, short Mask)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 VTS_I4 VTS_DATE VTS_R4 VTS_I2 VTS_I2 VTS_I2 ;
		InvokeHelper(0x6d, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, nCurveIndex, nIndex, Time, Value, State, Position, Mask);
		return result;
	}
	BOOL CanContinueEnum(long Id, short nCurveIndex, long nIndex)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_I2 VTS_I4 ;
		InvokeHelper(0x6e, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Id, nCurveIndex, nIndex);
		return result;
	}
	BOOL DelPoint(short nCurveIndex, long nIndex)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 VTS_I4 ;
		InvokeHelper(0x6f, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, nCurveIndex, nIndex);
		return result;
	}
	short GetCurveCount()
	{
		short result;
		InvokeHelper(0x70, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
		return result;
	}
	long GetCurve(short nIndex)
	{
		long result;
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0x71, DISPATCH_METHOD, VT_I4, (void*)&result, parms, nIndex);
		return result;
	}
	BOOL RemoveBitmapHandle(long hBitmap, BOOL bDel)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_BOOL ;
		InvokeHelper(0x72, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, hBitmap, bDel);
		return result;
	}
	BOOL RemoveBitmapHandle2(short nIndex, BOOL bDel)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 VTS_BOOL ;
		InvokeHelper(0x73, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, nIndex, bDel);
		return result;
	}
	long GetBitmap(short nIndex)
	{
		long result;
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0x74, DISPATCH_METHOD, VT_I4, (void*)&result, parms, nIndex);
		return result;
	}
	short GetBitmapState(short nIndex)
	{
		short result;
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0x75, DISPATCH_METHOD, VT_I2, (void*)&result, parms, nIndex);
		return result;
	}
	short GetBitmapState2(long hBitmap)
	{
		short result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x76, DISPATCH_METHOD, VT_I2, (void*)&result, parms, hBitmap);
		return result;
	}
	BOOL SetBuddy(long hBuddy, short State)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_I2 ;
		InvokeHelper(0x77, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, hBuddy, State);
		return result;
	}
	short GetBuddyCount()
	{
		short result;
		InvokeHelper(0x78, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
		return result;
	}
	long GetBuddy(short nIndex)
	{
		long result;
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0x79, DISPATCH_METHOD, VT_I4, (void*)&result, parms, nIndex);
		return result;
	}
	void SetCurveTitle(LPCTSTR pCurveTitle)
	{
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x7a, DISPATCH_METHOD, VT_EMPTY, NULL, parms, pCurveTitle);
	}
	CString GetCurveTitle()
	{
		CString result;
		InvokeHelper(0x7b, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	BOOL SetHUnit(LPCTSTR pHUnit)
	{
		BOOL result;
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x7c, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, pHUnit);
		return result;
	}
	CString GetHUnit()
	{
		CString result;
		InvokeHelper(0x7d, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	BOOL SetHPrecision(short Precision)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0x7e, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Precision);
		return result;
	}
	short GetHPrecision()
	{
		short result;
		InvokeHelper(0x7f, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
		return result;
	}
	BOOL SetCurveIndex(long Id, short nIndex)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_I2 ;
		InvokeHelper(0x80, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Id, nIndex);
		return result;
	}
	short GetCurveIndex(long Id)
	{
		short result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x81, DISPATCH_METHOD, VT_I2, (void*)&result, parms, Id);
		return result;
	}
	BOOL SetGridMode(short GridMode)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0x82, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, GridMode);
		return result;
	}
	short GetGridMode()
	{
		short result;
		InvokeHelper(0x83, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
		return result;
	}
	void SetBenchmark(DATE Time, float Value)
	{
		static BYTE parms[] = VTS_DATE VTS_R4 ;
		InvokeHelper(0x84, DISPATCH_METHOD, VT_EMPTY, NULL, parms, Time, Value);
	}
	void GetBenchmark(DATE * pTime, float * pValue)
	{
		static BYTE parms[] = VTS_PDATE VTS_PR4 ;
		InvokeHelper(0x85, DISPATCH_METHOD, VT_EMPTY, NULL, parms, pTime, pValue);
	}
	short GetPower(long Id)
	{
		short result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x86, DISPATCH_METHOD, VT_I2, (void*)&result, parms, Id);
		return result;
	}
	long TrimCurve2(long Id, short State, DATE BTime, DATE ETime, short Mask, short nStep, BOOL bAll)
	{
		long result;
		static BYTE parms[] = VTS_I4 VTS_I2 VTS_DATE VTS_DATE VTS_I2 VTS_I2 VTS_BOOL ;
		InvokeHelper(0x87, DISPATCH_METHOD, VT_I4, (void*)&result, parms, Id, State, BTime, ETime, Mask, nStep, bAll);
		return result;
	}
	BOOL ChangeId(long Id, long NewId)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_I4 ;
		InvokeHelper(0x88, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Id, NewId);
		return result;
	}
	BOOL CloneCurve(long Id, long NewId)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_I4 ;
		InvokeHelper(0x89, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Id, NewId);
		return result;
	}
	BOOL UniteCurve(long DesId, long nInsertPos, long Id, long nBegin, long nCount)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_I4 ;
		InvokeHelper(0x8a, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, DesId, nInsertPos, Id, nBegin, nCount);
		return result;
	}
	BOOL UniteCurve2(long DesId, long nInsertPos, long Id, DATE BTime, DATE ETime, short Mask)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_I4 VTS_I4 VTS_DATE VTS_DATE VTS_I2 ;
		InvokeHelper(0x8b, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, DesId, nInsertPos, Id, BTime, ETime, Mask);
		return result;
	}
	BOOL UniteCurve3(long DesId, DATE fInsertPos, long Id, long nBegin, long nCount)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_DATE VTS_I4 VTS_I4 VTS_I4 ;
		InvokeHelper(0x8c, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, DesId, fInsertPos, Id, nBegin, nCount);
		return result;
	}
	BOOL UniteCurve4(long DesId, DATE fInsertPos, long Id, DATE BTime, DATE ETime, short Mask)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_DATE VTS_I4 VTS_DATE VTS_DATE VTS_I2 ;
		InvokeHelper(0x8d, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, DesId, fInsertPos, Id, BTime, ETime, Mask);
		return result;
	}
	BOOL OffSetCurve(long Id, double TimeSpan, float ValueStep, short Operator)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_R8 VTS_R4 VTS_I2 ;
		InvokeHelper(0x8e, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Id, TimeSpan, ValueStep, Operator);
		return result;
	}
	long ArithmeticOperate(long DesId, long Id, short Operator)
	{
		long result;
		static BYTE parms[] = VTS_I4 VTS_I4 VTS_I2 ;
		InvokeHelper(0x8f, DISPATCH_METHOD, VT_I4, (void*)&result, parms, DesId, Id, Operator);
		return result;
	}
	void ClearTempBuff()
	{
		InvokeHelper(0x90, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	BOOL PreMallocMem(long Id, long size)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_I4 ;
		InvokeHelper(0x91, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Id, size);
		return result;
	}
	long GetMemSize(long Id)
	{
		long result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x92, DISPATCH_METHOD, VT_I4, (void*)&result, parms, Id);
		return result;
	}
	BOOL IsCurve(long Id)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x93, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Id);
		return result;
	}
	void SetSorptionRange(short Range)
	{
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0x94, DISPATCH_METHOD, VT_EMPTY, NULL, parms, Range);
	}
	short GetSorptionRange()
	{
		short result;
		InvokeHelper(0x95, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
		return result;
	}
	BOOL IsLegend(LPCTSTR pSign)
	{
		BOOL result;
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x96, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, pSign);
		return result;
	}
	short AddLegendHelper(long Id, LPCTSTR pSign, long PenColor, short PenStyle, short LineWidth, BOOL bUpdate)
	{
		short result;
		static BYTE parms[] = VTS_I4 VTS_BSTR VTS_I4 VTS_I2 VTS_I2 VTS_BOOL ;
		InvokeHelper(0x97, DISPATCH_METHOD, VT_I2, (void*)&result, parms, Id, pSign, PenColor, PenStyle, LineWidth, bUpdate);
		return result;
	}
	BOOL GetActualPoint(long x, long y, DATE * pTime, float * pValue)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_I4 VTS_PDATE VTS_PR4 ;
		InvokeHelper(0x98, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, x, y, pTime, pValue);
		return result;
	}
	long GetPointFromScreenPoint(long Id, long x, long y, short MaxRange)
	{
		long result;
		static BYTE parms[] = VTS_I4 VTS_I4 VTS_I4 VTS_I2 ;
		InvokeHelper(0x99, DISPATCH_METHOD, VT_I4, (void*)&result, parms, Id, x, y, MaxRange);
		return result;
	}
	void EnableFullScreen(BOOL bEnable)
	{
		static BYTE parms[] = VTS_BOOL ;
		InvokeHelper(0x9a, DISPATCH_METHOD, VT_EMPTY, NULL, parms, bEnable);
	}
	DATE GetEndTime()
	{
		DATE result;
		InvokeHelper(0x9b, DISPATCH_METHOD, VT_DATE, (void*)&result, NULL);
		return result;
	}
	float GetEndValue()
	{
		float result;
		InvokeHelper(0x9c, DISPATCH_METHOD, VT_R4, (void*)&result, NULL);
		return result;
	}
	void SetZLength(short ZLength)
	{
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0x9d, DISPATCH_METHOD, VT_EMPTY, NULL, parms, ZLength);
	}
	short GetZLength()
	{
		short result;
		InvokeHelper(0x9e, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
		return result;
	}
	BOOL SetCanvasBkBitmap(short nIndex)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0x9f, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, nIndex);
		return result;
	}
	short GetCanvasBkBitmap()
	{
		short result;
		InvokeHelper(0xa0, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
		return result;
	}
	void SetLeftBkColor(long Color)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0xa1, DISPATCH_METHOD, VT_EMPTY, NULL, parms, Color);
	}
	long GetLeftBkColor()
	{
		long result;
		InvokeHelper(0xa2, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	void SetBottomBkColor(long Color)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0xa3, DISPATCH_METHOD, VT_EMPTY, NULL, parms, Color);
	}
	long GetBottomBkColor()
	{
		long result;
		InvokeHelper(0xa4, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	BOOL SetZOffset(long Id, short nOffset, BOOL bUpdate)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_I2 VTS_BOOL ;
		InvokeHelper(0xa5, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Id, nOffset, bUpdate);
		return result;
	}
	long GetZOffset(long Id)
	{
		long result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0xa6, DISPATCH_METHOD, VT_I4, (void*)&result, parms, Id);
		return result;
	}
	void EnableFocusState(BOOL bEnable)
	{
		static BYTE parms[] = VTS_BOOL ;
		InvokeHelper(0xa7, DISPATCH_METHOD, VT_EMPTY, NULL, parms, bEnable);
	}
	BOOL SetReviseToolTip(short Type)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0xa8, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Type);
		return result;
	}
	short GetReviseToolTip()
	{
		short result;
		InvokeHelper(0xa9, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
		return result;
	}
	long ExportMetaFile(LPCTSTR pFileName, long Id, long nBegin, long nCount, BOOL bAll, short Style)
	{
		long result;
		static BYTE parms[] = VTS_BSTR VTS_I4 VTS_I4 VTS_I4 VTS_BOOL VTS_I2 ;
		InvokeHelper(0xaa, DISPATCH_METHOD, VT_I4, (void*)&result, parms, pFileName, Id, nBegin, nCount, bAll, Style);
		return result;
	}
	void LimitOnePage(BOOL bLimit)
	{
		static BYTE parms[] = VTS_BOOL ;
		InvokeHelper(0xab, DISPATCH_METHOD, VT_EMPTY, NULL, parms, bLimit);
	}
	BOOL FixCoor(DATE MinTime, DATE MaxTime, float MinValue, float MaxValue, short Mask)
	{
		BOOL result;
		static BYTE parms[] = VTS_DATE VTS_DATE VTS_R4 VTS_R4 VTS_I2 ;
		InvokeHelper(0xac, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, MinTime, MaxTime, MinValue, MaxValue, Mask);
		return result;
	}
	short GetFixCoor(DATE * pMinTime, DATE * pMaxTime, float * pMinValue, float * pMaxValue)
	{
		short result;
		static BYTE parms[] = VTS_PDATE VTS_PDATE VTS_PR4 VTS_PR4 ;
		InvokeHelper(0xad, DISPATCH_METHOD, VT_I2, (void*)&result, parms, pMinTime, pMaxTime, pMinValue, pMaxValue);
		return result;
	}
	BOOL RefreshLimitedOrFixedCoor()
	{
		BOOL result;
		InvokeHelper(0xae, DISPATCH_METHOD, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	BOOL SetCanvasBkMode(short CanvasBkMode)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0xaf, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, CanvasBkMode);
		return result;
	}
	short GetCanvasBkMode()
	{
		short result;
		InvokeHelper(0xb0, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
		return result;
	}
	void EnablePreview(BOOL bEnable)
	{
		static BYTE parms[] = VTS_BOOL ;
		InvokeHelper(0xb1, DISPATCH_METHOD, VT_EMPTY, NULL, parms, bEnable);
	}
	void SetWaterMark(LPCTSTR pWaterMark)
	{
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0xb2, DISPATCH_METHOD, VT_EMPTY, NULL, parms, pWaterMark);
	}
	long GetSysState()
	{
		long result;
		InvokeHelper(0xb3, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	void SetTension(float Tension)
	{
		static BYTE parms[] = VTS_R4 ;
		InvokeHelper(0xb4, DISPATCH_METHOD, VT_EMPTY, NULL, parms, Tension);
	}
	float GetTension()
	{
		float result;
		InvokeHelper(0xb5, DISPATCH_METHOD, VT_R4, (void*)&result, NULL);
		return result;
	}
	long GetFont()
	{
		long result;
		InvokeHelper(0xb6, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	BOOL SetXYFormat(LPCTSTR pSign, short Format)
	{
		BOOL result;
		static BYTE parms[] = VTS_BSTR VTS_I2 ;
		InvokeHelper(0xb7, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, pSign, Format);
		return result;
	}
	short GetXYFormat(LPCTSTR pSign)
	{
		short result;
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0xb8, DISPATCH_METHOD, VT_I2, (void*)&result, parms, pSign);
		return result;
	}
	short GetXYFormat2(short nIndex)
	{
		short result;
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0xb9, DISPATCH_METHOD, VT_I2, (void*)&result, parms, nIndex);
		return result;
	}
	long LoadPlugIn(LPCTSTR pFileName, short Type, long Mask)
	{
		long result;
		static BYTE parms[] = VTS_BSTR VTS_I2 VTS_I4 ;
		InvokeHelper(0xba, DISPATCH_METHOD, VT_I4, (void*)&result, parms, pFileName, Type, Mask);
		return result;
	}
	BOOL AppendLegendEx(LPCTSTR pSign, long BeginNodeColor, long EndNodeColor, long SelectedNodeColor, short NodeModeEx)
	{
		BOOL result;
		static BYTE parms[] = VTS_BSTR VTS_I4 VTS_I4 VTS_I4 VTS_I2 ;
		InvokeHelper(0xbb, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, pSign, BeginNodeColor, EndNodeColor, SelectedNodeColor, NodeModeEx);
		return result;
	}
	BOOL GetLegendEx(LPCTSTR pSign, long * pBeginNodeColor, long * pEndNodeColor, long * pSelectedNodeColor, short * pNodeModeEx)
	{
		BOOL result;
		static BYTE parms[] = VTS_BSTR VTS_PI4 VTS_PI4 VTS_PI4 VTS_PI2 ;
		InvokeHelper(0xbc, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, pSign, pBeginNodeColor, pEndNodeColor, pSelectedNodeColor, pNodeModeEx);
		return result;
	}
	BOOL GetLegendEx2(short nIndex, long * pBeginNodeColor, long * pEndNodeColor, long * pSelectedNodeColor, short * pNodeModeEx)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 VTS_PI4 VTS_PI4 VTS_PI4 VTS_PI2 ;
		InvokeHelper(0xbd, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, nIndex, pBeginNodeColor, pEndNodeColor, pSelectedNodeColor, pNodeModeEx);
		return result;
	}
	long GetSelectedNodeIndex(long Id)
	{
		long result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0xbe, DISPATCH_METHOD, VT_I4, (void*)&result, parms, Id);
		return result;
	}
	BOOL SetSelectedNodeIndex(long Id, long NewNodeIndex)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_I4 ;
		InvokeHelper(0xbf, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Id, NewNodeIndex);
		return result;
	}
	long LoadLuaScript(LPCTSTR pFileName, short Type, long Mask)
	{
		long result;
		static BYTE parms[] = VTS_BSTR VTS_I2 VTS_I4 ;
		InvokeHelper(0xc0, DISPATCH_METHOD, VT_I4, (void*)&result, parms, pFileName, Type, Mask);
		return result;
	}
	void SetShortcutKeyMask(long ShortcutKey)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0xc1, DISPATCH_METHOD, VT_EMPTY, NULL, parms, ShortcutKey);
	}
	long GetShortcutKeyMask()
	{
		long result;
		InvokeHelper(0xc2, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	long GetFrceHDC()
	{
		long result;
		InvokeHelper(0xc3, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	BOOL SetBottomSpace(short Space)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0xc4, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Space);
		return result;
	}
	short GetBottomSpace()
	{
		short result;
		InvokeHelper(0xc5, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
		return result;
	}
	CString GetEndTime2()
	{
		CString result;
		InvokeHelper(0xc6, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	CString GetTimeData2(short nCurveIndex, long nIndex)
	{
		CString result;
		static BYTE parms[] = VTS_I2 VTS_I4 ;
		InvokeHelper(0xc7, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, nCurveIndex, nIndex);
		return result;
	}
	short AddComment(DATE Time, float Value, short Position, short nBkBitmap, short Width, short Height, long TransColor, LPCTSTR pComment, long TextColor, short XOffSet, short YOffSet, BOOL bUpdate)
	{
		short result;
		static BYTE parms[] = VTS_DATE VTS_R4 VTS_I2 VTS_I2 VTS_I2 VTS_I2 VTS_I4 VTS_BSTR VTS_I4 VTS_I2 VTS_I2 VTS_BOOL ;
		InvokeHelper(0xc8, DISPATCH_METHOD, VT_I2, (void*)&result, parms, Time, Value, Position, nBkBitmap, Width, Height, TransColor, pComment, TextColor, XOffSet, YOffSet, bUpdate);
		return result;
	}
	BOOL DelComment(long nIndex, BOOL bAll, BOOL bUpdate)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_BOOL VTS_BOOL ;
		InvokeHelper(0xc9, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, nIndex, bAll, bUpdate);
		return result;
	}
	long GetCommentNum()
	{
		long result;
		InvokeHelper(0xca, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	BOOL GetComment(long nIndex, DATE * pTime, float * pValue, short * pPosition, short * pBkBitmap, short * pWidth, short * pHeight, long * pTransColor, BSTR * pComment, long * pTextColor, short * pXOffSet, short * pYOffSet)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_PDATE VTS_PR4 VTS_PI2 VTS_PI2 VTS_PI2 VTS_PI2 VTS_PI4 VTS_PBSTR VTS_PI4 VTS_PI2 VTS_PI2 ;
		InvokeHelper(0xcb, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, nIndex, pTime, pValue, pPosition, pBkBitmap, pWidth, pHeight, pTransColor, pComment, pTextColor, pXOffSet, pYOffSet);
		return result;
	}
	short SetComment(long nIndex, DATE Time, float Value, short Position, short nBkBitmap, short Width, short Height, long TransColor, LPCTSTR pComment, long TextColor, short XOffSet, short YOffSet, short Mask, BOOL bUpdate)
	{
		short result;
		static BYTE parms[] = VTS_I4 VTS_DATE VTS_R4 VTS_I2 VTS_I2 VTS_I2 VTS_I2 VTS_I4 VTS_BSTR VTS_I4 VTS_I2 VTS_I2 VTS_I2 VTS_BOOL ;
		InvokeHelper(0xcc, DISPATCH_METHOD, VT_I2, (void*)&result, parms, nIndex, Time, Value, Position, nBkBitmap, Width, Height, TransColor, pComment, TextColor, XOffSet, YOffSet, Mask, bUpdate);
		return result;
	}
	BOOL SwapCommentIndex(long nIndex, long nOldIndex, BOOL bUpdate)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_I4 VTS_BOOL ;
		InvokeHelper(0xcd, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, nIndex, nOldIndex, bUpdate);
		return result;
	}
	BOOL ShowComment(long nIndex, BOOL bShow, BOOL bUpdate)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_BOOL VTS_BOOL ;
		InvokeHelper(0xce, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, nIndex, bShow, bUpdate);
		return result;
	}
	BOOL IsCommentVisiable(long nIndex)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0xcf, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, nIndex);
		return result;
	}
	void SetEventMask(long Event)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0xd0, DISPATCH_METHOD, VT_EMPTY, NULL, parms, Event);
	}
	BOOL SetFixedZoomMode(short ZoomMode)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0xd1, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, ZoomMode);
		return result;
	}
	short GetFixedZoomMode()
	{
		short result;
		InvokeHelper(0xd2, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
		return result;
	}
	BOOL FixedZoom(short ZoomMode, short x, short y, BOOL bHoldMode)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 VTS_I2 VTS_I2 VTS_BOOL ;
		InvokeHelper(0xd3, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, ZoomMode, x, y, bHoldMode);
		return result;
	}
	BOOL SetCommentPosition(short Position)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0xd4, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Position);
		return result;
	}
	short GetCommentPosition()
	{
		short result;
		InvokeHelper(0xd5, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
		return result;
	}
	BOOL GetPixelPoint(DATE Time, float Value, long * px, long * py)
	{
		BOOL result;
		static BYTE parms[] = VTS_DATE VTS_R4 VTS_PI4 VTS_PI4 ;
		InvokeHelper(0xd6, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Time, Value, px, py);
		return result;
	}
	BOOL GetMemInfo(long * pTempBuffSize, long * pAllBuffSize, float * pUseRate, long * pId)
	{
		BOOL result;
		static BYTE parms[] = VTS_PI4 VTS_PI4 VTS_PR4 VTS_PI4 ;
		InvokeHelper(0xd7, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, pTempBuffSize, pAllBuffSize, pUseRate, pId);
		return result;
	}
	BOOL IsCurveClosed(long Id)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0xd8, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Id);
		return result;
	}
	BOOL GetPosData(short nCurveIndex, long nIndex, long * px, long * py)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 VTS_I4 VTS_PI4 VTS_PI4 ;
		InvokeHelper(0xd9, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, nCurveIndex, nIndex, px, py);
		return result;
	}
	void EnableHZoom(BOOL bEnable)
	{
		static BYTE parms[] = VTS_BOOL ;
		InvokeHelper(0xda, DISPATCH_METHOD, VT_EMPTY, NULL, parms, bEnable);
	}
	BOOL SetHZoom(short Zoom)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0xdb, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Zoom);
		return result;
	}
	short GetHZoom()
	{
		short result;
		InvokeHelper(0xdc, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
		return result;
	}
	BOOL MoveCurveToLegend(long Id, LPCTSTR pSign)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_BSTR ;
		InvokeHelper(0xdd, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Id, pSign);
		return result;
	}
	BOOL ChangeLegendName(LPCTSTR pFrom, LPCTSTR pTo)
	{
		BOOL result;
		static BYTE parms[] = VTS_BSTR VTS_BSTR ;
		InvokeHelper(0xde, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, pFrom, pTo);
		return result;
	}
	BOOL SetAutoRefresh(short TimeInterval, short NumInterval)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 VTS_I2 ;
		InvokeHelper(0xdf, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, TimeInterval, NumInterval);
		return result;
	}
	long GetAutoRefresh()
	{
		long result;
		InvokeHelper(0xe0, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	void EnableSelectCurve(BOOL bEnable)
	{
		static BYTE parms[] = VTS_BOOL ;
		InvokeHelper(0xe1, DISPATCH_METHOD, VT_EMPTY, NULL, parms, bEnable);
	}
	void SetToolTipDelay(short Delay)
	{
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0xe2, DISPATCH_METHOD, VT_EMPTY, NULL, parms, Delay);
	}
	short GetToolTipDelay()
	{
		short result;
		InvokeHelper(0xe3, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
		return result;
	}
	BOOL SetLimitOnePageMode(short Mode)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0xe4, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Mode);
		return result;
	}
	short GetLimitOnePageMode()
	{
		short result;
		InvokeHelper(0xe5, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
		return result;
	}
	BOOL AddInfiniteCurve(long Id, DATE Time, float Value, short State, BOOL bUpdate)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_DATE VTS_R4 VTS_I2 VTS_BOOL ;
		InvokeHelper(0xe6, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Id, Time, Value, State, bUpdate);
		return result;
	}
	BOOL DelInfiniteCurve(long Id, BOOL bAll, BOOL bUpdate)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 VTS_BOOL VTS_BOOL ;
		InvokeHelper(0xe7, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Id, bAll, bUpdate);
		return result;
	}
	BOOL SetGraduationSize(long size)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0xe9, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, size);
		return result;
	}
	long GetGraduationSize()
	{
		long result;
		InvokeHelper(0xea, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	void SetMouseWheelMode(short Mode)
	{
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0xeb, DISPATCH_METHOD, VT_EMPTY, NULL, parms, Mode);
	}
	short GetMouseWheelMode()
	{
		short result;
		InvokeHelper(0xec, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
		return result;
	}
	BOOL SetMouseWheelSpeed(short Speed)
	{
		BOOL result;
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0xed, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, Speed);
		return result;
	}
	short GetMouseWheelSpeed()
	{
		short result;
		InvokeHelper(0xee, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
		return result;
	}
	BOOL SetHLegend(LPCTSTR pHLegend)
	{
		BOOL result;
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0xef, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, pHLegend);
		return result;
	}
	CString GetHLegend()
	{
		CString result;
		InvokeHelper(0xf0, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	void Refresh()
	{
		InvokeHelper(DISPID_REFRESH, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}

// Properties
//

long GetForeColor()
{
	long result;
	GetProperty(0x1, VT_I4, (void*)&result);
	return result;
}
void SetForeColor(long propVal)
{
	SetProperty(0x1, VT_I4, propVal);
}
long GetBackColor()
{
	long result;
	GetProperty(0x2, VT_I4, (void*)&result);
	return result;
}
void SetBackColor(long propVal)
{
	SetProperty(0x2, VT_I4, propVal);
}
long GetAxisColor()
{
	long result;
	GetProperty(0x3, VT_I4, (void*)&result);
	return result;
}
void SetAxisColor(long propVal)
{
	SetProperty(0x3, VT_I4, propVal);
}
long GetGridColor()
{
	long result;
	GetProperty(0x4, VT_I4, (void*)&result);
	return result;
}
void SetGridColor(long propVal)
{
	SetProperty(0x4, VT_I4, propVal);
}
long GetPageChangeMSG()
{
	long result;
	GetProperty(0x5, VT_I4, (void*)&result);
	return result;
}
void SetPageChangeMSG(long propVal)
{
	SetProperty(0x5, VT_I4, propVal);
}
long GetMSGRecWnd()
{
	long result;
	GetProperty(0x6, VT_I4, (void*)&result);
	return result;
}
void SetMSGRecWnd(long propVal)
{
	SetProperty(0x6, VT_I4, propVal);
}
long GetTitleColor()
{
	long result;
	GetProperty(0x7, VT_I4, (void*)&result);
	return result;
}
void SetTitleColor(long propVal)
{
	SetProperty(0x7, VT_I4, propVal);
}
long GetFootNoteColor()
{
	long result;
	GetProperty(0x8, VT_I4, (void*)&result);
	return result;
}
void SetFootNoteColor(long propVal)
{
	SetProperty(0x8, VT_I4, propVal);
}
long GetRegister1()
{
	long result;
	GetProperty(0xe8, VT_I4, (void*)&result);
	return result;
}
void SetRegister1(long propVal)
{
	SetProperty(0xe8, VT_I4, propVal);
}


};
