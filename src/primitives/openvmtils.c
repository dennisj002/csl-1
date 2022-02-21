
#include "../include/csl.h"

void
OpenVmTil_Verbosity ( )
{
    Do_C_Pointer_StackAccess ( (byte* )& _O_->Verbosity );
}

void
OpenVmTil_ShowMachineCodeInstructions ( )
{
    Do_C_Pointer_StackAccess ( (byte* ) & _O_->Dbi );
}

void
Ovt_Optimize ( )
{
    DataStack_Push ( ( int64 ) GetState ( _CSL_, OPTIMIZE_ON ) ? 1 : 0 ) ;
}

void
Ovt_Inlining ( )
{
    DataStack_Push ( ( int64 ) GetState ( _CSL_, INLINE_ON ) ? 1 : 0 ) ;
}

// allows variables to be created on first use without a "var" declaration

void
Ovt_AutoVar ( )
{
    DataStack_Push ( ( int64 ) GetState ( _O_, AUTO_VAR ) ? 1 : 0 ) ;
}

void
Ovt_AutoVarOff ( )
{
    SetState ( _O_, AUTO_VAR, false ) ;
}

// allows variables to be created on first use without a "var" declaration

void
Ovt_AutoVarOn ( )
{
    SetState ( _O_, AUTO_VAR, true ) ;
}

void
OpenVmTil_DataStackSize ( )
{
    DataStack_Push ( ( int64 ) & _O_->DataStackSize ) ;
}

void
OpenVmTil_CodeSize ( )
{
    DataStack_Push ( ( int64 ) & _O_->MachineCodeSize ) ;
}

void
OpenVmTil_SessionObjectsSize ( )
{
    DataStack_Push ( ( int64 ) & _O_->SessionObjectsSize ) ;
}

void
OpenVmTil_CompilerTempObjectsSize ( )
{
    DataStack_Push ( ( int64 ) & _O_->CompilerTempObjectsSize ) ;
}

void
OpenVmTil_ObjectsSize ( )
{
    DataStack_Push ( ( int64 ) & _O_->ObjectSpaceSize ) ;
}

void
OpenVmTil_DictionarySize ( )
{
    DataStack_Push ( ( int64 ) & _O_->DictionarySize ) ;
}

void
OpenVmTil_Print_DataSizeofInfo ( int64 flag )
{
    if ( flag || ( _O_->Verbosity > 1 ) )
    {
        Printf ( "\ndlnode size : %d bytes, ", sizeof (dlnode ) ) ;
        Printf ( "dllist size : %d bytes, ", sizeof (dllist ) ) ;
        Printf ( "dobject size : %d bytes, ", sizeof ( dobject ) ) ;
        Printf ( "DLNode size : %d bytes, ", sizeof ( DLNode ) ) ;
        Printf ( "AttributeInfo size : %d bytes, ", sizeof (AttributeInfo ) ) ;
        //Printf ( "\nObject size : %d bytes, ", sizeof (Object ) ) ;
        Printf ( "\nSymbol size : %d bytes, ", sizeof (Symbol ) ) ;
        Printf ( "Word size : %d bytes, ", sizeof (Word ) ) ;
        Printf ( "ListObject size : %d bytes, ", sizeof ( ListObject ) ) ;
        Printf ( "WordData size : %d bytes, ", sizeof (WordData ) ) ;
        Printf ( "Context size : %d bytes, ", sizeof (Context ) ) ;
        Printf ( "\nSystem size : %d bytes, ", sizeof (System ) ) ;
        Printf ( "Debugger size : %d bytes, ", sizeof (Debugger ) ) ;
        Printf ( "MemorySpace size : %d bytes, ", sizeof (MemorySpace ) ) ;
        Printf ( "ReadLiner size : %d bytes, ", sizeof (ReadLiner ) ) ;
        Printf ( "Lexer size : %d bytes, ", sizeof (Lexer ) ) ;
        Printf ( "\nInterpreter size : %d bytes, ", sizeof (Interpreter ) ) ;
        Printf ( "Finder size : %d bytes, ", sizeof (Finder ) ) ;
        Printf ( "Compiler size : %d bytes, ", sizeof (Compiler ) ) ;
        Printf ( "ByteArray size : %d bytes, ", sizeof (ByteArray ) ) ;
        Printf ( "NamedByteArray size : %d bytes, ", sizeof (NamedByteArray ) ) ;
        Printf ( "\nMemChunk size : %d bytes, ", sizeof ( MemChunk ) ) ;
        Printf ( "CSL size : %d bytes, ", sizeof (CSL ) ) ;
        Printf ( "OpenVimTil size : %d bytes, ", sizeof (OpenVmTil ) ) ;
        Printf ( "OVT_Static size : %d bytes, ", sizeof (OVT_StaticMemSystem ) ) ;
        Printf ( "OVT_MemSystem size : %d bytes, ", sizeof (OVT_MemSystem ) ) ;
        Printf ( "\nStack size : %d bytes", sizeof (Stack ) ) ;
    }
}

