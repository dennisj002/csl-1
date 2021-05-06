
#include "../include/csl.h"

#ifdef LINUX
struct termios SavedTerminalAttributes ;

void
_DisplaySignal ( int64 signal )
{
    if ( signal )
    {
        byte * location ; 
        if ( _O_->SigSegvs < 2 ) location = ( byte* ) Context_Location ( ) ;
        else location = (byte*) "" ;
        switch ( signal )
        {
            case SIGSEGV:
            {
                printf ( "\nSIGSEGV : memory access violation : address = 0x%016lx : %s", (uint64) _O_->SigAddress, location ) ;
                fflush ( stdout ) ;
                break ;
            }
            case SIGFPE:
            {
                Printf ( "\nSIGFPE : arithmetic exception - %s", location ) ;
                break ;
            }
            case SIGILL:
            {
                Printf ( "\nSIGILL : illegal instruction - %s", location ) ;
                break ;
            }
            case SIGTRAP:
            {
                Printf ( "\nSIGTRAP : int3/trap - %s", location ) ;
                break ;
            }
            default: break ;
        }
    }
}

void
Linux_SetupSignals ( sigjmp_buf * sjb, int64 startTimes )
{
    struct sigaction signalAction ;
    // from http://www.linuxjournal.com/article/6483
    int64 i, result ;
    Mem_Clear ( ( byte* ) & signalAction, sizeof ( struct sigaction ) ) ;
    Mem_Clear ( ( byte* ) sjb, sizeof ( *sjb ) ) ;
    signalAction.sa_sigaction = OpenVmTil_SignalAction ;
    signalAction.sa_flags = SA_SIGINFO | SA_RESTART ; // restarts the set handler after being used instead of the default handler
    for ( i = SIGHUP ; i <= _NSIG ; i ++ )
    {
        result = sigaction ( i, &signalAction, NULL ) ;
        d0 ( if ( ( result && ( startTimes ) && ( _O_ && ( _O_->Verbosity > 2 ) ) ) printf ( "\nLinux_SetupSignals : signal number = " INT_FRMT_02 " : result = " INT_FRMT " : This signal can not have a handler.", i, result ) ) ) ;
    }
    //signal ( SIGWINCH, SIG_IGN ) ; // a fix for a netbeans problem but causes crash with gcc 6.x -O2+
}

void
Linux_RestoreTerminalAttributes ( )
{
    tcsetattr ( STDIN_FILENO, TCSANOW, &SavedTerminalAttributes ) ;
}
struct termios term ;

void
Linux_SetInputMode ( struct termios * savedTerminalAttributes )
{
    struct termios term ; //terminalAttributes ;
    // Make sure stdin is a terminal. /
    if ( ! isatty ( STDIN_FILENO ) )
    {
        Printf ( "Not a terminal.\n" ) ;
        exit ( EXIT_FAILURE ) ;
    }

    // Save the terminal attributes so we can restore them later. /
    memset ( savedTerminalAttributes, 0, sizeof ( struct termios ) ) ;
    tcgetattr ( STDIN_FILENO, savedTerminalAttributes ) ;
    atexit ( Linux_RestoreTerminalAttributes ) ;

    tcgetattr ( STDIN_FILENO, &term ) ; //&terminalAttributes ) ;
#if 0
    //terminalAttributes.c_iflag &= ~ ( IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR
    //        | IGNCR | ICRNL | IXON ) ;
    //terminalAttributes.c_lflag &= ~ ( ICANON | ECHO | ECHONL | ISIG ) ; // | IEXTEN ) ;
    terminalAttributes.c_lflag &= ~ ( ICANON | ECHO | ECHONL ) ; // | ISIG ) ; // | IEXTEN ) ;
    //terminalAttributes.c_cflag &= ~ ( CSIZE | PARENB ) ;
    //terminalAttributes.c_cflag |= CS8 ;
    //terminalAttributes.c_cc [ VMIN ] = 1 ;
    //terminalAttributes.c_cc [ VTIME ] = 0 ;
    tcsetattr ( STDIN_FILENO, TCSANOW, &terminalAttributes ) ;
#else
    // from http://stackoverflow.com/questions/4217037/catch-ctrl-c-in-c
    term.c_iflag |= IGNBRK ;
    term.c_iflag &= ~ ( INLCR | ICRNL | IXON | IXOFF ) ;
    term.c_lflag &= ~ ( ICANON | ECHO | ECHOK | ECHOE | ECHONL | ISIG | IEXTEN ) ;
    //term.c_lflag &= ~ ( ICANON | ISIG | IEXTEN ) ;
    term.c_cc[VMIN] = 1 ;
    term.c_cc[VTIME] = 0 ;
    tcsetattr ( fileno ( stdin ), TCSANOW, &term ) ;
#endif    
}

void
_LinuxInit ( struct termios * savedTerminalAttributes )
{
    Linux_SetInputMode ( savedTerminalAttributes ) ; // nb. save first !! then copy to _O_ so atexit reset from global _O_->SavedTerminalAttributes
    //Linux_SetupSignals ( 1 ) ; //_O_ ? ! _O_->StartedTimes : 1 ) ;
}

void
LinuxInit ()
{
    _LinuxInit ( &SavedTerminalAttributes ) ; // nb. save first !! then copy to _O_ so atexit reset from global _O_->SavedTerminalAttributes
    //Linux_SetupSignals ( 1 ) ; //_O_ ? ! _O_->StartedTimes : 1 ) ;
}

#endif


