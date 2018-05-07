#if !defined(AFX_ST_CURVE_HPP__A736A297_FA0A_4cf1_BA9B_E12251C13461__INCLUDED_)
#define AFX_ST_CURVE_HPP__A736A297_FA0A_4cf1_BA9B_E12251C13461__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ST_Curve.hpp : header file
//

#pragma comment(lib, "ST_Curve.lib")

/*===============================================================================
函数名：GetDIBFromDDB
参数：hDC       设备句柄，不可为0
	  hBitmap   DDB位图句柄
返回值：LPBITMAPINFO类型指针，如果为0，则失败
函数功能及用法：
通过位图句柄（DDB）得到DIB数据，如果函数执行成功，调用者负责释放内存，格式如下：
LocalFree((HLOCAL) lpbi); //lpbi为本函数的返回值
===============================================================================*/
extern "C"
__declspec(dllimport) LPBITMAPINFO __stdcall GetDIBFromDDB(HDC hDC/*[in]*/, HBITMAP hBitmap/*[in]*/);

/*===============================================================================
函数名：ExportBMP
参数：hBitmap   DDB位图句柄
	  pFileName 指定文件名，如果为空，本控件弹出文件选择框供用户选择
返回值：真代表成功，否则失败。
函数功能及用法：
将DDB位图保存到文件中，图像格式由辍决定，支持bmp png jpg gif等格式
注：只提供UNICODE版（因为控件只提供UNICODE版），如果要在mbcs版下使用，需要做字符串转换
===============================================================================*/
extern "C"
__declspec(dllimport) BOOL __stdcall ExportBMP(HBITMAP hBitmap/*[in]*/, const unsigned short* pFileName/*[in]*/);

/*===============================================================================
函数名：CheckUpdate
参数：pHomePage   用于接收官方主页，不需要则传入NULL
	  pVersion	  用于接收版本号，不需要则传入NULL
	  pModifyTime 用于接收修改时间，不需要则传入NULL
返回值：-2 未知故障，官方主页可能被黑了，-1 网络故障，0 成功，但无更新，1 成功，且有更新
函数功能及用法：
检测控件是否有更新，附带传出官方主页地址，版本号以及控件的修改时间
===============================================================================*/
extern "C"
__declspec(dllimport) int __stdcall CheckUpdate(BSTR FAR* pHomePage/*[out]*/, BSTR FAR* pVersion/*[out]*/, BSTR FAR* pModifyTime/*[out]*/);

#endif // !defined(AFX_ST_CURVE_HPP__A736A297_FA0A_4cf1_BA9B_E12251C13461__INCLUDED_)