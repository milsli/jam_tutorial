#ifndef MINIDUMP_H
#define MINIDUMP_H

#ifdef  WIN32
LONG WINAPI CreateMiniDump( EXCEPTION_POINTERS* pep );

bool MiniDumpAvailable( );
#else 
bool MiniDumpAvailable( ) { return false ; }
#endif

#endif