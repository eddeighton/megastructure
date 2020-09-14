
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
    virtual void* getRoot() = 0;
};

typedef IMegaHost* (*CreateMegaHostFPtr)( void* );
typedef void (*DestroyMegaHostFPtr)( IMegaHost* );

}

extern "C"
{
    MEGAHOST_API megastructure::IMegaHost* createMegaHost( void* pEngineInterface );
    MEGAHOST_API void destroyMegaHost( const megastructure::IMegaHost* );
}

#endif //MEGASRUCTURE_HOST_24_JULY_2020