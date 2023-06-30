#include "tutorial1_hello_world.h"
#include <cstring>

////////////////////////////////////////////////////////////////////////////////
// Macro creates dynamically linkable Module creation & destruction procedures.
////////////////////////////////////////////////////////////////////////////////
JAM_MODULE_ENTRY( HelloWorldModule )
////////////////////////////////////////////////////////////////////////////////
int HelloWorldModule::Setup( int iArgc, char * apcArgv[] )
{
    counter = 0;
    counter_max = 0; // default

    // Example initialization of memeber variables
    for( int i = 1; i < iArgc; i++ ) 
    {
        if( std::strcmp( apcArgv[i], "-counter" ) == 0 
            && 
            i + 1 < iArgc )
        {            
            std::sscanf( apcArgv[++i], "%d", &counter );
        }
    }

    if( counter_max <= 0 ) 
        counter_max = 1000; // Lets make it 1K by default

    // Default Module::Setup returns 0
    // Returning value other than 0 ends application execution
    return JAM::Module::Setup( iArgc, apcArgv );
}
////////////////////////////////////////////////////////////////////////////////
int HelloWorldModule::Cleanup( void )
{    
    // Default Module::Setup returns 0
    // Returning value other than 0 means module was unable to properly clean up
    return JAM::Module::Cleanup( );
}
////////////////////////////////////////////////////////////////////////////////
int HelloWorldModule::Work( void )
{
    if( counter == 0 )
        std::printf( "printf: Hello World \n" );

    if( counter == 0 )
        std::cout << "cout: Hello World" << std::endl;

    counter ++;

    // Default Module::Work returns 0
    // Returning value other than 0 ends application execution

    if( counter >= counter_max )
        return 1; // Finish it when number of iterations has passed
    else 
        return JAM::Module::Work( );
}
////////////////////////////////////////////////////////////////////////////////
