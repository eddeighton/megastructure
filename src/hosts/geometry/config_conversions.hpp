
#ifndef ED_CONVERSIONS_09_OCT_2020
#define ED_CONVERSIONS_09_OCT_2020

#include "ed/nodeio.hpp"

namespace Ed
{
    inline OShorthandStream& operator<<( OShorthandStream& os, const FVector4& v )
    {
        return os << v.X << v.Y << v.Z << v.W;
    }

    inline IShorthandStream& operator>>( IShorthandStream& os, FVector4& v )
    {
        return os >> v.X >> v.Y >> v.Z >> v.W;
    }
    
    inline OShorthandStream& operator<<( OShorthandStream& os, const FVector& v )
    {
        return os << v.X << v.Y << v.Z;
    }

    inline IShorthandStream& operator>>( IShorthandStream& os, FVector& v )
    {
        return os >> v.X >> v.Y >> v.Z;
    }
    
    inline OShorthandStream& operator<<( OShorthandStream& os, const FVector2D& v )
    {
        return os << v.X << v.Y;
    }

    inline IShorthandStream& operator>>( IShorthandStream& os, FVector2D& v )
    {
        return os >> v.X >> v.Y;
    }
}

#endif //ED_CONVERSIONS_09_OCT_2020