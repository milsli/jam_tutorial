#ifdef WIN32

#include <windows.h>
#include <tchar.h>
#include <DbgHelp.h>
#include <stdio.h>
#include <crtdbg.h>
#include <time.h>
#include <jam_application.h>
#include "MiniDump.h"
#include "StackWalk.h"

//#pragma comment ( lib, "dbghelp.lib" )
///////////////////////////////////////////////////////////////////////////////
// Copied from dbghelp.h prototype of MiniDumpWriteDump method
///////////////////////////////////////////////////////////////////////////////
typedef BOOL (WINAPI * MiniDumpWriteDumpPtrPrototype)
(
    IN HANDLE hProcess,
    IN DWORD ProcessId,
    IN HANDLE hFile,
    IN MINIDUMP_TYPE DumpType,
    IN CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, OPTIONAL
    IN CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, OPTIONAL
    IN CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam OPTIONAL
);

MiniDumpWriteDumpPtrPrototype MiniDumpWriteDumpPtr = NULL;
///////////////////////////////////////////////////////////////////////////////
class MiniDumpWriteDumpProxy 
{
public:
    MiniDumpWriteDumpProxy( void ) 
    { 
        const char * acPaths[] = { 
            "..\\..\\external\\bin\\dbghelp.dll",
            "dbghelp.dll",
        };

        for( int i = 0; i < sizeof( acPaths ) / sizeof( acPaths[0] ); i++ ) 
        {
            hmodule = LoadLibrary( acPaths[i] );
            if( hmodule ) {
                MiniDumpWriteDumpPtr = 
                    (MiniDumpWriteDumpPtrPrototype)GetProcAddress( hmodule, "MiniDumpWriteDump" );
                if( MiniDumpWriteDumpPtr != NULL ) break;
                FreeLibrary( hmodule );
            }
        }

    }

    ~MiniDumpWriteDumpProxy( void ) 
    { 
        if( hmodule ) 
            FreeLibrary( hmodule );
    }

protected:
    HMODULE hmodule;
};
///////////////////////////////////////////////////////////////////////////////
static MiniDumpWriteDumpProxy miniDumpWriteProxy;
///////////////////////////////////////////////////////////////////////////////
// Function declarations 
///////////////////////////////////////////////////////////////////////////////
static BOOL CALLBACK MiniDumpCallback(
	PVOID                            pParam, 
	const PMINIDUMP_CALLBACK_INPUT   pInput, 
	PMINIDUMP_CALLBACK_OUTPUT        pOutput 
); 

static bool IsDataSectionNeeded( const WCHAR* pModuleName ); 
///////////////////////////////////////////////////////////////////////////////
// Minidump creation function 
///////////////////////////////////////////////////////////////////////////////
bool MiniDumpAvailable(  )
{
    return MiniDumpWriteDumpPtr != NULL;
}
///////////////////////////////////////////////////////////////////////////////
LONG WINAPI  CreateMiniDump( EXCEPTION_POINTERS* pep ) 
{
    if (!MiniDumpAvailable() ) return EXCEPTION_EXECUTE_HANDLER;
    TCHAR output[ 1024 ] = "";

    TCHAR FullPath[MAX_PATH];
    GetModuleFileName(NULL, FullPath, sizeof( FullPath ));
    _stprintf( output + strlen( output ), _T( "ERROR: Application: %s crashed \n" ), FullPath );

    TCHAR Path[MAX_PATH] = _T("JamApp.dmp");

    {
        TCHAR File[MAX_PATH];
        _splitpath( FullPath, NULL, NULL, File, NULL );

        _tzset();
        time_t ltime;
        time( &ltime );
        struct tm time;

        _localtime64_s( &time, &ltime );

        _stprintf( Path,
                  _T( "%s_crash_%d-%d-%d_%d-%d-%d.dmp" ), 
                  File, 
                  time.tm_year + 1900,
                  time.tm_mon + 1,
                  time.tm_mday,
                  time.tm_hour,
                  time.tm_min,
                  time.tm_sec );
    }        

    HANDLE  hFile = CreateFile( Path, GENERIC_READ | GENERIC_WRITE, 0,
                               NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);  
	
	if( ( hFile != NULL ) && ( hFile != INVALID_HANDLE_VALUE ) )
	{
		// Create the minidump 
		MINIDUMP_EXCEPTION_INFORMATION mdei; 

		mdei.ThreadId           = GetCurrentThreadId(); 
		mdei.ExceptionPointers  = pep; 
		mdei.ClientPointers     = FALSE; 

		MINIDUMP_CALLBACK_INFORMATION mci; 

		mci.CallbackRoutine     = (MINIDUMP_CALLBACK_ROUTINE)MiniDumpCallback;
		mci.CallbackParam       = 0;
#if 1
		MINIDUMP_TYPE mdt       = MiniDumpNormal;
#else 
		MINIDUMP_TYPE mdt       = (MINIDUMP_TYPE)(MiniDumpWithPrivateReadWriteMemory |
		                                          MiniDumpWithDataSegs |
		                                          MiniDumpWithHandleData |
		                                          MiniDumpWithFullMemoryInfo |
		                                          MiniDumpWithThreadInfo |
		                                          MiniDumpWithUnloadedModules );
#endif
        
		BOOL rv = ~0;
        for( unsigned int attempts = 100; attempts && rv; attempts-- )
        {
            rv = (*MiniDumpWriteDumpPtr)( GetCurrentProcess(), GetCurrentProcessId(),
			    hFile, mdt, (pep != 0) ? &mdei : 0, 0, &mci );

            // occasionaly due to some magic timing issues MiniDump may fail
            // someone on the web suggested that its temporal and to retry when it happens
            // so we hold on a sec, and hope maybe next attempt will be more succesful

            if( !rv )
                Sleep( 100 ); 
        }

		if( !rv )
			_stprintf( output + strlen( output ), _T("MiniDump WriteDump failed. Error: %s (0x%x)\n"), 
                                  JAM::Application::ModuleImageError(),
                                  GetLastError() ); 
		else 
            _stprintf( output + strlen( output ), _T("Minidump created in %s \n"), Path );

		// Close the file 
		CloseHandle( hFile ); 
	}
	else 
	{
        _stprintf( output + strlen( output ), _T("Minidump CreateFile failed. Error: %s (0x%x)\n"), 
            JAM::Application::ModuleImageError( ),
            GetLastError() );
	}
    
    JAM::Application::DefaultOutput( output, strlen( output ) );
    OutputDebugString( output );

    CreateStackWalk( pep );

    MessageBox( NULL, output, "App Crashed", MB_OK );

    return EXCEPTION_EXECUTE_HANDLER;
}
///////////////////////////////////////////////////////////////////////////////
// Custom minidump callback 
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Custom minidump callback 
//

