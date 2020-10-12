
#ifndef UNREAL_INTERFACE_28_JULY_2020
#define UNREAL_INTERFACE_28_JULY_2020

#pragma warning( push )

#ifdef EG_CLANG_COMPILATION
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch"
#endif

//massive hack here to get the unreal engine preprocessor definitions ( all 300 )
#include "X:/tsp/src/tsp/Intermediate/Build/Win64/UE4Editor/Development/tsp/Definitions.tsp.h"

//get the required unreal engine maths stuff
#include "Runtime/Core/Public/CoreTypes.h"
#include "Runtime/Core/Public/Math/UnrealMath.h"

#ifdef EG_CLANG_COMPILATION
#pragma clang diagnostic pop
#endif

#pragma warning( pop )

//fix ups to including unreal magic platform stuff
#undef check
#undef TEXT

#include "egcomponent/traits.hpp"

namespace Unreal
{
    void log( const wchar_t* msg );
}

using F2 = FVector2D;
using F3 = FVector;
using F4 = FVector4;

// User defined class template specialization
namespace msgpack {
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
namespace adaptor {

//FVector2D
template<>
struct convert<FVector2D> {
    msgpack::object const& operator()(msgpack::object const& o, FVector2D& v) const {
        if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
        if (o.via.array.size != 2) throw msgpack::type_error();
        v = FVector2D(
            o.via.array.ptr[0].as<float>(),
            o.via.array.ptr[1].as<float>());
        return o;
    }
};

template<>
struct pack<FVector2D> {
    template <typename Stream>
    packer<Stream>& operator()(msgpack::packer<Stream>& o, FVector2D const& v) const {
        // packing member variables as an array.
        o.pack_array(2);
        o.pack(v.X);
        o.pack(v.Y);
        return o;
    }
};

template <>
struct object_with_zone<FVector2D> {
    void operator()(msgpack::object::with_zone& o, FVector2D const& v) const {
        o.type = type::ARRAY;
        o.via.array.size = 2;
        o.via.array.ptr = static_cast<msgpack::object*>(
            o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size, MSGPACK_ZONE_ALIGNOF(msgpack::object)));
        o.via.array.ptr[0] = msgpack::object(v.X, o.zone);
        o.via.array.ptr[1] = msgpack::object(v.Y, o.zone);
    }
};

//FVector
template<>
struct convert<FVector> {
    msgpack::object const& operator()(msgpack::object const& o, FVector& v) const {
        if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
        if (o.via.array.size != 3) throw msgpack::type_error();
        v = FVector(
            o.via.array.ptr[0].as<float>(),
            o.via.array.ptr[1].as<float>(),
            o.via.array.ptr[2].as<float>());
        return o;
    }
};

template<>
struct pack<FVector> {
    template <typename Stream>
    packer<Stream>& operator()(msgpack::packer<Stream>& o, FVector const& v) const {
        // packing member variables as an array.
        o.pack_array(3);
        o.pack(v.X);
        o.pack(v.Y);
        o.pack(v.Z);
        return o;
    }
};

template <>
struct object_with_zone<FVector> {
    void operator()(msgpack::object::with_zone& o, FVector const& v) const {
        o.type = type::ARRAY;
        o.via.array.size = 3;
        o.via.array.ptr = static_cast<msgpack::object*>(
            o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size, MSGPACK_ZONE_ALIGNOF(msgpack::object)));
        o.via.array.ptr[0] = msgpack::object(v.X, o.zone);
        o.via.array.ptr[1] = msgpack::object(v.Y, o.zone);
        o.via.array.ptr[2] = msgpack::object(v.Z, o.zone);
    }
};

//FVector4
template<>
struct convert<FVector4> {
    msgpack::object const& operator()(msgpack::object const& o, FVector4& v) const {
        if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
        if (o.via.array.size != 4) throw msgpack::type_error();
        v = FVector4(
            o.via.array.ptr[0].as<float>(),
            o.via.array.ptr[1].as<float>(),
            o.via.array.ptr[2].as<float>(),
            o.via.array.ptr[3].as<float>());
        return o;
    }
};

template<>
struct pack<FVector4> {
    template <typename Stream>
    packer<Stream>& operator()(msgpack::packer<Stream>& o, FVector4 const& v) const {
        // packing member variables as an array.
        o.pack_array(4);
        o.pack(v.X);
        o.pack(v.Y);
        o.pack(v.Z);
        o.pack(v.W);
        return o;
    }
};

template <>
struct object_with_zone<FVector4> {
    void operator()(msgpack::object::with_zone& o, FVector4 const& v) const {
        o.type = type::ARRAY;
        o.via.array.size = 4;
        o.via.array.ptr = static_cast<msgpack::object*>(
            o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size, MSGPACK_ZONE_ALIGNOF(msgpack::object)));
        o.via.array.ptr[0] = msgpack::object(v.X, o.zone);
        o.via.array.ptr[1] = msgpack::object(v.Y, o.zone);
        o.via.array.ptr[2] = msgpack::object(v.Z, o.zone);
        o.via.array.ptr[3] = msgpack::object(v.W, o.zone);
    }
};

//FQuat
template<>
struct convert<FQuat> {
    msgpack::object const& operator()(msgpack::object const& o, FQuat& v) const {
        if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
        if (o.via.array.size != 4) throw msgpack::type_error();
        v = FQuat(
            o.via.array.ptr[0].as<float>(),
            o.via.array.ptr[1].as<float>(),
            o.via.array.ptr[2].as<float>(),
            o.via.array.ptr[3].as<float>());
        return o;
    }
};

