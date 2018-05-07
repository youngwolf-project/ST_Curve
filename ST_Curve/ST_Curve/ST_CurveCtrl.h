#pragma once

#include "structs.h"
#include <gdiplus.h>

#define BUDDYMSG	(WM_APP + 1)
/*
wParam	0			设置联动服务器给客户机
lParam	HWND		联动服务器窗口句柄
发送方向：联动服务器->联动客户机

wParam	1			取消联动关系
lParam	HWND(0)		联动客户机窗口句柄
发送方向：双向，如果联动服务器发消息给联动客户机，则lParam为0，反之为客户机窗口句柄

wParam	2			设置当前页开始时间
lParam	HCOOR_TYPE*		新的开始时间
发送方向：双向

wParam	3			设置时间间隔
lParam	HCOOR_TYPE*		新的时间间隔
发送方向：双向

wParam	4			设置放大率
lParam	short		新的放大率
发送方向：双向

wParam	5			设置纵坐标位置，如果纵坐标位置不同，就算横坐标开始时间相同，那么曲线还是会有一点不同步
lParam	short		新的纵坐标位置，其值就是所有客户机和服务器的最小LeftSpace的最大值
发送方向：联动服务器->联动客户机

wParam	6			询问客户机的LeftSpace，返回客户机的最小LeftSpace
lParam	0
发送方向：联动服务器->联动客户机

wParam	7			客户机请求重新确定LeftSpace，即所有客户机和服务器的最小LeftSpace的最大值
lParam	0
发送方向：联动客户机->联动服务器

wParam	8			设置水平放大率
lParam	short		新的放大率
*/

//#define MOVEBUDDYMSG	(WM_APP + 2) //已经放弃添加此功能，因为二次开发者来实现此功能将更加的灵活多样
/*
BOOL SetMoveBuddy(OLE_HANDLE hBuddy, short Relation, long MaxExpand); //设置移动伙伴关系
hBuddy：对方窗口句柄HWND，如果为0，则取消移动伙伴关系
Relation，从低位起：
1－本窗口顶部与hBuddy底部关联；2－本窗口底部与hBuddy顶部关联；此时两窗口的扩张、收缩行为相反
3－本窗口顶部与hBuddy顶部关联；4－本窗口底部与hBuddy底部关联；此时两窗口的扩张、收缩行为相同
MaxExpand：本控件的扩张、收缩值
高二字节为上边沿的最大扩张、收缩值（绝对值）
低二字节为下边沿的最大扩张、收缩值（绝对值）
如果最大扩张值为0，则最大扩张到伙伴的窗口高度为0
如果最大收缩值为0，则最大收缩到自己的窗口高度为0

wParam	1			扩张、收缩窗口上边沿
lParam				具体移动值，正为扩张，负为收缩

wParam	2			扩张、收缩窗口下边沿
lParam				具体移动值，正为扩张，负为收缩
*/

//#define VMOVE		(WM_APP + 3)

#define LEFTSPACE    5  //左空白，坐标最左边与左边框的距离
//#define VSTEP        21 //纵坐标屏幕步长
//#define HSTEP        21 //横坐标屏幕步长
//#define Hypotenuse	 29.698484809834996 //斜边

#define MOVEMODE     1 //移动模式
#define DRAGMODE	 2 //拖动模式
#define ZOOMIN       3 //放大模式
#define ZOOMOUT      4 //缩小模式

#define REPORTPAGE	 1
#define REPORTDELAY	 100

#define SHOWTOOLTIP  2
#define SHOWDELAY    100
//#define HIDETIP	 3
//#define HIDETIPF	 5000

#define BATCHEXPORTBMP 4

#define HIDEHELPTIP	 5 //控件在开始运行的时候，自动显示帮助，并且在10秒后自动隐藏
#define HIDECOPYRIGHTINFO 6 //控件在开始运行的时候，显示版权信息，并且在10秒后自动隐藏（优先级最低，还没有做）
#define HIDEDELAY	 10000

#define AUTOREFRESH	 7 //按时间间隔自动刷新

#define GETSTEP(V, ZOOM) (!(ZOOM) ? V : ((ZOOM) > 0 ? V / ((ZOOM) * .25f + 1) : V * (-(ZOOM) * .25f + 1)))

#define UPDATERECT(MASK, F) \
{ \
	if (Mask & (MASK)) \
		F(hDC); \
}

#define ERASEBKG(MASK, R, bMoveRect) \
if (Mask & (MASK)) \
{ \
	rect = R; \
	if (bMoveRect) \
		MOVERECT(rect, m_ShowMode); \
	BitBlt(hDC, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, hBackDC, rect.left, rect.top, SRCCOPY); \
	if (IsRectEmpty(&InvalidRect)) \
		InvalidRect = rect; \
	else \
		UnionRect(&InvalidRect, &InvalidRect, &rect); \
}

#define REFRESHFOCUS(F1, F2) \
{ \
	F1; \
	if (SysState & 0x100) \
	{ \
		SelectObject(hDC, GetStockObject(NULL_BRUSH)); \
		SelectObject(hDC, hAxisPen); \
		Rectangle(hDC, 0, 0, WinWidth, WinHeight); \
	} \
	else \
	{ \
		BitBlt(hDC, 0, 0, WinWidth, 1, hFrceDC, 0, 0, SRCCOPY); \
		BitBlt(hDC, WinWidth - 1, 0, 1, WinHeight, hFrceDC, WinWidth - 1, 0, SRCCOPY); \
		BitBlt(hDC, 0, 0, 1, WinHeight, hFrceDC, 0, 0, SRCCOPY); \
		BitBlt(hDC, 0, WinHeight - 1, WinWidth, 1, hFrceDC, 0, WinHeight - 1, SRCCOPY); \
	} \
	F2; \
}

#define MOVEPOINT(POINT, MASK) \
{ \
	if ((MASK) & 1) \
		POINT.x = WinWidth - POINT.x - 1; \
	if ((MASK) & 2) \
		POINT.y = WinHeight - POINT.y - 1; \
}

#define MOVERECT(RECT, MASK) \
{ \
	if ((MASK) & 1) \
	{ \
		auto Temp = RECT.right; \
		RECT.right = WinWidth - RECT.left; \
		RECT.left = WinWidth - Temp; \
	} \
	if ((MASK) & 2) \
	{ \
		auto Temp = RECT.bottom; \
		RECT.bottom = WinHeight - RECT.top; \
		RECT.top = WinHeight - Temp; \
	} \
}

#define MOVERECTLIMIT(RECT, LIMITRECT, MASK) \
{ \
	if ((MASK) & 1) \
	{ \
		auto Temp = RECT.right; \
		auto TotalWidth = LIMITRECT.left + LIMITRECT.right; \
		RECT.right = TotalWidth - RECT.left; \
		RECT.left = TotalWidth - Temp; \
	} \
	if ((MASK) & 2) \
	{ \
		auto Temp = RECT.bottom; \
		auto TotalHeight = LIMITRECT.top + LIMITRECT.bottom; \
		RECT.bottom = TotalHeight - RECT.top; \
		RECT.top = TotalHeight - Temp; \
	} \
}

#define CHANGEMOUSESTATE \
{ \
	POINT pt; \
	GetCursorPos(&pt); \
	ScreenToClient(&pt); \
	PostMessage(WM_MOUSEMOVE, 0, (pt.y << 16) + pt.x); \
	PostMessage(WM_SETCURSOR, (WPARAM) m_hWnd, HTCLIENT | WM_MOUSEMOVE); \
}

#define DELRANGE(CON1, CON2, C1, C2, C3, bUseMore, C4) \
{ \
	for (auto i = begin(MainDataListArr); i < end(MainDataListArr);) \
	{ \
		auto tempIter = i++; \
		if (bAll || Address == *tempIter) \
		{ \
			auto pDataVector = tempIter->pDataVector; \
			auto j = begin(*pDataVector); \
			if (bUseMore && 2 == tempIter->Power) \
				while (j < end(*pDataVector)) \
					if (C4) \
					{ \
						auto k = j++; \
						if (DoDelMainData(tempIter, k, j, bUpdate) & 8) \
						{ \
							i = tempIter; \
							break; \
						} \
					} \
					else \
						++j; \
			else \
			{ \
				CON1; \
				for (; j < end(*pDataVector) && (C1); ++j); \
				if (j < end(*pDataVector)) \
				{ \
					vector<MainData>::iterator k = NullDataIter; \
					if (C2) \
						k = end(*pDataVector); \
					else \
						for (CON2; k < end(*pDataVector) && (C3); ++k); \
					if (j < k && DoDelMainData(tempIter, j, k, bUpdate) & 8) \
						i = tempIter; \
				} \
			} \
		} \
	} \
}

#define CHANGE_MAP_MODE(HDC, SHOW_MODE) \
if (0 == ((SHOW_MODE) & 3)) \
{ \
	SetMapMode(HDC, MM_TEXT); \
	SetViewportOrgEx(HDC, 0, 0, nullptr); \
} \
else \
{ \
	CHANGE_PRINT_MAP_MODE(HDC, WinWidth, WinHeight, 0, 0, SHOW_MODE); \
}

#define CHANGE_PRINT_MAP_MODE(HDC, VIEW_WIDTH, VIEW_HEIGHT, XOFFSET, YOFFSET, SHOW_MODE) \
SetMapMode(HDC, MM_ANISOTROPIC); \
SetWindowExtEx(HDC, WinWidth, WinHeight, nullptr); \
switch ((SHOW_MODE) & 3) \
{ \
case 0: \
	SetViewportExtEx(HDC, VIEW_WIDTH, VIEW_HEIGHT, nullptr); \
	SetViewportOrgEx(HDC, XOFFSET, YOFFSET, nullptr); \
	break; \
case 1: \
	SetViewportExtEx(HDC, -(VIEW_WIDTH), VIEW_HEIGHT, nullptr); \
	SetViewportOrgEx(HDC, VIEW_WIDTH - 1 + XOFFSET, YOFFSET, nullptr); \
	break; \
case 2: \
	SetViewportExtEx(HDC, VIEW_WIDTH, -(VIEW_HEIGHT), nullptr); \
	SetViewportOrgEx(HDC, XOFFSET,  VIEW_HEIGHT - 1 + YOFFSET, nullptr); \
	break; \
case 3: \
	SetViewportExtEx(HDC, -(VIEW_WIDTH), -(VIEW_HEIGHT), nullptr); \
	SetViewportOrgEx(HDC, VIEW_WIDTH - 1 + XOFFSET, VIEW_HEIGHT - 1 + YOFFSET, nullptr); \
	break; \
}

//2012.8.5
//CHVIEWORG这个宏有什么作用？不调用后果是什么？
/*
#define CHVIEWORG(HDC, WIDTH, HEIGHT, MASK) \
switch ((MASK) & 3) \
{ \
case 1: \
	SetViewportOrgEx(HDC, WIDTH, 0, 0); \
	break; \
case 2: \
	SetViewportOrgEx(HDC, 0, HEIGHT, 0); \
	break; \
case 3: \
	SetViewportOrgEx(HDC, WIDTH, HEIGHT, 0); \
	break; \
}
*/
#define DRAWGRID(VAR1, ROOT1, END1, STEP, VAR2, ROOT2, END2) \
{ \
	VAR1 = ROOT1; \
	auto n = 0; \
	while (VAR1 < END1) \
	{ \
		if (!(SysState & 4) || !(n % (m_hInterval + 1))) \
			for (VAR2 = ROOT2; VAR2 < (END2); VAR2 += 7) \
				SetPixelV(hDC, ix, iy, m_gridColor); \
		VAR1 += STEP; \
		++n; \
	} \
}

#define DRAWGRID2(VAR1, ROOT1, END1, STEP, VAR2, ROOT2, END2) \
{ \
	VAR1 = ROOT1; \
	auto n = (END1 - ROOT1) / STEP; \
	while (VAR1 < END1) \
	{ \
		if (!(SysState & 4) || !(n % (m_vInterval + 1))) \
			for (VAR2 = ROOT2; VAR2 < (END2); VAR2 += 7) \
				SetPixelV(hDC, ix, iy, m_gridColor); \
		VAR1 += STEP; \
		--n; \
	} \
}

#define DRAWSOLIDGRIP(VAR, ROOT, END, STEP, BEGINX, BEGINY, ENDX, ENDY) \
{ \
	VAR = ROOT; \
	auto n = 0; \
	while (VAR < END) \
	{ \
		if (!(SysState & 4) || !(n % (m_hInterval + 1))) \
		{ \
			MoveToEx(hDC, BEGINX, BEGINY, nullptr); \
			LineTo(hDC, ENDX, ENDY); \
		} \
		VAR += STEP; \
		++n; \
	} \
}

#define DRAWSOLIDGRIP2(VAR, ROOT, END, STEP, BEGINX, BEGINY, ENDX, ENDY) \
{ \
	VAR = ROOT; \
	auto n = (END - ROOT) / STEP; \
	while (VAR < END) \
	{ \
		if (!(SysState & 4) || !(n % (m_vInterval + 1))) \
		{ \
			MoveToEx(hDC, BEGINX, BEGINY, nullptr); \
			LineTo(hDC, ENDX, ENDY); \
		} \
		VAR += STEP; \
		--n; \
	} \
}

