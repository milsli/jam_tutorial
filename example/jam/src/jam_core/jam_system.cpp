#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS 1
#endif
#include<jam_system.h>
///////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
#include <windows.h>
#include <mmsystem.h>
#include <sys/timeb.h>
#include <time.h>
#include <io.h>
#else
#include <dlfcn.h> // Linux & SGI
#include <sys/time.h>
#include <stdlib.h>
#include <stdarg.h>
#endif

#include <cstring>
#include <cctype>
#include <cstdio>

namespace JAM {

///////////////////////////////////////////////////////////////////////////////
System::ModuleImage System::CreateModuleImage( const char * pcName )
{
#ifdef WIN32
    return LoadLibrary( pcName );
#else
	return dlopen( pcName, RTLD_LAZY );
#endif
}
///////////////////////////////////////////////////////////////////////////////
void System::DeleteModuleImage( ModuleImage mi )
{
#ifdef WIN32
    FreeLibrary( mi );
#else
	dlclose( mi );
#endif
}
///////////////////////////////////////////////////////////////////////////////
System::PtrModuleImageFunction 
	System::GetModuleImageFunction( ModuleImage mi, const char * pcFunction )
{
#ifdef WIN32
	return (PtrModuleImageFunction)GetProcAddress( mi, pcFunction );
#else
	return (PtrModuleImageFunction)dlsym( mi, pcFunction );
#endif
}
///////////////////////////////////////////////////////////////////////////////
const char * System::ModuleImageError( void )
{
#ifdef WIN32
    DWORD dwError = GetLastError( );

    static char acMessage[ 1024 ];

    if( !FormatMessage 
        ( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
          NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),//Default
          acMessage, sizeof( acMessage ), NULL ) )
                wsprintf( acMessage, "Unknown error(%xh) ", dwError );

