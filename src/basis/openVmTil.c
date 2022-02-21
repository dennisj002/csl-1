
#include "../include/csl.h"
#define VERSION ((byte*) "0.920.530" ) 

// inspired by :: Foundations of Mathematical Logic [Foml] by Haskell Curry
// CT/Oop (Category Theory, Object Oriented Programming, Type Theory), 
// Formal Language Theory (Panini, Chomsky) 
// C/C++/C#, Lisp, RPN/Lag - Reverse Polish Notation - (Left Associative Grammar)10
// Automata Theory : State Machines, Push Down Automata (PDA), Turing Machines :: 
// Also inspired by : Laws of Form, by G.S. Brown, Kurt Godel, etc.
// csl : context sensitive language; C shell (extension) language
// cfr : C, Forth, reason
// cflr : C, Forth, Lisp, reason
// til : a toolkit for implementing languages (maybe even a compiler compiler) based on these ideas,

OpenVmTil * _O_ ;

int
main ( int argc, char * argv [ ] )
{
    LinuxInit ( ) ;
    openvmtil ( argc, argv ) ;
}

void
openvmtil ( int64 argc, char * argv [ ] )
{
    OpenVmTil_Run ( argc, argv ) ;
}

void
_OpenVmTil_Init ( OpenVmTil * ovt, int64 resetHistory )
{
    ovt->MemorySpace0 = MemorySpace_New ( ovt, "DefaultMemorySpace" ) ;
    ovt->CSL_InternalSpace = NBA_OvtNew ( ( byte* ) "CSLInternalSpace", ovt->CSLSize, T_CSL ) ;
    ovt->InternalObjectSpace = NBA_OvtNew ( ( byte* ) "InternalObjectSpace", ovt->InternalObjectsSize, INTERNAL_OBJECT_MEM ) ; // used in _DObject_ValueDefinition_Init
    ovt->BufferList = _dllist_New ( OPENVMTIL ) ; // put it here to minimize allocating chunks for each node and the list
    ovt->RecycledWordList = _dllist_New ( OPENVMTIL ) ; // put it here to minimize allocating chunks for each node and the list
    ovt->RecycledOptInfoList = _dllist_New ( OPENVMTIL ) ; // put it here to minimize allocating chunks for each node and the list
    ovt->VersionString = VERSION ;
    // ? where do we want the init file ?
    if ( _File_Exists ( ( byte* ) "./init.csl" ) )
    {
        ovt->InitString = ( byte* ) "\"./init.csl\" _include" ; // could allow override with a startup parameter
        SetState ( ovt, OVT_IN_USEFUL_DIRECTORY, true ) ;
    }
    else
    {
        ovt->InitString = ( byte* ) "\"/usr/local/lib/csl/init.csl\" _include" ; // could allow override with a startup parameter
        SetState ( ovt, OVT_IN_USEFUL_DIRECTORY, false ) ;
    }
    if ( ovt->Verbosity > 1 )
    {
        Printf ( "\nRestart : All memory freed, allocated and initialized as at startup. "
            "termios, verbosity and memory category allocation sizes preserved. verbosity = %d.", ovt->Verbosity ) ;
        OpenVmTil_Print_DataSizeofInfo ( 0 ) ;
    }
    //ovt->ThrowBuffer = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    ovt->PrintBuffer = _Buffer_NewPermanent ( BUFFER_SIZE ) ;
    _OpenVmTil_ColorsInit ( ovt ) ;
}

OpenVmTil *
OpenVmTil_New ( OpenVmTil * ovt, int64 argc, char * argv [ ] )
{
    int64 restartCondition, startedTimes = 0, allocSize ;
    if ( ! ovt ) restartCondition = INITIAL_START ;
    else
    {
        restartCondition = FULL_RESTART ;
        startedTimes = ovt->StartedTimes ;
    }

    ovt = _OpenVmTil_Allocate ( ovt ) ;
    TimerInit ( &ovt->Timer ) ;

    OVT_SetRestartCondition ( ovt, restartCondition ) ;
    ovt->Argc = argc ;
    ovt->Argv = argv ;
    ovt->StartedTimes = startedTimes ;
    OVT_GetStartupOptions ( ovt ) ;

    allocSize = 430 * K ;
    ovt->InternalObjectsSize = 1 * M ;
    ovt->ObjectSpaceSize = 1 * M ;
    ovt->LispSpaceSize = 1 * M ;
    ovt->LispTempSize = 1 * M ;
    ovt->LispCopySize = 1 * M ;
    ovt->CompilerTempObjectsSize = 2 * allocSize ;
    ovt->BufferSpaceSize = allocSize ; //35 * ( sizeof ( Buffer ) + BUFFER_SIZE ) ;
    ovt->MachineCodeSize = allocSize ;
    ovt->StringSpaceSize = allocSize ;
    ovt->DictionarySize = 1 * M ; //100 * K ;
    ovt->CSLSize = ( 140 * K ) ;
    ovt->OpenVmTilSize = ( 6 * K ) ;
    ovt->DataStackSize = 8 * K ; //1 * MB ; //8 * MB ;
    ovt->TempObjectsSize = 200 * K ; //COMPILER_TEMP_OBJECTS_SIZE ;
    ovt->WordRecylingSize = 1 * K * ( sizeof (Word ) + sizeof (WordData ) ) ; //50 * K ; //COMPILER_TEMP_OBJECTS_SIZE ;
    ovt->SessionObjectsSize = 50 * K ;

    _OpenVmTil_Init ( ovt, 0 ) ;
    Linux_SetupSignals ( &ovt->JmpBuf0, 1 ) ;
    return ovt ;
}

