#include "../../include/csl.h"

inline void
_Block_Eval ( block blck )
{
    ( ( block ) blck ) ( ) ;
}

void
Block_Eval ( block blck )
{
    if ( blck )
    {
        _Block_Eval ( blck ) ;
    }
}

// a 'block' is merely a notation borrowed from C
// for a pointer to an anonymous subroutine 'call'

void
CSL_TurnOffBlockCompiler ( )
{
    Context * cntx = _Context_ ;
    Compiler * compiler = cntx->Compiler0 ;
    if ( ! GetState ( compiler, LISP_MODE ) ) CSL_LeftBracket ( ) ;
    _CSL_RemoveNamespaceFromUsingListAndClear ( ( byte* ) "__labels__" ) ;
    _CSL_FinishWordDebugInfo ( cntx->CurrentWordBeingCompiled ) ;
    CSL_NonCompilingNs_Clear ( compiler ) ;
    SetState ( compiler, COMPILE_MODE | VARIABLE_FRAME, false ) ;
    cntx->LastCompiledWord = cntx->CurrentWordBeingCompiled ;
    cntx->CurrentWordBeingCompiled = 0 ;
    compiler->LHS_Word = 0 ;
}

void
CSL_TurnOnBlockCompiler ( )
{
    _CSL_RightBracket ( ) ;
}

// blocks are a notation for subroutines or blocks of code compiled in order,
// nested in code, for any purpose, worded or anonymously
// we currently jmp over them to code which pushes
// a pointer to the beginning of the block on the stack
// so the next word will see it on the top of the stack
// some combinators take more than one block on the stack

BlockInfo *
_CSL_BeginBlock0 ( )
{
    Context * cntx = _Context_ ;
    Compiler * compiler = cntx->Compiler0 ;
    BlockInfo *bi = BlockInfo_New ( ) ;
    if ( ( ! CompileMode ) || ( ! Compiler_BlockLevel ( compiler ) ) )// first block
    {
        OVT_CheckCodeSpaceForRoom ( ) ;
        if ( ! _Context_->CurrentWordBeingCompiled ) _Context_->CurrentWordBeingCompiled = compiler->Current_Word_Create ;
        CSL_RecycleInit_CSL_N_M_Node_WordList ( ) ;
        CSL_TurnOnBlockCompiler ( ) ;
    }
    compiler->LHS_Word = 0 ;
    bi->OriginalActualCodeStart = Here ;
    byte * compiledAtAddress = Compile_UninitializedJump ( ) ;
    bi->PtrToJumpOffset = Here - INT32_SIZE ; // before CSL_CheckCompileLocalFrame after CompileUninitializedJump
    Stack_Push_PointerToJmpOffset ( compiledAtAddress ) ;
    bi->bp_First = Here ; // after the jump for inlining
    return bi ;
}

BlockInfo *
_CSL_BeginBlock1 ( BlockInfo * bi )
{
    Compiler * compiler = _Context_->Compiler0 ;
    if ( _Stack_IsEmpty ( compiler->BlockStack ) )
    {
        // remember : we always jmp around the blocks to the combinator ; the combinator sees the blocks on the stack and uses them otherwise they are lost
        // the jmps are optimized out so the word->Definition is a call to the first combinator
        // we always add a frame and if not needed we move the blocks to overwrite the extra code
        bi->LocalFrameStart = Here ; // before _Compile_AddLocalFrame
        _Compiler_AddLocalFrame ( compiler ) ; // cf EndBlock : if frame is not needed we use BI_Start else BI_FrameStart -- ?? could waste some code space ??
        if ( compiler->NumberOfRegisterArgs ) Compile_Init_LocalRegisterParamenterVariables ( compiler ) ; // this function is called twice to deal with words that have locals before the first block and regular colon words
        CSL_TypeStackReset ( ) ;
    }
    bi->AfterLocalFrame = Here ; // after _Compiler_AddLocalFrame and Compile_InitRegisterVariables
    return bi ;
}