#define CHECKCOLOR(COLOR) \
{ \
	COLOR &= 0x80FFFFFF; \
	if (COLOR & 0x80000000) \
	{ \
		COLOR &= 0xFFFFFF; \
		CHOOSECOLOR cc; \
		memset(&cc, 0, sizeof(CHOOSECOLOR)); \
		cc.lStructSize = sizeof(CHOOSECOLOR); \
		cc.hwndOwner = m_hWnd; \
		cc.rgbResult = COLOR; \
		cc.lpCustColors = CustClr; \
		cc.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT; \
		if (ChooseColor(&cc)) \
			COLOR =cc.rgbResult; \
		else \
			return; \
	} \
}

#define DEFAULT_foreColor		RGB(0xff, 0xff, 0xff)
#define DEFAULT_backColor		RGB(0x27, 0x27, 0x27)
#define DEFAULT_axisColor		RGB(0, 0xff, 0x80)
#define DEFAULT_gridColor		RGB(0xa0, 0xa0, 0xa0)
#define DEFAULT_titleColor		RGB(0xff, 0xff, 0xff) //默认时，标题颜色等于前景色
#define DEFAULT_footNoteColor	RGB(0xbe, 0xbe, 0xbe) //默认时，脚注颜色等于前景色的3/4
#define DEFAULT_pageChangeMSG	0L

#define OnePageLength	(CanvasRect[1].right - CanvasRect[1].left - nZLength * HSTEP)

#define GETPAGENUM(T1, T2) \
{ \
	nPageNum = 0; \
	if (nVisibleCurve > 0 && (T2) > (T1)) \
	{ \
		MainData NoUse; \
		NoUse.Time = T1; \
		CalcOriginDatumPoint(NoUse, 1); \
		auto TotalLen = -NoUse.ScrPos.x; \
		NoUse.Time = T2; \
		CalcOriginDatumPoint(NoUse, 1); \
		TotalLen += NoUse.ScrPos.x; \
		auto PageLen = OnePageLength; \
		nPageNum = TotalLen / PageLen; \
		if (TotalLen % PageLen) \
			++nPageNum; \
	} \
}

#define CHANGEONERANGE(VALUE, NEW, O, P, NP) \
if (VALUE O (NEW)) \
{ \
	VALUE = NEW; \
	P = NP; \
}

#define CHANGERANGE(MIN, MAX, NEW, P1, P2, NP) \
{ \
	CHANGEONERANGE(MIN, NEW, >, P1, NP) \
	else \
	CHANGEONERANGE(MAX, NEW, <, P2, NP) \
}

#define DOFILL1VALUE(pVALUE, VALUE) do_fill_1_value(pVALUE, VALUE)

#define DOFILL2VALUE(pVALUE1, pVALUE2, VALUE1, VALUE2) \
DOFILL1VALUE(pVALUE1, VALUE1); \
DOFILL1VALUE(pVALUE2, VALUE2)

#define DOFILL3VALUE(pVALUE1, pVALUE2, pVALUE3, VALUE1, VALUE2, VALUE3) \
DOFILL2VALUE(pVALUE1, pVALUE2, VALUE1, VALUE2); \
DOFILL1VALUE(pVALUE3, VALUE3)

#define DOFILL4VALUE(pVALUE1, pVALUE2, pVALUE3, pVALUE4, VALUE1, VALUE2, VALUE3, VALUE4) \
DOFILL2VALUE(pVALUE1, pVALUE2, VALUE1, VALUE2); \
DOFILL2VALUE(pVALUE3, pVALUE4, VALUE3, VALUE4)

#define DOFILL8VALUE(pVALUE1, pVALUE2, pVALUE3, pVALUE4, pVALUE5, pVALUE6, pVALUE7, pVALUE8, VALUE1, VALUE2, VALUE3, VALUE4, VALUE5, VALUE6, VALUE7, VALUE8) \
DOFILL4VALUE(pVALUE1, pVALUE2, pVALUE3, pVALUE4, VALUE1, VALUE2, VALUE3, VALUE4); \
DOFILL4VALUE(pVALUE5, pVALUE6, pVALUE7, pVALUE8, VALUE5, VALUE6, VALUE7, VALUE8)

#define FILL2VALUE(CON, pVALUE1, pVALUE2, VALUE1, VALUE2) \
{ \
	CON \
	{ \
		DOFILL2VALUE(pVALUE1, pVALUE2, VALUE1, VALUE2); \
		return TRUE; \
	} \
	return FALSE; \
}

#define SYNBUDDYS(wParam, lParam) \
{ \
	if (hBuddyServer) \
	{ \
		if (!(BuddyState & 1)) \
			::SendMessage(hBuddyServer, BUDDYMSG, (WPARAM) wParam, (LPARAM) lParam); \
	} \
	else if (pBuddys) \
		for (auto i = begin(*pBuddys); i < end(*pBuddys); ++i) \
			::SendMessage(*i, BUDDYMSG, (WPARAM) wParam, (LPARAM) lParam); \
}

#define CANCELBUDDYS \
{ \
	if (hBuddyServer) \
	{ \
		::SendMessage(hBuddyServer, BUDDYMSG, 1, (LPARAM) m_hWnd); \
		hBuddyServer = nullptr; \
	} \
	else if (pBuddys) \
	{ \
		for (auto i = begin(*pBuddys); i < end(*pBuddys); ++i) \
			::SendMessage(*i, BUDDYMSG, 1, 0); \
		pBuddys->clear(); \
		delete pBuddys; \
		pBuddys = nullptr; \
		CHLeftSpace(ActualLeftSpace); \
	} \
}

#define REMOVEBUDDY(hBuddy) \
{ \
	if (pBuddys) \
	{ \
		auto i = find(begin(*pBuddys), end(*pBuddys), hBuddy); \
		if (i < end(*pBuddys)) \
		{ \
			pBuddys->erase(i); \
			if (pBuddys->empty()) \
			{ \
				delete pBuddys; \
				pBuddys = nullptr; \
			} \
			auto MaxLeftSpace = GetMaxLeftSpace(ActualLeftSpace); \
			if (MaxLeftSpace < LeftSpace) \
			{ \
				BROADCASTLEFTSPACE(MaxLeftSpace); \
				CHLeftSpace(MaxLeftSpace); \
			} \
		} \
	} \
}

#define BROADCASTLEFTSPACE(NEWLEFTSPACE) \
{ \
	if (pBuddys) \
		for (auto i = begin(*pBuddys); i < end(*pBuddys); ++i) \
			::SendMessage(*i, BUDDYMSG, 5, (LPARAM) NEWLEFTSPACE); \
}

#define GETTIMEEND \
auto n = 0; \
auto pTemp = Buff + i; \
while (pTemp > pDateTime) \
{ \
	if (',' == *pTemp--) \
		++n; \
	if (2 == n) \
		break; \
} \
if (pTemp > pDateTime) \
{ \
	*++pTemp = 0;

//因为是按%c来读取字符，所以要减去0x30才是真正的数据，当然也可以改为用%d来读，不过%d要用int来装
#define SCANDATA(F, S, S1, S2) \
if (!State) \
{ \
	State = 2; \
	auto pDateTime = pRowStart; \
	while (pDateTime < Buff + i && ',' != *pDateTime) \
		++pDateTime; \
	if (pDateTime < Buff + i) \
	{ \
		++pDateTime; \
		GETTIMEEND \
			for (auto i = pDateTime; i < pTemp; ++i) \
				if ('.' != *i && !_istdigit(*i)) \
				{ \
					State = 1; \
					break; \
				} \
			*pTemp = ','; \
		} \
	} \
} \
ASSERT(pBinBuff - BinBuff <= BINBUFFLEN); \
if (pBinBuff - BinBuff >= BINBUFFLEN) \
{ \
	nImportRow += iAddMemMainData(BinBuff, (long) (pBinBuff - BinBuff), bAddTrail); \
	pBinBuff = BinBuff; \
} \
if (2 == State) \
{ \
	if (4 == F(pRowStart, S, pBinBuff, pBinBuff + 4, pBinBuff + 4 + 8, pBinBuff + 4 + 8 + 4)) \
	{ \
		++nTotalRow; \
		pBinBuff += 18; \
	} \
} \
else if (1 == F(pRowStart, S1, pBinBuff)) \
{ \
	auto pDateTime = pRowStart; \
	while (pDateTime < Buff + i && ',' != *pDateTime) \
		++pDateTime; \
	if (pDateTime < Buff + i) \
	{ \
		++pDateTime; \
		GETTIMEEND \
			HCOOR_TYPE Time; \
			auto hr = VarDateFromStr(CComBSTR(pDateTime), LANG_USER_DEFAULT, 0, &Time); \
			if (SUCCEEDED(hr)) /*千万不要用alt的字符串转换宏，这样可能会因为栈被用光而崩溃，这里使用CComBSTR类来做字符串转换*/ \
			{ \
				*(HCOOR_TYPE*) (pBinBuff + 4) = Time; \
				++pTemp; \
				if (2 == F(pTemp, S2, pBinBuff + 4 + 8, pBinBuff + 4 + 8 + 4)) \
				{ \
					++nTotalRow; \
					pBinBuff += 18; \
				} \
			} \
		} \
	} \
}

#define WRITEFILE(PRINT, Buff, PRE, OPDT, EXT) \
{ \
	auto len = PRINT(Buff, BUFF_A_W_LEN, PRE"%d,", Address); \
	if (ShowMode & 0x80) \
		len += PRINT(Buff + len, BUFF_A_W_LEN - len, PRE"%lf,", MainDataIter->Time); \
	else if (ISHVALUEVALID(MainDataIter->Time)) \
	{ \
		BSTR bstr; \
		auto hr = VarBstrFromDate(*MainDataIter, LANG_USER_DEFAULT, 0, &bstr); \
		ASSERT(SUCCEEDED(hr)); \
		OPDT; \
		SysFreeString(bstr); \
	} \
	len += PRINT(Buff + len, BUFF_A_W_LEN - len, PRE"%f,", MainDataIter->Value); \
	len += PRINT(Buff + len, BUFF_A_W_LEN - len, PRE"%d\r\n", MainDataIter->State); \
	EXT; \
	WriteFile(hFile, Buff, len * sizeof(*Buff), &WritedLen, nullptr); \
	ASSERT(len * sizeof(*Buff) == WritedLen); \
}

#define DELETEDC(hDC) \
if (hDC) \
{ \
	DeleteDC(hDC); \
	hDC = nullptr; \
}

#define DELETEOBJECT(hOb) \
if (hOb) \
{ \
	DeleteObject(hOb); \
	hOb = nullptr; \
}

#define FILLVALUE(V, XY) \
V = 0; \
while (++nNum) \
{ \
	V += i->ScrPos.XY; \
	if (0x30 == ValueMask) \
		Value += i->Value; \
	if (i == EndPos) \
		break; \
	++i; \
} \
V /= nNum; \
if (0x30 == ValueMask) \
	Value /= nNum;

#define FILLPATH(MASK, X1, Y1, X2, Y2) \
if (MASK & DataListIter->FillDirection) \
{ \
	if (2 == DataListIter->Power || (MASK & 5 && EndDrawPos->ScrPos.x != j->ScrPos.x || MASK & 0xA && EndDrawPos->ScrPos.y != j->ScrPos.y)) \
	{ \
		if (3 == CurveMode) \
			pPath->AddCurve((Gdiplus::Point*) &points.front(), (int) points.size(), Tension); \
		if (EndDrawPos->ScrPos.x != j->ScrPos.x || EndDrawPos->ScrPos.y != j->ScrPos.y) \
			if (3 == CurveMode) \
			{ \
				Gdiplus::Point point[] = {Gdiplus::Point(points.back().x, points.back().y), Gdiplus::Point(X1, Y1), Gdiplus::Point(X2, Y2), \
					Gdiplus::Point(points.front().x, points.front().y)}; \
				pPath->AddLines(point, 4); \
			} \
			else \
			{ \
				POINT NoUse = {X1, Y1}; \
				points.push_back(NoUse); \
				NoUse.x = X2; \
				NoUse.y = Y2; \
				points.push_back(NoUse); \
			} \
		if (3 == CurveMode) \
		{ \
			UnlockGdiplus; /*让GDI+生效，马上要使用了*/\
			pGraphics->FillPath(pBrush, pPath); \
			LockGdiplus; /*用完后，马上让GDI+失效*/\
			pPath->Reset(); \
		} \
		else \
			Polygon(hDC, &points.front(), (int) points.size()); \
		if (DataListIter->FillDirection & 0x30) \
			FillValue(hDC, DataListIter, j, EndDrawPos, DataListIter->FillDirection & (0x30 | MASK), LegendIter); \
	} \
}

#define MOVEWITHMOUS(CON) \
if (CON && nVisibleCurve > 0) \
{ \
	short xStep = 0; \
	short yStep = 0; \
	if (m_MoveMode & 1) \
	{ \
		auto hd = m_ShowMode & 1 ? -1 : 1; \
		xStep = (short) (hd * (spoint.x - BeginMovePoint.x) / HSTEP); \
	} \
	if (m_MoveMode & 2) \
	{ \
		auto vd = m_ShowMode & 2 ? -1 : 1; \
		yStep = (short) (vd * (BeginMovePoint.y - spoint.y) / VSTEP); \
	} \
	MoveCurve(xStep, yStep); \
	BeginMovePoint.x += xStep * (m_ShowMode & 1 ? -HSTEP : HSTEP); \
	BeginMovePoint.y -= yStep * (m_ShowMode & 2 ? -VSTEP : VSTEP); \
}

