#include <jam_application.h>
#include <fstream>
#include <cstring>

////////////////////////////////////////////////////////////////////////////////
#define JAM_APPLICATION_CONFIG_SWITCH  "-appcfg"
#define JAM_APPLICATION_CONFIG_FILE    "jamapp.cfg"
#define JAM_APPLICATION_CONFIG_COMMENT '#'
////////////////////////////////////////////////////////////////////////////////
using namespace std;

namespace JAM {
////////////////////////////////////////////////////////////////////////////////
// Application
////////////////////////////////////////////////////////////////////////////////
int Application::Main( int iArgc, char * apcArgv[] )
{
    PacketPool * pPacketPool = new PacketPool;

    SetPacketPool( *pPacketPool );

    Setup( iArgc, apcArgv );

    while( !Work( ) );

    Cleanup( );

    SetPacketPool( );

    return 0;
}
////////////////////////////////////////////////////////////////////////////////
int Application::Setup( int iArgc, char * apcArgv[] )
{    
    ////////////////////////////////////////////////////////////////////////////
    // Read config file (if exists)
    ////////////////////////////////////////////////////////////////////////////
    char acConfigFile[ 256 ] = JAM_APPLICATION_CONFIG_FILE;
	
	bool * pbSkipArgs = new bool[ iArgc ];
	if( pbSkipArgs )
		memset( pbSkipArgs, false,  iArgc * sizeof( *pbSkipArgs ) );
    ////////////////////////////////////////////////////////////////////////////
    //Record directory where app is located (this does not need to be work dir!)
    char acPath[256];
    strncpy( acPath, apcArgv[0], sizeof( acPath ) );
    int iLen = strlen( acPath );
    while( iLen > 0 && acPath[ iLen ] != '\\' && acPath[ iLen ] != '/' ) iLen--;
    char *pcFile = acPath + iLen + 1;
    int iFile = sizeof( acPath ) - iLen - 1;
    ////////////////////////////////////////////////////////////////////////////
    for( int i = 1; i < iArgc - 1; i++ ) {
        if ( strcmp( JAM_APPLICATION_CONFIG_SWITCH, apcArgv[ i ] ) ) continue;
		pbSkipArgs[i] = true;
		pbSkipArgs[i+1] = true;
        strcpy( acConfigFile, apcArgv[ i + 1 ] );
    }
    ////////////////////////////////////////////////////////////////////////////
    int iRet = Module::Setup( iArgc, apcArgv );

    ifstream ifs( acConfigFile, ios::in /* | ios::nocreate*/ );
    if ( ( !ifs || !ifs.is_open() ) )
		FormatOutput
			( "WARNING: Failed to open Jam App config: %s \n", acConfigFile );

    while( ifs ) {
        char ac[256] = "", *apc[ 64 ];

        if( !( ifs.getline( ac, sizeof( ac ) - 1 ) ) ) continue;

        int iC = 0, iCP = ParseLine( ac, apc, VECTOR_LENGTH( apc ) );
		while( iC < iCP && apc[iC][0] != JAM_APPLICATION_CONFIG_COMMENT ) iC++;
        
        Module * pm = 0;
        if( iC ) {
            pm = CreateModuleObject( apc[ 0 ] );
            if( !pm && iLen > 0 ) { // try loading module from app directory
                strncpy( pcFile, apc[0], iFile );
                pm = CreateModuleObject( acPath );
            }
        }
        if ( !pm ) continue;

        pm->SetPacketPool( GetPacketPool() );

		for ( int i = 1; i < iArgc && iC < VECTOR_LENGTH( apc ); i++ )
			if( !pbSkipArgs || !pbSkipArgs[ i ] ) apc[ iC++ ] = apcArgv[ i ];

        int iChildRet = pm->Setup( iC, apc );
        iRet = iRet ? iRet: iChildRet;

        AddChild( pm );
    }    

	if( pbSkipArgs )
		delete [] pbSkipArgs;

    return iRet;
}
////////////////////////////////////////////////////////////////////////////////
int Application::Cleanup( void )
{
    int iChildrenRet = 0;

	Module ** apm = new Module * [ ChildrenCount() ];
	ModuleImage *ami = new ModuleImage [ ChildrenCount() ];

	if( apm ) {
		int iCount = ChildrenCount();

		// First pass : call cleanup functions to free resources
		// we will free module images later because some code may 
		// want to call fuctions in other modules it would be 
		// impossible if these functions were removed from memory
		for ( int i = iCount - 1; i >= 0; i -- ) {
		    apm[i] = GetChild( i );
			int iChildRet = apm[i]->Cleanup();            
			RemoveChild( i );
			iChildrenRet = iChildrenRet ? iChildrenRet: iChildRet;

		}

        // Second pass : if someone was lazy and did not free resources in
        // Cleanup we give him the chance to free stuff in module destructor
        // called as result of delete (done in DeleteModuleObject)
		for ( int i = iCount - 1; i >= 0; i -- )
			ami[ i ] = DeleteModuleObject( apm[i] );

		// Third pass : now free libraries 
		// ( we could not free them earlier because some library could still 
		//   keep pointers to methods or static data located in other modules )
		// we had to let remove these references first and then remove the code
		for ( int i = iCount - 1; i >= 0; i -- )
            if( ami[i] ) DeleteModuleImage( ami[i] );

		delete[] apm;
        delete[] ami;
	}

    int iRet = Module::Cleanup( );
    return iRet ? iRet : iChildrenRet;
}
////////////////////////////////////////////////////////////////////////////////
int Application::Work( void )
{
   if ( !ChildrenCount() ) return 1;

    int iRet = Module::Work( );

    for ( int i = 0; i < ChildrenCount(); i ++ ) {
        int iChildRet = GetChild( i )->Work( );
        iRet = iRet ? iRet : iChildRet;
    }

    return iRet;
}
////////////////////////////////////////////////////////////////////////////////
} // namespace JAM