template<>
struct pack<FQuat> {
    template <typename Stream>
    packer<Stream>& operator()(msgpack::packer<Stream>& o, FQuat const& v) const {
        // packing member variables as an array.
        o.pack_array(4);
        o.pack(v.X);
        o.pack(v.Y);
        o.pack(v.Z);
        o.pack(v.W);
        return o;
    }
};

template <>
struct object_with_zone<FQuat> {
    void operator()(msgpack::object::with_zone& o, FQuat const& v) const {
        o.type = type::ARRAY;
        o.via.array.size = 4;
        o.via.array.ptr = static_cast<msgpack::object*>(
            o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size, MSGPACK_ZONE_ALIGNOF(msgpack::object)));
        o.via.array.ptr[0] = msgpack::object(v.X, o.zone);
        o.via.array.ptr[1] = msgpack::object(v.Y, o.zone);
        o.via.array.ptr[2] = msgpack::object(v.Z, o.zone);
        o.via.array.ptr[3] = msgpack::object(v.W, o.zone);
    }
};

//FMatrix
template<>
struct convert<FMatrix> {
    msgpack::object const& operator()(msgpack::object const& o, FMatrix& v) const {
        if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
        if (o.via.array.size != 16) throw msgpack::type_error();
        int index = 0;
        for (int RowIndex = 0; RowIndex != 4; RowIndex++)
        {
            for (int ColumnIndex = 0; ColumnIndex != 4; ColumnIndex++)
            {
                v.M[ RowIndex ][ ColumnIndex ] = o.via.array.ptr[ index++ ].as< float >();
            }
        }
        return o;
    }
};

template<>
struct pack<FMatrix> {
    template <typename Stream>
    packer<Stream>& operator()(msgpack::packer<Stream>& o, FMatrix const& v) const {
        // packing member variables as an array.
        o.pack_array(16);
        for (int RowIndex = 0; RowIndex != 4; RowIndex++)
        {
            for (int ColumnIndex = 0; ColumnIndex != 4; ColumnIndex++)
            {
                o.pack( v.M[ RowIndex ][ ColumnIndex ] );
            }
        }
        return o;
    }
};

template <>
struct object_with_zone<FMatrix> {
    void operator()(msgpack::object::with_zone& o, FMatrix const& v) const {
        o.type = type::ARRAY;
        o.via.array.size = 16;
        o.via.array.ptr = static_cast<msgpack::object*>(
            o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size, MSGPACK_ZONE_ALIGNOF(msgpack::object)));
        int index = 0;
        for (int RowIndex = 0; RowIndex != 4; RowIndex++)
        {
            for (int ColumnIndex = 0; ColumnIndex != 4; ColumnIndex++)
            {
                o.via.array.ptr[ index++ ] = msgpack::object( v.M[ RowIndex ][ ColumnIndex ], o.zone );
            }
        }
    }
};

//FTransform
template<>
struct convert<FTransform> {
    msgpack::object const& operator()(msgpack::object const& o, FTransform& v) const {
        
        if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
        if (o.via.array.size != 10) throw msgpack::type_error();
        
        const FQuat fq(
            o.via.array.ptr[0].as<float>(),
            o.via.array.ptr[1].as<float>(),
            o.via.array.ptr[2].as<float>(),
            o.via.array.ptr[3].as<float>());
        
        const FVector ft(
            o.via.array.ptr[4].as<float>(),
            o.via.array.ptr[5].as<float>(),
            o.via.array.ptr[6].as<float>());
        
        const FVector fs(
            o.via.array.ptr[7].as<float>(),
            o.via.array.ptr[8].as<float>(),
            o.via.array.ptr[9].as<float>());
        
        v.SetComponents(fq,ft,fs);
        return o;
    }
};

template<>
struct pack<FTransform> {
    template <typename Stream>
    packer<Stream>& operator()(msgpack::packer<Stream>& o, FTransform const& v) const {
        // packing member variables as an array.
        const FQuat   fq = v.GetRotation();
        const FVector ft = v.GetTranslation();
        const FVector fs = v.GetScale3D();
        
        o.pack_array(10);
        o.pack( fq.X );
        o.pack( fq.Y );
        o.pack( fq.Z );
        o.pack( fq.W );
        
        o.pack( ft.X );
        o.pack( ft.Y );
        o.pack( ft.Z );
        
        o.pack( fs.X );
        o.pack( fs.Y );
        o.pack( fs.Z );
        
        return o;
    }
};

template <>
struct object_with_zone<FTransform> {
    void operator()(msgpack::object::with_zone& o, FTransform const& v) const {
        o.type = type::ARRAY;
        o.via.array.size = 10;
        o.via.array.ptr = static_cast<msgpack::object*>(
            o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size, MSGPACK_ZONE_ALIGNOF(msgpack::object)));
            
        const FQuat   fq = v.GetRotation();
        const FVector ft = v.GetTranslation();
        const FVector fs = v.GetScale3D();
        
        o.via.array.ptr[ 0 ] =  msgpack::object( fq.X );
        o.via.array.ptr[ 1 ] =  msgpack::object( fq.Y );
        o.via.array.ptr[ 2 ] =  msgpack::object( fq.Z );
        o.via.array.ptr[ 3 ] =  msgpack::object( fq.W );
        
        o.via.array.ptr[ 4 ] =  msgpack::object( ft.X );
        o.via.array.ptr[ 5 ] =  msgpack::object( ft.Y );
        o.via.array.ptr[ 6 ] =  msgpack::object( ft.Z );
        
        o.via.array.ptr[ 7 ] =  msgpack::object( fs.X );
        o.via.array.ptr[ 8 ] =  msgpack::object( fs.Y );
        o.via.array.ptr[ 9 ] =  msgpack::object( fs.Z );
    }
};

} // namespace adaptor
} // MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS)
} // namespace msgpack

#endif //UNREAL_INTERFACE_28_JULY_2020