#define NormalizeRect(rect) \
if (rect.left > rect.right) \
{ \
	rect.left ^= rect.right; rect.right ^= rect.left; rect.left ^= rect.right; \
} \
if (rect.top > rect.bottom) \
{ \
	rect.top ^= rect.bottom; rect.bottom ^= rect.top; rect.top ^= rect.bottom; \
}

#define TRIMCURVE(CON1, C1, bUseMore, C2) \
long re = 0; \
for (auto i = begin(MainDataListArr); i < end(MainDataListArr); ++i) \
	if (bAll || Address == *i) \
	{ \
		auto pDataVector = i->pDataVector; \
		auto j = begin(*pDataVector); \
		if (bUseMore && 2 == i->Power) \
		{ \
			for (auto k = 0; j < end(*pDataVector); ++k, ++j)\
				if (!(k % nStep) && C2) \
				{ \
					j->AllState = State; \
					++re; \
					SysState |= 0x200; \
				} \
		} \
		else \
		{ \
			CON1; \
			if (j < end(*pDataVector)) \
			{ \
				for (auto k = 0; j < end(*pDataVector) && C1; k += nStep, j += nStep) \
					if (!(k % nStep)) \
					{ \
						j->AllState = State; \
						++re; \
					} \
				SysState |= 0x200; \
			} \
		} \
		if (!bAll) \
			break; \
	} \
if (re) \
	UpdateRect(hFrceDC, CanvasRectMask); \
return re;

#define PREPAREUNITECURVE(C, CON, CALL) \
auto DataListIter = FindMainData(Address); \
if (NullDataListIter != DataListIter) \
{ \
	auto DesDataListIter = FindMainData(DesAddr); \
	if (NullDataListIter != DesDataListIter) \
	{ \
		auto pDesDataVector = DesDataListIter->pDataVector; \
		vector<MainData>::iterator InsertIter = NullDataIter; \
		if (C) \
			CON; \
		CALL; \
		return TRUE; \
	} \
} \
return FALSE;

#define UNITECURVE(CON1, CON2, C1, C2, CON3, bUseMore, C3) \
auto pDataVector = DataListIter->pDataVector; \
auto pDesDataVector = DesDataListIter->pDataVector; \
auto i = begin(*pDataVector); \
auto bFound = FALSE; \
if (bUseMore && 2 == DataListIter->Power) \
	for (; i < end(*pDataVector) && C3 && (bFound = TRUE); ++i) \
	{ \
		if (NullDataIter == InsertIter) \
			AddMainData2(DesDataListIter->Address, i->Time, i->Value, i->State, 0, FALSE); \
		else \
			pDesDataVector->insert(InsertIter, *i); \
	} \
else \
{ \
	CON1; \
	if (i < end(*pDataVector)) \
	{ \
		if (NullDataIter == InsertIter) \
			for (CON2; i < end(*pDataVector) && C1 && (bFound = TRUE); ++i) \
				AddMainData2(DesDataListIter->Address, i->Time, i->Value, i->State, 0, FALSE); \
		else \
		{ \
			auto k = i; \
			if (C2) \
				CON3; \
			else \
				i = end(*pDataVector); \
			if (k < i) \
			{ \
				bFound = TRUE; \
				pDesDataVector->insert(InsertIter, k, i); \
			} \
		} \
	} \
	if (bFound) \
	{ \
		if (NullDataIter != InsertIter) \
			InvalidCurveSet.insert(DesDataListIter->Address); \
		UpdateRect(hFrceDC, CanvasRectMask); \
	} \
}

#define OFFSETVALUE(Style, OldValue, OpValue) \
if ('+' == Style) \
	OldValue += OpValue; \
else \
	OldValue *= OpValue;

#define OFFSETCURVE(MainData) \
if (ChMask & 1) \
{ \
	OFFSETVALUE(Operator1, (MainData).Time, TimeSpan); \
	(MainData).ScrPos.x -= XOff; \
} \
if (ChMask & 2) \
{ \
	OFFSETVALUE(Operator2, (MainData).Value, ValueStep); \
	(MainData).ScrPos.y -= YOff; \
}

#define GetSorptionRect(RECT, X, Y, RANGE) \
RECT.left = X - RANGE; \
RECT.right = X + RANGE +  1; \
RECT.top = Y - RANGE; \
RECT.bottom = Y + RANGE + 1;

//当位图资源变得无效时，去掉对资源的引用
//当去除的位图资源在当前判断的引用资源的前面的时候，执行刷新
#define UpdateImageIndex(nImageIndex, OP1, OP2, MASK) \
if ((size_t) nImageIndex >= BitBmps.size()) \
{ \
	OP1; \
	OP2; \
	UpdateMask |= MASK; \
} \
else if (nImageIndex >= nIndex) \
{ \
	OP2; \
	UpdateMask |= MASK; \
}

#define MakeHotspotRect \
PreviewHotspotRect.right = VAxisRect.right; \
PreviewHotspotRect.left = PreviewHotspotRect.right - 5; \
PreviewHotspotRect.top = VAxisRect.bottom; \
PreviewHotspotRect.bottom = PreviewHotspotRect.top + 5; \
MOVERECT(PreviewHotspotRect, m_ShowMode);

#define MakePreviewRect \
PreviewRect.left = CanvasRect[1].left; \
PreviewRect.right = PreviewRect.left + 4 * HSTEP + 1; \
PreviewRect.bottom = CanvasRect[1].bottom; \
PreviewRect.top = PreviewRect.bottom - 2 * VSTEP - 1; \
MOVERECT(PreviewRect, m_ShowMode);

#define MakeNodeRect(RECT, POINT, PenWidth, NodeMode) \
{ \
	auto x = m_ShowMode & 1 ? 1 : 0, y = m_ShowMode & 2 ? 1 : 0; \
	auto step = 0; \
	if (NodeMode) \
	{ \
		x += 1; \
		y += 1; \
		step = 2; \
	} \
	RECT.left = (POINT).x - PenWidth / 2 - x; \
	RECT.top = (POINT).y - PenWidth / 2 - y; \
	RECT.right = RECT.left + PenWidth + step; \
	RECT.bottom = RECT.top + PenWidth + step; \
}

#define DrawNode(POINT, PenWidth, NodeMode) \
{ \
	RECT rect; \
	DrawNodeRect(rect, POINT, PenWidth, NodeMode); \
}

#define DrawNodeRect(RECT, POINT, PenWidth, NodeMode) \
{ \
	MakeNodeRect(RECT, POINT, PenWidth, NodeMode); \
	ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &RECT, nullptr, 0, nullptr); \
}

#define CreateCommentRect(pos) \
if (4 == pos) \
{ \
	if (Width) \
		PosX -= Width / 2; \
	if (Height) \
		PosY -= Height / 2; \
} \
else \
{ \
	if (Width && pos > 1) \
		PosX -= Width; \
	if (Height && (1 == pos || 3 == pos)) \
		PosY -= Height; \
}

#define ISCURVESHOWN(DATALISTITER) (NullLegendIter == (DATALISTITER)->LegendIter  || (DATALISTITER)->LegendIter->State)
#define ISCURVEINPAGE(DATALISTITER, bPart, bXOnly) (NullDataIter != GetFirstVisiblePos(DATALISTITER, bPart, bXOnly))
#define ISCURVEVISIBLE(DATALISTITER, bPart, bXOnly) (ISCURVESHOWN(DATALISTITER) && ISCURVEINPAGE(DATALISTITER, bPart, bXOnly))

#define GETPAGESTARTTIME(BT, SP) (BT + (HCoorData.nScales - nZLength) * HCoorData.fCurStep * ((SP)  - 1))

//设置二分之一前景色
#define SetOneHalfColor(HDC) \
auto OneHalfColor = (m_foreColor & 0xFEFEFE) >> 1; \
if (IsColorsSimilar(OneHalfColor, m_backColor)) \
	OneHalfColor = ~OneHalfColor & 0xFFFFFF; \
SetTextColor(HDC, OneHalfColor);

//设置四分之三前景色
#define SetThreeQuarterColor(HDC) \
auto ThreeQuarterColor = (m_foreColor & 0xFEFEFE) >> 1; \
ThreeQuarterColor += (ThreeQuarterColor & 0xFEFEFE) >> 1; \
if (IsColorsSimilar(ThreeQuarterColor, m_backColor)) \
	ThreeQuarterColor = ~ThreeQuarterColor & 0xFFFFFF; \
SetTextColor(HDC, ThreeQuarterColor);

#define LockGdiplus \
if (pGraphics) \
	pGraphics->GetHDC();

#define UnlockGdiplus \
if (pGraphics) \
	pGraphics->ReleaseHDC(hDC);

#define RemovePlugInOrScript(OP, bUpdate) \
long Mask = 0; \
if (pFormatXCoordinate && pFormatXCoordinate OP luaFormatXCoordinate) \
{ \
	pFormatXCoordinate = nullptr; \
	UpdateMask |= HLabelRectMask; \
	Mask |= 1; \
} \
if (pFormatYCoordinate && pFormatYCoordinate OP luaFormatYCoordinate) \
{ \
	pFormatYCoordinate = nullptr; \
	UpdateMask |= VLabelRectMask; \
	Mask |= 2; \
} \
if (pTrimXCoordinate && pTrimXCoordinate OP luaTrimXCoordinate) \
{ \
	pTrimXCoordinate = nullptr; \
	Mask |= 4; \
} \
if (pTrimYCoordinate && pTrimYCoordinate OP luaTrimYCoordinate) \
{ \
	pTrimYCoordinate = nullptr; \
	Mask |= 8; \
} \
if (pCalcTimeSpan && pCalcTimeSpan OP luaCalcTimeSpan) \
{ \
	pCalcTimeSpan = nullptr; \
	UpdateMask |= HLabelRectMask; \
	Mask |= 0x10; \
} \
if (pCalcValueStep && pCalcValueStep OP luaCalcValueStep) \
{ \
	pCalcValueStep = nullptr; \
	UpdateMask |= VLabelRectMask; \
	Mask |= 0x20; \
} \
if (bUpdate && !(Mask & 0x22 && SetLeftSpace()) && Mask & 0x33) \
{ \
	if (nVisibleCurve > 0) \
		UpdateMask |= CanvasRectMask; \
	UpdateRect(hFrceDC, UpdateMask); \
} \
return Mask;

#define UpdateForPlugInOrScript \
if (!(UpdateMask & VLabelRectMask && SetLeftSpace()) && UpdateMask & (VLabelRectMask | HLabelRectMask)) \
{ \
	if (nVisibleCurve > 0) \
		UpdateMask |= CanvasRectMask; \
	UpdateRect(hFrceDC, UpdateMask); \
}

#define CloseLua \
if (g_L) \
{ \
	lua_close(g_L); \
	g_L = nullptr; \
}

#define ClosePlugIn \
if (hPlugIn) \
{ \
	FreeLibrary(hPlugIn); \
	hPlugIn = nullptr; \
}

#define ReportPageChanges \
if (!(SysState & 1) && (m_pageChangeMSG && m_iMSGRecWnd || EventState & 1)) \
	SetTimer(REPORTPAGE, REPORTDELAY, nullptr)

#define FIRE_1(D, Mask, FUN) \
if (EventState & Mask) \
	FUN(D)

#define FIRE_2(D1, D2, Mask, FUN) \
if (EventState & Mask) \
	FUN(D1, D2)

#define FIRE_3(D1, D2, D3, Mask, FUN) \
if (EventState & Mask) \
	FUN(D1, D2, D3)

#define FIRE_PageChange(D1, D2) FIRE_2(D1, D2, 1, FirePageChange)
#define FIRE_BeginTimeChange(D) FIRE_1(D, 2, FireBeginTimeChange)
#define FIRE_BeginValueChange(D) FIRE_1(D, 4, FireBeginValueChange)
#define FIRE_TimeSpanChange(D) FIRE_1(D, 8, FireTimeSpanChange)
#define FIRE_ValueStepChange(D) FIRE_1(D, 0x10, FireValueStepChange)
#define FIRE_ZoomChange(D) FIRE_1(D, 0x20, FireZoomChange)
#define FIRE_SelectedCurveChange(D) FIRE_1(D, 0x40, FireSelectedCurveChange)
#define FIRE_LegendVisableChange(D1, D2) FIRE_2(D1, D2, 0x80, FireLegendVisableChange)
#define FIRE_SorptionChange(D1, D2, D3) FIRE_3(D1, D2, D3, 0x100, FireSorptionChange)
#define FIRE_CurveStateChange(D1, D2) FIRE_2(D1, D2, 0x200, FireCurveStateChange)
#define FIRE_ZoomModeChange(D) FIRE_1(D, 0x400, FireZoomModeChange)
#define FIRE_HZoomChange(D) FIRE_1(D, 0x800, FireHZoomChange)
#define FIRE_BatchExportImageChange(D) FIRE_1(D, 0x1000, FireBatchExportImageChange)

#define SYNA_TO_TRUE_POINT \
ScrPos = spoint; \
ClientToScreen(&ScrPos); \
SetCursorPos(ScrPos.x, ScrPos.y)

#define IsCursorNotInCanvas(POINT) (nVisibleCurve <= 0 || SysState & 0x80000000 && PtInRect(&PreviewRect, POINT) || !PtInRect(CanvasRect, POINT))

