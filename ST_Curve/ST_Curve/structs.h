
#include "conf.h"

#include <vector>
#include <set>
#include <algorithm>
using namespace std;

/*已经放弃添加此功能，因为二次开发者来实现此功能将更加的灵活多样
//移动联动需要的数据结构
typedef struct tagMoveBuddy
{
	HWND hBuddy; //伙伴句柄
	BYTE Relation;
//	从低位起：
//	1－本窗口顶部与hBuddy底部关联；2－本窗口底部与hBuddy顶部关联；此时两窗口的扩张、收缩行为相反
//	3－本窗口顶部与hBuddy顶部关联；4－本窗口底部与hBuddy底部关联；此时两窗口的扩张、收缩行为相同
	DWORD MaxExpand; //扩张、收缩最大值，参看SetMoveBuddy的MaxExpand
	short CurExpand; //当前的扩张、收缩值，负是处于收缩状态，正是处于扩张状态，0是处于原始状态
	RECTS rect; //鼠标拖动区域，这个矩形高度暂时确定为3个象素
}MoveBuddy;
*/

//图例数据结构
typedef struct tagLegendData //占用16字节，如果成员的位置组织的不好，将占用40字节以上，修改的时候注意，这个结构没有任何填充字节
{
	tagLegendData(LPCTSTR _pSign){ASSERT(_pSign); auto len = _tcslen(_pSign) + 1; pSign = new TCHAR[len]; _tcscpy_s(pSign, len, _pSign);}

	vector<long> Addrs; //占16字节
	LPTSTR pSign;
	short SignWidth; //图例文字打印时占的宽度，是象素，而不是字符串长度

	//下面两个BYTE填充前面的short留下的多余空间
	BYTE PenStyle; //画笔样式，参看CreatePen函数
	BYTE LineWidth; //画笔宽度
	COLORREF PenColor; //画笔颜色

	COLORREF BrushColor; //画刷颜色
	BYTE BrushStyle; //填充曲线下面区域的画刷
	//255－不填充；127－solid brush样式，参看CreateSolidBrush函数，颜色为BrushColor
	//0-126－hatch brush样式，参看CreateHatchBrush函数，颜色为BrushColor
	//128-254－pattern brush样式，参看CreatePatternBrush函数，(BrushStyle - 128)即为位图在BitBmps里面的序号

	BYTE CurveMode; //曲线外观，0－默认外观，两点之间用直线相连；1－先垂直后水平的方波；2－先水平后垂直的方波；3－基数样条曲线
	BYTE NodeModeEx : 4; //按位算，从低位起：1-是否显示头节点；2-是否显示尾节点；3-是否显示选中点；
	BYTE NodeMode : 4; //0：不显示节点；1按曲线颜色显示节点；2按曲线颜色的反色显示节点
	BYTE Lable : 7; //标记坐标点，从低位起：1-是否显示X值，2-是否显示Y值，3-是否隐藏单位，4-是否显示单行
	BYTE State : 1; //为1，则处于显示状态，为0则不显示

	COLORREF BeginNodeColor; //开始点节点颜色，是否使用要看NodeModeEx
	COLORREF EndNodeColor; //结束点节点颜色，是否使用要看NodeModeEx
	COLORREF SelectedNodeColor; //被选中点的颜色

	bool operator ==(LPCTSTR _pSign){return 0 == _tcscmp(pSign, _pSign);}
	bool operator ==(const tagLegendData& _LegendData){return this == &_LegendData;}
	bool operator !=(const tagLegendData& _LegendData){return !(*this == _LegendData);}
	bool operator ==(const tagLegendData* pLegendData){return this == pLegendData;}
	bool operator !=(const tagLegendData* pLegendData){return !(this == pLegendData);}
	operator LPCTSTR(){return pSign;}
	operator COLORREF(){return PenColor;}
}LegendData;

typedef struct tagActualPoint
{
	HCOOR_TYPE Time;
	float Value;
}ActualPoint;

