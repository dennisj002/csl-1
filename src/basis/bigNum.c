#include "../include/csl.h"

mpfr_t *
_BigNum_New ( byte * token )
{
    double bf ;
    long i, bi ;
    mpfr_t *bfr = ( mpfr_t* ) Mem_Allocate ( sizeof ( mpfr_t ), ( CompileMode ? OBJECT_MEM : COMPILER_TEMP ) ) ; //OBJECT_MEM) ) ;
    if ( token )
    {
        for ( i = 0 ; token [i] ; i ++ )
        {
            if ( token [i] == '.' )
            {
                if ( sscanf ( ( char* ) token, "%lf", &bf ) )
                {
                    mpfr_init_set_d ( *bfr, bf, MPFR_RNDN ) ;
                    return bfr ;
                }
                else
                {
                    mpfr_init_set_si ( *bfr, ( long ) 0, MPFR_RNDN ) ;
                    return bfr ;
                }
            }
        }
        if ( sscanf ( ( char* ) token, "%ld", &bi ) ) mpfr_init_set_si ( *bfr, bi, MPFR_RNDN ) ;
        else mpfr_init_set_si ( *bfr, ( long ) 0, MPFR_RNDN ) ;
    }
    else mpfr_init_set_si ( *bfr, ( long ) 0, MPFR_RNDN ) ;
    return bfr ;
}

//"For a, A, e, E, f and F specifiers: this is the number of digits to be printed after the decimal point" 

void
BigNum_GetPrintfPrecision_BigNum ( )
{
    DataStack_Push ( _Context_->System0->BigNum_Printf_Precision ) ; // this precision is used by BigNum_FPrint like printf
}

// width is a parameter to mpfr_printf; it works like printf and sets minimum number of characters to print
// "Minimum number of characters to be printed. If the value to be printed is shorter than this number, 
// the result is padded with blank spaces. The value is not truncated even if the result is larger."

void
BigNum_GetPrintfWidth ( )
{
    DataStack_Push ( ( int64 ) _Context_->System0->BigNum_Printf_Width ) ;
}

// internal mpfr bit precision

void
BigNum_GetAndPrint_BitPrecision ( )
{
    mpfr_prec_t precision = mpfr_get_default_prec ( ) ; // number of decimal digits
    Printf ( "\nBigNum Internal Bit Precision = %ld", precision ) ;
}

// set from within BigNum namespace

void
BigNum_Set_PrintfWidth ( )
{
    int64 width = DataStack_Pop ( ) ;
    _Context_->System0->BigNum_Printf_Width = width ;
}

void
BigNum_Set_PrintfPrecision ( )
{
    int64 precision = DataStack_Pop ( ) ;
    _Context_->System0->BigNum_Printf_Precision = precision ;
}

// set from within BigNum namespace

void
BN_SetDefaultBitPrecision ( long precision )
{
    mpfr_set_default_prec ( precision ) ; // "precision is the number of [bignum internal] bits used to represent the significand of a floating-point number"
}

#if 0

void
_BigNum_SetDefaultBitPrecision ( int64 precision )
{
    //long precision = mpfr_get_si ( *prec, MPFR_RNDN ) ;
    //mpfr_set_default_prec ( precision ) ; // "precision is the number of bits used to represent the significand of a floating-point number"
    BN_SetDefaultBitPrecision ( precision ) ;
}
#endif

void
BigNum_Set_InternalBitPrecision ( )
{
    int64 precision = DataStack_Pop ( ) ; // number of decimal digits
    BN_SetDefaultBitPrecision ( precision ) ;
}

void
BigNum_StateShow ( )
{
    BigNum_Info ( ) ;
    BigNum_GetAndPrint_BitPrecision ( ) ;
    Printf ( ( byte * ) "\nBigNum :: Width = %d : Precision = %d", _Context_->System0->BigNum_Printf_Width, _Context_->System0->BigNum_Printf_Precision ) ;
}