void
OVT_Exit ( )
{
    if ( _O_->Verbosity > 0 ) Printf ( "bye\n" ) ;
    exit ( 0 ) ;
}

void
OVT_StartupMessage ( Boolean promptFlag )
{
    if ( _O_->Verbosity > 0 )
    {
        DefaultColors ;
        //if ( CSL->InitSessionCoreTimes > 1 ) CSL_NewLine () ;
        if ( promptFlag && ( _O_->Restarts < 2 ) )
        {
            System_Time ( _CSL_->Context0->System0, 0, ( char* ) "\nSystem Startup", 1 ) ;
            OVT_Time ( ( char* ) "OVT Startup", 1 ) ;

            _CSL_Version ( promptFlag ) ;
        }
        if ( _O_->Verbosity > 1 )
        {
            Printf ( "\nOpenVmTil : csl comes with ABSOLUTELY NO WARRANTY; for details type `license' in the source directory." ) ;
            Printf ( "\nType 'tc' 'demo' for starters" ) ;
            Printf ( "\nType 'bye' to exit" ) ;
        }
    }
    else if ( promptFlag && ( _O_->Restarts < 2 ) ) _O_->Verbosity = 1 ;
}

void
_OVT_Ok ( Boolean control )
{
    if ( _O_->Verbosity > 3 )
    {
        _CSL_SystemState_Print ( 0 ) ;
        Printf ( "\n<Esc> - break, <Ctrl-C> - quit, <Ctrl-D> - restart, \'bye\'/\'exit\' - leave." ) ;
    }
    _Context_Prompt (_O_->OVT_CSL->Context0, control) ;
}

void
OVT_Ok ( )
{
    _OVT_Ok ( 1 ) ;
    //_CSL_Prompt ( _O_->Verbosity && ( ( _O_->RestartCondition < RESET_ALL ) || _O_->StartTimes > 1 ) ) ;
}


Boolean cli ;
int lic, csl_returnValue = 0 ;

#if 0
int s9_main ( int argc, char **argv ) ;
void
doPrompt ( )
{
    if ( ( lic == '\r' ) || ( lic == '\n' ) ) printf ( "s9> " ) ;
    else printf ( "\ns9> " ) ;
    fflush ( stdout ) ;
}

int
s9_getChar ( FILE * f )
{
    if ( cli ) lic = _Lexer_NextChar ( _Lexer_ ) ;
    else lic = fgetc ( f ) ;
    return lic ;
}

void
s9_ungetChar ( int c, FILE * f )
{
    if ( cli ) ReadLine_UnGetChar ( _ReadLiner_ ) ;
    else ungetc ( c, f ) ;
}

char * csl_buffer ;
void
CSL_S9fes ( )
{
    ReadLiner * rl = _ReadLiner_ ;
    FILE * svFile = rl->InputFile ;
    rl->InputFile = stdin ;
    ReadLine_Init ( rl, _CSL_Key ) ;
    SetState ( _Context_->System0, ADD_READLINE_TO_HISTORY, true ) ;
    byte * snp = rl->NormalPrompt, *sap = rl->AltPrompt ;
    rl->AltPrompt = ( byte* ) "l9< " ;
    rl->NormalPrompt = ( byte* ) "l9> " ;
    csl_buffer = Buffer_New_pbyte ( BUFFER_SIZE ) ;

    s9_main ( 1, ( char*[] ) { "s9" } ) ;

    _Lexer_->TokenBuffer [0] = 0 ;
    rl->NormalPrompt = snp ;
    rl->AltPrompt = sap ;
    rl->InputFile = svFile ;
    Printf ( "s9fes : s9 : exiting ... \n" ) ;
    if ( csl_returnValue == 2 ) DataStack_Push ( (int64) csl_buffer ) ;
}
#endif