BOOL CALLBACK MiniDumpCallback(
	PVOID                            pParam, 
	const PMINIDUMP_CALLBACK_INPUT   pInput, 
	PMINIDUMP_CALLBACK_OUTPUT        pOutput 
) 
{
	BOOL bRet = FALSE; 


	// Check parameters 

	if( pInput == 0 ) 
		return FALSE; 

	if( pOutput == 0 ) 
		return FALSE; 


	// Process the callbacks 

	switch( pInput->CallbackType ) 
	{
		case IncludeModuleCallback: 
		{
			// Include the module into the dump 
			bRet = TRUE; 
		}
		break; 

		case IncludeThreadCallback: 
		{
			// Include the thread into the dump 
			bRet = TRUE; 
		}
		break; 

		case ModuleCallback: 
		{
			// Are data sections available for this module ? 

			if( pOutput->ModuleWriteFlags & ModuleWriteDataSeg ) 
			{
				// Yes, they are, but do we need them? 

				if( !IsDataSectionNeeded( pInput->Module.FullPath ) ) 
				{
					wprintf( L"Excluding module data sections: %s \n", pInput->Module.FullPath ); 

					pOutput->ModuleWriteFlags &= (~ModuleWriteDataSeg); 
				}
			}

			bRet = TRUE; 
		}
		break; 

		case ThreadCallback: 
		{
			// Include all thread information into the minidump 
			bRet = TRUE;  
		}
		break; 

		case ThreadExCallback: 
		{
			// Include this information 
			bRet = TRUE;  
		}
		break; 

		case MemoryCallback: 
		{
			// We do not include any information here -> return FALSE 
			bRet = FALSE; 
		}
		break; 

		case CancelCallback: 
			break; 
	}

	return bRet; 

}


///////////////////////////////////////////////////////////////////////////////
// This function determines whether we need data sections of the given module 
//

bool IsDataSectionNeeded( const WCHAR* pModuleName ) 
{
#if 1
	// Check parameters 
    return true;
#else 
	if( pModuleName == 0 ) 
	{
		_ASSERTE( _T("Parameter is null.") ); 
		return false; 
	}


	// Extract the module name 

	WCHAR szFileName[_MAX_FNAME] = L""; 

	_wsplitpath( pModuleName, NULL, NULL, szFileName, NULL ); 


	// Compare the name with the list of known names and decide 

	// Note: For this to work, the executable name must be "mididump.exe"
	if( wcsicmp( szFileName, L"mididump" ) == 0 ) 
	{
		return true; 
	}
	else if( wcsicmp( szFileName, L"ntdll" ) == 0 ) 
	{
		return true; 
	}


	// Complete 

	return false; 
#endif
}


#endif