
#ifndef MEGASRUCTURE_HOST_24_JULY_2020
#define MEGASRUCTURE_HOST_24_JULY_2020


#ifdef IS_MEGAHOST_COMPONENT
#define MEGAHOST_API __declspec( dllexport )
#else
#define MEGAHOST_API __declspec( dllimport )
#endif

namespace megastructure
{

struct MEGAHOST_API IMegaHost
{
    virtual ~IMegaHost();
    virtual void runCycle() = 0;
};

MEGAHOST_API IMegaHost* createMegaHost( void* pEngineInterface );
MEGAHOST_API void destroyMegaHost( const IMegaHost* );

}


#endif //MEGASRUCTURE_HOST_24_JULY_2020