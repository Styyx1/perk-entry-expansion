#pragma once

#include "SpeechCheck/Hooks.hpp"


namespace PEE
{
	inline static void Install()
	{
		//Set allocator here

		SPCK::Install();
	}
}