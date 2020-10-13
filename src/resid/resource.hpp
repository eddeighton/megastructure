
#ifndef RESOURCE_13_OCT_2020
#define RESOURCE_13_OCT_2020

#include "boost/filesystem/path.hpp"

#include <memory>
#include <vector>
#include <string>

namespace resource
{

class ResourceID
{
public:
    using Ptr = std::shared_ptr< ResourceID >;
    using PtrVector = std::vector< Ptr >;
    
    ResourceID( const std::string& strName, const std::string& strPath )
        :   m_strName( strName ), m_strPath( strPath )
    {}
    virtual ~ResourceID();
    
    const std::string& getName() const { return m_strName; }
    const std::string& getPath() const { return m_strPath; }
    int getID() const { return m_id; }
    void setID( int id ) { m_id = id; }

protected:
    std::string m_strName, m_strPath;
    int m_id;
};

class ShipID : public ResourceID
{
public:
    static const std::string ID;
    static const std::string ID_interior;
    static const std::string ID_interior_contour;
    static const std::string ID_exterior;
    static const std::string ID_exterior_contour;
    enum Part
    {
        eInterior,
        eInteriorContour,
        eExterior,
        eExteriorContour
    };
    
    ShipID( const std::string& strName, const std::string& strPath, Part part )
        :   ResourceID( strName, strPath ),
            m_part( part )
    {}
    
    const Part getPart() const { return m_part; }
    
    
private:
    Part m_part;
};

class RoomID : public ResourceID
{
public:
    static const std::string ID;
    static const std::string ID_shape;
    RoomID( const std::string& strName, const std::string& strPath, bool bIsShape )
        :   ResourceID( strName, strPath ),
            m_isShape( bIsShape )
    {}
    
    bool isShape() const { return m_isShape; }
    
private:
    bool m_isShape;
};

class ObjectID : public ResourceID
{
public:
    static const std::string ID;
    ObjectID( const std::string& strName, const std::string& strPath )
        :   ResourceID( strName, strPath )
    {}
    
};

class ResourceNamespace
{
public:
    static const std::string g_strRootNamespace;

    using Ptr = std::shared_ptr< ResourceNamespace >;
    using PtrVector = std::vector< Ptr >;
    
    ResourceNamespace( const std::string& strName )
        :   m_strName( strName )
    {
    }

    void addNamespace( Ptr pNamespace ) { m_children.push_back( pNamespace ); }
    void addResource( ResourceID::Ptr pResource ) { m_resources.push_back( pResource ); }
    
    const std::string& getName() const { return m_strName; }
    const PtrVector& getChildren() const { return m_children; }
    const ResourceID::PtrVector& getResources() const { return m_resources; }
    
private:
    std::string m_strName;
    PtrVector m_children;
    ResourceID::PtrVector m_resources;
};

resource::ResourceNamespace::Ptr load( const boost::filesystem::path& workspacePath, const std::string& strProject );

}

#endif //RESOURCE_13_OCT_2020