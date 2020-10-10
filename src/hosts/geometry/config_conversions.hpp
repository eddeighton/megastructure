
#ifndef ED_CONVERSIONS_09_OCT_2020
#define ED_CONVERSIONS_09_OCT_2020

#include "ed/nodeio.hpp"

namespace Ed
{
    inline OShorthandStream& operator<<( OShorthandStream& os, const FVector& v )
    {
        return os << v.X << v.Y << v.Z;
    }

    inline IShorthandStream& operator>>( IShorthandStream& os, FVector& v )
    {
        return os >> v.X >> v.Y >> v.Z;
    }
}

#endif //ED_CONVERSIONS_09_OCT_2020