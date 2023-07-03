#ifdef WIN32

#include <windows.h>
#include <string>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <crtdbg.h>
#include <time.h>
#include <jam_application.h>
#include "StackWalk.h"

#define gle (GetLastError())
#define lenof(a) (sizeof(a) / sizeof((a)[0]))
#define MAXNAMELEN 1024 // max name length for found symbols
#define IMGSYMLEN ( sizeof IMAGEHLP_SYMBOL )
#define TTBUFLEN 65536 // for a temp buffer


// imagehlp.h must be compiled with packing to eight-byte-boundaries,
// but does nothing to enforce that. I am grateful to Jeff Shanholtz
// <JShanholtz@premia.com> for finding this problem.
#pragma pack( push, before_imagehlp, 8 )
#include <imagehlp.h>
#pragma pack( pop, before_imagehlp )
///////////////////////////////////////////////////////////////////////////////
// SymCleanup()
typedef BOOL (__stdcall *tSC)( IN HANDLE hProcess );
static tSC pSC = NULL;
///////////////////////////////////////////////////////////////////////////////
// SymFunctionTableAccess()
#ifdef _WIN64
typedef PVOID (__stdcall *tSFTA)(HANDLE hProcess, DWORD64 AddrBase);
#else
typedef PVOID (__stdcall *tSFTA)( HANDLE hProcess, DWORD AddrBase );
#endif
static tSFTA pSFTA = NULL;
///////////////////////////////////////////////////////////////////////////////
// SymGetLineFromAddr()
typedef BOOL (__stdcall *tSGLFA)( IN HANDLE hProcess, IN DWORD dwAddr,
    OUT PDWORD pdwDisplacement, OUT PIMAGEHLP_LINE Line );
static tSGLFA pSGLFA = NULL;
///////////////////////////////////////////////////////////////////////////////
// SymGetModuleBase()
#ifdef _WIN64
typedef DWORD64 (__stdcall *tSGMB)(IN HANDLE hProcess, IN DWORD64 dwAddr);
#else
typedef DWORD (__stdcall *tSGMB)( IN HANDLE hProcess, IN DWORD dwAddr );
#endif
static tSGMB pSGMB = NULL;
///////////////////////////////////////////////////////////////////////////////
// SymGetModuleInfo()
typedef BOOL (__stdcall *tSGMI)( IN HANDLE hProcess, IN DWORD dwAddr, OUT PIMAGEHLP_MODULE ModuleInfo );
static tSGMI pSGMI = NULL;
///////////////////////////////////////////////////////////////////////////////
// SymGetOptions()
typedef DWORD (__stdcall *tSGO)( VOID );
static tSGO pSGO = NULL;
///////////////////////////////////////////////////////////////////////////////
// SymGetSymFromAddr()
typedef BOOL (__stdcall *tSGSFA)( IN HANDLE hProcess, IN DWORD dwAddr,
    OUT PDWORD pdwDisplacement, OUT PIMAGEHLP_SYMBOL Symbol );
static tSGSFA pSGSFA = NULL;
///////////////////////////////////////////////////////////////////////////////
// SymInitialize()
typedef BOOL (__stdcall *tSI)( IN HANDLE hProcess, IN PSTR UserSearchPath, IN BOOL fInvadeProcess );
static tSI pSI = NULL;
///////////////////////////////////////////////////////////////////////////////
// SymLoadModule()
typedef DWORD (__stdcall *tSLM)( IN HANDLE hProcess, IN HANDLE hFile,
    IN PSTR ImageName, IN PSTR ModuleName, IN DWORD BaseOfDll, IN DWORD SizeOfDll );
static tSLM pSLM = NULL;
///////////////////////////////////////////////////////////////////////////////
// SymSetOptions()
typedef DWORD (__stdcall *tSSO)( IN DWORD SymOptions );
static tSSO pSSO = NULL;
///////////////////////////////////////////////////////////////////////////////
// StackWalk()
typedef BOOL (__stdcall *tSW)( DWORD MachineType, HANDLE hProcess,
    HANDLE hThread, LPSTACKFRAME StackFrame, PVOID ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
    PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine,
    PTRANSLATE_ADDRESS_ROUTINE TranslateAddress );