void
_BigNum_Init ( int64 precision )
{
    mpfr_free_cache ( ) ;
    _Context_->System0->BigNum_Printf_Width = 2 ;
    _Context_->System0->BigNum_Printf_Precision = precision ; // default  mpfr precision
    //BN_SetDefaultBitPrecision ( precision ) ;
}

void
BigNum_Init ( )
{
    //int64 w = DataStack_Pop ( ) ;
    int64 p = DataStack_Pop ( ) ;
    _BigNum_Init ( p ) ;
    Namespace_DoNamespace_Name ( ( byte* ) "BigNum" ) ;
}
#if 0

void
BigNum_FPrint ( )
{
    mpfr_t * value = ( mpfr_t* ) DataStack_Pop ( ) ;
    if ( _O_->Verbosity ) mpfr_printf ( "%*.*Rf", _Context_->System0->BigNum_Printf_Width, _Context_->System0->BigNum_Printf_Precision, *value ) ;
    fflush ( stdout ) ;
}

// scientific format

void
BigNum_EPrint ( )
{
    mpfr_t * value = ( mpfr_t* ) DataStack_Pop ( ) ;
    if ( _O_->Verbosity ) mpfr_printf ( "%*.*Re", _Context_->System0->BigNum_Printf_Width, _Context_->System0->BigNum_Printf_Precision, *value ) ;
    fflush ( stdout ) ;
}
#else

void
BigNum_Info ( )
{
    Printf ( "\nMPFR library: %-12s\nMPFR header:  %s (based on %d.%d.%d)",
        mpfr_get_version ( ), MPFR_VERSION_STRING, MPFR_VERSION_MAJOR,
        MPFR_VERSION_MINOR, MPFR_VERSION_PATCHLEVEL ) ;
}

int64
Check_Error_BigNum ( uint64 value )
{
    //if ( _O_->Verbosity > 1 )
    {
        if ( _LC_ && ( ( uint64 ) ( value ) < 1000000000 ) )
        {
            //Error_LC_BigNum ( (uint64) value ) ; // could be true/false
            Printf ( "\nBigNum : Error : LC : value not a BigNum = " UINT_FRMT_0x08 "\n", value ) ;
            return true ;
        }
    }
    return false ;
}

void
_BigNum_FPrint ( mpfr_t * value )
{
    byte * format ;
    if ( _O_->Verbosity )
    {
        if ( ! Check_Error_BigNum ( ( uint64 ) value ) )
        {
            if ( NUMBER_BASE_GET ( ) == 10 ) format = ( byte* ) " %*.*Rf" ;
            else if ( NUMBER_BASE_GET ( ) == 2 ) format = ( byte* ) " %*.*Rb" ;
            else if ( NUMBER_BASE_GET ( ) == 16 ) format = ( byte* ) " %*.*Rx" ;
            mpfr_printf ( ( char* ) format, _Context_->System0->BigNum_Printf_Width, _Context_->System0->BigNum_Printf_Precision, *value ) ;
            fflush ( stdout ) ;
        }
    }
}

void
_BigNum_snfPrint2 ( char * buf, byte * name, mpfr_t * value )
{
    byte * format ;
    if ( _O_->Verbosity )
    {
        if ( ! Check_Error_BigNum ( ( uint64 ) value ) )
        {
            if ( name )
            {
                if ( NUMBER_BASE_GET ( ) == 10 ) format = ( byte* ) " %s = %*.*Rf" ;
                else if ( NUMBER_BASE_GET ( ) == 2 ) format = ( byte* ) " %s = %*.*Rb" ;
                else if ( NUMBER_BASE_GET ( ) == 16 ) format = ( byte* ) " %s = %*.*Rx" ;
                mpfr_sprintf ( buf, format, name, _Context_->System0->BigNum_Printf_Width, _Context_->System0->BigNum_Printf_Precision, *value ) ;
            }
            else
            {
                if ( NUMBER_BASE_GET ( ) == 10 ) format = ( byte* ) " %*.*Rf" ;
                else if ( NUMBER_BASE_GET ( ) == 2 ) format = ( byte* ) " %*.*Rb" ;
                else if ( NUMBER_BASE_GET ( ) == 16 ) format = ( byte* ) " %*.*Rx" ;
                mpfr_sprintf ( buf, format, _Context_->System0->BigNum_Printf_Width, _Context_->System0->BigNum_Printf_Precision, *value ) ;
            }
            fflush ( stdout ) ;
        }
    }
}

