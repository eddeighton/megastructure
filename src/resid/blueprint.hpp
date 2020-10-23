
#ifndef BLUEPRINT_RESOURCES_23_OCT_2020
#define BLUEPRINT_RESOURCES_23_OCT_2020

#include "boost/filesystem/path.hpp"

#include <memory>
#include <vector>
#include <string>

namespace resource
{

class BlueprintResource
{
public:
    using Ptr = std::shared_ptr< BlueprintResource >;
    using PtrVector = std::vector< Ptr >;
    
    BlueprintResource( const boost::filesystem::path& blueprintFilePath )
        :   m_blueprintFilePath( blueprintFilePath )
    {}
    virtual ~BlueprintResource();

    const boost::filesystem::path& getBlueprintPath() const { return m_blueprintFilePath; }

protected:
    boost::filesystem::path m_blueprintFilePath;
};

class ShipBlueprint : public BlueprintResource
{
public:
    static const std::string ID;
    static const std::string ID_interior;
    static const std::string ID_exterior;
    static const std::string ID_contour;
    static const std::string ID_whole;
    
    enum Part
    {
        eInterior,
        eInteriorContour,
        eExterior,
        eExteriorContour
    };
    
    ShipBlueprint( const boost::filesystem::path& blueprintFilePath, Part part )
        :   BlueprintResource( blueprintFilePath ),
            m_part( part )
    {}
    
    const Part getPart() const { return m_part; }
    
    const char* getPartName() const
    {
        static const char* szPartNames[] = 
        {
            "Interior",
            "Interior Contour",
            "Exterior",
            "Exterior Contour"
        };
        return szPartNames[ m_part ];
    }
    
    
private:
    Part m_part;
};

class RoomBlueprint : public BlueprintResource
{
public:
    static const std::string ID;
    static const std::string ID_shape;
    
    RoomBlueprint( const boost::filesystem::path& blueprintFilePath, bool bIsShape )
        :   BlueprintResource( blueprintFilePath ),
            m_isShape( bIsShape )
    {}
    
    bool isShape() const { return m_isShape; }
    
private:
    bool m_isShape;
};

class ObjectBlueprint : public BlueprintResource
{
public:
    static const std::string ID;
    
    ObjectBlueprint( const boost::filesystem::path& blueprintFilePath )
        :   BlueprintResource( blueprintFilePath )
    {}
};

class BlueprintNamespace
{
public:
    using Ptr = std::shared_ptr< BlueprintNamespace >;
    using PtrVector = std::vector< Ptr >;
    
    BlueprintNamespace( const std::string& strName )
        :   m_strName( strName )
    {
    }
    virtual ~BlueprintNamespace();

    void addNamespace( Ptr pNamespace ) { m_children.push_back( pNamespace ); }
    void addBlueprint( BlueprintResource::Ptr pResource ) { m_blueprints.push_back( pResource ); }
    
    const std::string& getName() const { return m_strName; }
    const PtrVector& getChildren() const { return m_children; }
    const BlueprintResource::PtrVector& getBlueprints() const { return m_blueprints; }
    
    std::string m_strName;
    PtrVector m_children;
    BlueprintResource::PtrVector m_blueprints;
};

class ShipNamespace : public BlueprintNamespace
{
public:
    ShipNamespace( const std::string& strName )
        :   BlueprintNamespace( strName )
    {
    }
};

resource::BlueprintNamespace::Ptr loadBlueprints( const boost::filesystem::path& workspacePath, const std::string& strProject );


}

#endif //BLUEPRINT_RESOURCES_23_OCT_2020