
#pragma once

/*
#ifdef MEGASTRUCTURE_COMPONENT
#define MEGASTRUCTURE_API __declspec( dllexport )
#else
#define MEGASTRUCTURE_API __declspec( dllimport )
#endif
	*/
	
#include <string>
	
	
namespace megastructure
{
	
    inline std::string generateUniqueString();
		
	static const std::string MEGA_PORT = "1001";
	static const std::string EG_PORT = "1002";
}