void
BigNum_FPrint ( )
{
    _BigNum_FPrint ( ( mpfr_t* ) DataStack_Pop ( ) ) ;
}

#if 0

void
BigNum_FPrint2 ( )
{
    Context * cntx = _Context_ ;
    char * format ;
    mpfr_t * value = ( mpfr_t* ) DataStack_Pop ( ) ;
    if ( _O_->Verbosity )
    {
#if 1        
        if ( NUMBER_BASE_GET ( ) == 10 ) format = "%*.*Rf" ;
        else if ( NUMBER_BASE_GET ( ) == 2 ) format = "%*.*Rb" ;
        else if ( NUMBER_BASE_GET ( ) == 16 ) format = "%*.*Rx" ;
        mpfr_printf ( format, _Context_->System0->BigNum_Printf_Width, _Context_->System0->BigNum_Printf_Precision, *value ) ;
#else
        mpfr_printf ( "%*.*Rf", 16, 16, *value ) ;
#endif        
    }
    fflush ( stdout ) ;
}
#endif
// scientific format

void
BigNum_EPrint ( )
{
    //Set_SCA ( 0 ) ; // this is not compiled
    mpfr_t * value = ( mpfr_t* ) DataStack_Pop ( ) ;
    if ( ! Check_Error_BigNum ( ( uint64 ) value ) )
    {
        if ( _O_->Verbosity ) mpfr_printf ( "%*.*Re", _Context_->System0->BigNum_Printf_Width, _Context_->System0->BigNum_Printf_Precision, *value ) ;
        fflush ( stdout ) ;
    }
}
#endif

void
BigNum_PopTwoOperands_PushFunctionResult ( mpf2andOutFunc func )
{
    //Set_SCA ( 0 ) ; // this is not compiled
    mpfr_t *result = _BigNum_New ( 0 ) ;
    mpfr_t *op1 = ( mpfr_t* ) DataStack_Pop ( ) ;
    mpfr_t * op2 = ( mpfr_t* ) DataStack_Pop ( ) ;
    func ( *result, *op2, *op1, MPFR_RNDN ) ;
    d0 ( _CSL_->SaveSelectedCpuState ( ) ) ;
    DataStack_Push ( ( int64 ) result ) ;
}

#define USE_FUNCTION 1

void
BigNum_Add ( )
{
    BigNum_PopTwoOperands_PushFunctionResult ( mpfr_add ) ;
}

void
BigNum_Multiply ( )
{
    BigNum_PopTwoOperands_PushFunctionResult ( mpfr_mul ) ;
}

void
BigNum_Divide ( )
{
    BigNum_PopTwoOperands_PushFunctionResult ( mpfr_div ) ;
}

void
BigNum_Subtract ( )
{
    BigNum_PopTwoOperands_PushFunctionResult ( mpfr_sub ) ;
}

void
_BigNum_OpEqualTemplate ( mpf2andOutFunc func )
{
    //Set_SCA ( 0 ) ; // this is not compiled
    mpfr_t *result = _BigNum_New ( 0 ) ;
    mpfr_t * op2 = ( mpfr_t* ) DataStack_Pop ( ) ;
    mpfr_t **p_op1 = ( mpfr_t** ) DataStack_Pop ( ) ;
    func ( *result, **p_op1, *op2, MPFR_RNDN ) ;
    *p_op1 = result ;
    d0 ( _CSL_->SaveSelectedCpuState ( ) ) ;
}

void
BigNum_PlusEqual ( )
{
    _BigNum_OpEqualTemplate ( mpfr_add ) ;
}

void
BigNum_MinusEqual ( )
{
    _BigNum_OpEqualTemplate ( mpfr_sub ) ;
}

