#ifndef JAM_MODULE_H
#define JAM_MODULE_H

////////////////////////////////////////////////////////////////////////////////
#include<jam_system.h>
#include<jam_packet.h>

namespace JAM {
////////////////////////////////////////////////////////////////////////////////
class JAM_API Module : public JAM::System {
public:
	////////////////////////////////////////////////////////////////////////////
	// Basic rules of Module construction & destruction:
	// 1 - Setup & Cleanup effectively replace Constructor & Destructor	
	// 2 - Setup & Cleanup might get called in any sequence (eg: S,S,C,S,C,C).
	// 3 - Setup may call Cleanup to reset variables.
	// 4 - Cleanup might be repeatedly & recursively called. Be safe:reset vars.
	// 5 - Basic Constructor only sets simple variables. No allocation allowed.
	// 6 - Advanced Constructor (if needed) does Basic init & calls Setup 
	// 7 - Destructor does nothing except the call to Cleanup
	////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////
	virtual int Setup        ( int iArgc, char * apcArgv[] );
	virtual int Setup        ( int iArgc, char * apcArgv[], PacketPool & pp );
	virtual int Cleanup      ( void );
	virtual int Cleanup      ( PacketPool & pp );
    virtual int Work         ( void );
	virtual int Work         ( PacketPool & pp );
	////////////////////////////////////////////////////////////////////////////
    void        SetPacketPool( PacketPool & packetPool = *(PacketPool*)0 );
    PacketPool& GetPacketPool( void );
	Module *    GetChild      ( int iChild );
	int		    ChildrenCount ( void );
    ////////////////////////////////////////////////////////////////////////////
protected:
    ////////////////////////////////////////////////////////////////////////////
    Module          ( void );
    virtual ~Module ( void );
    ////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////
	Module        **pModuleParents, **pModuleChildren;
	int             iCountParents, iSizeParents, iCountChildren, iSizeChildren;
    PacketPool     *pPacketPool;
	////////////////////////////////////////////////////////////////////////////
protected:
	int             AddChild      ( Module * pModuleChild );
	int             RemoveChild   ( int iChild );
	////////////////////////////////////////////////////////////////////////////
	int             AddParent     ( Module * pModuleParent );
	int             RemoveParent  ( int iParent );
	Module *        GetParent     ( int iParent );
	int		        ParentsCount  ( void );
	////////////////////////////////////////////////////////////////////////////
protected:
    ////////////////////////////////////////////////////////////////////////////
    static Module * CreateModuleObject( char * pcModule );
    static ModuleImage DeleteModuleObject( Module * pModule );
    ////////////////////////////////////////////////////////////////////////////
    ModuleImage moduleImage;
	////////////////////////////////////////////////////////////////////////////
};

} // namespace JAM

////////////////////////////////////////////////////////////////////////////////
#define JAM_MODULE_ENTRY( ModuleClass )                                        \
JAM_DLLMAIN                                                                    \
JAM_ENTRY JAM::Module * CreateModuleObject( void ) { return new ModuleClass; } \
JAM_ENTRY void DeleteModuleObject( ModuleClass * pModule ) { delete pModule; } 

////////////////////////////////////////////////////////////////////////////////
// Our Module Creation & Destruction generator macro. 
// These are used by JAM to create objects from dynamically linked Module. 
////////////////////////////////////////////////////////////////////////////////
#if defined( WIN32 ) && !defined( JAM_NO_DLLMAIN )
#define JAM_DLLMAIN BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) { return 1; }
#else 
#define JAM_DLLMAIN
#endif
////////////////////////////////////////////////////////////////////////////////

#endif
