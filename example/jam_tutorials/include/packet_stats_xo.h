#ifndef PACKET_STATS_XO_H
#define PACKET_STATS_XO_H

#include <jam_packet.h>

struct PacketStatsXO: public JAM::Packet
{
    PacketStatsXO( void ) : JAM::Packet( "PacketStatsXO", sizeof( PacketStatsXO ) )
    {
        for( unsigned int i = 0; i < sizeof( wins ) / sizeof( wins[0] ); wins[i++] = 0 );
    }

    unsigned int wins[ 256 ];
};


#endif

