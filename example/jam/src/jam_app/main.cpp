////////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
// This is required for using win console functions
#define _WIN32_WINNT 0x0501

#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include"MiniDump.h"
#include"StackWalk.h"

#else// WIN32 

#include<fstream>
#include<cstdio>

#endif // WIN32

#include <jam_application.h>
///////////////////////////////////////////////////////////////////////////////
using namespace std;
static bool AddConsole( void );
static bool TestConsole( void );
static bool RedirectConsole( const char * file = "jamapp_log.txt" );
///////////////////////////////////////////////////////////////////////////////
int main( int iArgCount, char * apcArgValues[] )
{
#if defined(WIN32) && !defined( _DEBUG ) && !defined( DEBUG )
    RedirectConsole();
    //TestConsole(); 
#endif

    return JAM::Application().Main( iArgCount, apcArgValues );
}
////////////////////////////////////////////////////////////////////////////////
#if defined( WIN32 ) && !defined( JAM_NO_WINMAIN ) 
////////////////////////////////////////////////////////////////////////////////


int WINAPI WinMain
( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int iShowCmd )
{
	lpCmdLine = GetCommandLine();

    char * apcArgValues[ 64 ];

#if defined( _DEBUG ) || defined( DEBUG )
    bool UseMiniDump = false;
#else
    bool UseMiniDump = true;
#endif

    int iArgCount = JAM::Application::ParseLine( lpCmdLine,
        apcArgValues, sizeof( apcArgValues ) / sizeof( apcArgValues[0] ) );


#if defined( _DEBUG ) || defined( DEBUG )
	AddConsole();
    //TestConsole();
#endif   

    int iRet = 0;

#ifdef WIN32
    for( int i = 1; i < iArgCount; i++ )
    {
        if( !_stricmp( apcArgValues[i], "-NoDump" ) )
            UseMiniDump = false;

        if( !_stricmp( apcArgValues[i], "-Dump" ) )
            UseMiniDump = true;
    }

    char * pc = NULL;

    if( UseMiniDump && MiniDumpAvailable() ) {
#if 1
        SetUnhandledExceptionFilter( CreateMiniDump );

        iRet = main( iArgCount, apcArgValues );
#else
        __try
        {    
            iRet = main( iArgCount, apcArgValues );
        }
	    __except( CreateStackWalk( GetExceptionInformation() ) )
	    {
	    }
#endif
    } else 
#endif
    {   
        iRet = main( iArgCount, apcArgValues ); 
    }
        


#if defined( WIN32 )
    // Workaround for Debuger Memory Dump at the end of Mondo session in VS 8.0
    TerminateProcess( GetCurrentProcess(), iRet );
#endif

    return iRet;
}
#endif // defined( WIN32 ) && !defined( JAM_NO_WINMAIN ) 
///////////////////////////////////////////////////////////////////////////////
#if !defined( WIN32 )
bool AddConsole( void )
{

}

bool TestConsole( void )
{

}