static tSW pSW = NULL;
///////////////////////////////////////////////////////////////////////////////
// UnDecorateSymbolName()
typedef DWORD (__stdcall WINAPI *tUDSN)( PCSTR DecoratedName, PSTR UnDecoratedName,
    DWORD UndecoratedLength, DWORD Flags );
static tUDSN pUDSN = NULL;
///////////////////////////////////////////////////////////////////////////////
struct ModuleEntry
{
    std::string imageName;
    std::string moduleName;
    DWORD baseAddress;
    DWORD size;
};
///////////////////////////////////////////////////////////////////////////////
typedef std::vector< ModuleEntry > ModuleList;
typedef ModuleList::iterator ModuleListIter;
///////////////////////////////////////////////////////////////////////////////
static void ShowStack( HANDLE hThread, CONTEXT& c ); // dump a stack
static bool fillThreadList( std::vector<DWORD> & threadIds, DWORD pid );
static void enumAndLoadModuleSymbols( HANDLE hProcess, DWORD pid, bool silent );
static bool fillModuleList( ModuleList& modules, DWORD pid, HANDLE hProcess, bool silent );
static bool fillModuleListTH32( ModuleList& modules, DWORD pid, bool silent );
static bool fillModuleListPSAPI( ModuleList& modules, DWORD pid, HANDLE hProcess, bool silent );

///////////////////////////////////////////////////////////////////////////////
class StackWalkWriteDumpProxy 
{
public:
    StackWalkWriteDumpProxy( void ) 
    { 
        const char * acPaths[] = { 
            "..\\..\\external\\bin\\imagehlp.dll",
            "imagehlp.dll",
        };

        for( int i = 0; i < sizeof( acPaths ) / sizeof( acPaths[0] ); i++ ) 
        {
            // we load imagehlp.dll dynamically because the NT4-version does not
            // offer all the functions that are in the NT5 lib
            hmodule = LoadLibrary( acPaths[i] );
            if ( hmodule == NULL ) continue;

            pSC = (tSC) GetProcAddress( hmodule, "SymCleanup" );
            pSFTA = (tSFTA) GetProcAddress( hmodule, "SymFunctionTableAccess" );
            pSGLFA = (tSGLFA) GetProcAddress( hmodule, "SymGetLineFromAddr" );
            pSGMB = (tSGMB) GetProcAddress( hmodule, "SymGetModuleBase" );
            pSGMI = (tSGMI) GetProcAddress( hmodule, "SymGetModuleInfo" );
            pSGO = (tSGO) GetProcAddress( hmodule, "SymGetOptions" );
            pSGSFA = (tSGSFA) GetProcAddress( hmodule, "SymGetSymFromAddr" );
            pSI = (tSI) GetProcAddress( hmodule, "SymInitialize" );
            pSSO = (tSSO) GetProcAddress( hmodule, "SymSetOptions" );
            pSW = (tSW) GetProcAddress( hmodule, "StackWalk" );
            pUDSN = (tUDSN) GetProcAddress( hmodule, "UnDecorateSymbolName" );
            pSLM = (tSLM) GetProcAddress( hmodule, "SymLoadModule" );

            if ( pSC == NULL || pSFTA == NULL || pSGMB == NULL || pSGMI == NULL ||
                pSGO == NULL || pSGSFA == NULL || pSI == NULL || pSSO == NULL ||
                pSW == NULL || pUDSN == NULL || pSLM == NULL )
            {
                pSC = NULL; 
                pSFTA = NULL;
                pSGMB = NULL;
                pSGMI = NULL;
                pSGO = NULL;
                pSGSFA = NULL;
                pSI = NULL; 
                pSSO = NULL;
                pSW = NULL;
                pUDSN = NULL;
                pSLM = NULL;
                FreeLibrary( hmodule );
                hmodule = NULL;
            }
        }

    }