    return acMessage;
#else
    const char * pcRet = dlerror( );
    return pcRet ? pcRet : "Unknown error";
#endif
}
///////////////////////////////////////////////////////////////////////////////
void System::DefaultOutput( const char * pcOut, int iLength )
{
	if( iLength <= 0 ) iLength = strlen( pcOut );
	fwrite( pcOut, iLength, 1, stdout );
        //fwrite("\n",strlen("\n"),1,stdout);
	fflush(stdout);
}
///////////////////////////////////////////////////////////////////////////////
void (*(System::OutputMessage))( const char *, int ) = &System::DefaultOutput;
///////////////////////////////////////////////////////////////////////////////
void System::FormatOutput( const char * pcFormat, ... )
{
	if( !OutputMessage ) return;
	static char ac[ 4 ][ 1024 ];
	static int  iBuffer = 0;

	char * pc = ac[ ++iBuffer %= VECTOR_LENGTH( ac  ) ];
    va_list vlArgPtr;
	va_start( vlArgPtr, pcFormat );
	vsprintf( pc, pcFormat, vlArgPtr );
	va_end( vlArgPtr );
	(*OutputMessage)( pc, 0 );
}
///////////////////////////////////////////////////////////////////////////////
char * System::FormatString( const char * pcFormat, ... )
{
	static char ac[ 4 ][ 1024 ];
	static int  iBuffer = 0;

	char * pc = ac[ ++iBuffer %= VECTOR_LENGTH( ac  ) ];
    va_list vlArgPtr;
	va_start( vlArgPtr, pcFormat );
	vsprintf( pc, pcFormat, vlArgPtr );
	va_end( vlArgPtr );
	return pc;
}
///////////////////////////////////////////////////////////////////////////////
void * System::Alloc( void * pv, size_t iSize )
{
	if ( iSize ) return realloc( pv, iSize );
	else {
		free( pv );
		return 0;
	}
}
///////////////////////////////////////////////////////////////////////////////
int System::ParseLine( char * pcLine, char * apcArgValues[], int iMaxArgCount )
{
    if ( !pcLine ) return 0;
    
    int iArgCount = 0;
    
    for ( char cPrev = '\0', *pc = pcLine;
          *pc && iArgCount < iMaxArgCount; cPrev = *pc, pc++ ) {
        
        if ( isspace( *pc ) ) *pc = '\0';
        else if( !cPrev ) apcArgValues[ iArgCount ++ ] = pc;
    }
    
    return iArgCount;
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
double System::dfTimeOffset = System::TimeOS( ) - System::Time( );
///////////////////////////////////////////////////////////////////////////////
double System::TimeOS( void )
{
#ifdef WIN32
    struct _timeb timebufferBase, timebuffer;
    
    _ftime( &timebufferBase );

    do { // Catch the strobe in case of inadequate resolution of system clock 
      _ftime( &timebuffer );
    } while( timebuffer.time == timebufferBase.time &&
            timebuffer.millitm == timebufferBase.millitm );
   
    return timebuffer.time + 0.001 * timebuffer.millitm;
#else 

    struct timeval timeval;

#ifdef SGI
    gettimeofday( &timeval );
#else /* other Unixes eg: QNX & LINUX */
    gettimeofday( &timeval, 0 );
#endif
    
    return timeval.tv_sec + 0.000001 * timeval.tv_usec;
  
#endif  
}
///////////////////////////////////////////////////////////////////////////////
double System::Time( void )
{
#ifdef WIN32
    return dfTimeOffset + 0.001 * timeGetTime();
#else
    struct timeval timeval;

#ifdef SGI
    gettimeofday( &timeval );
#else /* other Unixes eg: QNX & LINUX */
    gettimeofday( &timeval, 0 );
#endif

    return dfTimeOffset + timeval.tv_sec + 0.000001 * timeval.tv_usec;
#endif
}
///////////////////////////////////////////////////////////////////////////////
void System::Time( double dfTime )
{
    dfTimeOffset = 0.0;
    dfTimeOffset = dfTime - Time( );
}
///////////////////////////////////////////////////////////////////////////////
/// Funkcja zasypia na czas okreslony w sekundach
///////////////////////////////////////////////////////////////////////////////
void System::Sleep( double dfTime )
{
    MilliSleep( long( dfTime * 1000 ) );

#ifdef WIN32
    ::Sleep( long( dfTime * 1000 ) );
#else
    struct timespec tspec;
    tspec.tv_sec = long( dfTime );
    tspec.tv_nsec = long( ( dfTime - tspec.tv_sec ) * 1000000000);
    nanosleep( &tspec, 0 );
#endif

}
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Funkcja zasypia na czas okreslony w milisekundach
///////////////////////////////////////////////////////////////////////////////
void System::MilliSleep( long lMilliseconds )
{
#ifdef WIN32
    ::Sleep( lMilliseconds );
#else
    struct timespec tspec;
    tspec.tv_sec = lMilliseconds / 1000;
    tspec.tv_nsec = ( lMilliseconds - tspec.tv_sec * 1000 ) * 1000000;
    nanosleep( &tspec, 0 );
#endif
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Zmienna okreslajaca podstawe czasu ktora odejmujemy od rzeczywistego pomiaru
///////////////////////////////////////////////////////////////////////////////
long System::lMilliTimeOffset = 0;
///////////////////////////////////////////////////////////////////////////////
/// Funkcja ustawia zegar ktory mierzy czas w milisekundach
///////////////////////////////////////////////////////////////////////////////
void System::MilliTime( long lMilliseconds )
{
    lMilliTimeOffset = 0;

    lMilliTimeOffset = lMilliseconds - MilliTime( );
}
///////////////////////////////////////////////////////////////////////////////
// Funkcja zwraca czas mierzony w milisekundach
///////////////////////////////////////////////////////////////////////////////
long System::MilliTime( void )
{
#ifdef WIN32
    return lMilliTimeOffset + timeGetTime();
#else
   struct timeval timeval;

#ifdef SGI
    gettimeofday( &timeval );
#else /* other Unixes eg: QNX & LINUX */
    gettimeofday( &timeval, 0 );
#endif

   return lMilliTimeOffset +
      timeval.tv_sec * 1000 + timeval.tv_usec / 1000;
#endif
}
///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
// adler32 - compute the Adler-32 checksum of a data stream
// Copyright (C) 1995-1996 Mark Adler
///////////////////////////////////////////////////////////////////////////////
unsigned long System::CheckSum
    ( const void * pv, unsigned int len, unsigned long adler )
{    
    const unsigned char * buf = (const unsigned char *) pv; 
    const unsigned long BASE = 65521L; /* largest prime smaller than 65536 */
    const int NMAX = 5552;
    /* NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */

    unsigned long s1 = adler & 0xffff;
    unsigned long s2 = (adler >> 16) & 0xffff;

    if ( buf == 0 ) return 1L;

    while (len > 0) {
        int k = len < NMAX ? len : NMAX;

        len -= k;
        while (k >= 16) {
            s2 += ( s1 += buf[0x0] ); s2 += ( s1 += buf[0x1] ); 
            s2 += ( s1 += buf[0x2] ); s2 += ( s1 += buf[0x3] );            
            s2 += ( s1 += buf[0x4] ); s2 += ( s1 += buf[0x5] ); 
            s2 += ( s1 += buf[0x6] ); s2 += ( s1 += buf[0x7] );
                        
            s2 += ( s1 += buf[0x8] ); s2 += ( s1 += buf[0x9] ); 
            s2 += ( s1 += buf[0xA] ); s2 += ( s1 += buf[0xB] );            
            s2 += ( s1 += buf[0xC] ); s2 += ( s1 += buf[0xD] ); 
            s2 += ( s1 += buf[0xE] ); s2 += ( s1 += buf[0xF] );            
            buf += 16;
            k -= 16;
        }
        if (k != 0) do {
            s1 += *buf++;
            s2 += s1;
        } while (--k);
        s1 %= BASE;
        s2 %= BASE;
    }

    return (s2 << 16) | s1;
}
///////////////////////////////////////////////////////////////////////////////
unsigned long System::TimeMarker( void )
{
    static double dfTimeOffset = 0;
    
    const long lSecsOfDay = /*7 * */24 * 60 * 60;
    const long lSecsOfDayOffset = 4 * 60 * 60; // czwarta rano
    
    static union { unsigned long ulTimeSignature; float fTimeSignature; } ut;
    
    if ( !dfTimeOffset ) {
      dfTimeOffset = lSecsOfDayOffset +
        double( lSecsOfDay ) * ( long( Time( ) - lSecsOfDay ) / lSecsOfDay );

      ut.ulTimeSignature = 0;
    }                         
        
    unsigned long ulTime = ut.ulTimeSignature;

    // uwaga trick: 
    // wykorzystuje zjawisko ze floaty potraktowane jako liczby binarne 
    // calkowite zachowuja porzadek dla wartosci dodatnich czyli ze 
    // f1 > f2 ( f1 > 0 i f2 > 0 ) => * (unsigned*) &f1  > * (unsigned*) &f2
                
//  ut.fTimeSignature = float( Time( ) - dfTimeOffset );

    ut.ulTimeSignature = long( 1000 * ( Time( ) - dfTimeOffset ) );
    
    if ( ulTime >= ut.ulTimeSignature )
        ut.ulTimeSignature = ulTime + 1;

    return ut.ulTimeSignature;        
}
///////////////////////////////////////////////////////////////////////////////
double System::TimeMarkerDifference
                ( unsigned long ulOldMarker, unsigned long ulNewMarker )
{
    union { float f; unsigned long ul; } ut;

    ut.ul = ulNewMarker;

    double dfRet = double( ut.f );

    ut.ul = ulOldMarker;

    dfRet -= double( ut.f );
   
    //return dfRet;

    return 0.001 * ( ulNewMarker - ulOldMarker );
}
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
System::FrequencyMeter::FrequencyMeter
    ( double dfTimeUnit, int iSampleTicksNo, double dfInitValue ) :
    iIndex( 0 ), iCount( 0 ), pdfMeasurements( 0 ),
    dfTimeUnit( 0.0 ), dfAvg( 0.0 ), iMin( 0 ), iMax( 0 )
{
    Setup( dfTimeUnit, iSampleTicksNo, dfInitValue );
}
///////////////////////////////////////////////////////////////////////////////
System::FrequencyMeter::~FrequencyMeter( void ) 
{
    Cleanup();
}
///////////////////////////////////////////////////////////////////////////////
bool System::FrequencyMeter::Setup
    ( double dfTimeUnit, int iSampleTicksNo, double dfInitValue )
{
    this->dfTimeUnit = dfTimeUnit;

    pdfMeasurements = new double [ iSampleTicksNo ];
    if ( !pdfMeasurements ) return false;

    iCount = iSampleTicksNo;
    for( int i = 0; i < iCount; i++ ) pdfMeasurements[i] = dfInitValue;



    return true;    
}
///////////////////////////////////////////////////////////////////////////////
bool System::FrequencyMeter::Cleanup( void ) 
{
    if ( !pdfMeasurements ) return false;

    delete [ ] pdfMeasurements;
    pdfMeasurements = 0;
    iCount = 0;

    return true;
}
///////////////////////////////////////////////////////////////////////////////
System::FrequencyMeter & System::FrequencyMeter::AbsoluteUpdateTick
    ( double dfAbsoluteValue )
{
    double & dfMeasurement = pdfMeasurements[ iIndex ];
    double & dfPrev = pdfMeasurements[ ( iIndex ? iIndex : iCount ) - 1 ];
    double dfIncrementalValue = dfAbsoluteValue - dfPrev;

    dfAvg = ( dfAbsoluteValue - dfMeasurement ) / iCount;
    dfMeasurement = dfAbsoluteValue;

    // Find new extremes if previous were just flushed out
    if( iMin == iIndex ) {
        for( int i = 0; i < iCount; i ++ )
            if( pdfMeasurements[ i ] < pdfMeasurements[ iMin ] ) iMin = i;
    }

    if( iMax == iIndex ) {
        for( int i = 0; i < iCount; i ++ )
            if( pdfMeasurements[ i ] > pdfMeasurements[ iMax ] ) iMax = i;
    }
    
    ++iIndex %= iCount;

    return *this;
}
///////////////////////////////////////////////////////////////////////////////
System::FrequencyMeter & System::FrequencyMeter::IncrementalUpdateTick
    ( double dfIncrementalValue )
{
    double & dfMeasurement = pdfMeasurements[ iIndex ];
    double & dfPrev = pdfMeasurements[ ( iIndex ? iIndex : iCount ) - 1 ];
    double dfAbsoluteValue = dfPrev + dfIncrementalValue;

    dfAvg = ( dfAbsoluteValue - dfMeasurement ) / iCount;
    dfMeasurement = dfAbsoluteValue;

    // Find new extremes if previous were just flushed out
    if( iMin == iIndex ) {
        for( int i = 0; i < iCount; i ++ )
            if( pdfMeasurements[ i ] < pdfMeasurements[ iMin ] ) iMin = i;
    }

    if( iMax == iIndex ) {
        for( int i = 0; i < iCount; i ++ )
            if( pdfMeasurements[ i ] > pdfMeasurements[ iMax ] ) iMax = i;
    }

    ++iIndex %= iCount;

    return *this;
}
///////////////////////////////////////////////////////////////////////////////

} // namespace JAM