void
OpenVmTil_Run ( int64 argc, char * argv [ ] )
{
    OpenVmTil * ovt ;
    int64 restartCondition = INITIAL_START, restarts = 0, sigSegvs = 0 ;
    while ( 1 )
    {
        if ( _O_ )
        {
            sigSegvs = _O_->SigSegvs ;
            restarts = ++ _O_->Restarts ;
            if ( _O_->Restarts > 20 ) OVT_Exit ( ) ;
            if ( _O_->RestartCondition == COMPLETE_INITIAL_START ) OVT_FullRestartCompleteDelete ( ) ;
        }
        ovt = _O_ = OpenVmTil_New ( _O_, argc, argv ) ;
        OVT_SetRestartCondition ( ovt, restartCondition ) ;
        ovt->SigSegvs = sigSegvs ;
        ovt->Verbosity = 1 ;
        ovt->Restarts = restarts ;
        if ( ovt->Restarts ) OVT_ExceptionState_Print ( ) ;
        //CSL_NewLine ( ) ;
        if ( ! sigsetjmp ( ovt->JmpBuf0, 0 ) ) // nb. siglongjmp always comes to beginning of the block 
            CSL_Run ( ovt->OVT_CSL, ovt->RestartCondition ) ;
        restartCondition = ovt->RestartCondition ;
        OVT_SetRestartCondition ( ovt, restartCondition ) ;
    }
}

void
Ovt_RunInit ( OpenVmTil * ovt )
{
    ovt->StartedTimes ++ ;
    OVT_SetRestartCondition ( ovt, CSL_RUN_INIT ) ;
}

void
OVT_PrintStartupOptions ( OpenVmTil * ovt )
{
    int i ;
    for ( i = ovt->Argc ; i ; i -- ) Printf ( "\n\nOVT_GetStartupOptions :: ovt->Argv [%d] = %s\n\n", i, ovt->Argv [i] ) ;
    Printf ( "\n\nOVT_GetStartupOptions :: ovt->StartupFilename = %s\n\n", ovt->StartupFilename ) ;
    //if ( ovt->Verbosity > 1 ) Pause ( ) ;
}

void
OVT_GetStartupOptions ( OpenVmTil * ovt )
{
    int64 i ;
    byte * arg ;
    for ( i = 0 ; i < ovt->Argc ; i ++ )
    {
        arg = ovt->Argv [ i ] ;
        if ( String_Equal ( "-m", arg ) ) ovt->TotalMemSizeTarget = ( atoi ( ovt->Argv [ ++ i ] ) * MB ) ;
            // -s : a script file with "#! csl -s" -- as first line includes the script file, the #! whole line is treated as a comment
        else if ( String_Equal ( "-f", arg ) || ( String_Equal ( "-s", arg ) ) ) ovt->StartupFilename = ( byte* ) ovt->Argv [ ++ i ] ;
        else if ( String_Equal ( "-e", arg ) ) ovt->StartupString = ( byte* ) ovt->Argv [ ++ i ] ;
    }
}

// only partially working ??

void
OVT_RecycleAllWordsDebugInfo ( )
{
    SetState ( _CSL_, ( RT_DEBUG_ON | GLOBAL_SOURCE_CODE_MODE ), false ) ;
    Tree_Map_Namespaces ( _CSL_->Namespaces->W_List, ( MapSymbolFunction ) CSL_DeleteWordDebugInfo ) ;
    _OVT_MemListFree_WordRecyclingSpace ( ) ;
    OVT_FreeTempMem ( ) ;
    _CSL_->CSL_N_M_Node_WordList = _dllist_New ( T_CSL ) ;
}


