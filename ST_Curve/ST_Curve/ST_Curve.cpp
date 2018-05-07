// ST_Curve.cpp : CST_CurveApp 和 DLL 注册的实现。

#include "stdafx.h"
#include "ST_Curve.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CST_CurveApp theApp;

const GUID CDECL _tlid = { 0xce831aba, 0x2476, 0x4c2b, { 0xa5, 0x44, 0xcc, 0xf3, 0xba, 0xf7, 0x48, 0xf } };
const GUID CDECL CLSID_SafeItem = { 0x315e7f0e, 0x6f9c, 0x41a3, { 0xa6, 0x69, 0xa7, 0xe9, 0x62, 0x6d, 0x7c, 0xa0 } };
const WORD _wVerMajor = 1;
const WORD _wVerMinor = 0;



// CST_CurveApp::InitInstance - DLL 初始化

BOOL CST_CurveApp::InitInstance()
{
	auto bInit = COleControlModule::InitInstance();

	if (bInit)
	{
		// TODO: 在此添加您自己的模块初始化代码。
#ifdef _UNICODE
		_tsetlocale(LC_ALL, _T("chs"));
#endif
	}

	return bInit;
}



// CST_CurveApp::ExitInstance - DLL 终止

int CST_CurveApp::ExitInstance()
{
	// TODO: 在此添加您自己的模块终止代码。

	return COleControlModule::ExitInstance();
}

static HRESULT CreateComponentCategory(CATID catid, WCHAR* catDescription)
{
	ICatRegister* pcr = nullptr;
	auto hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
		nullptr, CLSCTX_INPROC_SERVER, IID_ICatRegister, (void**) &pcr);

	if (FAILED(hr))
		return hr;

	// Make sure the HKCR\Component Categories\{..catid...}
	// key is registered.
	CATEGORYINFO catinfo;
	catinfo.catid = catid;
	catinfo.lcid = 0x0409; // english

	// Make sure the provided description is not too long.
	// Only copy the first 127 characters if it is.
	auto len = wcslen(catDescription);
	if(len > 127)
		len = 127;
	wcsncpy_s(catinfo.szDescription, catDescription, len);
	// Make sure the description is null terminated.
	catinfo.szDescription[len] = 0;

    hr = pcr->RegisterCategories(1, &catinfo);
	pcr->Release();

    return hr;
}

static HRESULT RegisterCLSIDInCategory(REFCLSID clsid, CATID catid)
{
	// Register your component categories information.
	ICatRegister* pcr = nullptr;
	auto hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
		nullptr, CLSCTX_INPROC_SERVER, IID_ICatRegister, (void**) &pcr);

	if (SUCCEEDED(hr))
	{
		// Register this category as being "implemented" by the class.
		CATID rgcatid[1];
		rgcatid[0] = catid;
		hr = pcr->RegisterClassImplCategories(clsid, 1, rgcatid);
		pcr->Release();
	}

	return hr;
}

static HRESULT UnRegisterCLSIDInCategory(REFCLSID clsid, CATID catid)
{
	ICatRegister* pcr = nullptr;
	auto hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
		nullptr, CLSCTX_INPROC_SERVER, IID_ICatRegister, (void**) &pcr);

	if (SUCCEEDED(hr))
	{
		// Unregister this category as being "implemented" by the class.
		CATID rgcatid[1];
		rgcatid[0] = catid;
		hr = pcr->UnRegisterClassImplCategories(clsid, 1, rgcatid);
		pcr->Release();
	}

	return hr;
}

// DllRegisterServer - 将项添加到系统注册表

STDAPI DllRegisterServer(void)
{
	AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!AfxOleRegisterTypeLib(AfxGetInstanceHandle(), _tlid))
		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(TRUE))
		return ResultFromScode(SELFREG_E_CLASS);

	// Mark the control as safe for initializing.
	// return for safety functions
	auto hr = CreateComponentCategory(CATID_SafeForInitializing,
		L"Controls safely initializable from persistent data!");
	if (FAILED(hr))
		return hr;

	hr = RegisterCLSIDInCategory(CLSID_SafeItem, CATID_SafeForInitializing);
	if (FAILED(hr))
		return hr;

	// Mark the control as safe for scripting.
	hr = CreateComponentCategory(CATID_SafeForScripting,
		L"Controls safely scriptable!");
	if (FAILED(hr))
		return hr;

	hr = RegisterCLSIDInCategory(CLSID_SafeItem, CATID_SafeForScripting);
	if (FAILED(hr))
		return hr;

	return NOERROR;
}



// DllUnregisterServer - 将项从系统注册表中移除

STDAPI DllUnregisterServer(void)
{
	AFX_MANAGE_STATE(_afxModuleAddrThis);

	// Remove entries from the registry.
	// return for safety functions
	auto hr = UnRegisterCLSIDInCategory(CLSID_SafeItem, CATID_SafeForInitializing);
	if (SUCCEEDED(hr))
		hr = UnRegisterCLSIDInCategory(CLSID_SafeItem, CATID_SafeForScripting);

	if (!AfxOleUnregisterTypeLib(_tlid, _wVerMajor, _wVerMinor))
		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(FALSE))
		return ResultFromScode(SELFREG_E_CLASS);

	return hr;
}
