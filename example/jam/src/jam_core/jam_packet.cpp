#include<jam_packet.h>
#include<cstring>
#include<cassert>
#include<map> 
#include<string>

//#if defined( _DEBUG ) || defined( DEBUG )
#define CHECK_FOR_PACKET_MISMATCHES
//#endif

namespace JAM {

typedef std::map< std::pair< std::string, size_t >, void * > PacketMap;

////////////////////////////////////////////////////////////////////////////////
PacketPool::PacketPool( void )
{
    packets = static_cast<void*>( new PacketMap );
}
////////////////////////////////////////////////////////////////////////////////
PacketPool::~PacketPool( void )
{    
    delete static_cast<PacketMap*>( packets );
    packets = 0;
}
////////////////////////////////////////////////////////////////////////////////
void* PacketPool::Get
    ( const char * name, size_t size, const void *pv, int iAlloc )
{
    if ( !packets && !iAlloc ) return 0;

    if( !packets )
        packets = static_cast<void*>( new PacketMap );

    PacketMap & packetMap = *static_cast<PacketMap*>( packets );

    PacketMap::key_type key( name, size );

    PacketMap::iterator it = packetMap.find( key );

    if( it != packetMap.end() ) return it->second;

#ifdef CHECK_FOR_PACKET_MISMATCHES

    key.second = 0; // Look for packet of the same name but any size
    it = packetMap.lower_bound( key );
    key.second = size; // restore original size we will need it later

    // Found packet with the same name
    if( it != packetMap.end() && it->first.first == key.first ) {
      // This will Warn only once when packet does not match size requested
      // and packet of requested size does not exists in the pool
      FormatOutput( "WARNING: %s (%s) - Size Requested: %d Found: %d\n",
         "Found requested packet of the same name but different size",
         name, size, it->first.second );
    }
#endif       

    void * packet = Alloc( 0, size );

    if( !packet ) {

        FormatOutput( "ERROR: %s (%s) - Size Requested: %d\n",
           "Could not allocate storeage for new packet", name, size );

        return 0;
    }

    memcpy( packet, pv, size );
    packetMap[ key ] = packet;

    return packet;
}
////////////////////////////////////////////////////////////////////////////////
void PacketPool::Remove( void* packet )
{
    if( !packet ) return;

    PacketMap & map = *static_cast<PacketMap*>( packets );

    for( PacketMap::iterator it= map.begin(); it != map.end(); ++it )
    {
        if( it->second == packets ) {
            map.erase( it );
            return;
        }
    }
}
////////////////////////////////////////////////////////////////////////////////


} // namespace JAM
