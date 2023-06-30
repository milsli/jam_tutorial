#ifndef JAM_SYSTEM_H
#define JAM_SYSTEM_H

////////////////////////////////////////////////////////////////////////////////
// Operating System & Environment function abstraction layer class
// System dependent utility functions for the JAM derived objects
// All functions of this class are static 
////////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
#include<windows.h>
#else // UNIX systems

#endif

#include<iostream>
////////////////////////////////////////////////////////////////////////////////
#ifndef VECTOR_LENGTH
#define VECTOR_LENGTH( v ) ( sizeof( (v) ) / sizeof( (v)[0] ) )
#endif

#ifndef MIN
#define MIN( a, b ) ( (a) < (b) ? (a) : (b) )
#endif

#ifndef MAX
#define MAX( a, b ) ( (a) > (b) ? (a) : (b) )
#endif

#ifndef BOUND
#define BOUND( x, a, b ) ( (x) < (a) ? (a) : ( (x) > (b) ? (b) : (x) ) )
#endif
////////////////////////////////////////////////////////////////////////////////
// Special export keywords are needed in Windows
////////////////////////////////////////////////////////////////////////////////
#ifndef WIN32
#define JAM_IMPORT
#define JAM_EXPORT
#define JAM_ENTRY extern "C"
#else
#define JAM_IMPORT __declspec(dllimport)
#define JAM_EXPORT __declspec(dllexport)
#define JAM_ENTRY extern "C" __declspec(dllexport)
#endif

#ifdef JAM_EXPORTS
#define JAM_API JAM_EXPORT
#else 
#define JAM_API JAM_IMPORT
#endif

////////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
#define JAM_MODULE_IMAGE_PREFIX ""
#if defined( _DEBUG ) || defined( DEBUG )
#define JAM_MODULE_IMAGE_SUFFIX "_d.DLL"
#else
#define JAM_MODULE_IMAGE_SUFFIX ".DLL"
#endif
#else 
#define JAM_MODULE_IMAGE_PREFIX ""
#if defined( _DEBUG ) || defined( DEBUG )
#define JAM_MODULE_IMAGE_SUFFIX "_d.so"
#else
#define JAM_MODULE_IMAGE_SUFFIX ".so"
#endif
#endif
////////////////////////////////////////////////////////////////////////////////
namespace JAM {

class JAM_API System
{
public:
	////////////////////////////////////////////////////////////////////////////
	// Dynamic loading routines & auxilliary types
	////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
	typedef HINSTANCE ModuleImage;    
#else 
    typedef void * ModuleImage;    
#endif

    typedef void (*PtrModuleImageFunction)(void);

    static ModuleImage  CreateModuleImage( const char * pcModule );
    static void         DeleteModuleImage( ModuleImage mi );
    static PtrModuleImageFunction
		                GetModuleImageFunction( ModuleImage mi, const char * pcSym );
    static const char * ModuleImageError( void );

	////////////////////////////////////////////////////////////////////////////
	// Function does what realloc does plus frees pointer when iSize = 0 
	////////////////////////////////////////////////////////////////////////////
	static void *		Alloc( void * pv, size_t iSize );

	////////////////////////////////////////////////////////////////////////////
    static void MilliSleep  ( long lMilliseconds );
    static void MilliTime   ( long lMilliseconds ); // Sets time to Milliseconds
    static long MilliTime   ( void );               // Gets time in Milliseconds
    
    static long lMilliTimeOffset;
	////////////////////////////////////////////////////////////////////////////
    static double Time      ( void );
    static void   Time      ( double dfTime );
    static void   Sleep     ( double dfTime );

    static double dfTimeOffset;
    static double TimeOS( void );
    ////////////////////////////////////////////////////////////////////////////
    // Utilities
	////////////////////////////////////////////////////////////////////////////
    static unsigned long TimeMarker( void );
    static double        TimeMarkerDifference( unsigned long ulOldMarker, 
                                               unsigned long ulNewMarker );
    ////////////////////////////////////////////////////////////////////////////
    unsigned long CheckSum
        ( const void * pv, unsigned int len, unsigned long adler );
	////////////////////////////////////////////////////////////////////////////
	// Function takes line and replaces all white chars with zeroes (ends)
    // Then stores pointers to just separated strings in apcArgValues vector
    // Returns number of strings found in command line
	////////////////////////////////////////////////////////////////////////////
   	static int ParseLine
            ( char * pcLine, char * apcArgValues[], int iMaxArgCount );

	static char * FormatString( const char * pcFormat, ... );
	static void   FormatOutput( const char * pcFormat, ... );

	static void (*OutputMessage)( const char * pcOut, int iLength );
	static void DefaultOutput( const char * pcOut, int iLength );
	////////////////////////////////////////////////////////////////////////////
    class JAM_API FrequencyMeter 
    {
    public:
        FrequencyMeter( double dfTimeUnit = 1.0, 
                    int iSampleTicksNo = 100, double dfInitValue = 0.0 );

        ~FrequencyMeter( void );

        bool Setup( double dfTimeUnit, 
                int iSampleTicksNo = 100, double dfInitValue = 0.0 );
        bool Cleanup( void );

        FrequencyMeter & IncrementalUpdateTick( double dfIncrementalValue );
	    FrequencyMeter & AbsoluteUpdateTick( double dfAbsoluteValue );

        double AvgPeriod( void )
            { return dfAvg; }

        double MinPeriod( void )
            { return pdfMeasurements[ iMin ]; }

        double MaxPeriod( void )
            { return pdfMeasurements[ iMax ]; }

        double AvgFrequency( void )
            { return dfAvg > 0 ? dfTimeUnit / dfAvg : -dfTimeUnit; }

        double MinFrequency( void )
            { return pdfMeasurements[ iMax ] > 0 ? 
                 dfTimeUnit / pdfMeasurements[ iMax ] : -dfTimeUnit; }

        double MaxFrequency( void )
            { return pdfMeasurements[ iMin ] > 0 ? 
                 dfTimeUnit / pdfMeasurements[ iMin ] : -dfTimeUnit; }

    protected:
	    double * pdfMeasurements, dfAvg, dfTimeUnit;
	    int     iIndex, iCount, iMin, iMax;
    };
    ////////////////////////////////////////////////////////////////////////////
};

} // namespace JAM

////////////////////////////////////////////////////////////////////////////////
#endif