#define SCROLLCURVE(MASK, DEC, ADD, xStep, yStep) \
if (m_MoveMode & MASK) \
{ \
	auto hd = m_ShowMode & 1 ? -1 : 1; \
	auto vd = (m_ShowMode & 3) < 2 ? -1 : 1; \
	switch (wParam & 0xFFFF) \
	{ \
	case DEC: \
		if (MoveCurve(hd * -xStep, vd * yStep)) \
			return 0; \
		break; \
	case ADD: \
		if (MoveCurve(hd * xStep, vd * -yStep)) \
			return 0; \
		break; \
	} \
}

#ifdef _WIN64
	#define Format64bitHandle(HANDLETYPE, LOW32BIT) ((HANDLETYPE) (((ULONGLONG) m_register1 << 32) + (ULONG) LOW32BIT))
	#define SplitHandle(H) (m_register1 = (OLE_HANDLE) ((ULONGLONG) H >> 32), (OLE_HANDLE) H)
#else
	#define Format64bitHandle(HANDLETYPE, LOW32BIT) ((HANDLETYPE) LOW32BIT)
	#define SplitHandle(H) ((OLE_HANDLE) H)
#endif

//以下宏只能用于在调用_sntprintf之后，对返回长度的校正
//_sntprintf_s会在有数据被截断时，返回-1，但此时结束符是置了的
//注意：这个宏只能用在目的缓存写满而被截的情况，而且只能用在CoorData操作相关的地方
#define adjust_poly_len(Coor_Data) adjust_poly_len2(Coor_Data.pPolyTexts[nIndex].n, PolyTextLen - 1)

#define adjust_poly_len2(d, m) \
if (-1 == d) \
	d = m

#define MINTIME	-657434.0 //100-1-1
#define MAXTIME	2958465.0 //9999-12-31

#define ISHVALUEVALID(DV) (MINTIME <= DV && DV <= MAXTIME)
#define ISHVALUEINVALID(DV) (!(m_ShowMode & 0x80) && !ISHVALUEVALID(DV))

#define IsTimeInvalidate	(dStepTime < .01 / (24 * 60 * 60)) //不能小于0.01秒，不限制上限（2.1.0.1）
#define IsValueInvalidate	(thisValueStep < .000001f) //不能小于.000001，不限制上限（2.1.0.1）

#define IsMainDataStateValidate(STATE) ((State & 0xFF) <= 2 && (USHORT) (State & 0xFF00) <= 0x100)

// ST_CurveCtrl.h : CST_CurveCtrl ActiveX 控件类的声明。


// CST_CurveCtrl : 有关实现的信息，请参阅 ST_CurveCtrl.cpp。

class CST_CurveCtrl : public COleControl
{
	DECLARE_DYNCREATE(CST_CurveCtrl)

// 构造函数
public:
	CST_CurveCtrl();

// 重写
public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual DWORD GetControlFlags();
	virtual BOOL OnSetExtent(LPSIZEL lpSizeL);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void Serialize(CArchive& ar);
	virtual void OnResetState();
protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

// 实现
protected:
	IWebBrowser2* pWebBrowser; //用于确定控件是否运行于IE中

	ULONG_PTR m_gdiplusToken;

	HWND	hBuddyServer;  //联动服务器句柄，联动客户机保存该值，联动服务器该值为0
	vector<HWND>* pBuddys; //联动客户机数组，联动服务器保存该值，联动客户机该值为0
	BYTE BuddyState;
	//从低位开始，第一位，正在处理联动伙伴发来的消息（此时不可反发回去）
	//这些状态，只对客户机有效，服务器必须要将消息反发回去

	//如何校正坐标提示，低7位：
	//0－始终按Z坐标等于0来校正坐标提示，其实就是不校正
	//1－始终按选中曲线的Z坐标来校正坐标提示，如果没有选中曲线，则等效于0
	//2－如果选中的曲线在画布中，则按选中曲线的Z坐标来校正坐标提示，当没有选中曲线，或者选中曲线不在画布中时，等效于0
	//3－只在曲线点上显示坐标提示
	BYTE	m_ReviseToolTip;

	WORD	MouseMoveMode; //鼠标移动模式：移动模式、拖动模式、缩放模式
	HCURSOR	hZoomIn, hZoomOut, hMove, hDrag; //放大、缩小、移动、拖动时的鼠标，移动时隐藏鼠标

	//当前鼠标点所在的曲线地址及原始坐标
	long m_CurActualAddress;
	ActualPoint m_CurActualPoint;

	/*已经放弃添加此功能，因为二次开发者来实现此功能将更加的灵活多样
	MoveBuddy* pMoveBuddy[2]; //移动联动伙伴数据结构（最多两个，即顶部和底部），没有移动联动关系的该值为0
	*/

	//下面的临时变量因为太频繁用到，所以申请为类成员变量，注意不能申请为全局变量，否则当多个控件在同一容器中时有问题
	//可能会出现访问冲突
	RECT	InvalidRect; //这个用于判断刷新区域（大分部情况下是这个用处）

#define StrBuffLen	128
#define StrTitleLen	128
#define StrWaterMarkLen	48
	TCHAR	StrBuff[StrBuffLen];	 //字符串缓存
	TCHAR	CurveTitle[StrTitleLen]; //曲线标题
	TCHAR	FootNote[StrTitleLen];   //曲线脚注
	TCHAR	WaterMark[StrWaterMarkLen]; //水印
	BatchExport* m_BE; //定时导出图片时使用

	HPEN	hAxisPen;  //绘制坐标轴时使用
	HRGN	hScreenRgn; //绘制区域，打印时用另外的

	HFONT	hFont; //所有文字绘制时用的字体，打印时用另外的
	HFONT	hTitleFont; //标题字体
	BYTE	fHeight, fWidth; //字的高度和宽度

	BYTE	m_ShowMode;
	//从低位起，第一位：1－Y轴显示在右边，0－Y轴显示在左边；第二位：1－X轴显示在上边，0－X轴显示在下边
	//第三位：1－不显示年月日，0－显示
	//第四位：1－不显示时分秒，0－显示
	//第八位如果为1，则横坐标将以值的方式显示，此时第三、四位无效
	BYTE	m_MoveMode;
	//从低位起：
	//1－是否允许在水平上移动曲线
	//2－是否允许在垂直上移动曲线
	//3－是否为快速移动模式，即在按住鼠标移动过程中移动曲线，否则称为慢速移动模式，即在鼠标左键弹起时移动曲线
	//第8位如果为1，则在鼠标移动（非拖动曲线）时，显示为手型，否则显示为十字架，此时坐标提示失效，吸附效应也失效
	BYTE	m_BkMode; //背景位图的显示模式，在m_hBkBitmap不为0时才有效，低7位：0－平铺，1－居中，2－拉伸；第8位：是否裁剪掉画布
	BYTE	m_CanvasBkMode; //画布背景显示模式，意义同m_BkMode，只是不能设置第8位

	short	m_vInterval, m_hInterval;
	//垂直、水平轴上，每个刻度值之间的刻度数，就像直尺最小刻度是1毫米，而刻度值最小为1厘米，此时刻度间隔为9

	short	m_LegendSpace; //图例字符最大个数，超长的将不可见
	short	LeftSpace;  //纵轴左边距
	short	ActualLeftSpace; //真正的纵轴左边距，如果没有设置联动关系，则等于LeftSpace

	short	Zoom; //缩放，正为放大，负为缩小
	short	HZoom; //水平上的缩放，与Zoom相加才是真正的缩放，这个值主要用于实现通过鼠标滚轮只在水平上缩放（像股票软件）

	short	TopSpace;	//上空白
	short	RightSpace;	//右空白
	short	BottomSpace;//下空白
	BYTE	BottomSpaceLine; //下空白（行数）
	BYTE	CommentPosition; //注解位置，0-最底层 1-最高层

	typedef BOOL (CST_CurveCtrl::*OnMouseWheel)(int zDelta);
	OnMouseWheel OnMouseWheelFun[4];

	BOOL OnMouseWheelZoom(int zDelta);
	BOOL OnMouseWheelHZoom(int zDelta);
	BOOL OnMouseWheelHMove(int zDelta);
	BOOL OnMouseWheelVMove(int zDelta);

	//0-垂直移动， 1-缩放，2-水平缩放， 3-水平移动
	BYTE	MouseWheelMode; //从低位起，每两位为一组，依次是：shift alt ctrl和不按键
	BYTE	MouseWheelSpeed; //鼠标滚轮速度（倍数）
	//还有两个字节未使用

	ActualPoint BenchmarkData; //计算OriginPoint屏幕坐标时的基准
	MainData LeftTopPoint; //所有曲线所占区域最小X与最大Y
	MainData RightBottomPoint; //所有曲线所占区域最大X与最小Y
#define m_MinTime LeftTopPoint.Time
#define m_MaxValue LeftTopPoint.Value
#define m_MaxTime RightBottomPoint.Time
#define m_MinValue RightBottomPoint.Value
	MainData OriginPoint; //原点
	//这里都没有使用MainData的State成员

	CoorData<HCOOR_TYPE> HCoorData; //横坐标
	CoorData<float> VCoorData;  //纵坐标

	set<long> InvalidCurveSet;
	//还没有更新LeftTopPoint、RightBottomPoint和nPower等成员的曲线（只在AddMainData2和InsertMainData2函数里面使用）

#define StrUintLen	16
#define StrPreExpLen 8
	TCHAR	Unit[StrUintLen]; //单位
	TCHAR	HUnit[StrUintLen]; //水平值单位
	TCHAR	VPrecisionExp[StrPreExpLen]; //显示纵坐标时的打印字符串，其中包括精度
	TCHAR	HPrecisionExp[StrPreExpLen]; //显示横坐标时的打印字符串，其中包括精度

	long	MaxLength, CutLength; //每条曲线最大点数，达到最大点数后从前面截掉的点数
	int		nVisibleCurve; //当前处于显示状态的曲线条数

	vector<LegendData> LegendArr;
	vector<CString> HLegend;
	vector<DataListHead<MainData>> MainDataListArr;
	vector<InfiniteCurveData> InfiniteCurveArr;
	vector<CommentData> CommentDataArr;
	//图例和曲线用向量表达
	size_t CurCurveIndex;

	//专门供Polygon和Polyline函数使用的缓存，在此申请避免重复的分配与释放，这种做法基于现在内存都很大的事实
	//默认为画布上每一个横坐标分配一个空间
	vector<POINT> points;

	//下面每一个矩形数据组的第二个值为正常坐标下的坐标，第一个值为当前坐标系下的坐标
	RECT	UnitRect, LegendMarkRect, TimeRect;
	RECT	VAxisRect, HAxisRect, VLabelRect, HLabelRect, LegendRect[2], CanvasRect[2];
	RECT	CurveTitleRect; //这个矩形没有映射坐标
	RECT	FootNoteRect;

	//全局位置窗口相关
	RECT	PreviewHotspotRect; //点击这个窗口，弹出全局位置窗口（屏幕位置）
	RECT	PreviewRect; //全局位置窗口的位置（屏幕位置）
	POINT	PreviewPoint; //当前窗口在全局位置窗口里面的位置（矩形的左上角点）

	int		WinWidth, WinHeight;
	float	Tension; //绘制基数样条曲线时的系数
	UINT	SysState;
	//从低位起：
	// 1－是否正在打印
	// 2－是否正在按页导出图片
	// 3－是否只显示网格在主刻度之上，默认为假
	// 4－是否允许选中曲线
	// 5－是否开启水平缩放（默认在按下alt并滚动鼠标时执行，可修改）
	// 6－需要把spoint与当前坐标同步（当按下alt键之后，需要这样处理，具体看cpp文件）
	// 7－需要把当前坐标与spoint同步
	// 8－版权信息是否已经隐藏（还未实现）
	// 9－是否处于焦点状态
	//10－是否有未绘制到hFrceDC的内容（比如添加点是没有指定bEnsureViaible为TRUE等情况）
	//11－是否允许缩放
	//12－坐标提示是否处于显示中
	//13－基点是否已经设置
	//14－是否允许自动调整曲线层次
	//15－是否允许吸附效应
	//16－是否已经吸附，如果已经吸附，则m_CurActualPoint里面就是被吸附的点
	//17－开源之后不再使用
	//18－是否自动修整坐标（在定点缩放后自动执行TrimCoor函数）
	//19－是否允许显示简单的帮助信息在背景之上
	//20－是否显示横向网格
	//21－是否显示纵向网格
	//22－是否处于全屏显示状态之下
	//23－是否显示焦点状态
	//24－是否绘制实线网格
	//25－是否固定X轴开始坐标
	//26－是否固定X轴结束坐标
	//27－是否固定Y轴开始坐标
	//28－是否固定Y轴结束坐标
	//29－是否只显示一页，此时25到28位无效
	//30－是否强行阻止刷新（使用了SetRedraw函数）
	//31－是否阻止DrawCurve函数移动曲线（只在垂直方向上有效）
	//32－全局位置窗口是否处于显示中

	UINT EventState;
	//从低位起：
	//1 －PageChange
	//2 －BeginTimeChange
	//3 －BeginValueChange
	//4 －TimeSpanChange
	//5 －ValueStepChange
	//6 －ZoomChange
	//7 －SelectedCurveChange
	//8 －LegendVisableChange
	//9 －SorptionChange
	//10－CurveStateChange
	//11－ZoomModeChange
	//12－HZoomChange
	//13－BatchExportImageChange
#define ALL_EVENT_MASK	0x1FFF

