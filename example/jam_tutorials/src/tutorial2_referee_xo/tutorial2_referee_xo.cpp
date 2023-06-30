#include<jam_module.h>
#include<packet_xo.h>
#include<packet_stats_xo.h>
#include<cstdio>

////////////////////////////////////////////////////////////////////////////////
class ModuleRefereeXO: public JAM::Module {
public:
    int Setup( int argc, char * argv[] );
    int Work( void );
    int Cleanup( void );
protected:
    int rounds;
};
////////////////////////////////////////////////////////////////////////////////
// Macro creates dynamically linkable Module creation & destruction procedures.
////////////////////////////////////////////////////////////////////////////////
JAM_MODULE_ENTRY( ModuleRefereeXO )
////////////////////////////////////////////////////////////////////////////////
int ModuleRefereeXO::Setup( int iArgc, char * apcArgv[] )
{
    // Example initialization of memeber variables
    rounds = 1;

    for( int i = 1; i < iArgc; i++ )
    {
        if( !std::strcmp( apcArgv[i], "-rounds" ) && i + 1 < iArgc )
        {
            ++i;
            std::sscanf( apcArgv[i], "%d", &rounds );
            if( rounds < 0 || rounds > 100 )
                rounds = 1;
        }

        if( !std::strcmp( apcArgv[i], "-dimension" ) && i + 1 < iArgc )
        {
            ++i;

            PacketXO * pxo = JAM::RetrievePacket( GetPacketPool(), pxo );
            
            unsigned int dimension = pxo->dimension;
            std::sscanf( apcArgv[i], "%d", &dimension );
            if( dimension > 2 && dimension < pxo->max_dimension )
            {
                pxo->dimension = dimension;
            }
        }

    }

    std::cout << "Number of rounds to play: " << rounds << std::endl;

    // Default Module::Setup returns 0
    // Returning value other than 0 ends application execution
    return JAM::Module::Setup( iArgc, apcArgv );
}
////////////////////////////////////////////////////////////////////////////////
int ModuleRefereeXO::Work( void )
{
    PacketXO * pxo = JAM::RetrievePacket( GetPacketPool(), pxo );
    PacketStatsXO * psxo = JAM::RetrievePacket( GetPacketPool(), psxo );

    unsigned int winners = 0;

    { // find first diagonal winners
        char mark = pxo->board[0][0];
        for( unsigned int i = 1; mark && i < pxo->dimension; i++ )
        {
            if( mark != pxo->board[i][i] )
                mark = '\0';
        }

        if( mark ) {
            psxo->wins[ unsigned char( mark ) ]++;
            winners++;
        }
    }
        
    {   // find second diagonal winners
        char mark = pxo->board[pxo->dimension-1][0];
        for( unsigned int i = 1; mark && i < pxo->dimension; i++ )
        {
            if( mark != pxo->board[pxo->dimension-1 - i][i] )
                mark = '\0';
        }

        if( mark ) {
            psxo->wins[ unsigned char( mark ) ]++;
            winners++;
        }
    }

    // find column winners
    for( unsigned int x = 0; x < pxo->dimension; x++ )
    {
        char mark = pxo->board[0][x];
        for( unsigned int y = 1; mark && y < pxo->dimension; y++ )
        {
            if( mark != pxo->board[y][x] )
                mark = '\0';
        }

        if( mark ) {
            psxo->wins[ unsigned char( mark ) ]++;
            winners++;
        }
    }

    // find row winners
    for( unsigned int y = 0; y < pxo->dimension; y++ )
    {
        char mark = pxo->board[y][0];
        for( unsigned int x = 1; mark && x < pxo->dimension; x++ )
        {
            if( mark != pxo->board[y][x] )
                mark = '\0';
        }

        if( mark ) {
            psxo->wins[ unsigned char( mark ) ]++;
            winners++;
        }
    }

    // count free fields to see if the round has finished if there are no winners
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

    if( free_fields == 0 || winners > 0 )
    {

        std::cout << "Game: " << std::endl;
        for( unsigned int y = 0; y < pxo->dimension; y++ )
        {
            for( unsigned int x = 0; x < pxo->dimension; x++ )
            {
                char mark = pxo->board[y][x];
                if( mark == '\0' ) mark = '.';
                std::cout << mark << " ";
            }

            std::cout << std::endl;
        }

        if( 0 == --rounds )
            return 1; // End of game

        // Few more rounds to go ? Reset the board
        for( unsigned int y = 0; y < pxo->dimension; y++ ) 
        {
            for( unsigned int x = 0; x < pxo->dimension; x++ ) 
            {
                pxo->board[y][x] = '\0';
            }
        }
    }

    // Default Module::Work returns 0
    // Returning value other than 0 means module was unable to properly clean up
    return JAM::Module::Work( );
}
////////////////////////////////////////////////////////////////////////////////
int ModuleRefereeXO::Cleanup( void )
{
    // Default Module::Setup returns 0
    // Returning value other than 0 means module was unable to properly clean up
    return JAM::Module::Cleanup( );
}
////////////////////////////////////////////////////////////////////////////////
