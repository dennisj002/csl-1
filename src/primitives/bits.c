#include "../include/csl.h"

// ( n ttt -- )

void
CSL_JMP ( )
{
    if ( CompileMode )
    {
        byte * compiledAtAddress = Compile_UninitializedJump ( ) ; // at the end of the 'if block' we need to jmp over the 'else block'
        CSL_CalculateAndSetPreviousJmpOffset_ToHere ( ) ;
        Stack_Push_PointerToJmpOffset (compiledAtAddress) ;
    }
    else
    {
        Error_Abort ( "", "\njmp : can only be used in compile mode." ) ;
    }
}

void
CSL_Compile_Jcc ( )
{
    int64 ttt = DataStack_Pop ( ) ;
    int64 n = DataStack_Pop ( ) ;
    byte * compiledAtAddress = _Compile_Jcc (n, ttt, 0 , 0) ; // we do need to store and get this logic set by various conditions by the compiler : _Compile_SET_tttn_REG
    Stack_Push_PointerToJmpOffset (compiledAtAddress) ;
}

void
CSL_Jcc_Label ( )
{
    int64 ttt = DataStack_Pop ( ) ;
    int64 n = DataStack_Pop ( ) ;
    _Compile_Jcc (n, ttt, 0 , 0) ;
    GotoInfo_New ( ( byte* ) DataStack_Pop ( ), GI_GOTO ) ;
}

void
CSL_JmpToHere ( )
{
    CSL_CalculateAndSetPreviousJmpOffset_ToHere ( ) ;
}

void
CSL_BitWise_NOT ( ) // xor
{
    if ( CompileMode )
    {
        Compile_BitWise_NOT ( _Context_->Compiler0 ) ;
    }
    else
    {
        //if ( Dsp ) { Dsp [0] = ~ Dsp[0] ; }
        TOS = (! TOS) ;
    }
}

void
CSL_BitWise_NEG ( ) // xor
{
    if ( CompileMode )
    {
        Compile_BitWise_NEG ( _Context_->Compiler0 ) ;
    }
    else
    {
        TOS = ~ TOS ;
    }
}

void
CSL_BitWise_OR ( ) // xor
{
    if ( CompileMode )
    {
        Compile_X_Group1 ( _Context_->Compiler0, OR, TTT_ZERO, NEGFLAG_NZ ) ;
    }
    else
    {
        _DspReg_ [ - 1 ] = _DspReg_ [ - 1 ] | TOS ;
        DataStack_Drop ( ) ;
    }
}

void
CSL_BitWise_OrEqual ( ) // -=
{
    if ( CompileMode )
    {
        Compile_X_OpEqual ( _Context_->Compiler0, CSL_BitWise_OR ) ; //OR ) ;
    }
    else
    {
        int64 *x, n ;
        n = DataStack_Pop ( ) ;
        x = ( int64* ) DataStack_Pop ( ) ;
        *x = ( * x ) | n ;
        //_DataStack_SetTop ( Dsp, _DataStack_Pop () + _DataStack_GetTop ( Dsp ) ) ;
        //CSL->set_DspReg_FromDataStackPointer ( ) ; // update DSP reg
    }
}

void
CSL_BitWise_AND ( ) // xor
{
    if ( CompileMode )
    {
        Compile_X_Group1 ( _Context_->Compiler0, AND, TTT_ZERO, NEGFLAG_NZ ) ;
    }
    else
    {
        _DspReg_ [ - 1 ] = _DspReg_ [ - 1 ] & TOS ;
        DataStack_Drop ( ) ;
    }
}

void
CSL_BitWise_AndEqual ( ) // -=
{
    if ( CompileMode )
    {
        Compile_X_OpEqual ( _Context_->Compiler0, CSL_BitWise_AND ) ; //AND ) ;
    }
    else
    {
        int64 *x, n ;
        n = DataStack_Pop ( ) ;
        x = ( int64* ) DataStack_Pop ( ) ;
        *x = ( * x ) & n ;
        //_DataStack_SetTop ( Dsp, _DataStack_Pop () + _DataStack_GetTop ( Dsp ) ) ;
    }
}

void
CSL_BitWise_XOR ( ) // xor
{
    if ( CompileMode )
    {
        Compile_X_Group1 ( _Context_->Compiler0, XOR, TTT_ZERO, NEGFLAG_NZ ) ;
    }
    else
    {
        _DspReg_ [ - 1 ] = _DspReg_ [ - 1 ] ^ TOS ;
        DataStack_Drop ( ) ;
    }
}

void
CSL_BitWise_XorEqual ( ) // -=
{
    if ( CompileMode )
    {
        Compile_X_OpEqual ( _Context_->Compiler0, CSL_BitWise_XOR ) ; //XOR ) ;
    }
    else
    {
        int64 *x, n ;
        n = DataStack_Pop ( ) ;
        x = ( int64* ) DataStack_Pop ( ) ;
        *x = ( * x ) ^ n ;
        //_DataStack_SetTop ( Dsp, _DataStack_Pop () + _DataStack_GetTop ( Dsp ) ) ;
    }
}

void
CSL_ShiftLeft ( ) // lshift
{
    if ( CompileMode )
    {
        Compile_ShiftLeft ( ) ;
    }
    else
    {
        _DspReg_ [ - 1 ] = _DspReg_ [ - 1 ] << TOS ;
        DataStack_Drop ( ) ;
    }
}

void
CSL_ShiftRight ( ) // rshift
{
    if ( CompileMode )
    {
        Compile_ShiftRight ( ) ;
    }
    else
    {
        _DspReg_ [ - 1 ] = _DspReg_ [ - 1 ] >> TOS ;
        DataStack_Drop ( ) ;
    }
}

void
CSL_ShiftLeft_Equal ( ) // <<=
{
    Compiler * compiler = _Context_->Compiler0 ;
    if ( GetState ( compiler, BLOCK_MODE ) )
    {
        //Compile_X_Shift ( compiler, SHL, 0, 1 ) ;
        Compile_X_OpEqual ( _Compiler_, CSL_ShiftLeft ) ;
    }
    else
    {
        int64 *x, n ;
        n = DataStack_Pop ( ) ;
        x = ( int64* ) DataStack_Pop ( ) ;
        *x = * x << n ;
        //_DataStack_SetTop ( Dsp, _DataStack_Pop () + _DataStack_GetTop ( Dsp ) ) ;
    }
}

void
CSL_ShiftRight_Equal ( ) // >>=
{
    if ( GetState ( _Context_->Compiler0, BLOCK_MODE ) )
    {
        //Compile_X_Shift ( _Context_->Compiler0, SHR, 0, 1 ) ;
        Compile_X_OpEqual ( _Compiler_, CSL_ShiftRight ) ;
    }
    else
    {
        int64 *x, n ;
        n = DataStack_Pop ( ) ;
        x = ( int64* ) DataStack_Pop ( ) ;
        *x = * x >> n ;
        //_DataStack_SetTop ( Dsp, _DataStack_Pop () + _DataStack_GetTop ( Dsp ) ) ;
    }
}