	USHORT VSTEP; //纵坐标屏幕步长
	USHORT HSTEP; //横坐标屏幕步长

	//自动刷新，高2字节中的低15位代表数量间隔，低2字节代表时间间隔，单位是1/10秒，所以最长间隔6553.5秒
	//影响到的接口有：AddMainData(2)
	UINT AutoRefresh;

	HCOOR_TYPE FixedBeginTime, FixedEndTime;
	float FixedBeginValue, FixedEndValue;

	USHORT nZLength; //Z轴长度，单位为刻度数
	short nCanvasBkBitmap; //画布背景位图
	COLORREF LeftBkColor, BottomBkColor;

	POINT	BeginMovePoint; //开始移动点
	POINT	LastMousePoint; //上次鼠标坐标，用于抹掉十字交叉线
	RECT	ToolTipRect; //正在显示的tooltip占的矩形，用于抹掉tooltip
	short	SorptionRange;

	short	nBkBitmap; //背景位图序号
	vector<BitBmp> BitBmps;
	HBITMAP	hBackBmp; //内存背景位图，提高刷新速度
	HBITMAP hFrceBmp; //内存前景位图，提高刷新速度
	HDC		hBackDC;  //内存兼容DC，绘制hBackBmp
	HDC		hFrceDC;  //内存兼容DC，绘制hFrceDC
	HDC		hTempDC;  //内存兼容DC，用在各个可以临时使用的地方
	void	DrawBkgImage(HDC hDC, HBITMAP hBitmap, RECT& rect, UINT BkMode);
	void	DrawBkg(BOOL bReCreate = FALSE);  //绘制背景到BkBitmap中
	void	InitFrce(BOOL bReCreate = FALSE); //初始化前景DC

	void AddBitmap(HBITMAP hBitmap, UINT State);

	//绘制函数
	void DrawGrid(HDC hDC);   //绘制网格
	void DrawUnit(HDC hDC);   //绘制单位
	void DrawVAxis(HDC hDC);  //绘制纵坐标
	void DrawHAxis(HDC hDC);  //绘制横坐标
	void DrawVLabel(HDC hDC); //绘制纵刻度
	void DrawHLabel(HDC hDC); //绘制横刻度
	void DrawLegend(HDC hDC); //绘制图例
	void DrawTime(HDC hDC);   //绘制单位
	void DrawLegendSign(HDC hDC); //绘制单位
	void DrawCurveTitle(HDC hDC); //绘制标题
	void DrawFootNote(HDC hDC); //绘制脚注
	void DrawPreviewView(HDC hDC); //绘制全局位置窗口
	void DrawComment(HDC hDC); //绘制注解
	UINT GetPoints(vector<DataListHead<MainData>>::iterator DataListIter,
		vector<MainData>::iterator BeginPos, vector<MainData>::iterator EndPos, vector<MainData>::iterator& EndDrawPos,
		UINT CurveMode, BOOL bClosedAndFilled, BOOL bDrawNode, HDC hDC, UINT NodeMode, UINT PenWidth);
	void FillValue(HDC hDC, vector<DataListHead<MainData>>::iterator DataListIter,
		vector<MainData>::iterator BeginPos, vector<MainData>::iterator EndPos, UINT FillDirection, vector<LegendData>::iterator LegendIter);
	void DrawCurve(HDC hDC, HRGN hRgn, vector<DataListHead<MainData>>& DataListArr, UINT& CurvePosition, BOOL bInfiniteCurve = FALSE,
		vector<DataListHead<MainData>>::iterator DataListIter = NullDataListIter, vector<MainData>::iterator BeginPos = NullDataIter);
	void DrawCurve(HDC hDC, HRGN hRgn,
		vector<DataListHead<MainData>>::iterator DataListIter = NullDataListIter, vector<MainData>::iterator BeginPos = NullDataIter); //绘制曲线
	//区域刷新，在Mask中为1的位，则相应的区域将被刷新，lpRect则是需要在屏幕上刷新的区域
	//Mask中每一位对应的区域如下：
#define UnitRectMask		1
#define LegendMarkRectMask	2
#define TimeRectMask		4
#define VAxisRectMask		8
#define HAxisRectMask		16
#define VLabelRectMask		32
#define HLabelRectMask		64
#define LegendRectMask		128
#define CanvasRectMask		256
#define CurveTitleRectMask	512
#define FootNoteRectMask	1024
#define PreviewRectMask		2048
#define TitleRectMask		(CurveTitleRectMask | UnitRectMask | LegendMarkRectMask)
#define AllRectMask			(TimeRectMask | VAxisRectMask | HAxisRectMask | VLabelRectMask | HLabelRectMask | LegendRectMask | CanvasRectMask | TitleRectMask | FootNoteRectMask)
#define PrintRectMask		(AllRectMask & ~(CanvasRectMask | CurveTitleRectMask | FootNoteRectMask))
#define ForeRectMask		(AllRectMask & ~(VAxisRectMask | HAxisRectMask | CanvasRectMask))
#define MostRectMask		(AllRectMask & ~(TitleRectMask | LegendRectMask))
	void UpdateRect(HDC hDC, UINT Mask, vector<DataListHead<MainData>>::iterator DataListIter = NullDataListIter, BOOL bUpdate = TRUE);
	void DrawAcrossLine(const LPPOINT pPoint); //抹除上一次的十字交叉线及坐标提示，并且绘制十字交叉线
	void RepairAcrossLine(HDC hDC, LPCRECT pRect, BOOL bCheckBound = TRUE);
	void ShowToolTip(HDC hDC);
	BOOL SetLeftSpace(BOOL bCanvasWidthCh = FALSE); //计算LeftSpace的值，如果纵轴有移动，则返回真，否则返回假
	BOOL CHLeftSpace(short ThisLeftSpace, BOOL bCanvasWidthCh = FALSE); //修改LeftSpace，返回值同SetLeftSpace
	short GetMaxLeftSpace(short RootLeftSpace);
	//获取所有客户机的LeftSpace中的最大值，RootLeftSpace联动服务器的最小LeftSpace
	short RestoreLeftSpace(); //获取自己的最小LeftSpace

	BOOL IsLineInCanvas(vector<LegendData>::iterator LegendIter, const POINT& p1, const POINT& p2);
//	inline int IsPointOutdrop(POINT& p);
//	inline int IsLineOutdrop(POINT& p1, POINT& p2);
	BOOL IsPointVisible(vector<DataListHead<MainData>>::iterator DataListIter, vector<MainData>::iterator DataIter, BOOL bPart, BOOL bXOnly, UINT Mask = 3);
	vector<MainData>::iterator GetFirstVisiblePos(long Address);
	vector<MainData>::iterator GetFirstVisiblePos(vector<DataListHead<MainData>>::iterator DataListIter,
		BOOL bPart, BOOL bXOnly, UINT* pPosition = nullptr, vector<MainData>::iterator DataIter = NullDataIter);
	//Mask从低位起，1－是否检测后一点，2－是否检测前一点
	size_t DoGetPointFromScreenPoint(vector<DataListHead<MainData>>::iterator DataListIter, long x, long y, short MaxRange);

	UINT CheckVPosition(float fBeginValue); //检测是否可设置开始值为fBeginValue
	UINT CheckHPosition(HCOOR_TYPE fBeginTime); //检测是否可设置开始值为fBeginTime
	UINT CheckPosition(short xStep, short yStep); //检测是否可按给定的距离移动曲线

	//更新指定曲线的幂次
	void UpdatePower(vector<DataListHead<MainData>>::iterator DataListIter);

	//真正的求合集
	void DoUniteCurve(vector<DataListHead<MainData>>::iterator DataListIter, vector<MainData>::iterator InsertIter,
		vector<DataListHead<MainData>>::iterator DesDataListIter, long nBegin, long nCount);
	void DoUniteCurve(vector<DataListHead<MainData>>::iterator DataListIter, vector<MainData>::iterator InsertIter,
		vector<DataListHead<MainData>>::iterator DesDataListIter, HCOOR_TYPE BTime, HCOOR_TYPE ETime, short Mask);

	//1.3.0.6
	long ExportMetaFileFromTime(LPCTSTR pFileName, long Address, vector<DataListHead<MainData>>::iterator ExportIter, HCOOR_TYPE MinTime, HCOOR_TYPE MaxTime, short Style);

	//1.3.0.7
	UINT UpdateFixedValues(HCOOR_TYPE MinTime, HCOOR_TYPE MaxTime, float MinValue, float MaxValue, UINT Mask);
	//Mask代表参数的有效性，从低起，依次是（低4位）：MinTime、MaxTime、MinValue、MaxValue
	//返回值从低位起，依次是：SetBeginTime2、SetTimeSpan、SetBeginValue、SetValueStep函数的执行结果——TRUE则置1，否则置0（这些函数可返回FALSE、TRUE、2）

	/*
	重新定位曲线位置
	OpStyle从低位开始
	第1位：1－强行垂直居中
	第2位：1－保持本页纵坐标
	第3位：1－保持本页横坐标位置
	返回值
	第1位：1－本页非空
	第2位：1－移动过曲线
	*/
	UINT	ReSetCurvePosition(UINT OpStyle, BOOL bUpdate, vector<DataListHead<MainData>>::iterator DataListIter = NullDataListIter);
	//根据鼠标坐标得到点击了第几个图例，鼠标必须已经在图例范围之内（LegendRect），由于图例可以隐藏，所以这个序号不一定等于在全部图例中的序号
	size_t	GetLegendIndex(long y);
	BOOL	SelectLegendFromIndex(size_t nIndex); //按序号选中图例（只影响该图例的第一条曲线），如果已选中，则取消选中
	void	ChangeSelectState(vector<DataListHead<MainData>>::iterator DataListIter);
	HCOOR_TYPE	GetNearFrontPos(HCOOR_TYPE DateTime, HCOOR_TYPE BenchmarkTime);
	void	ReSetUIPosition(int cx, int cy);
	//更新指定曲线的MinTime、MaxTime值和MinValue、MaxValue值，最后一个参数用于在绘制实时曲线的时候
	void	UpdateOneRange(vector<DataListHead<MainData>>::iterator DataListIter, vector<MainData>::iterator DataIter = NullDataIter);
	void	UpdateTotalRange(BOOL bRectOnly = FALSE);
	void	ShowLegendFromIndex(size_t nIndex);  //按序号显示图例（影响该图例的所有曲线），如果已经显示，则隐藏
	vector<LegendData>::iterator FindLegend(LPCTSTR pSign); //查找图例
	vector<LegendData>::iterator FindLegend(long Address, BOOL bInLegend = FALSE); //查找图例
	COLORREF FindLegend(vector<LegendData>::iterator LegendPos, long Address, BOOL bDel = FALSE, BOOL bAll = FALSE);
	BOOL DoDelLegend(vector<LegendData>::iterator LegendPos, long Address, BOOL bDelAll, BOOL bUpdate);
	void ReSetDataListLegend(vector<LegendData>::iterator BeginPos, vector<LegendData>::iterator EndPos);
	void DoMoveCurveToLegend(long Address, vector<LegendData>::iterator& LegendIter, BOOL bUpdate);
	//删除点坐标点，返回值，按位算，从低位开始：1、2－未使用 3－是否删除了CurCurveIter，4－是否删除了DataListIter整条曲线
	UINT DoDelMainData(vector<DataListHead<MainData>>::iterator& DataListIter, vector<MainData>::iterator BeginPos, vector<MainData>::iterator EndPos, BOOL bUpdate);
	vector<DataListHead<MainData>>::iterator FindMainData(long Address); //查找曲线
	//绘制实时数据
	//VisibleState从低位起（AddMainData, AddMainData2的同名参数意义一样）：
	//1－是否马上绘制添加的点（忽略该参数）
	//2－保持纵坐标不变
	//3－保持横坐标不变
	void RefreshRTData(vector<DataListHead<MainData>>::iterator DataListIter, vector<MainData>::iterator Pos, short VisibleState);
	//移动曲线，bCheckBound用于确定对移动合法性的检测
	UINT MoveCurve(short xStep, short yStep, BOOL bUpdate = TRUE, BOOL bCheckBound = TRUE);
	UINT MoveCurve(SIZE size);
	//从实际坐标计算屏幕坐标，如果原点坐标未变，只是画布大小或者位置改变，则ChMask不为0，从低位起，第1位代表水平方向，第2位代表垂直方向
	void CalcOriginDatumPoint(MainData& ap, UINT ChMask = 3, int XOff = 0, int YOff = 0, vector<DataListHead<MainData>>::iterator DataListIter = NullDataListIter);
	//从屏幕点转到实际坐标值
	ActualPoint CalcActualPoint(const POINT& point);
	//计算让指定点可见时需要的移动量
	SIZE MakePointVisible(vector<DataListHead<MainData>>::iterator DataListIter, vector<MainData>::iterator DataIter, short VisibleState = 0);
	//曲线打印函数，返回实际打印的页数加1
	BOOL DoPrintCurve(HDC hPrintDC,
					  vector<DataListHead<MainData>>::iterator DataListIter,
					  HCOOR_TYPE BeginTime,
					  LPCTSTR pTitle,
					  LPCTSTR pFootNote,
					  int ViewWidth,
					  int ViewHeight,
					  int OrgX,
					  int OrgY,
					  WORD PageFrom,
					  WORD PageTo,
					  short Flag);
	void ClearCurve();
	void ClearLegend();
	void DeleteGDIObject();

