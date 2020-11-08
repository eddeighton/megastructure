
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
    
    inline OShorthandStream& operator<<( OShorthandStream& os, const FQuat& v )
    {
        return os << v.X << v.Y << v.Z << v.W;
    }

    inline IShorthandStream& operator>>( IShorthandStream& os, FQuat& v )
    {
        return os >> v.X >> v.Y >> v.Z >> v.W;
    }
    
    inline OShorthandStream& operator<<( OShorthandStream& os, const FMatrix& v )
    {
        for (int RowIndex = 0; RowIndex != 4; RowIndex++)
        {
            for (int ColumnIndex = 0; ColumnIndex != 4; ColumnIndex++)
            {
                os << v.M[ RowIndex ][ ColumnIndex ];
            }
        }
        return os;
    }

    inline IShorthandStream& operator>>( IShorthandStream& os, FMatrix& v )
    {
        for (int RowIndex = 0; RowIndex != 4; RowIndex++)
        {
            for (int ColumnIndex = 0; ColumnIndex != 4; ColumnIndex++)
            {
                os >> v.M[ RowIndex ][ ColumnIndex ];
            }
        }
        return os;
    }
    
    inline OShorthandStream& operator<<( OShorthandStream& os, const FTransform& v )
    {
        const FQuat   fq = v.GetRotation();
        const FVector ft = v.GetTranslation();
        const FVector fs = v.GetScale3D();
        
        return os << fq << ft << fs;
    }

    inline IShorthandStream& operator>>( IShorthandStream& os, FTransform& v )
    {
        FQuat   fq;
        FVector ft;
        FVector fs;
        
        os >> fq >> ft >> fs;
        v.SetComponents( fq, ft, fs );
        
        return os;
    }
}

#endif //ED_CONVERSIONS_09_OCT_2020