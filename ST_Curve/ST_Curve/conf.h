
#if !defined __CONF__
#define __CONF__

#define ST_OLE_COLOR	long
#define ST_OLE_HANDLE	long

#define LANG_TYPE		1 //1 for Chinese(P.R.C.), 2 for English(U.S.), 3 for ...
#define USE_DATE_FOR_HCOOR

#ifdef USE_DATE_FOR_HCOOR
	#define HCOOR_TYPE		DATE
	#define HCOOR_VT_TYPE	VT_DATE
	#define HCOOR_VTS_TYPE	VTS_DATE
	#define HCOOR_VTS_PTYPE	VTS_PDATE
#else
	#define HCOOR_TYPE		double
	#define HCOOR_VT_TYPE	VT_R8
	#define HCOOR_VTS_TYPE	VTS_R8
	#define HCOOR_VTS_PTYPE	VTS_PR8
#endif

#endif //#define __CONF__
