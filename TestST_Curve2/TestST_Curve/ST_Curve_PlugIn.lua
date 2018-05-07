--[[
--如果调用失败，控件内部会捕获，并返回一个空字符串，
--这就不知道是失败还是本接口本来就返回了nil，所以最好保证调用
--本接口不要失败！
function FormatXCoordinate(Address, DateTime, Action) --序号1
return nil
end
--]]

--如果调用失败，控件内部会捕获，并返回一个空字符串，
--这就不知道是失败还是本接口本来就返回了nil，所以最好保证调用
--本接口不要失败！
function FormatYCoordinate(Address, Value, Action) --序号2
	if 1 == Action --绘制坐标轴开始
		then
	elseif 2 == Action --绘制坐标轴过程中
		then
		return string.format("%.1f kw", Value)
	elseif 3 == Action --绘制坐标轴结束
		then
	elseif 4 == Action --显示坐标提示（Tooltip）
		then
		return string.format("%.2f Y坐标提示(lua)", Value)
	elseif 5 == Action --绘制坐标
		then
		return string.format("%.2f 绘制Y坐标", Value)
	elseif 6 == Action --计算Y轴位置时调用，必须和action等于2时返回值完一样，或者更长也可
		then
		return string.format("%.1f kw ", Value) --后面多了一个空格（相比action等于2时），这是可以的
		--[[
		这里有一个技巧：这里返回的字符串，仅仅是用来计算一个长度，以便确定Y轴的位置
		所以你完全可以返回任意字符串，因为这里个数才是有意义的，字符串内容并没有什么意义
		有了这个技巧，你可以在这里返回恒定的字符串，则其长度也就是恒定的，进而Y轴的显示位置也就是恒定的了
		你还可以返回比Action等于2时返回的字符串更长，这样可以让Y坐标与Y轴空出来一定的距离
		--]]
	elseif 7 == Action --绘制填充值
		then
		return string.format("%.0f", Value) --无论精度是多少，都显示到整数位
	end
end

--如果调用失败，将采用控件内部的默认处理方式处理
function TrimXCoordinate(DateTime) --序号3
	--这就是控件内部的实现，在插件里面仍然这样实现，是为了和以前兼容，
	--二次开发者完全可以随意修改
	return tonumber(string.format("%.0f", DateTime + .5)) --修正到整数
end

--如果调用失败，将采用控件内部的默认处理方式处理
function TrimYCoordinate(Value)  --序号4
	--这就是控件内部的实现，在插件里面仍然这样实现，是为了和以前兼容，
	--二次开发者完全可以随意修改
	return tonumber(string.format("%.0f", Value + .5)) --修正到整数
end

--这就是控件内部的实施缩放的实现，在插件里面仍然这样实现，是为了和以前兼容，
--二次开发者完全可以随意修改
function GETSTEP(V, ZOOM)
	if (0 == ZOOM)
		then
		return V
	elseif (ZOOM > 0)
		then
		return V / (ZOOM * .25 + 1)
	else
		return V * (-ZOOM * .25 + 1)
	end
end

--如果调用失败，将采用控件内部的默认处理方式处理
function CalcTimeSpan(TimeSpan, Zoom, HZoom) --序号5
	--这里演示一个变态用法——不缩放横坐标，以显示插件方式的灵活性
	return TimeSpan;
	--下面是老的处理方法
--	return GETSTEP(TimeSpan, Zoom + HZoom)
end

--如果调用失败，将采用控件内部的默认处理方式处理
function CalcValueStep(ValueStep, Zoom)  --序号6
	if (Zoom > 0) --这里演示一下自定义缩放速度，当放大时，我们放大Zoom倍，而不是控件默认的1/4倍
		then
		return ValueStep / (Zoom + 1)
	else
		return ValueStep * (-Zoom + 1)
	end

	--下面是老的处理方法
--	return GETSTEP(ValueStep, Zoom);
end
--更详细的用法参看ST_Curve_PlugIn.hpp头文件
--print(FormatYCoordinate(0, 100.9, 2))

