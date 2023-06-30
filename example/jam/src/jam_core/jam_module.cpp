#include <jam_module.h>
#include <cstring>

////////////////////////////////////////////////////////////////////////////////
using namespace std;
////////////////////////////////////////////////////////////////////////////////
#define JAM_MODULE_CREATE_OBJECT   "CreateModuleObject"
#define JAM_MODULE_DELETE_OBJECT   "DeleteModuleObject"


namespace JAM {
////////////////////////////////////////////////////////////////////////////////
typedef Module *(*PtrCreateModuleObject)  ( void );
typedef void    (*PtrDeleteModuleObject)  ( Module * );
////////////////////////////////////////////////////////////////////////////////
Module * Module::CreateModuleObject( char * pcName )
{
    if ( !pcName ) return 0;
    char ac[256] = JAM_MODULE_IMAGE_PREFIX;
    strcat( ac, pcName );

    // check for suffix chars at the end of plugin name
    int iSuffixPos = strlen( ac ) - strlen( JAM_MODULE_IMAGE_SUFFIX );
    if( iSuffixPos <= 0 || strcmp( ac + iSuffixPos, JAM_MODULE_IMAGE_SUFFIX ) )
         strcat( ac, JAM_MODULE_IMAGE_SUFFIX ); // and add suffix if not present

    ModuleImage mi = CreateModuleImage( ac );

    if ( !mi ) {
		FormatOutput( "ERROR: CreateModuleImage failed: %s \n-reason: %s \n",
			           ac, ModuleImageError() );
		return 0;
	}

	PtrCreateModuleObject pCMO = (PtrCreateModuleObject)
		GetModuleImageFunction( mi, JAM_MODULE_CREATE_OBJECT );

    if ( !pCMO ) {
		FormatOutput
			( "ERROR: Map CreateModuleObject function failed: %s \n-reason: %s \n",
			   ac, ModuleImageError() );

		DeleteModuleImage( mi );
		return 0;
    }

	Module * pModule = (*pCMO)();

    if ( !pModule ) {
		FormatOutput( "ERROR: CreateModuleObject failed: %s \n-reason: %s",
			          ac, "CreateModuleObject function returned NULL\n" );
		DeleteModuleImage( mi );
		return 0;
	}

    pModule->moduleImage = mi;

	FormatOutput( "INFO: CreateModuleImage succeeded: %s \n", ac );

    return pModule;
}
////////////////////////////////////////////////////////////////////////////////
Module::ModuleImage Module::DeleteModuleObject( Module * pModule )
{
    if ( !pModule ) return NULL;

    ModuleImage mi = pModule->moduleImage;

	PtrDeleteModuleObject pDMO = mi ? (PtrDeleteModuleObject)
		GetModuleImageFunction( mi, JAM_MODULE_DELETE_OBJECT ) : 0;

    if ( pDMO ) (*pDMO)( pModule );
	else delete pModule; /* try brute delete instead */

	return mi;
}
////////////////////////////////////////////////////////////////////////////////
// Module
////////////////////////////////////////////////////////////////////////////////
Module::Module( void )
{
    pModuleParents = 0;
    iCountParents = iSizeParents = 0;

    pModuleChildren = 0;
    iCountChildren = iSizeChildren = 0;

    pPacketPool = 0;
}
////////////////////////////////////////////////////////////////////////////////
Module::~Module( void )
{

}
////////////////////////////////////////////////////////////////////////////////
int Module::Setup( int iArgc, char * apcArgv[], PacketPool & packetPool )
{
    return 0;
}
////////////////////////////////////////////////////////////////////////////////
int Module::Setup( int iArgc, char * apcArgv[] )
{
    return Setup( iArgc, apcArgv, GetPacketPool() );
}
////////////////////////////////////////////////////////////////////////////////
int Module::Cleanup( PacketPool & packetPool )
{
    return 0;
}
////////////////////////////////////////////////////////////////////////////////
int Module::Cleanup( void )
{
    return Cleanup( GetPacketPool() );
}
////////////////////////////////////////////////////////////////////////////////
int Module::Work( PacketPool & packetPool )
{
    return 0;
}
////////////////////////////////////////////////////////////////////////////////
int Module::Work( void )
{
    return Work( GetPacketPool() );
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
int Module::AddChild( Module * pModuleChild )
{
	if ( iCountChildren == iSizeChildren ) {
		int iSize = iSizeChildren ? iSizeChildren << 1 : 1;
		Module ** pm =
            (Module **)Alloc( pModuleChildren, iSize * sizeof(Module *) );

		if( !pm ) return 0; // Shall I raise the exception ?

		pModuleChildren = pm;
		iSizeChildren = iSize;
	}

	pModuleChildren[ iCountChildren++ ] = pModuleChild;
    pModuleChild->AddParent( this );

	return iCountChildren;
}
////////////////////////////////////////////////////////////////////////////////
int Module::RemoveChild( int i )
{
	if( unsigned( i ) >= unsigned( iCountChildren ) ) return 0;

    GetChild( i )->RemoveParent( 0 );

	iCountChildren--;

    for( ; i < iCountChildren; i++ )
		pModuleChildren[ i ] = pModuleChildren[ i + 1 ];

	if ( iCountChildren + iCountChildren <= iSizeChildren ) {
		int iSize = iSizeChildren >> 1;
		Module ** pm =
            (Module **)Alloc( pModuleChildren, iSize * sizeof(Module *) );

		if ( pm || !iSize ) {

			pModuleChildren = pm;
			iSizeChildren = iSize;
		}

	}

	return iCountChildren;
}
////////////////////////////////////////////////////////////////////////////////
Module * Module::GetChild( int i )
{
	return unsigned( i ) < unsigned( iCountChildren ) ? pModuleChildren[ i ] : 0;
}
////////////////////////////////////////////////////////////////////////////////
int	Module::ChildrenCount( void )
{
	return iCountChildren;
}
////////////////////////////////////////////////////////////////////////////////
int Module::AddParent( Module * pModuleParent )
{
	if ( iCountParents == iSizeParents ) {
		int iSize = iSizeParents ? iSizeParents << 1 : 1;
		Module ** pm = (Module **)Alloc( pModuleParents, iSize );

		if( !pm ) return 0; // Raise the exception ?

		pModuleParents = pm;
		iSizeParents = iSize;
	}

	pModuleParents[ iCountParents++ ] = pModuleParent;

	return iCountParents;
}
////////////////////////////////////////////////////////////////////////////////
int Module::RemoveParent( int i )
{
	if( unsigned( i ) >= unsigned( iCountParents ) ) return 0;

	iCountParents--;

	for( ; i < iCountParents; i++ )
		pModuleParents[ i ] = pModuleParents[ i + 1 ];

	if ( iCountParents + iCountParents == iSizeParents ) {
		int iSize = iSizeParents >> 1;
		Module ** pm = (Module **)Alloc( pModuleParents, iSize );

		if ( pm || !iSize ) {

			pModuleParents = pm;
			iSizeParents = iSize;
		}
	}

	return iCountParents;
}
////////////////////////////////////////////////////////////////////////////////
Module * Module::GetParent( int i )
{
	return unsigned( i ) < unsigned( iCountParents ) ? pModuleParents[ i ] : 0;
}
////////////////////////////////////////////////////////////////////////////////
int	Module::ParentsCount( void )
{
	return iCountParents;
}
////////////////////////////////////////////////////////////////////////////////
void Module::SetPacketPool ( PacketPool & packetPool )
{
    pPacketPool = &packetPool;
}
////////////////////////////////////////////////////////////////////////////////
PacketPool& Module::GetPacketPool ( void )
{
    return *pPacketPool;
}
////////////////////////////////////////////////////////////////////////////////

} // namespace JAM
