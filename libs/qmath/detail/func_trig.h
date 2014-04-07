#pragma once

namespace math
{
	template<typename T> T degrees(T const& radians);
	template<typename T> T radians(T const& degrees);

	//////////////////////////////////////////////////////////////////////////

	//component wise trig for vectors.
	//vec3 val = sin(angles)  is equivalent to
	//	val.x = sin(angles.x)
	//	val.y = sin(angles.y)
	//	val.z = sin(angles.z)
	template<typename T, class Policy = standard> T cos(T const& angle);
	template<typename T, class Policy = standard> T sin(T const& angle);
	template<typename T, class Policy = standard> T tan(T const& angle);
	template<typename T, class Policy = standard> T acos(T const& angle);
	template<typename T, class Policy = standard> T asin(T const& angle);
	template<typename T, class Policy = standard> T atan(T const& angle);
	template<typename T, class Policy = standard> T atan2(T const& y, T const& x);
	template<typename T, class Policy = standard> void sin_cos(T const& angle, T& sin, T& cos);

}
