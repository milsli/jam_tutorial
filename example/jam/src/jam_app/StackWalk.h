#ifndef STACKWALK_H
#define STACKWALK_H

#ifdef  WIN32
LONG WINAPI CreateStackWalk( EXCEPTION_POINTERS* pep );

bool StackWalkAvailable( );
#else 
bool StackWalkAvailable( ) { return false ; }
#endif

#endif