#if !defined(AFX_ST_CURVE_PLUGIN_HPP__B6FA90DD_7D0D_4bb1_9D2C_DCCABDFF1272__INCLUDED_)
#define AFX_ST_CURVE_PLUGIN_HPP__B6FA90DD_7D0D_4bb1_9D2C_DCCABDFF1272__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ST_Curve_PlugIn.hpp : header file
//

#include <wtypes.h>

/*===============================================================================
本头文件定义ST_Curve控件支持的第1类插件的函数接口签名
目前ST_Curve暂时只支持1类插件，以后可能还会有2、3...类插件
第1类插件主要包括以下内容：
1.自定义坐标值的显示
2.修整坐标
3.自定义缩放公式

特别注意：
本插件中所有接口函数返回的字符串指针所指向的内存，一定不能是临时的，因为控件需要保存下来
使用，内存至少要保存到调用下一个接口的时候不被覆盖，比如控件调用了FormatXCoordinate，
插件返回一个指针，这个指针指向的内存一定不能被覆盖，直到再次调用FormatXCoordinate
或者本插件其它接口时（比如FormatYCoordinate），换句话说，本插件里面所有接口可以共享
同一片内存，但这片内存不能被本插件里面非接口函数的行为所覆盖。具体看TestST_Curve2工程，
里面有一个插件的实现
  
接口函数不要指定为__stdcall类型，这样插件开发者不得不将导出文件在def文件里面定义一次
不然导出的函数将会是类似于_FormatXCoordinate@2，加上extern "C"也一样，为了减少插件开发
者的麻烦，我没有用__stdcall修饰函数，插件开发不要擅自将下面的函数加上__stdcall修饰，
否则会有问题，因为控件里面是没有用__stdcall修饰的，而且还会造成控件查找插件接口函数失败
	
注：只提供UNICODE版（因为控件只提供UNICODE版），如果要在mbcs版下使用，需要做字符串转换
只要是实现了（并且需要加载）的接口，都要求必须实现所有Action（目前从1至7），除非
上面明确说了不会被调用
===============================================================================*/

/*===============================================================================
函数名：FormatXCoordinate FormatYCoordinate
参数：DateTime Value 控件传来的参数，要求把这些值类型数据转换成字符串
	  Action		 转换用途
函数功能及用法：
ST_Curve回调插件里面的函数，传入值类型数据，插件转换为字符串
DateTime Value 就是控件的XY坐标值

Address参数的解释如下：
当Action等于4的时候，其值是要进行坐标提示的曲线地址，只有在吸附效应
开启（参看SetSorptionRange接口），并且已经被成功吸附的情况下才有意义，
否则恒等于0x7fffffff（因为控件内部也不知道曲线地址）；
当Action等于5的时候，其值就是要绘制坐标的曲线地址，不会出现0x7fffffff的情况

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
	只有FormatYCoordinate会传入该值
	
7-绘制填充值
	关于什么是填充值，参看SetFillDirection接口
	只有FormatYCoordinate会传入该值，返回的字符串长度限制127个字符

以上所有的返回值，除特殊说明外，均不支持换行符
===============================================================================*/
extern "C"
__declspec(dllexport) LPCWSTR FormatXCoordinate(long Address, DATE DateTime, UINT Action); //序号1
extern "C"
__declspec(dllexport) LPCWSTR FormatYCoordinate(long Address, float Value, UINT Action); //序号2


/*===============================================================================
函数名：TrimXCoordinate TrimYCoordinate
参数：DateTime Value 控件传来的参数，要求把这些值修整
函数功能及用法：
修整坐标，如果用lua实现，当调用lua失败时，前者返回当前时间，后者返回.0f
===============================================================================*/
extern "C"
__declspec(dllexport) DATE TrimXCoordinate(DATE DateTime); //序号3
extern "C"
__declspec(dllexport) float TrimYCoordinate(float Value);  //序号4

/*===============================================================================
函数名：CalcTimeSpan CalcValueStep
参数：TimeSpan		当前间隔，或者期望间隔，具体看bReverse参数
	  ValueStep		同上
	  Zoom HZoom	缩放和水平缩放
函数功能及用法：
二次开发者应该根据Zoom和HZoom的值，去计算出一个施加缩放后的值，返回给控件，比如Zoom等于1
HZoom等于0时，二次开发者可以返回 TimeSpan * 2（缩小了一倍，默认在控件中，Zoom等于1是放大
1/4的，可见有了这两个接口之后，二次开发者完全可以自定义控件的缩放行为，缩放速度，缩放加速度等，
只要你想得到）
注：有一点二次开发者必须要注意，放大缩小必须保证圆场，什么意思呢，就是负负得正的意思，比如：
double a = ...;
double a1 = CalcTimeSpan(a, 1, 0); //把a放大1陪
double a2 = CalcTimeSpan(a1, -1, 0); //把a1缩小1陪
此时，a应该等于a2，即用放大n倍的结果，去缩小n倍，得到的结果应该等于原始值（相当于没有放大也没有缩小）
这在限制控件只显示一页的时候需要使用，如果你确定不会使用“限制一页”这个功能，则可以不遵守上面的规定
===============================================================================*/
extern "C"
__declspec(dllexport) double CalcTimeSpan(double TimeSpan, short Zoom, short HZoom); //序号5
extern "C"
__declspec(dllexport) float CalcValueStep(float ValueStep, short Zoom);  //序号6

//目前总共6个接口，所以，想要加载所有接口的话，LoadPlugIn和LoadLuaScript接口的最后一个参数应该是0x3F

#endif // !defined(AFX_ST_CURVE_PLUGIN_HPP__B6FA90DD_7D0D_4bb1_9D2C_DCCABDFF1272__INCLUDED_)