	BOOL IsCurveClosed2(vector<DataListHead<MainData>>::iterator DataListIter);

	int FormatToolTipString(long Address, HCOOR_TYPE DateTime, float Value, UINT Action); //返回X和Y坐标的分隔位置，即\n的位置
	UINT DrawSelectedNode(HDC hDC, HRGN hRen, vector<DataListHead<MainData>>::iterator DataListIter, UINT PenWidth, size_t OldSelectedNode);
	void UpdateSelectedNode(vector<DataListHead<MainData>>::iterator DataListIter, size_t OldSelectedNode);
	BOOL CanCurveBeDrawnAlone(vector<DataListHead<MainData>>::iterator DataListIter);
	void MoveSelectNodeForward();
	void MoveSelectNodeBackward();

	BOOL DoFixedZoom(const POINT& point); //不做条件判断，调用前要保证合法
	void DoSetFixedZoomMode(WORD ZoomMode, POINT& point); //不做条件判断，调用前要保证合法（存在可见曲线时调用）
	void DoSetFixedZoomMode(WORD ZoomMode); //不做条件判断，调用前要保证合法（不存在可见曲线时调用）

	vector<InfiniteCurveData>::iterator FindInfiniteCurve(long Address); //查找无限曲线

	//插件相关
	//主要特点是将值转换成字符串
	typedef const TCHAR* FormatXCoordinate(long Address, HCOOR_TYPE DateTime, UINT Action); //序号0
	typedef const TCHAR* FormatYCoordinate(long Address, float Value, UINT Action); //序号1
	//函数的序号用于控制本控件需要加载哪些函数，0-31，可见每一类插件最多只支持32个接口
	//如果函数太多，就要对插件分类

/*
Address参数的解释如下：
要进行坐标提示的曲线地址，只有在吸附效应开启（参看SetSorptionRange接口）并且
只在曲线点上做坐标提示（参看SetReviseToolTip接口）的情况下才有意义，否则恒等于0x7fffffff
所以要想使用Address参数，就不得不损失一些功能
比如画布里面任意点坐标提示功能（可以通过SetReviseToolTip设置，默认不是）
此参数多用于多坐标系下，而且还只是在坐标提示的时候，一般大家并不需要关心这个参数

Action参数的解释如下：
1-绘制坐标轴开始，此时DateTime和Value无效，返回值无效
2-绘制坐标轴过程中，此时DateTime和Value有效
返回的字符串中，支持换行符（\n），最多支持一个，如果没有换行符，将显示为一行
有换行符的时候，将显示为两行，每行最多32个字符，Y轴不支持换行符
3-绘制坐标轴结束，此时DateTime和Value无效，返回值无效

4-显示坐标提示（Tooltip）
	此时FormatXCoordinate和FormatYCoordinate返回的字符串的长度和必须在127个字符之内
	多余的丢弃
	如果插件只提供了FormatXCoordinate和FormatYCoordinate其中一个，则未提供者采用
	控件原来的打印方式，两个坐标的打印结果加起来仍然必须在127个字符之内

5-绘制坐标，长度要求同4
	关于什么是绘制坐标，参看SetXYFormat接口

6-计算Y轴位置时调用，用于定位Y轴与边框的距离，以便显示得下Y坐标
	插件开发者可以利用这一点控制Y轴位置的显示，返回的字符串必须在127个字符之内
	只会对Y坐标调用此接口

7-绘制填充值
	关于什么是填充值，参看SetFillDirection接口
	只会对Y坐标调用此接口，长度限制127个字符

	以上所有的返回值，除特殊说明外，均不支持换行符
*/
	FormatXCoordinate*	pFormatXCoordinate;
	FormatYCoordinate*	pFormatYCoordinate;

	//主要特点是返回值
	typedef HCOOR_TYPE TrimXCoordinate(HCOOR_TYPE DateTime); //序号2
	typedef float TrimYCoordinate(float Value); //序号3
	/*
	修整坐标，如果用Lua实现，当调用Lua失败时，将采用控件内部的默认处理方式处理
	*/
	typedef double CalcTimeSpan(double TimeSpan, short Zoom, short HZoom); //序号4
	typedef float CalcValueStep(float ValueStep, short Zoom); //序号5
	/*
	使用插件计算坐标间隔，会对返回值做合法性检验
	注：有一点二次开发者必须要注意，放大缩放必须保证圆场，什么意思呢，就是负负得正的意思，比如：
	double a = ...;
	double a1 = CalcTimeSpan(a, 1, 0); //把a放大1陪
	double a2 = CalcTimeSpan(a1, -1, 0); //把a1缩小1陪
	此时，a应该等于a2，即用放大n倍的结果，去缩小n倍，得到的结果应该等于原始值（相当于没有放大也没有缩小）
	这在限制控件只显示一页的时候需要使用，如果你确定不会使用“限制一页”这个功能，则可以不遵守上面的规定
	如果用Lua实现，当调用Lua失败时，将采用控件内部的默认处理方式处理
	*/

	TrimXCoordinate* pTrimXCoordinate;
	TrimYCoordinate* pTrimYCoordinate;
	CalcTimeSpan* pCalcTimeSpan;
	CalcValueStep* pCalcValueStep;
	//第1类插件定义结束
#define PlugInType1Mask		0x3F //第一类插件目前共了6个

	HMODULE hPlugIn;

	long FreePlugIn(UINT& UpdateMask, BOOL bUpdate); //释放插件（只会释放由插件dll加载的接口）
	long FreeLuaScript(); //释放Lua脚本（只会释放由Lua脚本加载的接口）

	USHORT ShortcutKey;
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
#define ALL_SHORTCUT_KEY	0xFFFF

	USHORT LimitOnePageMode; //限制一页模式：
	//0-恒满一页（横坐标最小点在画布最左边，横坐标最大点在画布最右边）
	//1-当点绘制到画布之外时，把横坐标最小点移到画布最左边，把横坐标间隔增至原来的2倍（循环直到这个点画到了画布之内）
	//2-同上，只是横坐标间隔增至原来的3倍
	//n-同上，只是横坐标间隔增至原来的n+1倍
	//取值0-16，其余值保留以后扩展

	USHORT FontPitch; //字体间距
	USHORT ToolTipDely; //显示tooltip的延时间隔，单位为毫秒，如果为0，则不显示tooltip

	//为了兼容32和64位而写的辅助函数，均以小i开头
	HWND m_iMSGRecWnd;
	void iSetFont(HFONT hFont);
	long iAddMemMainData(LPBYTE pMemMainData, long MemSize, BOOL bAddTrail);

	~CST_CurveCtrl();