    ~StackWalkWriteDumpProxy( void ) 
    { 
        if( hmodule ) {
            FreeLibrary( hmodule );
            hmodule = NULL;
        }
    }

protected:
    HMODULE hmodule;
};
///////////////////////////////////////////////////////////////////////////////
static StackWalkWriteDumpProxy stackWalkWriteProxy;
///////////////////////////////////////////////////////////////////////////////
// StackWalk creation function 
///////////////////////////////////////////////////////////////////////////////
bool StackWalkAvailable(  )
{
    return pSW != NULL;
}
///////////////////////////////////////////////////////////////////////////////
LONG WINAPI  CreateStackWalk( EXCEPTION_POINTERS* pep )
{
    if (!StackWalkAvailable() ) return EXCEPTION_EXECUTE_HANDLER;

    TCHAR output[ 1024 ] = "";
    TCHAR FullPath[MAX_PATH];
    GetModuleFileName(NULL, FullPath, sizeof( FullPath ));
    _stprintf( output + strlen( output ), _T( "\nERROR: Application: %s crashed \n" ), FullPath );

    JAM::Application::DefaultOutput( output, strlen( output ) );
    OutputDebugString( output );

    output[0] = '\0';
    
    TCHAR Path[MAX_PATH] = _T("");

    {
        TCHAR File[MAX_PATH];
        _splitpath( FullPath, NULL, NULL, File, NULL );

        _tzset();
        time_t ltime;
        time( &ltime );
        struct tm time;

        _localtime64_s( &time, &ltime );

        _stprintf( output,
                 _T( "%s crash %d-%d-%d %d:%d:%d stack trace: \n" ), 
                  File, 
                  time.tm_year + 1900,
                  time.tm_mon + 1,
                  time.tm_mday,
                  time.tm_hour,
                  time.tm_min,
                  time.tm_sec );
    }

    JAM::Application::DefaultOutput( output, strlen( output ) );

    char *tt = NULL;
    {
        HANDLE hProcess = GetCurrentProcess(); // hProcess normally comes from outside
        std::string symSearchPath;
        char *p;

        // NOTE: normally, the exe directory and the current directory should be taken
        // from the target process. The current dir would be gotten through injection
        // of a remote thread; the exe fir through either ToolHelp32 or PSAPI.

        tt = new char[TTBUFLEN]; // this is a _sample_. you can do the error checking yourself.

        // build symbol search path from:
        symSearchPath = "";
        // current directory
        if ( GetCurrentDirectory( TTBUFLEN, tt ) )
            symSearchPath += tt + std::string( ";" );
        // dir with executable
        if ( GetModuleFileName( 0, tt, TTBUFLEN ) )
        {
            for ( p = tt + strlen( tt ) - 1; p >= tt; -- p )
            {
                // locate the rightmost path separator
                if ( *p == '\\' || *p == '/' || *p == ':' )
                    break;
            }
            // if we found one, p is pointing at it; if not, tt only contains
            // an exe name (no path), and p points before its first byte
            if ( p != tt ) // path sep found?
            {
                if ( *p == ':' ) // we leave colons in place
                    ++ p;
                *p = '\0'; // eliminate the exe name and last path sep
                symSearchPath += tt + std::string( ";" );
            }
        }
        // environment variable _NT_SYMBOL_PATH
        if ( GetEnvironmentVariable( "_NT_SYMBOL_PATH", tt, TTBUFLEN ) )
            symSearchPath += tt + std::string( ";" );
        // environment variable _NT_ALTERNATE_SYMBOL_PATH
        if ( GetEnvironmentVariable( "_NT_ALTERNATE_SYMBOL_PATH", tt, TTBUFLEN ) )
            symSearchPath += tt + std::string( ";" );
        // environment variable SYSTEMROOT
        if ( GetEnvironmentVariable( "SYSTEMROOT", tt, TTBUFLEN ) )
            symSearchPath += tt + std::string( ";" );

        if ( symSearchPath.size() > 0 ) // if we added anything, we have a trailing semicolon
            symSearchPath = symSearchPath.substr( 0, symSearchPath.size() - 1 );

        printf( "symbols path: %s\n", symSearchPath.c_str() );

        // why oh why does SymInitialize() want a writeable string?
        strncpy( tt, symSearchPath.c_str(), TTBUFLEN );
        tt[TTBUFLEN - 1] = '\0'; // if strncpy() overruns, it doesn't add the null terminator

        // init symbol handler stuff (SymInitialize())
        if ( ! pSI( hProcess, tt, false ) )
        {
            printf( "SymInitialize(): Failure. Error: %lu\n", gle );
            delete [] tt;
            tt = NULL;
            return EXCEPTION_EXECUTE_HANDLER;
        }

        // SymGetOptions()
        DWORD symOptions; // symbol handler settings
        symOptions = pSGO();
        symOptions |= SYMOPT_LOAD_LINES;
        symOptions &= ~SYMOPT_UNDNAME;
        pSSO( symOptions ); // SymSetOptions()

        // Enumerate modules and tell imagehlp.dll about them.
        // On NT, this is not necessary, but it won't hurt.
        enumAndLoadModuleSymbols( GetCurrentProcess(), GetCurrentProcessId(), false );
    }

#if 1
    std::vector<DWORD> threadIds;
    fillThreadList( threadIds, GetCurrentProcessId() );
    for( unsigned int n = 0; n < threadIds.size(); n++ ) 
    {

        if( threadIds[n] == GetCurrentThreadId() )
            printf( "\n####### Exception Thread ####### \n" );

        printf( "\n\nThread No: %d ThreadId: %x \n\n", n, threadIds[n] ); 

        if( threadIds[n] == GetCurrentThreadId() ) {
            HANDLE hThread = NULL;
            // Is it neccessary ?
            DuplicateHandle( GetCurrentProcess(), GetCurrentThread(),
                             GetCurrentProcess(), &hThread, 
                             0, false, DUPLICATE_SAME_ACCESS );

            ShowStack( hThread, *(pep->ContextRecord) );
            CloseHandle( hThread );
        } else {
            HANDLE hThread = OpenThread( THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT, FALSE, threadIds[n] );
            if ( (DWORD) (-1L) != SuspendThread( hThread ) ) {
                CONTEXT c;
                memset( &c, '\0', sizeof c );
                c.ContextFlags = CONTEXT_FULL;

                // init CONTEXT record so we know where to start the stackwalk
                if ( GetThreadContext( hThread, &c ) ) {
                    ShowStack( hThread, c );
                } else {
                    printf( "GetThreadContext(): Failure. Error: %lu\n", gle );
                }
            } 
            else 
            {
                printf( "SuspendThread(): Failure. Error: %lu\n", gle );
            }   
            CloseHandle( hThread );
        }
     
    }
#endif
    
//    JAM::Application::DefaultOutput( output, strlen( output ) );

//    OutputDebugString( output );

    // de-init symbol handler etc. (SymCleanup())
    pSC( GetCurrentProcess );

    return EXCEPTION_EXECUTE_HANDLER;
}

