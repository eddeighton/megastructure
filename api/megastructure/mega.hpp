
#pragma once

/*
#ifdef MEGASTRUCTURE_COMPONENT
#define MEGASTRUCTURE_API __declspec( dllexport )
#else
#define MEGASTRUCTURE_API __declspec( dllimport )
#endif
	*/
	
#ifdef MEGASTRUCTURE_EG_COMPONENT
#define MEGASTRUCTURE_EG_API __declspec( dllexport )
#else
#define MEGASTRUCTURE_EG_API __declspec( dllimport )
#endif
	