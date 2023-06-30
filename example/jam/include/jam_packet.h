#ifndef JAM_PACKET_H
#define JAM_PACKET_H
////////////////////////////////////////////////////////////////////////////////
#include<jam_system.h>

namespace JAM {
////////////////////////////////////////////////////////////////////////////////
struct Packet {

    char   packet_name[256];
    size_t packet_size;
        
    Packet( const char * name, int size = 0 ) : packet_size( size ) {
        size_t i = 0;
        if( name )
            for( ;i < sizeof( packet_name ) && name[i] != '\0'; i++ )
                packet_name[i] = name[i];

        for( ;i < sizeof( packet_name ); i++ ) packet_name[i] = '\0';
    }
};
////////////////////////////////////////////////////////////////////////////////
class JAM_API PacketPool : public JAM::System {

    ////////////////////////////////////////////////////////////////////////////
    // Packet exchange machinery uses PacketPool class
    // 
    // Get retrieves packet of desired Type & Size from the PacketPool
    // iAlloc defines additional behaviour if Packet sought is not in the pool:
    // iAlloc == -1 creates the new packet in the pool
    // iAlloc ==  0 returns NULL
    // iAlloc ==  1 forces creation of new packet even if such packet exists
    // WARNING: Packet returned from Get is only valid til next Get & Remove op.
    //
    ////////////////////////////////////////////////////////////////////////////
public:
    PacketPool( void );
    virtual ~PacketPool( void );

    virtual void* Get
      ( const char *name, size_t size, const void *ppInit = 0, int iAlloc = -1);

    virtual void  Remove( void * packet );

protected:
    void * packets;
};
////////////////////////////////////////////////////////////////////////////////
template<class PacketClass>
PacketClass* RetrievePacket( PacketPool & PP, PacketClass*& pp, int iAlloc = -1)
{
    //It's static to make sure that members unitialized by constructor are zeroed
    static PacketClass p;
    if( !p.packet_size ) p.packet_size = sizeof( p );

    pp = (PacketClass*)
        PP.Get( p.packet_name, sizeof( PacketClass ), &p, iAlloc );

	return pp;
};
////////////////////////////////////////////////////////////////////////////////
} //namespace JAM

#endif
