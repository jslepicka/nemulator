#pragma once
template<typename T> T clamp(T v, T min, T max)
{
	return v < min ? min : v > max ? max : v;
}