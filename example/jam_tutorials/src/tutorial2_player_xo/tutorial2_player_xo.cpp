#include<jam_module.h>
#include<packet_xo.h>
#include<packet_stats_xo.h>
#include<cstdio>

////////////////////////////////////////////////////////////////////////////////
class ModulePlayerXO: public JAM::Module {
public:
    int Setup( int argc, char * argv[] );
    int Work( void );
    int Cleanup( void );
protected:
    char mark;
};
////////////////////////////////////////////////////////////////////////////////
// Macro creates dynamically linkable Module creation & destruction procedures.
////////////////////////////////////////////////////////////////////////////////
JAM_MODULE_ENTRY( ModulePlayerXO )
////////////////////////////////////////////////////////////////////////////////
int ModulePlayerXO::Setup( int iArgc, char * apcArgv[] )
{
    // Example initialization of memeber variables
    mark = 'o';

    for( int i = 1; i < iArgc; i++ )
    {
        char c = mark;
        std::sscanf( apcArgv[i], "%c", &c );
        if( isalpha( c ) ) {
            mark = (char)toupper( c );
        }
    }

    // Default Module::Setup returns 0
    // Returning value other than 0 ends application execution
    return JAM::Module::Setup( iArgc, apcArgv );
}
////////////////////////////////////////////////////////////////////////////////
int ModulePlayerXO::Work( void )
{
    PacketXO * pxo = JAM::RetrievePacket( GetPacketPool(), pxo );

    // 1 step count free fields
    unsigned int free_fields = 0;
    for( unsigned int y = 0; y < pxo->dimension; y++ ) 
    {
        for( unsigned int x = 0; x < pxo->dimension; x++ ) 
        {
            if( pxo->board[y][x] == '\0' )
            {
                free_fields ++;
            }
        }
    }

    if( free_fields ) 
    {
        // 2 step change randomly drawn free field to our mark
        unsigned int draw_field = unsigned int( rand() ) % free_fields;
        for( unsigned int y = 0; y < pxo->dimension; y++ )
        {
            for( unsigned int x = 0; x < pxo->dimension; x++ )
            {
                if( pxo->board[y][x] == '\0' )
                {
                    if( 0 == draw_field-- )
                        pxo->board[y][x] = mark;
                }
            }
        }
    }

    // Default Module::Work returns 0
    // Returning value other than 0 means module was unable to properly clean up
    return JAM::Module::Work( );
}
////////////////////////////////////////////////////////////////////////////////
int ModulePlayerXO::Cleanup( void )
{
    PacketStatsXO * psxo = JAM::RetrievePacket( GetPacketPool(), psxo );

    unsigned int wins = psxo->wins[ unsigned char( mark ) ];

    std::cout << "Player " << mark << " won " << wins << " games" << std::endl;

    // Default Module::Setup returns 0
    // Returning value other than 0 means module was unable to properly clean up
    return JAM::Module::Cleanup( );
}
////////////////////////////////////////////////////////////////////////////////
