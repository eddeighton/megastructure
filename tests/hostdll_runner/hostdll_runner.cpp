

#include <iostream>
#include <chrono>
#include <thread>
#include <future>
#include <string>
#include <memory>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

#include "common/processID.hpp"
#include "common/assert_verify.hpp"

#include "hostdll/hostdll.hpp"


#include "X:/tsp/impl/reddwarf/unreal/tsp/unreal.hpp"


struct Args
{
	boost::filesystem::path workspace_path;
};

bool parse_args( int argc, const char* argv[], Args& args )
{
	namespace po = boost::program_options;
	try
	{
		boost::program_options::options_description options;
		boost::program_options::variables_map variables;
		
		options.add_options()
			("help", "produce help message")
			("path",  po::value< boost::filesystem::path >( &args.workspace_path ), "Workspace Path" )
		;

		po::positional_options_description p;
		p.add( "component", -1 );

		po::store( po::command_line_parser( argc, argv).
					options( options ).
					positional( p ).run(),
					variables );
		po::notify( variables );
		
		if( variables.count("help") )
		{
			std::cout << options << "\n";
			return false;
		}
	}
	catch( std::exception& e )
	{
		std::cout << "Invalid input. Type '--help' for options\n" << e.what() << "\n";
		return false;
	}
	return true;
}

std::string readInput()
{
	std::string str;
	std::getline( std::cin, str );
	return str;
}

int main( int argc, const char* argv[] )
{
	Args args;
	if( !parse_args( argc, argv, args ) )
	{
		return 0;
	}
	
	try
	{	
        if( !boost::filesystem::exists( args.workspace_path ) )
        {
            THROW_RTE( "Invalid workspace path specified" );
        }
        const std::string strFilePath = args.workspace_path.string();
    
		std::future< std::string > inputStringFuture =
			std::async( std::launch::async, readInput );
            
        std::shared_ptr< megastructure::IMegaHost > pMegaHost( 
            createMegaHost( strFilePath.c_str(), nullptr ),
            []( const megastructure::IMegaHost* pMegaHost ){ destroyMegaHost( pMegaHost ); } );
            
        //SPDLOG_INFO( "Host: {} pid: {}", args.programName, Common::getProcessID() );
			
		while( true )
		{
			using namespace std::chrono_literals;
			const std::future_status status =
				inputStringFuture.wait_for( 100ms );
			if( status == std::future_status::deferred ||
				status == std::future_status::ready )
			{
				std::string strInput = inputStringFuture.get();
				if( strInput == "quit" )
				{
					break;
				}
                else if( strInput == "" )
                {
                    //do nothing
                }
                else if( strInput == "help" )
                {
                    std::cout << "help - this message\n";
                    std::cout << "test - attempt to get sim root and log stuff\n";
                    std::cout << "quit - quit this host\n";
                }
                else if( strInput == "test" )
                {
                    using ITSP = Iroot::Ireddwarf::Iunreal::Itsp;
                    
                    if( const Iroot* pRoot = (const Iroot*)pMegaHost->getRoot() )
                    {
                        std::cout << "Got Iroot" << std::endl;
                        
                        
                        auto bridgeIter = pRoot->get_u_root_reddwarf_unreal_tsp_Bridge();
                        for( std::size_t sz = 0U; sz != ITSP::IBridge::TOTAL; ++sz, bridgeIter.inc() )
                        {
                            const ITSP::IBridge* pEGBridge = bridgeIter.get();
                            switch( pEGBridge->getState() )
                            {
                                case IContext::eOff        :
                                    {
                                    }
                                    break;
                                case IContext::eRunning    :
                                    {
                                        const ITSP::IpcShipClass::IpcShipVariant::IpcInteriorSpace::IpcSpaceVariant* pVariant = pEGBridge->m_roomVariant();
                                        if( !pVariant )
                                        {
                                            std::cout << "Found bridge but no room variant" << std::endl;
                                        }
                                        else
                                        {
                                            std::cout << "Found bridge with room variant" << std::endl;
                                        }
                                        
                                        const ITSP::IShip::IShipRoom* pShipRoom = static_cast< const ITSP::IShip::IShipRoom* >( pEGBridge->ShipRoom() );
                                        if( !pShipRoom )
                                        {
                                            std::cout << "Ship room link missing" << std::endl;
                                        }
                                        else
                                        {
                                            const ITSP::IShip* pShip = static_cast< const ITSP::IShip* >( pShipRoom->getParent() );
                                            if( !pShip )
                                            {
                                                std::cout << "Ship room link parent ship null" << std::endl;
                                            }
                                            else
                                            {
                                                std::cout << "Found ship from room link with instance: " << pShip->getInstance() << std::endl;
                                            }
                                        }
                                    }
                                    break;
                                case IContext::eStopping   :
                                case IContext::eSuspended  :
                                    break;
                            }
                        }
                    }
                    else
                    {
                        std::cout << "No current root" << std::endl;
                    }
                        
                    
                    /*using TestProject = Iroot::Ireddwarf::Iunreal::Itestproject;
                    const Iroot* pRoot = (const Iroot*)pMegaHost->getRoot();
                    if( const Iroot::Ireddwarf* pRedDwarf = pRoot->reddwarf( 0 ) )
                    {
                        if( const Iroot::Ireddwarf::Iunreal* pUnreal = pRedDwarf->unreal( 0 ) )
                        {
                            if( const TestProject* pTestProject = pUnreal->testproject( 0 ) )
                            {
                                size_t iSim = pTestProject->Tank_begin();
                                while( const TestProject::ITank* pSimTank = pTestProject->Tank( iSim ) )
                                {
                                    std::cout << "Tank: " << pSimTank->getInstance() << " x: " << pSimTank->x() << " y: " << pSimTank->y() << " angle: " <<pSimTank->angle() << "\n";
                                    iSim = pTestProject->Tank_next( iSim );
                                }
                            }
                        }
                    }*/
                    
                }
				else
				{
					std::cout << "Unrecognised input: " << strInput << std::endl;
				}
				
				inputStringFuture =
					std::async( std::launch::async, readInput );
			}
			
			pMegaHost->runCycle();
		}
		
	}
	catch( std::exception& ex )
	{
		std::cout << "exception: " << ex.what() << std::endl;
		throw ex;
	}
	
	return 0;
}
