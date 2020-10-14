
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
    
    ResourceID( const std::string& strName, 
                const std::string& strUnrealResourcePath )
        :   m_strName( strName ),
            m_strUnrealResourcePath( strUnrealResourcePath )
    {}
    virtual ~ResourceID();
    
    const std::string& getName() const { return m_strName; }
    const std::string& getUnrealResourcePath() const { return m_strUnrealResourcePath; }
    int getID() const { return m_id; }
    
    void setID( int id ) { m_id = id; }

protected:
    std::string m_strName;
    std::string m_strUnrealResourcePath;
    int m_id;
};


class BlueprintResourceID : public ResourceID
{
public:
    BlueprintResourceID( const std::string& strName, 
                const std::string& strUnrealResourcePath, 
                const boost::filesystem::path& blueprintFilePath )
        :   ResourceID( strName, strUnrealResourcePath ),
            m_blueprintFilePath( blueprintFilePath )
    {}

    const boost::filesystem::path& getBlueprintPath() const { return m_blueprintFilePath; }

protected:
    boost::filesystem::path m_blueprintFilePath;
};

class ShipID : public BlueprintResourceID
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
    
    ShipID( const std::string& strName, 
                const std::string& strUnrealResourcePath, 
                const boost::filesystem::path& blueprintFilePath, 
                Part part )
        :   BlueprintResourceID( strName, strUnrealResourcePath, blueprintFilePath ),
            m_part( part )
    {}
    
    const Part getPart() const { return m_part; }
    
    
private:
    Part m_part;
};

class RoomID : public BlueprintResourceID
{
public:
    static const std::string ID;
    static const std::string ID_shape;
    
    RoomID( const std::string& strName, 
                const std::string& strUnrealResourcePath, 
                const boost::filesystem::path& blueprintFilePath, 
                bool bIsShape )
        :   BlueprintResourceID( strName, strUnrealResourcePath, blueprintFilePath ),
            m_isShape( bIsShape )
    {}
    
    bool isShape() const { return m_isShape; }
    
private:
    bool m_isShape;
};

class ObjectID : public BlueprintResourceID
{
public:
    static const std::string ID;
    
    ObjectID( const std::string& strName, 
                const std::string& strUnrealResourcePath, 
                const boost::filesystem::path& blueprintFilePath )
        :   BlueprintResourceID( strName, strUnrealResourcePath, blueprintFilePath )
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