void ShowStack( HANDLE hThread, CONTEXT& c )
{
    // normally, call ImageNtHeader() and use machine info from PE header
    DWORD imageType = IMAGE_FILE_MACHINE_I386;
    HANDLE hProcess = GetCurrentProcess(); // hProcess normally comes from outside
    int frameNum; // counts walked frames
    DWORD offsetFromSymbol; // tells us how far from the symbol we were
    IMAGEHLP_SYMBOL *pSym = (IMAGEHLP_SYMBOL *) malloc( IMGSYMLEN + MAXNAMELEN );
    char undName[MAXNAMELEN]; // undecorated name
    char undFullName[MAXNAMELEN]; // undecorated name with all shenanigans
    IMAGEHLP_MODULE Module;
    IMAGEHLP_LINE Line;
    
    STACKFRAME s; // in/out stackframe
    memset( &s, '\0', sizeof s );

    // init STACKFRAME for first call
    // Notes: AddrModeFlat is just an assumption. I hate VDM debugging.
    // Notes: will have to be #ifdef-ed for Alphas; MIPSes are dead anyway,
    // and good riddance.
#ifdef _WIN64
    s.AddrPC.Offset = c.Rip;
    s.AddrPC.Mode = AddrModeFlat;
    s.AddrFrame.Offset = c.Rbp;
    s.AddrFrame.Mode = AddrModeFlat;
#else
    s.AddrPC.Offset = c.Eip;
    s.AddrPC.Mode = AddrModeFlat;
    s.AddrFrame.Offset = c.Ebp;
    s.AddrFrame.Mode = AddrModeFlat;
#endif

    memset( pSym, '\0', IMGSYMLEN + MAXNAMELEN );
    pSym->SizeOfStruct = IMGSYMLEN;
    pSym->MaxNameLength = MAXNAMELEN;

    memset( &Line, '\0', sizeof Line );
    Line.SizeOfStruct = sizeof Line;

    memset( &Module, '\0', sizeof Module );
    Module.SizeOfStruct = sizeof Module;

    offsetFromSymbol = 0;

    printf( "\n--# FV EIP----- RetAddr- FramePtr StackPtr Symbol\n" );
    for ( frameNum = 0; ; ++ frameNum )
    {
        // get next stack frame (StackWalk(), SymFunctionTableAccess(), SymGetModuleBase())
        // if this returns ERROR_INVALID_ADDRESS (487) or ERROR_NOACCESS (998), you can
        // assume that either you are done, or that the stack is so hosed that the next
        // deeper frame could not be found.
        if ( ! pSW( imageType, hProcess, hThread, &s, &c, NULL,
            pSFTA, pSGMB, NULL ) )
            break;

        // display its contents
        printf( "\n%3d %c%c %08lx %08lx %08lx %08lx ",
            frameNum, s.Far? 'F': '.', s.Virtual? 'V': '.',
            s.AddrPC.Offset, s.AddrReturn.Offset,
            s.AddrFrame.Offset, s.AddrStack.Offset );

        if ( s.AddrPC.Offset == 0 )
        {
            printf( "(-nosymbols- PC == 0)\n" );
        }
        else
        { // we seem to have a valid PC
            // show procedure info (SymGetSymFromAddr())
            if ( ! pSGSFA( hProcess, s.AddrPC.Offset, &offsetFromSymbol, pSym ) )
            {
                if ( gle != 487 )
                    printf( "SymGetSymFromAddr(): Failure. Error: %lu\n", gle );
            }
            else
            {
                // UnDecorateSymbolName()
                pUDSN( pSym->Name, undName, MAXNAMELEN, UNDNAME_NAME_ONLY );
                pUDSN( pSym->Name, undFullName, MAXNAMELEN, UNDNAME_COMPLETE );
                printf( "%s", undName );
                if ( offsetFromSymbol != 0 )
                    printf( " %+ld bytes", (long) offsetFromSymbol );
                putchar( '\n' );
                printf( "    Sig:  %s\n", pSym->Name );
                printf( "    Decl: %s\n", undFullName );
            }

            // show line number info, NT5.0-method (SymGetLineFromAddr())
            if ( pSGLFA != NULL )
            { // yes, we have SymGetLineFromAddr()
                if ( ! pSGLFA( hProcess, s.AddrPC.Offset, &offsetFromSymbol, &Line ) )
                {
                    if ( gle != 487 )
                        printf( "SymGetLineFromAddr(): Failure. Error: %lu\n", gle );
                }
                else
                {
                    printf( "    Line: %s(%lu) %+ld bytes\n",
                        Line.FileName, Line.LineNumber, offsetFromSymbol );
                }
            } // yes, we have SymGetLineFromAddr()

            // show module info (SymGetModuleInfo())
            if ( ! pSGMI( hProcess, s.AddrPC.Offset, &Module ) )
            {
                printf( "SymGetModuleInfo): Failure. Error: %lu\n", gle );
            }
            else
            { // got module info OK
                char ty[80];
                switch ( Module.SymType )
                {
                case SymNone:
                    strcpy( ty, "-nosymbols-" );
                    break;
                case SymCoff:
                    strcpy( ty, "COFF" );
                    break;
                case SymCv:
                    strcpy( ty, "CV" );
                    break;
                case SymPdb:
                    strcpy( ty, "PDB" );
                    break;
                case SymExport:
                    strcpy( ty, "-exported-" );
                    break;
                case SymDeferred:
                    strcpy( ty, "-deferred-" );
                    break;
                case SymSym:
                    strcpy( ty, "SYM" );
                    break;
                default:
                    _snprintf( ty, sizeof ty, "symtype=%ld", (long) Module.SymType );
                    break;
                }

                printf( "    Mod:  %s[%s], base: %08lxh\n",
                    Module.ModuleName, Module.ImageName, Module.BaseOfImage );
                printf( "    Sym:  type: %s, file: %s\n",
                    ty, Module.LoadedImageName );
            } // got module info OK
        } // we seem to have a valid PC

        // no return address means no deeper stackframe
        if ( s.AddrReturn.Offset == 0 )
        {
            // avoid misunderstandings in the printf() following the loop
            SetLastError( 0 );
            break;
        }

    } // for ( frameNum )

    if ( gle != 0 )
        printf( "\nStackWalk(): Failure. Error: %lu\n", gle );

    ResumeThread( hThread );
    free( pSym );
}
////////////////////////////////////////////////////////////////////////////////
#pragma pack( push, 8 )
#define TH32CS_SNAPTHREAD   0x00000004
typedef struct tagTHREADENTRY32
{
    DWORD   dwSize;
    DWORD   cntUsage;
    DWORD   th32ThreadID;       // this thread
    DWORD   th32OwnerProcessID; // Process this thread is associated with
    LONG    tpBasePri;
    LONG    tpDeltaPri;
    DWORD   dwFlags;
} THREADENTRY32;
typedef THREADENTRY32 *  PTHREADENTRY32;
typedef THREADENTRY32 *  LPTHREADENTRY32;
#pragma pack( pop )
////////////////////////////////////////////////////////////////////////////////
bool fillThreadList( std::vector<DWORD> &threadIds, DWORD pid )
{
    // CreateToolhelp32Snapshot()
    typedef HANDLE (__stdcall *tCT32S)( DWORD dwFlags, DWORD th32ProcessId );
    // Module32First()
    typedef BOOL (WINAPI *tT32F)( HANDLE hSnapshot, THREADENTRY32 * lpte );
    // Module32Next()
    typedef BOOL (WINAPI *tT32N)( HANDLE hSnapshot, THREADENTRY32 * lpte );

    // I think the DLL is called tlhelp32.dll on Win9X, so we try both
    const char *dllname[] = { "kernel32.dll", "tlhelp32.dll" };
    HINSTANCE hToolhelp;
    tCT32S pCT32S;
    tT32F pT32F;
    tT32N pT32N;

    HANDLE hSnap;
    THREADENTRY32 te = { sizeof te };
    bool keepGoing;
    int i;

    for ( i = 0; i < lenof( dllname ); ++ i )
    {
        hToolhelp = LoadLibrary( dllname[i] );
        if ( hToolhelp == 0 )
            continue;
        pCT32S = (tCT32S) GetProcAddress( hToolhelp, "CreateToolhelp32Snapshot" );
        pT32F = (tT32F) GetProcAddress( hToolhelp, "Thread32First" );
        pT32N = (tT32N) GetProcAddress( hToolhelp, "Thread32Next" );
        if ( pCT32S != 0 && pT32F != 0 && pT32N != 0 )
            break; // found the functions!
        FreeLibrary( hToolhelp );
        hToolhelp = 0;
    }

    if ( hToolhelp == 0 ) // nothing found?
        return false;

    hSnap = pCT32S( TH32CS_SNAPTHREAD, 0 );
    if ( hSnap == (HANDLE) -1 )
        return false;

    keepGoing = !!pT32F( hSnap, &te );
    threadIds.clear();
    while ( keepGoing )
    {
        if( te.th32OwnerProcessID == pid )
        {
            threadIds.push_back( te.th32ThreadID );
/*
            printf( "\n\n     THREAD ID      = 0x%08X", te.th32ThreadID ); 
            printf( "\n     Base priority  = %d", te.tpBasePri ); 
            printf( "\n     Delta priority = %d", te.tpDeltaPri ); 
*/
        }
        keepGoing = !!pT32N( hSnap, &te );
    }

    CloseHandle( hSnap );

    FreeLibrary( hToolhelp );

    return threadIds.size() != 0;
}