void
BigNum_MultiplyEqual ( )
{
    _BigNum_OpEqualTemplate ( mpfr_mul ) ;
}

void
BigNum_DivideEqual ( ) // remainder discarded
{
    _BigNum_OpEqualTemplate ( mpfr_div ) ;
}

// ++

void
BigNum_PlusPlus ( )
{
    //Set_SCA ( 0 ) ; // this is not compiled
    mpfr_t *sum = _BigNum_New ( 0 ) ;
    mpfr_t * op1 = ( mpfr_t* ) _DataStack_GetTop ( ), *op2 = ( mpfr_t* ) _BigNum_New ( ( byte* ) "1" ) ;
    mpfr_add ( *sum, *op1, *op2, MPFR_RNDN ) ;
    _DataStack_SetTop ( ( int64 ) sum ) ;
}

void
BigNum_MinusMinus ( )
{
    //Set_SCA ( 0 ) ; // this is not compiled
    mpfr_t *sum = _BigNum_New ( 0 ) ;
    mpfr_t * op1 = ( mpfr_t* ) _DataStack_GetTop ( ), *op2 = ( mpfr_t* ) _BigNum_New ( ( byte* ) "1" ) ;
    mpfr_sub ( *sum, *op1, *op2, MPFR_RNDN ) ;
    _DataStack_SetTop ( ( int64 ) sum ) ;
}

void
BigNum_SquareRoot ( )
{
    //Set_SCA ( 0 ) ; // this is not compiled
    mpfr_t *rop = _BigNum_New ( 0 ) ;
    mpfr_t * op1 = ( mpfr_t* ) DataStack_Pop ( ) ;
    mpfr_sqrt ( *rop, *op1, MPFR_RNDN ) ;
    DataStack_Push ( ( int64 ) rop ) ;
}

void
BigNum_Power ( )
{
    //Set_SCA ( 0 ) ; // this is not compiled
    mpfr_t *rop = _BigNum_New ( 0 ) ;
    mpfr_t * expf = ( mpfr_t* ) DataStack_Pop ( ) ;
    mpfr_t * op1 = ( mpfr_t* ) DataStack_Pop ( ) ;
    mpfr_pow ( *rop, *op1, *expf, MPFR_RNDN ) ;
    DataStack_Push ( ( int64 ) rop ) ;
}

// returns op1 - op2

int64
BigNum_Cmp ( )
{
    //Set_SCA ( 0 ) ; // this is not compiled
    mpfr_t * op2 = ( mpfr_t* ) DataStack_Pop ( ) ;
    mpfr_t *op1 = ( mpfr_t* ) DataStack_Pop ( ) ;
    return mpfr_cmp ( *op1, *op2 ) ;
}

// op1 < op2 => (op1 - op2 < 0 )

void
BigNum_LessThan ( )
{
    DataStack_Push ( ( BigNum_Cmp ( ) < 0 ) ? 1 : 0 ) ;
}

// op1 <= op2 => (op1 - op2 <= 0 )

void
BigNum_LessThanOrEqual ( )
{
    DataStack_Push ( ( BigNum_Cmp ( ) <= 0 ) ? 1 : 0 ) ;
}

// op1 > op2 => (op1 - op2 > 0 )

void
BigNum_GreaterThan ( )
{
    DataStack_Push ( BigNum_Cmp ( ) > 0 ? 1 : 0 ) ;
}

// op1 >= op2 => (op1 - op2 >= 0 )

void
BigNum_GreaterThanOrEqual ( )
{
    DataStack_Push ( ( BigNum_Cmp ( ) >= 0 ) ? 1 : 0 ) ;
}

void
BigNum_LogicalEquals ( )
{
    DataStack_Push ( BigNum_Cmp ( ) == 0 ? 1 : 0 ) ;
}

void
BigNum_LogicalDoesNotEqual ( )
{
    DataStack_Push ( BigNum_Cmp ( ) == 0 ? 0 : 1 ) ;
}