void
_CSL_BeginBlock2 ( BlockInfo * bi )
{
    Compiler * compiler = _Context_->Compiler0 ;
    _Stack_Push ( compiler->BlockStack, ( int64 ) bi ) ; // _Context->CompileSpace->IndexStart before set frame size after turn on
    _Stack_Push ( compiler->CombinatorBlockInfoStack, ( int64 ) bi ) ; // _Context->CompileSpace->IndexStart before set frame size after turn on
    //if ( Is_DebugOn ) Stack_Print ( compiler->CombinatorBlockInfoStack, c_d ( "compiler->CombinatorBlockInfoStack" ), 0 ) ;
}

void
CSL_BeginBlock ( )
{
    BlockInfo * bi = _CSL_BeginBlock0 ( ) ;
    _CSL_BeginBlock1 ( bi ) ;
    _CSL_BeginBlock2 ( bi ) ;
}

void
CSL_FinalizeBlocks ( BlockInfo * bi )
{
    Compiler * compiler = _Context_->Compiler0 ;
    _CSL_InstallGotoCallPoints_Keyed ( bi, GI_RETURN ) ;
    if ( Compiler_IsFrameNecessary ( compiler ) )
    {
        Compiler_SetLocalsFrameSize_AtItsCellOffset ( compiler ) ;
        Compiler_RemoveLocalFrame ( compiler ) ;
        bi->bp_First = bi->LocalFrameStart ; // default 
    }
    else
    {
        if ( compiler->NumberOfRegisterVariables ) Compiler_RemoveLocalFrame ( compiler ) ;
        bi->bp_First = bi->AfterLocalFrame ;
    }
}

void
_CSL_EndBlock1 ( BlockInfo * bi )
{
    Compiler * compiler = _Context_->Compiler0 ;
    if ( ! Compiler_BlockLevel ( compiler ) ) CSL_FinalizeBlocks ( bi ) ;
    //compiler->LHS_Word = 0 ;
    Compiler_WordStack_SCHCPUSCA( 0, 0 )  ;
    _Compile_Return ( ) ;
    DataStack_Push ( ( int64 ) bi->bp_First ) ;
    bi->bp_Last = Here ;
    Compiler_CalculateAndSetPreviousJmpOffset ( compiler, bi->PtrToJumpOffset ) ;
    _SetOffsetForCallOrJump ( bi->PtrToJumpOffset, Here, 0 ) ;
}

byte *
_CSL_EndBlock2 ( BlockInfo * bi )
{
    Context * cntx = _Context_ ;
    Compiler * compiler = cntx->Compiler0 ;
    byte * first = bi->bp_First ;
    if ( ! Compiler_BlockLevel ( compiler ) )
    {
        _CSL_InstallGotoCallPoints_Keyed ( bi, GI_GOTO | GI_RECURSE ) ;
        CSL_TurnOffBlockCompiler ( ) ;
    }
    else if ( ! GetState ( compiler, C_BLOCK_INTERPRETER ) ) _Namespace_RemoveFromUsingList_ClearFlag ( bi->BI_LocalsNamespace, 1, 0 ) ; // a block namespace
    return first ;
}

void
CSL_EndBlock ( )
{
    Context * cntx = _Context_ ;
    Compiler * compiler = cntx->Compiler0 ;
    BlockInfo * bi = ( BlockInfo * ) Stack_Pop_WithExceptionOnEmpty ( compiler->BlockStack ) ;
    bi->LogicCodeWord = CSL_WordList ( 1 ) ;
    _CSL_EndBlock1 ( bi ) ;
    _CSL_EndBlock2 ( bi ) ;
    //if ( Is_DebugOn ) Stack_Print ( compiler->CombinatorBlockInfoStack, c_d ( "compiler->CombinatorBlockInfoStack" ), 0 ) ;
}

BlockInfo *
BlockInfo_New ( )
{
    BlockInfo *bi = ( BlockInfo * ) Mem_Allocate ( sizeof (BlockInfo ), COMPILER_TEMP ) ;
    return bi ;
}