void enumAndLoadModuleSymbols( HANDLE hProcess, DWORD pid, bool silent )
{
    ModuleList modules;
    ModuleListIter it;
    char *img, *mod;

    // fill in module list
    fillModuleList( modules, pid, hProcess, silent );

    for ( it = modules.begin(); it != modules.end(); ++ it )
    {
        // unfortunately, SymLoadModule() wants writeable strings
        img = new char[(*it).imageName.size() + 1];
        strcpy( img, (*it).imageName.c_str() );
        mod = new char[(*it).moduleName.size() + 1];
        strcpy( mod, (*it).moduleName.c_str() );

        if ( pSLM( hProcess, 0, img, mod, (*it).baseAddress, (*it).size ) == 0 )
        {
            if( !silent ) 
                printf( "Error %lu loading symbols for \"%s\"\n",
                    gle, (*it).moduleName.c_str() );
        } else {
            if ( !silent )
                printf( "Symbols loaded: \"%s\"\n", (*it).moduleName.c_str() );
        }

        delete [] img;
        delete [] mod;
    }
}



bool fillModuleList( ModuleList& modules, DWORD pid, HANDLE hProcess, bool silent )
{
    // try toolhelp32 first
    if ( fillModuleListTH32( modules, pid, silent ) )
        return true;
    // nope? try psapi, then
    return fillModuleListPSAPI( modules, pid, hProcess, silent );
}


