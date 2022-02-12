
#include "../include/csl.h"

byte *
_CSL_Token ( )
{
    byte * token = Lexer_ReadToken ( _Lexer_ ) ;
    return token ;
}

void
CSL_Token ( )
{
    byte * token = Lexer_ReadToken ( _Lexer_ ) ;
    DataStack_Push ( ( int64 ) token ) ;
}

void
CSL_TokenQID ( )
{
    byte * token ;
    Word * word ;
    while ( 1 )
    {
        token = Lexer_ReadToken ( _Lexer_ ) ;
        word = _Interpreter_TokenToWord ( _Interpreter_, token, - 1, - 1 ) ;
        if ( word )
        {
            Boolean isForwardDotted = ReadLiner_IsTokenForwardDotted ( _ReadLiner_, word->W_RL_Index ) ;
            if ( ( isForwardDotted ) || ( token[0] == '.' ) ) Interpreter_DoWord_Default ( _Interpreter_, word, word->W_RL_Index, word->W_SC_Index ) ; //  Word_Eval ( word ) ;
            else break ;
        }
    }
    if ( GetState ( _Lexer_, END_OF_LINE ) ) SetState ( _Interpreter_, END_OF_LINE, true ) ; //necessary to update interpreter state since we are pushing the last token
    DataStack_Push ( ( int64 ) token ) ;
}

byte *
_CSL_FilenameToken ( )
{
    byte * token = _Lexer_LexNextToken_WithDelimiters ( _Lexer_, 0, 1, 0, 1, LEXER_ALLOW_DOT ) ;
    return token ;
}

void
CSL_FilenameToken ( )
{
    byte * token = _CSL_FilenameToken ( ) ;
    DataStack_Push ( ( int64 ) token ) ;
}

void
CSL_SingleQuote ( )
{
    _CSL_SingleQuote ( ) ;
}

void
CSL_Tick ( )
{
    _CSL_SingleQuote ( ) ;
}

void
Parse_SkipUntil_EitherToken_OrNewline ( byte * end1, byte* end2 )
{
    byte * token ;
    int64 inChar ;
    ReadLiner * rl = _Context_->ReadLiner0 ;
    do
    {
        if ( ( token = Lexer_ReadToken ( _Context_->Lexer0 ) ) )
        {
            if ( ( end1 && String_Equal ( token, end1 ) ) || ( end2 && String_Equal ( token, end2 ) ) ) break ;
        }
        inChar = ReadLine_PeekNextChar ( rl ) ;
        if ( ( inChar == 0 ) || ( inChar == - 1 ) || ( inChar == eof ) || ReadLine_AreWeAtNewlineAfterSpaces ( rl ) ) break ;
    }
    while ( token ) ;
}

void
CSL_ParseObject ( )
{
    Lexer * lexer = _Context_->Lexer0 ;
    byte * token = ( byte* ) DataStack_Pop ( ) ;
    Lexer_ParseObject ( lexer, token ) ;
    DataStack_Push ( lexer->Literal ) ;
}

void
CSL_DoubleQuoteMacro ( )
{
    _Lexer_ParseTerminatingMacro ( _Lexer_, '\"', 1, 1 ) ;
}

void
_CSL_Word_ClassStructure_PrintData ( Word * typedefWord0, Word * word )
{
    Word * typedefWord = Word_UnAlias ( typedefWord0 ) ;
    //CSL_NewLine ( ) ;
    //Printf ( "\n%s", typedefWord->Name ) ;
    byte * dataBits = ( word->W_ObjectAttributes & ( STRUCTURE_TYPE ) ) ? ( byte* ) word->W_Value : ( byte* ) word ; //STRUCTURE_TYPE not quite right ??
    Printf ( "\n\t%16s :: size = %d :: at : %016lx", typedefWord->Name, //tdsci->TdsciToken, 
        ( int64 ) _CSL_VariableValueGet ( typedefWord->Name, ( byte* ) "size" ), dataBits ) ; //CSL_GetAndSet_ObjectByteSize ( tdsci->Tdsci_Field_Type_Namespace ),
        //&tdsci->DataPtr [ tdsci->Tdsci_Offset ] ) ;
    if ( typedefWord && word ) Object_PrintStructuredData ( dataBits, typedefWord ) ;
}

void
CSL_Word_Name_ClassStructure_PrintData ( )
{
    byte * token = ( byte* ) DataStack_Pop ( ) ;
    Word * typedefWord = Finder_Word_FindUsing ( _Finder_, token, 0 ) ;
    byte * token1 = ( byte* ) DataStack_Pop ( ) ;
    Word * word = Finder_Word_FindUsing ( _Finder_, token1, 0 ) ;
    _CSL_Word_ClassStructure_PrintData ( typedefWord, word ) ;
}

void
CSL_Ptr_ClassStructureName_PrintData ( )
{
    byte * token = ( byte* ) DataStack_Pop ( ) ;
    Word * typedefWord = Finder_Word_FindUsing ( _Finder_, token, 0 ) ;
    typedefWord = Word_UnAlias ( typedefWord ) ;
    byte * ptr = (byte*) DataStack_Pop ( ) ;
    //_CSL_Word_ClassStructure_PrintData ( typedefWord, ptr ) ;
    Object_PrintStructuredData ( ptr, typedefWord ) ;
}

void
CSL_Word_ClassStructure_PrintData ( )
{
    Word * typedefWord = ( Word* ) DataStack_Pop ( ) ;
    Word * word = ( Word* ) DataStack_Pop ( ) ;
    _CSL_Word_ClassStructure_PrintData ( typedefWord, word ) ;
#if 0 // debug    
    word = Finder_Word_FindUsing ( _Finder_, "wtdInclude", 0 ) ;
    Printf ( "\n&_Identifer.CodeSize = %lx :: CodeSize = %d", &word->CodeSize, word->CodeSize ) ;
#endif    
}


