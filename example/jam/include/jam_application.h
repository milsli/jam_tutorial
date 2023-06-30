#ifndef JAM_APPLICATION_H
#define JAM_APPLICATION_H
////////////////////////////////////////////////////////////////////////////////
#include<jam_module.h>

namespace JAM {
////////////////////////////////////////////////////////////////////////////////
class JAM_API Application : public Module  {
	// Application starts
	// Reads configuration.
	// Based on the configuration loads Module plugins.
	// Creates Module objects.
	// Enters the main loop. 
	// In each loop cycle passes control to the created Modules.
	// Stops the loop when one of the Modules returns request to finish.
	// Each Module is notified that execution has ended.
	// Deletes Module objects.
	// Frees Module plugins.
	// Aplication returns
public:
    // Main is intended as Application entry point
    int Main( int iArgc, char * apcArgv[] );
    // main entry procedure may look like this:
    // 
    // int main( int iArgc, char * pcArgv )
    // { 
    //    return Application().Main( iArgc, pcArgv ); 
    // }
protected:
    int Setup( int iArgc, char * apcArgv[] );
    int Cleanup( void );
    int Work( void );
protected:        
    int Setup( int iArgc, char * apcArgv[], PacketPool & ) { return 0; } 
    int Cleanup( PacketPool & ) { return 0; }
    int Work( PacketPool & ) { return 0; }
    
};
////////////////////////////////////////////////////////////////////////////////
} // namespace JAM
#endif
