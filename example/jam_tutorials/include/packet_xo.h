#ifndef PACKET_XO_H
#define PACKET_XO_H

#include <jam_packet.h>

struct PacketXO: public JAM::Packet
{
    PacketXO( void ) : JAM::Packet( "PacketXO", sizeof( PacketXO ) )
    {
        dimension = 3;
    }

    static const unsigned int max_dimension = 8;
    unsigned int              dimension;
    unsigned char             board[max_dimension][max_dimension];
};


#endif

