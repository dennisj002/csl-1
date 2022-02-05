#include "../include/csl.h"

// examples of the use of PrefixCombinators are in interpreter.csl

void
CSL_InfixModeOff ( )
{
    SetState ( _Context_, INFIX_MODE, false ) ;
    if ( ! Compiling ) Namespace_SetAsNotUsing_MoveToTail ( ( byte* ) "Infix" ) ;
}

void
CSL_InfixModeOn ( )
{
    SetState ( _Context_, INFIX_MODE, true ) ;
    if ( ! Compiling ) Namespace_DoNamespace_Name ( ( byte* ) "Infix" ) ;
}

void
CSL_PrefixModeOff ( )
{
    SetState ( _Context_, PREFIX_MODE, false ) ;
}

void
CSL_PrefixModeOn ( )
{
    SetState ( _Context_, PREFIX_MODE, true ) ;
}

void
CSL_C_Syntax_Off ( )
{
    Context * cntx = _Context_ ;
    SetState ( cntx, C_SYNTAX | PREFIX_MODE | INFIX_MODE, false ) ;
    CSL_SetInNamespaceFromBackground ( ) ; // before below Namespace_SetAsNotUsing_MoveToTail in case one of them is the background namespace
    Namespace_SetAsNotUsing_MoveToTail ( ( byte* ) "PrefixCombinators" ) ;
    Namespace_SetAsNotUsing_MoveToTail ( ( byte* ) "Infix" ) ;
    Namespace_SetAsNotUsing_MoveToTail ( ( byte* ) "C_Syntax" ) ;
    Context_SetSpecialTokenDelimiters ( cntx, 0, CONTEXT ) ;
}

void
CSL_C_Syntax_On ( )
{
    Context * cntx = _Context_ ;
    Compiler_Save_C_BackgroundNamespace ( cntx->Compiler0 ) ;
    SetState ( cntx, C_SYNTAX | PREFIX_MODE | INFIX_MODE, true ) ;
    Namespace_DoNamespace_Name ( ( byte* ) "C" ) ;
    Namespace_DoNamespace_Name ( ( byte* ) "PrefixCombinators" ) ;
    Namespace_DoNamespace_Name ( ( byte* ) "Infix" ) ;
    Namespace_DoNamespace_Name ( ( byte* ) "C_Syntax" ) ;
    Compiler_SetAs_InNamespace_C_BackgroundNamespace ( cntx->Compiler0 ) ;
    //Context_SetSpecialTokenDelimiters ( cntx, ( byte* ) " ,\n\r\t", CONTEXT ) ;
    //CSL_TypeCheckOn ( ) ;
}

// switch to the default forth, postfix mode

void
CSL_PostfixModeOn ( )
{
    CSL_C_Syntax_Off ( ) ;
    Namespace_SetAsNotUsing_MoveToTail ( ( byte* ) "Lisp" ) ;
    Namespace_SetAsNotUsing_MoveToTail ( ( byte* ) "LispTemp" ) ;
    Namespace_SetAsNotUsing_MoveToTail ( ( byte* ) "LispDefines" ) ;
}

void
CSL_AddressOf ( )
{
    SetState ( _Context_, ADDRESS_OF_MODE, true ) ; // turned off after one object
}

void
CSL_C_Semi ( )
{
    if ( ( ! Compiling ) )
    {
        CSL_InitSourceCode ( _CSL_ ) ;
    }
    Context_ClearQidInNamespace ( ) ;
    _Compiler_->LHS_Word = 0 ; // nb. : after NEW_INTERPRET
    CSL_TypeStackReset ( ) ;
}

void
CSL_C_Comma ( void )
{
    //Context_ClearQidInNamespace ( ) ;
    //_Compiler_->LHS_Word = 0 ; // nb. : after NEW_INTERPRET
}

void
CSL_End_C_Block ( )
{
    Context * cntx = _Context_ ;
    Compiler * compiler = cntx->Compiler0 ;
    CSL_EndBlock ( ) ; // NB. CSL_EndBlock changes cntx->Compiler0->BlockLevel
    if ( ! Compiler_BlockLevel ( compiler ) ) CSL_WordInitFinal ( ) ;
    else
    {
        // we're still compiling so ... ??

        Word * word = _Context_CurrentWord ( cntx ) ;
        word->W_NumberOfNonRegisterArgs = compiler->NumberOfArgs ;
        word->W_NumberOfNonRegisterLocals = compiler->NumberOfLocals ;
    }
    CSL_SetInNamespaceFromBackground ( ) ;
}

void
CSL_Begin_C_Block ( )
{
    if ( Compiling && GetState ( _Context_, C_SYNTAX ) )
    {
        if ( GetState ( _Compiler_, C_COMBINATOR_PARSING ) ) CSL_BeginBlock ( ) ;
        else Interpret_Until_Token ( _Interpreter_, ( byte* ) "}", 0 ) ;
    }
}