	DECLARE_OLECREATE_EX(CST_CurveCtrl)    // 类工厂和 guid
	DECLARE_OLETYPELIB(CST_CurveCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CST_CurveCtrl)     // 属性页 ID
	DECLARE_OLECTLTYPE(CST_CurveCtrl)		// 类型名称和杂项状态

// 消息映射
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	DECLARE_MESSAGE_MAP()

// 调度映射
	OLE_COLOR m_foreColor;
	afx_msg void OnForeColorChanged();
	OLE_COLOR m_backColor;
	afx_msg void OnBackColorChanged();
	OLE_COLOR m_axisColor;
	afx_msg void OnAxisColorChanged();
	OLE_COLOR m_gridColor;
	afx_msg void OnGridColorChanged();
	long m_pageChangeMSG;
	afx_msg void OnPageChangeMSGChanged();
	OLE_HANDLE m_MSGRecWnd;
	afx_msg void OnMSGRecWndChanged();
	OLE_COLOR m_titleColor;
	afx_msg void OnTitleColorChanged();
	OLE_COLOR m_footNoteColor;
	afx_msg void OnFootNoteColorChanged();
	OLE_HANDLE m_register1;
	afx_msg void OnRegister1Changed();
	afx_msg BOOL SetVInterval(short VInterval);
	afx_msg BOOL SetHInterval(short HInterval);
	afx_msg short GetScaleInterval();
	afx_msg void EnableHelpTip(BOOL bEnable);
	afx_msg BOOL SetLegendSpace(short LegendSpace);
	afx_msg short GetLegendSpace();
	afx_msg BOOL SetBeginValue(float fBeginValue);
	afx_msg float GetBeginValue();
	afx_msg BOOL SetBeginTime(LPCTSTR pBeginTime);
	afx_msg BOOL SetBeginTime2(HCOOR_TYPE fBeginTime);
	afx_msg BSTR GetBeginTime();
	afx_msg HCOOR_TYPE GetBeginTime2();
	afx_msg BOOL SetTimeSpan(double TimeStep);
	afx_msg double GetTimeSpan();
	afx_msg BOOL SetValueStep(float ValueStep);
	afx_msg float GetValueStep();
	afx_msg BOOL SetVPrecision(short Precision);
	afx_msg short GetVPrecision();
	afx_msg BOOL SetUnit(LPCTSTR pUnit);
	afx_msg BSTR GetUnit();
	afx_msg void TrimCoor();
	afx_msg short AddLegend(long Address, LPCTSTR pSign, OLE_COLOR PenColor, short PenStyle, short LineWidth, OLE_COLOR BrushColor, short BrushStyle, short CurveMode, short NodeMode, short Mask, BOOL bUpdate);
	afx_msg BOOL GetLegend(LPCTSTR pSign, OLE_COLOR* pPenColor, short* pPenStyle, short* pLineWidth, OLE_COLOR* pBrushColor, short* pBrushStyle, short* pCurveMode, short* pNodeMode);
	afx_msg BSTR QueryLegend(long Address);
	afx_msg short GetLegendCount();
	afx_msg BOOL GetLegend2(short nIndex, OLE_COLOR* pPenColor, short* pPenStyle, short* pLineWidth, OLE_COLOR* pBrushColor, short* pBrushStyle, short* pCurveMode, short* pNodeMode);
	afx_msg short GetLegendIdCount(short nIndex);
	afx_msg long GetLegendId(short nIndex, short nAddressIndex);
	afx_msg BOOL DelLegend(long Address, BOOL bAll, BOOL bUpdate);
	afx_msg BOOL DelLegend2(LPCTSTR pSign, BOOL bUpdate);
	afx_msg short AddMainData(long Address, LPCTSTR pTime, float Value, short State, short VisibleState, BOOL bAddTrail);
	afx_msg short AddMainData2(long Address, HCOOR_TYPE Time, float Value, short State, short VisibleState, BOOL bAddTrail);
	afx_msg void SetVisibleCoorRange(HCOOR_TYPE MinTime, HCOOR_TYPE MaxTime, float MinValue, float MaxValue, short Mask);
	afx_msg void GetVisibleCoorRange(HCOOR_TYPE* pMinTime, HCOOR_TYPE* pMaxTime, float* pMinValue, float* pMaxValue);
	afx_msg void DelRange(long Address, HCOOR_TYPE BTime, HCOOR_TYPE ETime, short Mask, BOOL bAll, BOOL bUpdate);
	afx_msg void DelRange2(long Address, long nBegin, long nCount, BOOL bAll, BOOL bUpdate);
	afx_msg BOOL FirstPage(BOOL bLast, BOOL bUpdate);
	afx_msg short GotoPage(short RelativePage, BOOL bUpdate);
	afx_msg BOOL SetZoom(short Zoom);
	afx_msg short GetZoom();
	afx_msg BOOL SetMaxLength(long MaxLength, long CutLength);
	afx_msg long GetMaxLength();
	afx_msg long GetCutLength();
	afx_msg BOOL SetShowMode(short ShowMode);
	afx_msg short GetShowMode();
	afx_msg BOOL SetBkBitmap(short nIndex);
	afx_msg short GetBkBitmap();
	afx_msg BOOL SetFillDirection(long Address, short FillDirection, BOOL bUpdate);
	afx_msg short GetFillDirection(long Address);
	afx_msg BOOL SetMoveMode(short MoveMode);
	afx_msg short GetMoveMode();
	afx_msg void SetFont(OLE_HANDLE hFont);
	afx_msg BOOL AddImageHandle(LPCTSTR pFileName, BOOL bShared);
	afx_msg void AddBitmapHandle(OLE_HANDLE hBitmap, BOOL bShared);
	afx_msg BOOL AddBitmapHandle2(OLE_HANDLE hInstance, LPCTSTR pszResourceName, BOOL bShared);
	afx_msg BOOL AddBitmapHandle3(OLE_HANDLE hInstance, long nIDResource, BOOL bShared);
	afx_msg long GetBitmapCount();
	afx_msg BOOL SetBkMode(short BkMode);
	afx_msg short GetBkMode();
	afx_msg BOOL ExportImage(LPCTSTR pFileName);
	afx_msg long ExportImageFromPage(LPCTSTR pFileName, long Address, long nBegin, long nCount, BOOL bAll, short Style);
	afx_msg long ExportImageFromTime(LPCTSTR pFileName, long Address, HCOOR_TYPE BTime, HCOOR_TYPE ETime, short Mask, BOOL bAll, short Style);
	afx_msg BOOL BatchExportImage(LPCTSTR pFileName, long nSecond);
	afx_msg void EnableAutoTrimCoor(BOOL bEnable);
	afx_msg long ImportFile(LPCTSTR pFileName, short Style, BOOL bAddTrail);
	afx_msg BOOL GetOneTimeRange(long Address, HCOOR_TYPE* pMinTime, HCOOR_TYPE* pMaxTime);
	afx_msg BOOL GetOneValueRange(long Address, float* pMinValue, float* pMaxValue);
	afx_msg BOOL GetOneFirstPos(long Address, HCOOR_TYPE* pTime, float* pValue, BOOL bLast);
	afx_msg BOOL GetTimeRange(HCOOR_TYPE* pMinTime, HCOOR_TYPE* pMaxTime);
	afx_msg BOOL GetValueRange(float* pMinValue, float* pMaxValue);
	afx_msg void GetViableTimeRange(HCOOR_TYPE* pMinTime, HCOOR_TYPE* pMaxTime);
	afx_msg long AddMemMainData(OLE_HANDLE pMemMainData, long MemSize, BOOL bAddTrail);
	afx_msg BOOL ShowCurve(long Address, BOOL bShow);
	afx_msg void SetFootNote(LPCTSTR pFootNote);
	afx_msg BSTR GetFootNote();
	afx_msg long TrimCurve(long Address, short State, long nBegin, long nCount, short nStep, BOOL bAll);
	afx_msg short PrintCurve(long Address, HCOOR_TYPE BTime, HCOOR_TYPE ETime, short Mask, short LeftMargin, short TopMargin, short RightMargin, short BottomMargin, LPCTSTR pTitle, LPCTSTR pFootNote, short Flag, BOOL bAll);
	afx_msg long GetEventMask();
	afx_msg long GetScaleNums();
	afx_msg long ReportPageInfo();
	afx_msg BOOL ShowLegend(LPCTSTR pSign, BOOL bShow);
	afx_msg BOOL SelectCurve(long Address, BOOL bSelect);
	afx_msg short DragCurve(short xStep, short yStep, BOOL bUpdate);
	afx_msg BOOL VCenterCurve(long Address, BOOL bUpdate);
	afx_msg BOOL GetSelectedCurve(long* pAddress);
	afx_msg void EnableAdjustZOrder(BOOL bEnable);
	afx_msg BOOL IsSelected(long Address);
	afx_msg BOOL IsLegendVisible(LPCTSTR pSign);
	afx_msg BOOL IsCurveVisible(long Address);
	afx_msg BOOL IsCurveInCanvas(long Address);
	afx_msg BOOL GotoCurve(long Address);
	afx_msg void EnableZoom(BOOL bEnable);
	afx_msg long GetCurveLength(long Address);
	afx_msg BSTR GetLuaVer();
	afx_msg HCOOR_TYPE GetTimeData(short nCurveIndex, long nIndex);
	afx_msg float GetValueData(short nCurveIndex, long nIndex);
	afx_msg short GetState(short nCurveIndex, long nIndex);
	afx_msg BOOL InsertMainData(short nCurveIndex, long nIndex, LPCTSTR pTime, float Value, short State, short Position, short Mask);
	afx_msg BOOL InsertMainData2(short nCurveIndex, long nIndex, HCOOR_TYPE Time, float Value, short State, short Position, short Mask);
	afx_msg BOOL CanContinueEnum(long Address, short nCurveIndex, long nIndex);
	afx_msg BOOL DelPoint(short nCurveIndex, long nIndex);
	afx_msg short GetCurveCount();
	afx_msg long GetCurve(short nIndex);
	afx_msg BOOL RemoveBitmapHandle(OLE_HANDLE hBitmap, BOOL bDel);
	afx_msg BOOL RemoveBitmapHandle2(short nIndex, BOOL bDel);
	afx_msg OLE_HANDLE GetBitmap(short nIndex);
	afx_msg short GetBitmapState(short nIndex);
	afx_msg short GetBitmapState2(OLE_HANDLE hBitmap);
	afx_msg BOOL SetBuddy(OLE_HANDLE hBuddy, short State);
	afx_msg short GetBuddyCount();
	afx_msg OLE_HANDLE GetBuddy(short nIndex);
	afx_msg void SetCurveTitle(LPCTSTR pCurveTitle);
	afx_msg BSTR GetCurveTitle();
	afx_msg BOOL SetHUnit(LPCTSTR pHUnit);
	afx_msg BSTR GetHUnit();
	afx_msg BOOL SetHPrecision(short Precision);
	afx_msg short GetHPrecision();
	afx_msg BOOL SetCurveIndex(long Address, short nIndex);
	afx_msg short GetCurveIndex(long Address);
	afx_msg BOOL SetGridMode(short GridMode);
	afx_msg short GetGridMode();
	afx_msg void SetBenchmark(HCOOR_TYPE Time, float Value);
	afx_msg void GetBenchmark(HCOOR_TYPE* pTime, float* pValue);
	afx_msg short GetPower(long Address);
	afx_msg long TrimCurve2(long Address, short State, HCOOR_TYPE BTime, HCOOR_TYPE ETime, short Mask, short nStep, BOOL bAll);
	afx_msg BOOL ChangeId(long Address, long NewAddr);
	afx_msg BOOL CloneCurve(long Address, long NewAddr);
	afx_msg BOOL UniteCurve(long DesAddr, long nInsertPos, long Address, long nBegin, long nCount);
	afx_msg BOOL UniteCurve2(long DesAddr, long nInsertPos, long Address, HCOOR_TYPE BTime, HCOOR_TYPE ETime, short Mask);
	afx_msg BOOL UniteCurve3(long DesAddr, HCOOR_TYPE fInsertPos, long Address, long nBegin, long nCount);
	afx_msg BOOL UniteCurve4(long DesAddr, HCOOR_TYPE fInsertPos, long Address, HCOOR_TYPE BTime, HCOOR_TYPE ETime, short Mask);
	afx_msg BOOL OffSetCurve(long Address, double TimeSpan, float ValueStep, short Operator);
	afx_msg long ArithmeticOperate(long DesAddr, long Address, short Operator);
	afx_msg void ClearTempBuff();
	afx_msg BOOL PreMallocMem(long Address, long size);
	afx_msg long GetMemSize(long Address);
	afx_msg BOOL IsCurve(long Address);
	afx_msg void SetSorptionRange(short Range);
	afx_msg short GetSorptionRange();
	afx_msg BOOL IsLegend(LPCTSTR pSign);
	afx_msg short AddLegendHelper(long Address, LPCTSTR pSign, OLE_COLOR PenColor, short PenStyle, short LineWidth, BOOL bUpdate);
	afx_msg BOOL GetActualPoint(long x, long y, HCOOR_TYPE* pTime, float* pValue);
	afx_msg long GetPointFromScreenPoint(long Address, long x, long y, short MaxRange);
	afx_msg void EnableFullScreen(BOOL bEnable);
	afx_msg HCOOR_TYPE GetEndTime();
	afx_msg float GetEndValue();
	afx_msg void SetZLength(short ZLength);
	afx_msg short GetZLength();
	afx_msg BOOL SetCanvasBkBitmap(short nIndex);
	afx_msg short GetCanvasBkBitmap();;
	afx_msg void SetLeftBkColor(OLE_COLOR Color);
	afx_msg OLE_COLOR GetLeftBkColor();
	afx_msg void SetBottomBkColor(OLE_COLOR Color);
	afx_msg OLE_COLOR GetBottomBkColor();
	afx_msg BOOL SetZOffset(long Address, short nOffset, BOOL bUpdate);
	afx_msg long GetZOffset(long Address);
	afx_msg void EnableFocusState(BOOL bEnable);
	afx_msg BOOL SetReviseToolTip(short Type);
	afx_msg short GetReviseToolTip();
	afx_msg long ExportMetaFile(LPCTSTR pFileName, long Address, long nBegin, long nCount, BOOL bAll, short Style);
	afx_msg void LimitOnePage(BOOL bLimit);
	afx_msg BOOL FixCoor(HCOOR_TYPE MinTime, HCOOR_TYPE MaxTime, float MinValue, float MaxValue, short Mask);
	afx_msg short GetFixCoor(HCOOR_TYPE* pMinTime, HCOOR_TYPE* pMaxTime, float* pMinValue, float* pMaxValue);
	afx_msg BOOL RefreshLimitedOrFixedCoor();
	afx_msg BOOL SetCanvasBkMode(short CanvasBkMode);
	afx_msg short GetCanvasBkMode();
	afx_msg void EnablePreview(BOOL bEnable);
	afx_msg void SetWaterMark(LPCTSTR pWaterMark);
	afx_msg long GetSysState();
	afx_msg void SetTension(float Tension);
	afx_msg float GetTension();
	afx_msg OLE_HANDLE GetFont();
	afx_msg BOOL SetXYFormat(LPCTSTR pSign, short Format);
	afx_msg short GetXYFormat(LPCTSTR pSign);
	afx_msg short GetXYFormat2(short nIndex);
	afx_msg long LoadPlugIn(LPCTSTR pFileName, short Type, long Mask);
	afx_msg BOOL AppendLegendEx(LPCTSTR pSign, OLE_COLOR BeginNodeColor, OLE_COLOR EndNodeColor, OLE_COLOR SelectedNodeColor, short NodeModeEx);
	afx_msg BOOL GetLegendEx(LPCTSTR pSign, OLE_COLOR* pBeginNodeColor, OLE_COLOR* pEndNodeColor, OLE_COLOR* pSelectedNodeColor, short* pNodeModeEx);
	afx_msg BOOL GetLegendEx2(short nIndex, OLE_COLOR* pBeginNodeColor, OLE_COLOR* pEndNodeColor, OLE_COLOR* pSelectedNodeColor, short* pNodeModeEx);
	afx_msg long GetSelectedNodeIndex(long Address);
	afx_msg BOOL SetSelectedNodeIndex(long Address, long NewNodeIndex);
	afx_msg long LoadLuaScript(LPCTSTR pFileName, short Type, long Mask);
	afx_msg void SetShortcutKeyMask(long ShortcutKey);
	afx_msg long GetShortcutKeyMask();
	afx_msg OLE_HANDLE GetFrceHDC();
	afx_msg BOOL SetBottomSpace(short Space);
	afx_msg short GetBottomSpace();
	afx_msg BSTR GetEndTime2();
	afx_msg BSTR GetTimeData2(short nCurveIndex, long nIndex);
	afx_msg short AddComment(HCOOR_TYPE Time, float Value, short Position, short nBkBitmap, short Width, short Height, OLE_COLOR TransColor, LPCTSTR pComment, OLE_COLOR TextColor, short XOffSet, short YOffSet, BOOL bUpdate);
	afx_msg BOOL DelComment(long nIndex, BOOL bAll, BOOL bUpdate);
	afx_msg long GetCommentNum();
	afx_msg BOOL GetComment(long nIndex, HCOOR_TYPE* pTime, float* pValue, short* pPosition, short* pBkBitmap, short* pWidth, short* pHeight, OLE_COLOR* pTransColor, BSTR* pComment, OLE_COLOR* pTextColor, short* pXOffSet, short* pYOffSet);
	afx_msg short SetComment(long nIndex, HCOOR_TYPE Time, float Value, short Position, short nBkBitmap, short Width, short Height, OLE_COLOR TransColor, LPCTSTR pComment, OLE_COLOR TextColor, short XOffSet, short YOffSet, short Mask, BOOL bUpdate);
	afx_msg BOOL SwapCommentIndex(long nIndex, long nOldIndex, BOOL bUpdate);
	afx_msg BOOL ShowComment(long nIndex, BOOL bShow, BOOL bUpdate);
	afx_msg BOOL IsCommentVisiable(long nIndex);
	afx_msg void SetEventMask(long Event);
	afx_msg BOOL SetFixedZoomMode(short ZoomMode);
	afx_msg short GetFixedZoomMode();
	afx_msg BOOL FixedZoom(short ZoomMode, short x, short y, BOOL bHoldMode);
	afx_msg BOOL SetCommentPosition(short Position);
	afx_msg short GetCommentPosition();
	afx_msg BOOL GetPixelPoint(HCOOR_TYPE Time, float Value, long* px, long* py);
	afx_msg BOOL GetMemInfo(long* pTempBuffSize, long* pAllBuffSize, float* pUseRate, long* pAddress);
	afx_msg BOOL IsCurveClosed(long Address);
	afx_msg BOOL GetPosData(short nCurveIndex, long nIndex, long* px, long* py);
	afx_msg void EnableHZoom(BOOL bEnable);
	afx_msg BOOL SetHZoom(short Zoom);
	afx_msg short GetHZoom();
	afx_msg BOOL MoveCurveToLegend(long Address, LPCTSTR pSign);
	afx_msg BOOL ChangeLegendName(LPCTSTR pFrom, LPCTSTR pTo);
	afx_msg BOOL SetAutoRefresh(short TimeInterval, short NumInterval);
	afx_msg long GetAutoRefresh();
	afx_msg void EnableSelectCurve(BOOL bEnable);
	afx_msg void SetToolTipDelay(short Delay);
	afx_msg short GetToolTipDelay();
	afx_msg BOOL SetLimitOnePageMode(short Mode);
	afx_msg short GetLimitOnePageMode();
	afx_msg BOOL AddInfiniteCurve(long Address, HCOOR_TYPE Time, float Value, short State, BOOL bUpdate);
	afx_msg BOOL DelInfiniteCurve(long Address, BOOL bAll, BOOL bUpdate);
	afx_msg BOOL SetGraduationSize(long size);
	afx_msg long GetGraduationSize();
	afx_msg void SetMouseWheelMode(short Mode);
	afx_msg short GetMouseWheelMode();
	afx_msg BOOL SetMouseWheelSpeed(short Speed);
	afx_msg short GetMouseWheelSpeed();
	afx_msg BOOL SetHLegend(LPCTSTR pHLegend);
	afx_msg BSTR GetHLegend();
	DECLARE_DISPATCH_MAP()

// 事件映射
	void FirePageChange(long wParam, long lParam)
		{FireEvent(eventidPageChange,EVENT_PARAM(VTS_I4 VTS_I4), wParam, lParam);}
	void FireBeginTimeChange(HCOOR_TYPE NewTime)
		{FireEvent(eventidBeginTimeChange,EVENT_PARAM(HCOOR_VTS_TYPE), NewTime);}
	void FireBeginValueChange(float NewValue)
		{FireEvent(eventidBeginValueChange,EVENT_PARAM(VTS_R4), NewValue);}
	void FireTimeSpanChange(double NewTimeSpan)
		{FireEvent(eventidTimeSpanChange,EVENT_PARAM(VTS_R8), NewTimeSpan);}
	void FireValueStepChange(float NewValueStep)
		{FireEvent(eventidValueStepChange,EVENT_PARAM(VTS_R4), NewValueStep);}
	void FireZoomChange(short NewZoom)
		{FireEvent(eventidZoomChange,EVENT_PARAM(VTS_I2), NewZoom);}
	void FireSelectedCurveChange(long NewAddr)
		{FireEvent(eventidSelectedCurveChange,EVENT_PARAM(VTS_I4), NewAddr);}
	void FireLegendVisableChange(long nIndex, short State)
		{FireEvent(eventidLegendVisableChange,EVENT_PARAM(VTS_I4 VTS_I2), nIndex, State);}
	void FireSorptionChange(long Address, long nIndex, short State)
		{FireEvent(eventidSorptionChange,EVENT_PARAM(VTS_I4 VTS_I4 VTS_I2), Address, nIndex, State);}
	void FireCurveStateChange(long Address, short State)
		{FireEvent(eventidCurveStateChange,EVENT_PARAM(VTS_I4 VTS_I2), Address, State);}
	void FireZoomModeChange(short NewMode)
		{FireEvent(eventidZoomModeChange,EVENT_PARAM(VTS_I2), NewMode);}
	void FireHZoomChange(short NewZoom)
		{FireEvent(eventidHZoomChange,EVENT_PARAM(VTS_I2), NewZoom);}
	void FireBatchExportImageChange(long FileNameIndex)
		{FireEvent(eventidBatchExportImageChange,EVENT_PARAM(VTS_I4), FileNameIndex);}
	DECLARE_EVENT_MAP()

// 调度和事件 ID
public:
	enum {
		dispidForeColor = 1L,
		dispidBackColor = 2L,
		dispidAxisColor = 3L,
		dispidGridColor = 4L,
		dispidPageChangeMSG = 5L,
		dispidMSGRecWnd = 6L,
		dispidTitleColor = 7L,
		dispidFootNoteColor = 8L,
		dispidSetVInterval = 9L,
		dispidSetHInterval = 10L,
		dispidGetScaleInterval = 11L,
		dispidEnableHelpTip = 12L,
		dispidSetLegendSpace = 13L,
		dispidGetLegendSpace = 14L,
		dispidSetBeginValue = 15L,
		dispidGetBeginValue = 16L,
		dispidSetBeginTime = 17L,
		dispidSetBeginTime2 = 18L,
		dispidGetBeginTime = 19L,
		dispidGetBeginTime2 = 20L,
		dispidSetTimeSpan = 21L,
		dispidGetTimeSpan = 22L,
		dispidSetValueStep = 23L,
		dispidGetValueStep = 24L,
		dispidSetVPrecision = 25L,
		dispidGetVPrecision = 26L,
		dispidSetUnit = 27L,
		dispidGetUnit = 28L,
		dispidTrimCoor = 29L,
		dispidAddLegend = 30L,
		dispidGetLegend = 31L,
		dispidQueryLegend = 32L,
		dispidGetLegendCount = 33L,
		dispidGetLegend2 = 34L,
		dispidGetLegendIdCount = 35L,
		dispidGetLegendId = 36L,
		dispidDelLegend = 37L,
		dispidDelLegend2 = 38L,
		dispidAddMainData = 39L,
		dispidAddMainData2 = 40L,
		dispidSetVisibleCoorRange = 41L,
		dispidGetVisibleCoorRange = 42L,
		dispidDelRange = 43L,
		dispidDelRange2 = 44L,
		dispidFirstPage = 45L,
		dispidGotoPage = 46L,
		dispidSetZoom = 47L,
		dispidGetZoom = 48L,
		dispidSetMaxLength = 49L,
		dispidGetMaxLength = 50L,
		dispidGetCutLength = 51L,
		dispidSetShowMode = 52L,
		dispidGetShowMode = 53L,
		dispidSetBkBitmap = 54L,
		dispidGetBkBitmap = 55L,
		dispidSetFillDirection = 56L,
		dispidGetFillDirection = 57L,
		dispidSetMoveMode = 58L,
		dispidGetMoveMode = 59L,
		dispidSetFont = 60L,
		dispidAddImageHandle = 61L,
		dispidAddBitmapHandle = 62L,
		dispidAddBitmapHandle2 = 63L,
		dispidAddBitmapHandle3 = 64L,
		dispidGetBitmapCount = 65L,
		dispidSetBkMode = 66L,
		dispidGetBkMode = 67L,
		dispidExportImage = 68L,
		dispidExportImageFromPage = 69L,
		dispidExportImageFromTime = 70L,
		dispidBatchExportImage = 71L,
		dispidEnableAutoTrimCoor = 72L,
		dispidImportFile = 73L,
		//74L 开源之后，不再使用
		dispidGetOneTimeRange = 75L,
		dispidGetOneValueRange = 76L,
		dispidGetOneFirstPos = 77L,
		dispidGetTimeRange = 78L,
		dispidGetValueRange = 79L,
		dispidGetViableTimeRange = 80L,
		dispidAddMemMainData = 81L,
		dispidShowCurve = 82L,
		//83L 开源之后，不再使用
		dispidSetFootNote = 84L,
		dispidGetFootNote = 85L,
		dispidTrimCurve = 86L,
		dispidPrintCurve = 87L,
		dispidGetEventMask = 88L,
		dispidGetScaleNums = 89L,
		dispidReportPageInfo = 90L,
		dispidShowLegend = 91L,
		dispidSelectCurve = 92L,
		dispidDragCurve = 93L,
		dispidVCenterCurve = 94L,
		dispidGetSelectedCurve = 95L,
		dispidEnableAdjustZOrder = 96L,
		dispidIsSelected = 97L,
		dispidIsLegendVisible = 98L,
		dispidIsCurveVisible = 99L,
		dispidIsCurveInCanvas = 100L,
		dispidGotoCurve = 101L,
		dispidEnableZoom = 102L,
		dispidGetCurveLength = 103L,
		dispidGetLuaVer = 104L,
		dispidGetTimeData = 105L,
		dispidGetValueData = 106L,
		dispidGetState = 107L,
		dispidInsertMainData = 108L,
		dispidInsertMainData2 = 109L,
		dispidCanContinueEnum = 110L,
		dispidDelPoint = 111L,
		dispidGetCurveCount = 112L,
		dispidGetCurve = 113L,
		dispidRemoveBitmapHandle = 114L,
		dispidRemoveBitmapHandle2 = 115L,
		dispidGetBitmap = 116L,
		dispidGetBitmapState = 117L,
		dispidGetBitmapState2 = 118L,
		dispidSetBuddy = 119L,
		dispidGetBuddyCount = 120L,
		dispidGetBuddy = 121L,
		dispidSetCurveTitle = 122L,
		dispidGetCurveTitle = 123L,
		dispidSetHUnit = 124L,
		dispidGetHUnit = 125L,
		dispidSetHPrecision = 126L,
		dispidGetHPrecision = 127L,
		dispidSetCurveIndex = 128L,
		dispidGetCurveIndex = 129L,
		dispidSetGridMode = 130L,
		dispidGetGridMode = 131L,
		dispidSetBenchmark = 132L,
		dispidGetBenchmark = 133L,
		dispidGetPower = 134L,
		dispidTrimCurve2 = 135L,
		dispidChangeId = 136L,
		dispidCloneCurve = 137L,
		dispidUniteCurve = 138L,
		dispidUniteCurve2 = 139L,
		dispidUniteCurve3 = 140L,
		dispidUniteCurve4 = 141L,
		dispidOffSetCurve = 142L,
		dispidArithmeticOperate = 143L,
		dispidClearTempBuff = 144L,
		dispidPreMallocMem = 145L,
		dispidGetMemSize = 146L,
		dispidIsCurve = 147L,
		dispidSetSorptionRange = 148L,
		dispidGetSorptionRange = 149L,
		dispidIsLegend = 150L,
		dispidAddLegendHelper = 151L,
		dispidGetActualPoint = 152L,
		dispidGetPointFromScreenPoint = 153L,
		dispidEnableFullScreen = 154L,
		dispidGetEndTime = 155L,
		dispidGetEndValue = 156L,
		dispidSetZLength = 157L,
		dispidGetZLength = 158L,
		dispidSetCanvasBkBitmap = 159L,
		dispidGetCanvasBkBitmap = 160L,
		dispidSetLeftBkColor = 161L,
		dispidGetLeftBkColor = 162L,
		dispidSetBottomBkColor = 163L,
		dispidGetBottomBkColor = 164L,
		dispidSetZOffset = 165L,
		dispidGetZOffset = 166L,
		dispidEnableFocusState = 167L,
		dispidSetReviseToolTip = 168L,
		dispidGetReviseToolTip = 169L,
		dispidExportMetaFile = 170L,
		dispidLimitOnePage = 171L,
		dispidFixCoor = 172L,
		dispidGetFixCoor = 173L,
		dispidRefreshLimitedOrFixedCoor = 174L,
		dispidSetCanvasBkMode = 175L,
		dispidGetCanvasBkMode = 176L,
		dispidEnablePreview = 177L,
		dispidSetWaterMark = 178L,
		dispidGetSysState = 179L,
		dispidSetTension = 180L,
		dispidGetTension = 181L,
		dispidGetFont = 182L,
		dispidSetXYFormat = 183L,
		dispidGetXYFormat = 184L,
		dispidGetXYFormat2 = 185L,
		dispidLoadPlugIn = 186L,
		dispidAppendLegendEx = 187L,
		dispidGetLegendEx = 188L,
		dispidGetLegendEx2 = 189L,
		dispidGetSelectedNodeIndex = 190L,
		dispidSetSelectedNodeIndex = 191L,
		dispidLoadLuaScript = 192L,
		dispidSetShortcutKeyMask = 193L,
		dispidGetShortcutKeyMask = 194L,
		dispidGetFrceHDC = 195L,
		dispidSetBottomSpace = 196L,
		dispidGetBottomSpace = 197L,
		dispidGetEndTime2=198L,
		dispidGetTimeData2 = 199L,
		dispidAddComment = 200L,
		dispidDelComment = 201L,
		dispidGetCommentNum = 202L,
		dispidGetComment = 203L,
		dispidSetComment = 204L,
		dispidSwapCommentIndex = 205L,
		dispidShowComment = 206L,
		dispidIsCommentVisiable = 207L,
		dispidSetEventMask = 208L,
		dispidSetFixedZoomMode = 209L,
		dispidGetFixedZoomMode = 210L,
		dispidFixedZoom = 211L,
		dispidSetCommentPosition = 212L,
		dispidGetCommentPosition = 213L,
		dispidGetPixelPoint = 214L,
		dispidGetMemInfo = 215L,
		dispidIsCurveClosed = 216L,
		dispidGetPosData = 217L,
		dispidEnableHZoom = 218L,
		dispidSetHZoom = 219L,
		dispidGetHZoom = 220L,
		dispidMoveCurveToLegend = 221L,
		dispidChangeLegendName = 222L,
		dispidSetAutoRefresh = 223L,
		dispidGetAutoRefresh = 224L,
		dispidEnableSelectCurve = 225L,
		dispidSetToolTipDelay = 226L,
		dispidGetToolTipDelay = 227L,
		dispidSetLimitOnePageMode = 228L,
		dispidGetLimitOnePageMode = 229L,
		dispidAddInfiniteCurve = 230L,
		dispidDelInfiniteCurve = 231L,
		dispidRegister1 = 232L,
		dispidSetGraduationSize = 233L,
		dispidGetGraduationSize = 234L,
		dispidSetMouseWheelMode = 235L,
		dispidGetMouseWheelMode = 236L,
		dispidSetMouseWheelSpeed = 237L,
		dispidGetMouseWheelSpeed = 238L,
		dispidSetHLegend = 239L,
		dispidGetHLegend = 240L,
		eventidPageChange = 1L,
		eventidBeginTimeChange = 2L,
		eventidBeginValueChange = 3L,
		eventidTimeSpanChange = 4L,
		eventidValueStepChange = 5L,
		eventidZoomChange = 6L,
		eventidSelectedCurveChange = 7L,
		eventidLegendVisableChange = 8L,
		eventidSorptionChange = 9L,
		eventidCurveStateChange = 10L,
		eventidZoomModeChange = 11L,
		eventidHZoomChange = 12L,
		eventidBatchExportImageChange = 13L,
	};
};