//主体数据结构
typedef struct tagOrigMainData //不要从ActualPoint继承，本结构占用内存16字节，如果从ActualPoint继承，则占用内存达24字节，因为ActualPoint就占了16字节
{
	//下面三个成员的位置很重要，参看WriteFile函数，在调整位置的时候，一定要注意内存占有量（主要是考虑对齐引起的填充），因为对这个结构的需求量是线性增长的
	HCOOR_TYPE Time;
	float Value;
	union
	{
		struct {
			//状态：
			//0－普通点，意思仅仅是非其它状态
			//1－断点，即这个不与后面的点相连
			//2－隐藏该点（前一点将和后一点直接连接，相当于没有这个点）
			BYTE State;
			//按位算：
			//第1位－不显示节点，哪怕图例指示需要显示
			//……以后还有待扩展
			BYTE StateEx;
		};
		USHORT AllState;
	};
	USHORT Reserved;

	operator HCOOR_TYPE(){return Time;}
	operator float(){return Value;}
	bool operator ==(const tagOrigMainData& _OrigMainData){return this == &_OrigMainData;}
	bool operator !=(const tagOrigMainData& _OrigMainData){return !(*this == _OrigMainData);}
	bool operator ==(const tagOrigMainData* pOrigMainData){return this == pOrigMainData;}
	bool operator !=(const tagOrigMainData* pOrigMainData){return !(this == pOrigMainData);}
	bool operator >=(const tagOrigMainData& _OrigMainData){return Time >= _OrigMainData.Time;}
	bool operator <=(const tagOrigMainData& _OrigMainData){return Time <= _OrigMainData.Time;}
}OrigMainData;

typedef struct tagMainData : OrigMainData //这样的继承关系，不会带来多余的填充（对齐引起的填充）
{
	POINT ScrPos; //在屏幕上的坐标
}MainData;

#define CommentLen	64
typedef struct tagCommentData : tagMainData
{
	short nBkBitmap; //-1不使用背景
	char XOffSet; //文字偏移量
	char YOffset; //文字偏移量
	COLORREF TransColor; //位图中当成透明处理的颜色
	COLORREF TextColor : 24; //字体颜色
	COLORREF Position : 7; //0-左上角 1-左下角 2-右上角 3-右下角 4-中心（本来是BYTE数据类型即可，但为了分配TextColor剩余的一个字节，只能申明成COLORREF）
	COLORREF State : 1; //为1，则处于显示状态，为0则不显示
	short Width; //注解的显示宽度
	short Height; //注解的显示高度
	TCHAR Comment[CommentLen];
}CommentData;

typedef struct tagBitBmp
{
	HBITMAP hBitBmp;
	UINT State; //从低位起，第1位－是否为共享（此时不自动释放句柄）；第2位－是否为控件创建的位图句柄
	//如果句柄为控件所创建，则不管是否为共享，都将由控件来释放句柄，如果句柄不是由控件创建，则控件只在非共享的情况下释放句柄

	operator HBITMAP(){return hBitBmp;}
	operator UINT(){return State;}
	bool operator ==(const HBITMAP hBitBmp){return this->hBitBmp == hBitBmp;}
	bool operator !=(const HBITMAP hBitBmp){return !(*this == hBitBmp);}
	bool operator ==(const tagBitBmp& _BitBmp){return this->hBitBmp == _BitBmp.hBitBmp;}
	bool operator !=(const tagBitBmp& _BitBmp){return !(*this == _BitBmp);}
	bool operator ==(const tagBitBmp* pBitBmp){return this->hBitBmp == pBitBmp->hBitBmp;}
	bool operator !=(const tagBitBmp* pBitBmp){return !(this == pBitBmp);}
}BitBmp;
/*
template <typename T = HCOOR_TYPE>
class IsInRange
{
public:
	IsInRange(T Min, T Max){ASSERT(Min <= Max); this->Min = Min; this->Max = Max;}
	bool operator()(const T Value){return Min <= Value && Value <= Max;}
protected:
	T Min, Max;
};
*/
/*
下面这个类用于解决vc2010下面，迭代器与未初始化迭代器之间的==比较时产生的断言错误
本类重载了==运算符，但只能在左边，比如下面的代码是不行的：
vector<int> v;
v.push_back(100);
null_iterator<vector<int>::iterator> null_iter; //一个未初始化迭代器
vector<int>::iterator iter = begin(v);
if (iter == null_iter) //iter != null_iter也一样
	; //操作
这里其实并没有调用null_iterator的==运算符，而是调用了null_iterator的T()运算符，结果成了：
if (iter == (vector<int>::iterator) null_iter)

正确的方法是把null_iter写在前面，如下：
if (null_iter == iter) //null_iter != iter也一样

至于为什么要重载一个T()运算符，是因为考虑到用null_iterator来初始化一个迭代器这种需求，比如：
vector<int>::iterator iter = null_iterator<vector<int>::iterator>();

注意，不能重载T&()运算符以返回一个T&，因为当null_iterator用在默认参数的时候，
编译器无法在T()还是T&()之间选择，造成错误，比如：
void fun(vector<int>::iterator iter = null_iterator<vector<int>::iterator>());

但返回T&也有一个好处，比如有这样一个函数：
void fun(vector<int>::iterator& iter);
则可以如下调用：
void fun(null_iterator<vector<int>::iterator>());
如果没有T&()，则无法编译，而只能如下而求其次：
null_iterator<vector<int>::iterator> NoUse;
void fun(NoUse);

权衡两种弊端，还是选择了不重载T&()，因为更多的地方需要默认参数，而不需要传递迭代器引用
*/
template<typename T>
class null_iterator
{
public:
	null_iterator() {}
	virtual ~null_iterator() {}

public:
	bool operator ==(T& iter) { return nullptr == iter._Ptr; }
	bool operator !=(T& iter) { return !(*this == iter); }
	operator T() { return m_t; } //将一个迭代器初始化为一个未初始化的迭代器
//	operator T&() { return m_t; } //不要返回引用，否则在作为默认参数的时候，编译器无法在T()还是T&()之间选择，造成错误

protected:
	T m_t; //未初始化迭代器
};