Namespace *
CSL_C_Class_New ( void )
{
    byte * name = ( byte* ) DataStack_Pop ( ) ;
    return DataObject_New ( C_CLASS, 0, name, 0, 0, 0, 0, 0, 0, 0, 0, - 1 ) ;
}

void
CSL_C_Infix_Equal ( )
{
    _CSL_C_Infix_EqualOp ( 0 ) ;
}

void
CSL_If_PrefixCombinators ( )
{
    Compiler * compiler = _Compiler_ ;
    Word * combinatorWord0 = CSL_WordList ( 0 ) ;
    combinatorWord0->W_SC_Index = _Lexer_->SC_Index ;
    byte svscp = GetState ( compiler, C_COMBINATOR_PARSING ) ;
    SetState ( compiler, C_COMBINATOR_PARSING, true ) ;
    compiler->TakesLParenAsBlock = true ;
    compiler->BeginBlockFlag = false ;
    int64 blocksParsed = CSL_Interpret_C_Blocks ( 2, 1, 0 ) ;
    _Context_->SC_CurrentCombinator = combinatorWord0 ;
    if ( blocksParsed > 2 ) CSL_TrueFalseCombinator3 ( ) ;
    else
    {
        d0 ( if ( Is_DebugOn ) Printf ( "\n\nbefore CSL_If2Combinator : blockStack depth = %d : %s : %s\n\n", _Stack_Depth ( compiler->BlockStack ), _Context_->CurrentlyRunningWord->Name, Context_Location ( ) ) ) ;
        CSL_If2Combinator ( ) ;
    }
    SetState ( compiler, C_COMBINATOR_PARSING, svscp ) ;
}

void
CSL_DoWhile_PrefixCombinators ( )
{
    SetState ( _Compiler_, IN_DO_WHILE, true ) ;
    Word * currentWord0 = CSL_WordList ( 0 ) ;
    currentWord0->W_SC_Index = _Lexer_->SC_Index ;
    byte * start = Here ;
    _Compiler_->BeginBlockFlag = false ;
    CSL_Interpret_C_Blocks ( 1, 0, 0 ) ;
    // just assume 'while' is there 
    byte * token = //Lexer_ReadToken ( _Context_->Lexer0 ) ; // drop the "while" token
        _Lexer_Next_NonDebugOrCommentTokenWord (_Context_->Lexer0, 0, 1, 0 ) ;
    _Compiler_->TakesLParenAsBlock = true ;
    _Compiler_->BeginBlockFlag = false ;
    CSL_Interpret_C_Blocks ( 1, 0, 0 ) ;
    _Context_->SC_CurrentCombinator = currentWord0 ;
    if ( ! CSL_DoWhileCombinator ( ) ) SetHere ( start, 1 ) ;
    SetState ( _Compiler_, IN_DO_WHILE, false ) ;
}

void
CSL_For_PrefixCombinators ( )
{
    Word * currentWord0 = CSL_WordList ( 0 ) ;
    currentWord0->W_SC_Index = _Lexer_->SC_Index ;
    _Compiler_->TakesLParenAsBlock = true ;
    _Compiler_->BeginBlockFlag = false ;
    CSL_Interpret_C_Blocks ( 4, 0, 1 ) ;
    _Context_->SC_CurrentCombinator = currentWord0 ;
    CSL_ForCombinator ( ) ;
}

void
CSL_Loop_PrefixCombinators ( )
{
    Word * currentWord0 = CSL_WordList ( 0 ) ;
    currentWord0->W_SC_Index = _Lexer_->SC_Index ;
    _Compiler_->BeginBlockFlag = false ;
    CSL_Interpret_C_Blocks ( 1, 0, 0 ) ;
    _Context_->SC_CurrentCombinator = currentWord0 ;
    CSL_LoopCombinator ( ) ;
}

void
CSL_While_PrefixCombinators ( )
{
    if ( GetState ( _Compiler_, IN_DO_WHILE ) ) return ;
    Word * currentWord0 = CSL_WordList ( 0 ) ;
    currentWord0->W_SC_Index = _Lexer_->SC_Index ;
    byte * start = Here ;
    _Compiler_->TakesLParenAsBlock = true ;
    _Compiler_->BeginBlockFlag = false ;
    CSL_Interpret_C_Blocks ( 2, 0, 0 ) ;
    _Context_->SC_CurrentCombinator = currentWord0 ;
    if ( ! CSL_WhileCombinator ( ) ) SetHere ( start, 1 ) ;
}

void
CSL_TypedefStructEnd ( void )
{
    Namespace_SetAsNotUsing ( ( byte* ) "C_Typedef" ) ;
    //Compiler_SetAs_InNamespace_C_BackgroundNamespace ( _Compiler_ ) ;
}