bool RedirectConsole( const char * file )
{
	// Release non windows version 
#if 0    
   FILE *pf = fopen( "jamapp_log.txt", "wt" );
   FILEBUF fb( pf );
   cout.rdbuf( fb );
   cerr.rdbuf( fb );
#endif    

    freopen("jamapp_log.txt", "r", stdin);
    freopen("jamapp_log.txt", "w", stdout);
    freopen("jamapp_log.txt", "w", stderr);

#if 0    
    std::ofstream filestr;
    filestr.open("jamapp_log.txt");
    std::streambuf* filestrbuf = filestr.rdbuf();
    cout.rdbuf(filestrbuf);
    cerr.rdbuf(filestrbuf);
#endif    
}
#else 
bool AddConsole( void )
{
  int hConHandle;
  long lStdHandle;
  FILE *fp;

  // Try to attach to a console
  if ( !AttachConsole (ATTACH_PARENT_PROCESS) ) {
    if( !AllocConsole() )
        return false;

    SetConsoleTitle( "JAM Application Console" );
    COORD coord = { 80, 5000 };
    SetConsoleScreenBufferSize( GetStdHandle( STD_OUTPUT_HANDLE ), coord );
  }

  //HANDLE hnd = (HANDLE)_get_osfhandle( _fileno(pf) );

  // redirect unbuffered STDOUT to the console
  lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
  hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
  fp = _fdopen( hConHandle, "w" );
  *stdout = *fp;
  setvbuf( stdout, NULL, _IONBF, 0 );

  // redirect unbuffered STDIN to the console
  lStdHandle = (long)GetStdHandle(STD_INPUT_HANDLE);
  hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
  fp = _fdopen( hConHandle, "r" );
  *stdin = *fp;
  setvbuf( stdin, NULL, _IONBF, 0 );

  // redirect unbuffered STDERR to the console
  lStdHandle = (long)GetStdHandle(STD_ERROR_HANDLE);
  hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
  fp = _fdopen( hConHandle, "w" );
  *stderr = *fp;
  setvbuf( stderr, NULL, _IONBF, 0 );

  return true;
}
///////////////////////////////////////////////////////////////////////////////
bool TestConsole( void )
{
    HANDLE hout = GetStdHandle( STD_OUTPUT_HANDLE );
    HANDLE herr = GetStdHandle( STD_ERROR_HANDLE );

    {
        std::cout<< "NOTIFY: Test Output : cout \n" << std::flush;
    }
    {
        std::cerr<< "NOTIFY: Test Output : cerr \n" << std::flush;
    }
    {
        fprintf( stdout, "NOTIFY: Test Output : fprintf stdout \n" );
        fflush( stdout );
    }
    {
        fprintf( stderr, "NOTIFY: Test Output : fprintf stderr \n" );
        fflush( stderr );
    }
    {
        DWORD dw = 0;
        char ac[] = "NOTIFY: Test Output : WriteFile STD_OUTPUT_HANDLE \n";        
        WriteFile( hout , ac, sizeof( ac ), &dw, NULL );
    }
    {
        DWORD dw = 0;
        char ac[] = "NOTIFY: Test Output : WriteConsole STD_OUTPUT_HANDLE \n";
        WriteConsole( hout , ac, sizeof( ac ), &dw, NULL );
    }
    {
        DWORD dw = 0;
        char ac[] = "NOTIFY: Test Output : WriteFile STD_ERROR_HANDLE \n";
        WriteFile( herr , ac, sizeof( ac ), &dw, NULL );
    }
    {
        DWORD dw = 0;
        char ac[] = "NOTIFY: Test Output : WriteConsole STD_ERROR_HANDLE \n";
        WriteConsole( herr , ac, sizeof( ac ), &dw, NULL );
    }

    return true;
}
///////////////////////////////////////////////////////////////////////////////
bool RedirectConsole( const char * filename )
{
	// Release non windows version 
    FILE *pf = fopen( filename, "wt" );
    *stdout = *pf;
    setvbuf( stdout, NULL, _IONBF, 0 );
    HANDLE hnd = (HANDLE)_get_osfhandle( _fileno(stdout) );
    SetStdHandle( STD_OUTPUT_HANDLE, hnd );

#if 1
   	SetStdHandle( STD_ERROR_HANDLE, hnd );
    *stderr = *pf;
    setvbuf( stderr, NULL, _IONBF, 0 );
#endif

    return true;

/*
  //

    freopen( "jamapp_log.txt", "w", stdout );
    freopen( "jamapp_log.txt", "w", stderr );

    std::ofstream filestr;
    filestr.open("jamapp_log.txt");
    std::streambuf* filestrbuf = filestr.rdbuf();
    cout.rdbuf(filestrbuf);
    cerr.rdbuf(filestrbuf);

*/


/*
    std::ofstream filestr;
    filestr.open("jamapp_log.txt");
    std::streambuf* filestrbuf = filestr.rdbuf();
    cout.rdbuf(filestrbuf);
    cerr.rdbuf(filestrbuf);
*/
}
///////////////////////////////////////////////////////////////////////////////
#endif