//不带任何参数申请一个iterator，就是下面的类型
#define NullDataIter (null_iterator<vector<MainData>::iterator>())
#define NullDataListIter (null_iterator<vector<DataListHead<MainData>>::iterator>())
#define NullInfiniteCurveIter (null_iterator<vector<InfiniteCurveData>::iterator>())
#define NullLegendIter (null_iterator<vector<LegendData>::iterator>())
#define NullAddrs (null_iterator<vector<long>::iterator>())
#define NullBitBmps (null_iterator<vector<BitBmp>::iterator>())

template<typename T>
void free_container(T& con)
{
#if _MSC_VER >= 1600
	T NoUse(move(con)); //vc2010及其以上的版本，只能这样才能彻底释放缓存，之前的版本调用clear()即可
#else
	con.clear();
#endif
}

template<typename T, typename V>
void do_fill_1_value(T* _p, V v)
{
	if (!IsBadWritePtr(_p, sizeof(T)))
		*_p = v;
}

template <typename T>
struct OrigDataListHead
{
	OrigDataListHead() : pDataVector(new vector<T>) {}

	long Address;
	vector<T>* pDataVector;
	//这里必须要定义为指针，否则在交换某两个DataListHead的时候，时间复杂度将是线性的
	//如果放弃交换两个DataListHead这种功能，将无法更改曲线的显示层次次序
	//定义成指针带来一个小问题，就是在删除DataListHead时，需要释放这个指针指向的内存
	//在新建DataListHead时，需要去动态分配这个指针指向的空间（这一步可以放在构造函数里面进行）
	//注意，不能在析构函数里面释放pDataVector，因为用于MainDataListArr.push_back的那个临时变量，在析构的时候
	//会错误的释放掉pDataVector，而这个pDataVector在MainDataListArr里面还要使用
	//但是比较利弊，还是采用定义为指针的形式

	bool operator ==(const OrigDataListHead& _OrigDataListHead){return this == &_OrigDataListHead;} //地址相等才认为相等
	bool operator !=(const OrigDataListHead& _OrigDataListHead){return !(*this == _OrigDataListHead);}
	bool operator ==(const OrigDataListHead* pOrigDataListHead){return this == pOrigDataListHead;} //地址相等才认为相等
	bool operator !=(const OrigDataListHead* pOrigDataListHead){return !(this == pOrigDataListHead);}
	operator long(){return Address;}
};

/*
由于DataListHead采用迭代器与其它数据结构关联，所以在迭代器失效的时候更新迭代器是非常必要的：
一：在添加图例的时候，当LegendArr出现内存不足需要重新分配内存时，更新添加图例以前所有图例所包函的所有DataListHead的LegendIter
二：在删除图例的时候，更新从删除位置开始的所有图例所包函的所有DataListHead的LegendIter
*/
template <typename T>
struct DataListHead : OrigDataListHead<T> //占用76字节，包括两个填充字节，将来可以扩展
{
	MainData LeftTopPoint;
	MainData RightBottomPoint;
	vector<LegendData>::iterator LegendIter;