// miscellaneous toolhelp32 declarations; we cannot #include the header
// because not all systems may have it
#define MAX_MODULE_NAME32 255
#define TH32CS_SNAPMODULE   0x00000008
#pragma pack( push, 8 )
typedef struct tagMODULEENTRY32
{
    DWORD   dwSize;
    DWORD   th32ModuleID;       // This module
    DWORD   th32ProcessID;      // owning process
    DWORD   GlblcntUsage;       // Global usage count on the module
    DWORD   ProccntUsage;       // Module usage count in th32ProcessID's context
    BYTE  * modBaseAddr;        // Base address of module in th32ProcessID's context
    DWORD   modBaseSize;        // Size in bytes of module starting at modBaseAddr
    HMODULE hModule;            // The hModule of this module in th32ProcessID's context
    char    szModule[MAX_MODULE_NAME32 + 1];
    char    szExePath[MAX_PATH];
} MODULEENTRY32;
typedef MODULEENTRY32 *  PMODULEENTRY32;
typedef MODULEENTRY32 *  LPMODULEENTRY32;
#pragma pack( pop )

bool fillModuleListTH32( ModuleList& modules, DWORD pid, bool silent )
{
    // CreateToolhelp32Snapshot()
    typedef HANDLE (__stdcall *tCT32S)( DWORD dwFlags, DWORD th32ProcessID );
    // Module32First()
    typedef BOOL (__stdcall *tM32F)( HANDLE hSnapshot, LPMODULEENTRY32 lpme );
    // Module32Next()
    typedef BOOL (__stdcall *tM32N)( HANDLE hSnapshot, LPMODULEENTRY32 lpme );

    // I think the DLL is called tlhelp32.dll on Win9X, so we try both
    const char *dllname[] = { "kernel32.dll", "tlhelp32.dll" };
    HINSTANCE hToolhelp;
    tCT32S pCT32S;
    tM32F pM32F;
    tM32N pM32N;

    HANDLE hSnap;
    MODULEENTRY32 me = { sizeof me };
    bool keepGoing;
    ModuleEntry e;
    int i;

    for ( i = 0; i < lenof( dllname ); ++ i )
    {
        hToolhelp = LoadLibrary( dllname[i] );
        if ( hToolhelp == 0 )
            continue;
        pCT32S = (tCT32S) GetProcAddress( hToolhelp, "CreateToolhelp32Snapshot" );
        pM32F = (tM32F) GetProcAddress( hToolhelp, "Module32First" );
        pM32N = (tM32N) GetProcAddress( hToolhelp, "Module32Next" );
        if ( pCT32S != 0 && pM32F != 0 && pM32N != 0 )
            break; // found the functions!
        FreeLibrary( hToolhelp );
        hToolhelp = 0;
    }

    if ( hToolhelp == 0 ) // nothing found?
        return false;

    hSnap = pCT32S( TH32CS_SNAPMODULE, pid );
    if ( hSnap == (HANDLE) -1 )
        return false;

    keepGoing = !!pM32F( hSnap, &me );
    while ( keepGoing )
    {
        // here, we have a filled-in MODULEENTRY32
        if( !silent )
            printf( "%08lXh %6lu %-15.15s %s\n", me.modBaseAddr, me.modBaseSize, me.szModule, me.szExePath );
        e.imageName = me.szExePath;
        e.moduleName = me.szModule;
        e.baseAddress = (DWORD) me.modBaseAddr;
        e.size = me.modBaseSize;
        modules.push_back( e );
        keepGoing = !!pM32N( hSnap, &me );
    }

    CloseHandle( hSnap );

    FreeLibrary( hToolhelp );

    return modules.size() != 0;
}


