#ifndef HelloWorldModule_H
#define HelloWorldModule_H

////////////////////////////////////////////////////////////////////////////////
#include<jam_module.h>
////////////////////////////////////////////////////////////////////////////////
class HelloWorldModule: public JAM::Module {
public:
    int Setup( int argc, char * argv[] );
    int Work( void );
    int Cleanup( void );
protected:
    int counter; 
    int counter_max;
};
////////////////////////////////////////////////////////////////////////////////

#endif