	size_t SelectedIndex; //被选中的点，它的节点将显示为图例的SelectedNodeColor颜色（如果节点有显示的话）

	BYTE FillDirection;
	//从低位起：
	//1－向下填充
	//2－向右填充
	//3－向上填充
	//4－向左填充
	//本控件没有对各个位进行互斥，但实际上，某些位的结合是没有意义的，比如既向上又向下填充，这样全屏都将被填充，互斥交给二次开发者
	//注意，要填充，还要图例的支持，如果图例指示不填充，则这个字节无效
	//第5、6位当成一个整体看，用于在填充的区域之中显示纵坐标值：
	//0－不显示值，1－显示第一个值，2－显示最后一个值，3－显示平均值
	//第7、8位只有在5、6位不为0的时候有效（并且填充模式只能是hatch或Pattern填充模式），用于指定显示纵坐标的颜色，具体意义如下：
	//第7位：
	//0－使用前景色，1－使用本条曲线的画笔的颜色
	//第8位如果为1，则使用第7位指示的颜色的反色
	//在填充模式为Solid模式时，7、8位无效，此时将使用Solid模式的填充色的反色来显示纵坐标

	BYTE Power; //幂次，0次和1次均当成1次，多余1次都当成2次

	short Zx, Zy; //Z轴坐标在X和Y坐标上的分量
	//还剩下两个字节没分配，浪费掉了
};

struct InfiniteCurveData : MainData
{
	long Address;

	//BYTE State;
	//这个值从OrigMainData继承而来，但意义不一样：0-水平，Value有效；1-垂直，Time有效

	//与DataListHead相对应的成员，意义也一样，但FillDirection只支持0-0xF，即不允许显示值
	vector<LegendData>::iterator LegendIter;
	BYTE FillDirection;

	//由于MainData重载了HCOOR_TYPE和float，所以本类不能通过重载long让泛型函数find正常工作
	operator long(){return Address;}
	bool operator ==(const long Address){return this->Address == Address;}
};

typedef struct tagBatchExport
{
	LPTSTR pFileName;
	LPTSTR pStart;
	UINT nWidth;
	UINT nFileNum;
	TCHAR cNumFormat[16];
}BatchExport;

#define PolyTextLen		32
template <typename T>
struct CoorData
{
	CoorData(){pPolyTexts = 0; pTexts = 0; nPolyText = 0; nScales = 0; RangeMask = 0;}
	CoorData(int _nPolyText){ASSERT(_nPolyText >= 0); nPolyText = _nPolyText; nScales = 0; RangeMask = 0; assign();}
	~CoorData(){clear();}

	void clear(){if (pPolyTexts) delete[] pPolyTexts; pPolyTexts = nullptr; if (pTexts) delete[] pTexts; pTexts = nullptr; nPolyText = 0;}
	void reserve(int _nPolyText)
	{
		if (_nPolyText > nPolyText) //只在内存不够的时候重新分配内存，内存有多余时不作处理，如果非要收回多余的内存，则先调用FreeMem一次再调用本函数即可
		{
			clear();
			nPolyText = _nPolyText;
			assign();
		}
	}

	T		fStep;      //坐标步长
	T		fCurStep;   //当前真正的坐标步长，当没有缩放的时候，它等于fTimeStep

	POLYTEXT* pPolyTexts;	//用于PolyTextOut函数
	TCHAR	(*pTexts)[PolyTextLen];	//每一次PolyTextOut函数时需要的字符串
	USHORT	nPolyText : 14;	//pTexts指向的地址中TCHAR[PolyTextLen]的个数
	USHORT	RangeMask : 2;  //从低位起，每一位依次代表fMinVisibleValue和fMaxVisibleValue的有效性，1为有效
	USHORT	nScales;		//坐标刻度个数

	T		fMinVisibleValue;
	T		fMaxVisibleValue;

protected:
	void assign()
	{
		pPolyTexts = new POLYTEXT[nPolyText];
		memset(pPolyTexts, 0, nPolyText * sizeof(POLYTEXT));
		pTexts = new TCHAR[nPolyText][PolyTextLen];
		for (auto i = 0; i < nPolyText; ++i)
			pPolyTexts[i].lpstr = pTexts[i];
	}
};