// miscellaneous psapi declarations; we cannot #include the header
// because not all systems may have it
typedef struct _MODULEINFO {
    LPVOID lpBaseOfDll;
    DWORD SizeOfImage;
    LPVOID EntryPoint;
} MODULEINFO, *LPMODULEINFO;



bool fillModuleListPSAPI( ModuleList& modules, DWORD pid, HANDLE hProcess, bool silent )
{
    // EnumProcessModules()
    typedef BOOL (__stdcall *tEPM)( HANDLE hProcess, HMODULE *lphModule, DWORD cb, LPDWORD lpcbNeeded );
    // GetModuleFileNameEx()
    typedef DWORD (__stdcall *tGMFNE)( HANDLE hProcess, HMODULE hModule, LPSTR lpFilename, DWORD nSize );
    // GetModuleBaseName() -- redundant, as GMFNE() has the same prototype, but who cares?
    typedef DWORD (__stdcall *tGMBN)( HANDLE hProcess, HMODULE hModule, LPSTR lpFilename, DWORD nSize );
    // GetModuleInformation()
    typedef BOOL (__stdcall *tGMI)( HANDLE hProcess, HMODULE hModule, LPMODULEINFO pmi, DWORD nSize );

    HINSTANCE hPsapi;
    tEPM pEPM;
    tGMFNE pGMFNE;
    tGMBN pGMBN;
    tGMI pGMI;

    int i;
    ModuleEntry e;
    DWORD cbNeeded;
    MODULEINFO mi;
    HMODULE *hMods = 0;
    char *tt = 0;

    hPsapi = LoadLibrary( "psapi.dll" );
    if ( hPsapi == 0 )
        return false;

    modules.clear();

    pEPM = (tEPM) GetProcAddress( hPsapi, "EnumProcessModules" );
    pGMFNE = (tGMFNE) GetProcAddress( hPsapi, "GetModuleFileNameExA" );
    pGMBN = (tGMFNE) GetProcAddress( hPsapi, "GetModuleBaseNameA" );
    pGMI = (tGMI) GetProcAddress( hPsapi, "GetModuleInformation" );
    if ( pEPM == 0 || pGMFNE == 0 || pGMBN == 0 || pGMI == 0 )
    {
        // yuck. Some API is missing.
        FreeLibrary( hPsapi );
        return false;
    }

    hMods = new HMODULE[TTBUFLEN / sizeof HMODULE];
    tt = new char[TTBUFLEN];
    // not that this is a sample. Which means I can get away with
    // not checking for errors, but you cannot. :)

    if ( ! pEPM( hProcess, hMods, TTBUFLEN, &cbNeeded ) )
    {
        if( !silent )
            printf( "EPM failed, Failure. Error: %lu\n", gle );
        goto cleanup;
    }

    if ( cbNeeded > TTBUFLEN )
    {
        if( !silent )
            printf( "More than %lu module handles. Huh?\n", lenof( hMods ) );
        goto cleanup;
    }

    for ( i = 0; i < int( cbNeeded / sizeof hMods[0] ); ++ i )
    {
        // for each module, get:
        // base address, size
        pGMI( hProcess, hMods[i], &mi, sizeof mi );
        e.baseAddress = (DWORD) mi.lpBaseOfDll;
        e.size = mi.SizeOfImage;
        // image file name
        tt[0] = '\0';
        pGMFNE( hProcess, hMods[i], tt, TTBUFLEN );
        e.imageName = tt;
        // module name
        tt[0] = '\0';
        pGMBN( hProcess, hMods[i], tt, TTBUFLEN );
        e.moduleName = tt;
        if( !silent )
            printf( "%08lXh %6lu %-15.15s %s\n", e.baseAddress,
                e.size, e.moduleName.c_str(), e.imageName.c_str() );

        modules.push_back( e );
    }

cleanup:
    if ( hPsapi )
        FreeLibrary( hPsapi );
    delete [] tt;
    delete [] hMods;

    return modules.size() != 0;
}



#endif // WIN32