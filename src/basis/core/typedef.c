
#include "../../include/csl.h"


// SEMI :: ';'
// NUMERIC :: DIGIT *
// _ID :: ALPHA_NUMERIC *
// TYPE_NAME :: _ID
// PTR :: '*' _ID
// ID :: _ID | PTR
// ARRAY :: ('[' NUMERIC ] ']')*
// ARRAY_FIELD :: ID ARRAY
// ARRAY_FIELDS :: ARRAY_FIELD (',' ARRAY_FIELD)*
// _ID_OR_ARRAY_FIELD : ID | ARRAY_FIELD
// ID_FIELDS : _ID_OR_ARRAY_FIELD ( ',' _ID_OR_ARRAY_FIELD )*
// TYPE_FIELD :: TYPE_NAME ID_FIELDS
// STRUCT_DEF :: '{' FIELD* '}'
// STRUCT_UNION_DEF :: _ID STRUCT_DEF | _ID STRUCT_DEF ID_FIELDS | STRUCT_DEF ID_FIELDS 
// STRUCT_UNION_FIELD :: TYPE_FIELD | STRUCT_UNION_DEF
// STRUCT_UNION :: ( 'struct' | 'union' ) STRUCT_UNION_FIELD
// FIELD :: STRUCT_UNION | TYPE_FIELD 
// TYPEDEF :: 'typedef' FIELD SEMI
// TYPE :: 'type' FIELD SEMI
// TYPED_FIELD :: TYPEDEF | TYPE

// we have read the idField : identifier = token

void
Parse_Identifier ( int64 t_type )
{
    TDSCI * tdsci = TDSCI_GetTop ( ) ;
    byte *identifier = tdsci->TdsciToken ;
    Word * id, *addToNs ;

#if 1 // debug
    if ( String_Equal ( identifier, "n_Attributes" ) )
        //if ( String_Equal ( identifier, "n_InUseFlag" ) ) //"T_ChunkSize" ) )
        Printf ( "" ) ;
#endif    
    //A_Parser_Debug ( ) ;
    tdsci->FieldName = identifier ;
    if ( GetState ( tdsci, TDSCI_PRINT ) )
    {
        if ( t_type == TD_TYPE_FIELD ) TDSCI_Print_Field ( t_type, tdsci->Tdsci_Field_Size ) ;
    }
    else if ( t_type == TD_TYPE_FIELD )
    {
        // with a nameless struct or union identifier are added to the background namespace
        addToNs = ( tdsci->Tdsci_StructureUnion_Namespace && tdsci->Tdsci_StructureUnion_Namespace->Name ) ?
            tdsci->Tdsci_StructureUnion_Namespace : tdsci->Tdsci_InNamespace ;
        tdsci->Tdsci_Field_Object = id = DataObject_New ( TD_TYPE_FIELD, 0, identifier, 0, 0, 0, tdsci->Tdsci_Offset, tdsci->Tdsci_Field_Size, addToNs, 0, 0, 0 ) ;
        TypeNamespace_Set ( id, tdsci->Tdsci_Field_Type_Namespace ) ;
        if ( GetState ( tdsci, TDSCI_POINTER ) )
        {
            id->W_ObjectAttributes |= T_POINTER ;
        }
        Class_Size_Set ( id, tdsci->Tdsci_Field_Size ) ; //+=  ( tdsci->Tdsci_Field_Size % 8 ) ) ;
        id->Offset = tdsci->Tdsci_Offset ;
    }
    else if ( t_type == PRE_STRUCTURE_ID )
    {
        tdsci->Tdsci_StructureUnion_Namespace = id = DataObject_New ( CLASS, 0, identifier, 0, 0, 0, 0, 0, 0, 0, 0, - 1 ) ;
        id->W_ObjectAttributes |= ( STRUCT | NAMESPACE ) ; //??
        id->Offset = tdsci->Tdsci_Offset ;
        tdsci->StructureUnionName = identifier ;

    }
    else if ( t_type == POST_STRUCTURE_ID ) //&& GetState ( tdsci, TDSCI_STRUCTURE_COMPLETED ) )
    {
        if ( tdsci->Tdsci_StructureUnion_Namespace ) id = Parse_Do_IdentifierAlias ( identifier, tdsci->Tdsci_StructureUnion_Size ) ;
        else tdsci->Tdsci_StructureUnion_Namespace = id = DataObject_New ( CLASS, 0, identifier, 0, 0, 0, 0, 0, tdsci->Tdsci_InNamespace, 0, - 1, - 1 ) ;
        Class_Size_Set ( id, tdsci->Tdsci_StructureUnion_Size ) ; //+= ( tdsci->Tdsci_StructureUnion_Size % 8 ) ) ;
        id->W_ObjectAttributes |= ( STRUCT | NAMESPACE ) ; //??
        id->Offset = tdsci->Tdsci_Offset ; //- tdsci->Tdsci_StructureUnion_Size ; // ??
        _CSL_Set_WordSourceCode ( _CSL_, tdsci->Tdsci_StructureUnion_Namespace, 1 ) ;
        //SetState ( tdsci, TDSCI_POST_STRUCT, true ) ;
    }
    else CSL_Parse_Error ( "Parse_Identifier : No identifier type given", identifier ) ;

    if ( ( _O_->Verbosity > 1 ) && ( ! GetState ( tdsci, TDSCI_PRINT ) ) ) TDSCI_DebugPrintWord ( id ) ; // print class field
    //A_Parser_Debug ( ) ;
}

void
Parse_Array ( )
{
    TDSCI * tdsci = TDSCI_GetTop ( ) ;
    int64 arrayDimensions [ 32 ] ; // 32 : max dimensions for now
    int64 size = tdsci->Tdsci_Field_Size, i, arrayDimensionSize ;
    byte *token = tdsci->TdsciToken ;
    memset ( arrayDimensions, 0, sizeof (arrayDimensions ) ) ;

    for ( i = 0 ; 1 ; i ++ )
    {
        if ( token && ( token[0] == '[' ) )
        {
            CSL_InterpretNextToken ( ) ; // next token must be an integer for the array dimension size
            arrayDimensionSize = DataStack_Pop ( ) ;
            size = size * arrayDimensionSize ;
            tdsci->Tdsci_Field_Size = size ;
            token = TDSCI_ReadToken ( ) ;
            if ( ! String_Equal ( ( char* ) token, "]" ) ) CSL_Exception ( SYNTAX_ERROR, 0, 1 ) ;
            else arrayDimensions [ i ] = arrayDimensionSize ;
            token = TDSCI_ReadToken ( ) ;
        }
        else
        {
            if ( GetState ( tdsci, TDSCI_PRINT ) )
            {
                __CSL_Dump ( &tdsci->DataPtr [ tdsci->Tdsci_Offset ], tdsci->Tdsci_Field_Size, 16 ) ;
            }
            else if ( i )
            {
                tdsci->Tdsci_Field_Object->ArrayDimensions = ( int64 * ) Mem_Allocate ( i * sizeof (int64 ), DICTIONARY ) ; //tdsci->Tdsci_Field_Size, DICTIONARY ) ;
                MemCpy ( tdsci->Tdsci_Field_Object->ArrayDimensions, arrayDimensions, i * sizeof (int64 ) ) ; //tdsci->Tdsci_Field_Size ) ;
                tdsci->Tdsci_Field_Object->ArrayNumberOfDimensions = i ;
            }
            break ;
        }
    }
}

Boolean
Parse_TypeNamespace_CheckForPointer ( int64 t_type )
{
    Boolean rtrn = false ;
    if ( t_type == TD_TYPE_FIELD )
    {
        Context * cntx = _Context_ ;
        TDSCI * tdsci = TDSCI_GetTop ( ) ;
        Namespace * type0 ;
        byte * token = tdsci->TdsciToken ;
        if ( String_Equal ( token, "struct" ) || String_Equal ( token, "union" ) ) token = TDSCI_ReadToken ( ) ;

        if ( token [0] != '*' )
        {
            type0 = _Namespace_Find ( token, 0, 0 ) ;
            if ( type0 )
            {
                tdsci->Tdsci_Field_Type_Namespace = type0 ;
                tdsci->Tdsci_Field_Size = CSL_Get_Namespace_SizeVar_Value ( type0 ) ;
                token = TDSCI_ReadToken ( ) ;
            }
        }
        if ( ( token [0] == '*' ) || ( ( token = Lexer_Peek_Next_NonDebugTokenWord ( cntx->Lexer0, 0 ) ), ( token [0] == '*' ) ) )
        {
            if ( ( ! type0 ) && ( t_type == TD_TYPE_FIELD ) && ( ! tdsci->Tdsci_Field_Type_Namespace ) )
            {
                // for forward reference 
                tdsci->Tdsci_Field_Type_Namespace = DataObject_New ( CLASS, 0, token, 0, 0, 0, 0, 0, 0, 0, 0, - 1 ) ;
                token = TDSCI_ReadToken ( ) ;
            }
            tdsci->Tdsci_Field_Size = CELL ;
            SetState ( tdsci, TDSCI_POINTER, true ) ;
            token = TDSCI_ReadToken ( ) ; // the '*'
            rtrn = true ;
        }
        else rtrn = false ;
        token = tdsci->TdsciToken ; // debugging 
    }
    return rtrn ;
}

void
Parse_Identifier_Or_Array_Field ( int64 t_type )
{
    TDSCI * tdsci = TDSCI_GetTop ( ) ;
    byte *token = tdsci->TdsciToken ;
    if ( token && ( token [0] != ';' ) )
    {
        Parse_TypeNamespace_CheckForPointer ( t_type ) ;
        Parse_Identifier ( t_type ) ;
        token = TDSCI_ReadToken ( ) ;
        if ( token && ( token [0] == '[' ) ) Parse_Array ( ) ;
        if ( t_type == TD_TYPE_FIELD ) Parse_IntraStructFieldAccounting ( ) ;
        SetState ( tdsci, TDSCI_POINTER, false ) ;
    }
}

void
Parse_Identifier_Fields ( int64 t_type )
{
    TDSCI * tdsci = TDSCI_GetTop ( ) ;
    byte * token ;
    while ( 1 )
    {
        Parse_Identifier_Or_Array_Field ( t_type ) ;
        if ( tdsci->TdsciToken )
        {
            if ( tdsci->TdsciToken[0] == ',' ) token = TDSCI_ReadToken ( ) ;
            if ( tdsci->TdsciToken[0] == ';' ) break ; //{ token = TDSCI_ReadToken ( ) ; break ; }
        }
        else break ;
    }
}

// SEMI :: ';'
// NUMERIC :: DIGIT *
// _ID :: ALPHA_NUMERIC *
// TYPE_NAME :: _ID
// PTR :: '*' _ID
// ID :: _ID | PTR
// ARRAY :: ('[' NUMERIC ] ']')*
// ARRAY_FIELD :: ID ARRAY
// ARRAY_FIELDS :: ARRAY_FIELD (',' ARRAY_FIELD)*
// _ID_OR_ARRAY_FIELD : ID | ARRAY_FIELD
// ID_FIELDS : _ID_OR_ARRAY_FIELD ( ',' _ID_OR_ARRAY_FIELD )*
// TYPE_FIELD :: TYPE_NAME ID_FIELDS
// STRUCT_DEF :: '{' FIELD* '}'
// STRUCT_UNION_DEF :: _ID STRUCT_DEF | _ID STRUCT_DEF ID_FIELDS | STRUCT_DEF ID_FIELDS 
// STRUCT_UNION_FIELD :: TYPE_FIELD | STRUCT_UNION_DEF
// STRUCT_UNION :: ( 'struct' | 'union' ) STRUCT_UNION_FIELD
// FIELD :: STRUCT_UNION | TYPE_FIELD 
// TYPEDEF :: 'typedef' FIELD SEMI
// TYPE :: 'type' FIELD SEMI
// TYPED_FIELD :: TYPEDEF | TYPE

void
Parse_Type_Field ( )
{
    TDSCI * tdsci = ( TDSCI * ) Stack_Top ( _Compiler_->TDSCI_StructUnionStack ) ;
    byte * token = tdsci->TdsciToken ;
    Parse_Identifier_Fields ( TD_TYPE_FIELD ) ;
    if ( ! tdsci->Tdsci_Field_Type_Namespace ) CSL_Parse_Error ( "Parse_Type_Field : No type field namespace.", token ) ;
}
// '{' token may or may not have already been parsed

void
Parse_StructUnionDef ( ) // 'struct' or 'union'
{
    TDSCI * tdsci = TDSCI_GetTop ( ) ;
    byte * token = tdsci->TdsciToken ;
    if ( ! token ) token = TDSCI_ReadToken ( ) ; //nb! for class ":{" 
    if ( String_Equal ( token, "struct" ) || String_Equal ( token, "union" ) )
    {
        token = TDSCI_ReadToken ( ) ;
    }
    if ( ( token [0] == '{' ) || ( token[1] == '{' ) || ( token[2] == '{' ) ) TDSCI_ReadToken ( ) ; // consider ":{" and +:{" tokens
    do
    {
        Parse_Field ( ) ;
        tdsci = TDSCI_GetTop ( ) ;
        if ( tdsci->TdsciToken && ( tdsci->TdsciToken [0] == ';' ) ) TDSCI_ReadToken ( ) ;
    }
    while ( tdsci->TdsciToken && ( tdsci->TdsciToken [0] != '}' ) ) ;

    //if ( tdsci->TdsciToken[1] != ';' ) 
    token = TDSCI_ReadToken ( ) ; //Parse_Identifier ( POST_STRUCTURE_ID ) ;// handled by TDSCI_Finalize ( ) ;
    if ( token && ( token[0] != ';' ) ) Parse_Post_Struct_Identifiers ( ) ;
    //else TDSCI_ReadToken ( ) ; 
}

// SEMI :: ';'
// NUMERIC :: DIGIT *
// _ID :: ALPHA_NUMERIC *
// TYPE_NAME :: _ID
// PTR :: '*' _ID
// ID :: _ID | PTR
// ARRAY :: ('[' NUMERIC ] ']')*
// ARRAY_FIELD :: ID ARRAY
// ARRAY_FIELDS :: ARRAY_FIELD (',' ARRAY_FIELD)*
// _ID_OR_ARRAY_FIELD : ID | ARRAY_FIELD
// ID_FIELDS : _ID_OR_ARRAY_FIELD ( ',' _ID_OR_ARRAY_FIELD )*
// TYPE_FIELD :: TYPE_NAME ID_FIELDS
// STRUCT_DEF :: '{' FIELD* '}'
// STRUCT_UNION_DEF :: _ID STRUCT_DEF | _ID STRUCT_DEF ID_FIELDS | STRUCT_DEF ID_FIELDS 
// STRUCT_UNION_FIELD :: TYPE_FIELD | STRUCT_UNION_DEF
// STRUCT_UNION :: ( 'struct' | 'union' ) STRUCT_UNION_FIELD
// FIELD :: STRUCT_UNION | TYPE_FIELD 
// TYPEDEF :: 'typedef' FIELD SEMI
// TYPE :: 'type' FIELD SEMI
// TYPED_FIELD :: TYPEDEF | TYPE

// 'struct' or 'union' tokens should have already been parsed

void
Parse_StructOrUnion_Field ( Namespace * ns, int64 structOrUnionTypeFlag )
{
    TDSCI * tdsci = Parse_PreStruct_Accounting ( ns, structOrUnionTypeFlag ) ;
    byte * token = tdsci->TdsciToken ; //( tdsci->TdsciToken ? tdsci->TdsciToken : TDSCI_ReadToken ( ) ) ;
    if ( token )
    {
        if ( ( token [0] != '{' ) && ( token[1] != '{' ) )
        {
            Parse_Identifier ( PRE_STRUCTURE_ID ) ;
            token = TDSCI_ReadToken ( ) ;
        }
        Parse_StructUnionDef ( ) ;
    }
    Parse_PostStructUnion_Accounting ( ) ;
    SetState ( tdsci, TDSCI_UNION | TDSCI_STRUCT, false ) ;
}

// SEMI :: ';'
// NUMERIC :: DIGIT *
// _ID :: ALPHA_NUMERIC *
// TYPE_NAME :: _ID
// PTR :: '*' _ID
// ID :: _ID | PTR
// ARRAY :: ('[' NUMERIC ] ']')*
// ARRAY_FIELD :: ID ARRAY
// ARRAY_FIELDS :: ARRAY_FIELD (',' ARRAY_FIELD)*
// _ID_OR_ARRAY_FIELD : ID | ARRAY_FIELD
// ID_FIELDS : _ID_OR_ARRAY_FIELD ( ',' _ID_OR_ARRAY_FIELD )*
// TYPE_FIELD :: TYPE_NAME ID_FIELDS
// STRUCT_DEF :: '{' FIELD* '}'
// STRUCT_UNION_DEF :: _ID STRUCT_DEF | _ID STRUCT_DEF ID_FIELDS | STRUCT_DEF ID_FIELDS 
// STRUCT_UNION_FIELD :: TYPE_FIELD | STRUCT_UNION_DEF
// STRUCT_UNION :: ( 'struct' | 'union' ) STRUCT_UNION_FIELD
// FIELD :: STRUCT_UNION | TYPE_FIELD 
// TYPEDEF :: 'typedef' FIELD SEMI
// TYPE :: 'type' FIELD SEMI
// TYPED_FIELD :: TYPEDEF | TYPE

void
Parse_Field ( )
{
    TDSCI * tdsci = TDSCI_GetTop ( ) ;
    Namespace * type0 ;
    int64 structOrUnionTypeFlag = 0 ;
    byte *token1, * token = tdsci->TdsciToken ? tdsci->TdsciToken : TDSCI_ReadToken ( ) ;
    if ( token && token[0] != ';' )
    {
        if ( String_Equal ( ( char* ) token, "struct" ) ) structOrUnionTypeFlag = TDSCI_STRUCT ;
        else if ( String_Equal ( ( char* ) token, "union" ) ) structOrUnionTypeFlag = TDSCI_UNION ;
        if ( ( structOrUnionTypeFlag & TDSCI_UNION ) && GetState ( tdsci, TDSCI_PRINT ) )
            Printf ( "\n%s", token ) ;
        if ( structOrUnionTypeFlag && ( ! ( Parse_TypeNamespace_CheckForPointer ( TD_TYPE_FIELD ) ) ) )
        {
            token1 = tdsci->TdsciToken ;
            if ( ( token1[0] == '{' ) || ( ! ( type0 = _Namespace_Find ( token1, 0, 0 ) ) || ( GetState ( tdsci, TDSCI_PRINT ) ) ) )
            {
                Parse_StructOrUnion_Field ( 0, structOrUnionTypeFlag ) ;
                //return ;
            }
        }
        else Parse_Type_Field ( ) ;
    }
}

int64
CSL_Parse_A_Typed_Field ( )
{
    Word * word = _Context_->CurrentEvalWord ;
    TDSCI * tdsci = TDSCI_Start ( word, 0, 0 ) ;
    Parse_Field ( ) ;
    //if ( tdsci->TdsciToken[0] == ';' ) TDSCI_ReadToken ( ) ;
    //tdsci = 
    TDSCI_Finalize ( ) ;

    return tdsci->Tdsci_StructureUnion_Size ;
}

// Struct : in functions here refers to struct or union types

TDSCI *
Parse_PreStruct_Accounting ( Namespace * ns, int64 structUnionTypeFlag )
{
    TDSCI * ctdsci = 0, *ntdsci ;
    Context * cntx = _Context_ ;
    SetState ( cntx->Compiler0, TDSCI_PARSING, true ) ;
    //int64 depth = Stack_Depth ( _CONTEXT_TDSCI_STACK ) ;
    ctdsci = TDSCI_GetTop ( ) ; // current -> previous
    ntdsci = TDSCI_Push_New ( ) ; // adding one to the Stack - new current
    if ( ns ) ntdsci->Tdsci_StructureUnion_Namespace = ns ;
    if ( ctdsci ) // previous tdsci - Tdsci_PreStructureUnion_Namespace
    {
        ntdsci->Tdsci_Offset = 0 ; //ctdsci->Tdsci_Offset ;
        ntdsci->TdsciToken = ctdsci->TdsciToken ;
        ntdsci->State = ctdsci->State & ( ~ ( TDSCI_UNION | TDSCI_STRUCT ) ) ; // transfer the non - struct/union state only
        ntdsci->State |= structUnionTypeFlag ;
        if ( GetState ( ctdsci, TDSCI_UNION ) ) ntdsci->DataPtr = ctdsci->DataPtr ;
        else ntdsci->DataPtr = & ctdsci->DataPtr [ ctdsci->Tdsci_Offset ] ;
        //if ( GetState ( ctdsci, TDSCI_STRUCT ) ) ntdsci->DataPtr = & ctdsci->DataPtr [ ctdsci->Tdsci_Offset ] ;
        //if ( GetState ( ntdsci, TDSCI_UNION ) ) ntdsci->DataPtr = & ctdsci->DataPtr [ ctdsci->Tdsci_Offset ] ;
        //if ( GetState ( ntdsci, TDSCI_STRUCT ) ) ntdsci->DataPtr = & ctdsci->DataPtr [ ctdsci->Tdsci_Offset ] ;
        ntdsci->Tdsci_InNamespace = ctdsci->Tdsci_StructureUnion_Namespace ? ctdsci->Tdsci_StructureUnion_Namespace : ctdsci->Tdsci_InNamespace ; //_CSL_Namespace_InNamespaceGet ( ) ;
        if ( ! ntdsci->Tdsci_StructureUnion_Namespace ) ntdsci->Tdsci_StructureUnion_Namespace = ctdsci->Tdsci_StructureUnion_Namespace ;
    }
    ntdsci->State |= structUnionTypeFlag ;

    return ntdsci ;
}

void
_Parse_PostStructUnion_Accounting ( TDSCI * jf_tdsci )
{
    TDSCI * sctdsci ;
    int64 offset = 0, sd ;
    do
    {
        if ( ( sd = Stack_Depth ( _CONTEXT_TDSCI_STACK ) ) > offset )
        {
            sctdsci = TDSCI_Pick ( offset ++ ) ;
            if ( sctdsci && GetState ( sctdsci, TDSCI_UNION ) )
            {
                if ( jf_tdsci->Tdsci_StructureUnion_Size > sctdsci->Tdsci_StructureUnion_Size )
                    sctdsci->Tdsci_StructureUnion_Size = jf_tdsci->Tdsci_StructureUnion_Size ;
            }
            else break ;
        }
        else break ;
    }
    while ( sctdsci ) ;
}

void
Parse_PostStructUnion_Accounting ( )
{
    TDSCI * jf_tdsci = 0, *ctdsci ;
    int64 depth = Stack_Depth ( _CONTEXT_TDSCI_STACK ) ;
    if ( depth > 1 ) jf_tdsci = TDSCI_Pop ( ) ; // just finished TDSCI
    ctdsci = TDSCI_GetTop ( ) ; // now 'current'
    //A_Parser_Debug ( ) ;
    if ( jf_tdsci && ( ! GetState ( ctdsci, TDSCI_POST_STRUCT ) ) ) // always should be there but check anyway
    {
        if ( GetState ( jf_tdsci, TDSCI_UNION ) ) //|| GetState ( ctdsci, TDSCI_UNION ) )
        {
            if ( GetState ( ctdsci, TDSCI_UNION ) ) _Parse_PostStructUnion_Accounting ( jf_tdsci ) ;
            else ctdsci->Tdsci_StructureUnion_Size += jf_tdsci->Tdsci_StructureUnion_Size ;
            if ( GetState ( ctdsci, TDSCI_PRINT ) ) Printf ( "\n" ) ;
        }
        else if ( GetState ( jf_tdsci, TDSCI_STRUCT ) )
        {
            if ( GetState ( ctdsci, TDSCI_UNION ) ) _Parse_PostStructUnion_Accounting ( jf_tdsci ) ;
            else ctdsci->Tdsci_StructureUnion_Size += jf_tdsci->Tdsci_StructureUnion_Size ;
        }
        ctdsci->Tdsci_Structure_Size = ctdsci->Tdsci_StructureUnion_Size ;
        if ( GetState ( ctdsci, TDSCI_UNION ) ) ctdsci->Tdsci_Offset = 0 ;
        else ctdsci->Tdsci_Offset = ctdsci->Tdsci_StructureUnion_Size ;
        ctdsci->TdsciToken = jf_tdsci->TdsciToken ;
        if ( ! ctdsci->Tdsci_StructureUnion_Namespace ) ctdsci->Tdsci_StructureUnion_Namespace = jf_tdsci->Tdsci_StructureUnion_Namespace ;
    }
    //A_Parser_Debug ( ) ;
}

void
Parse_IntraStructFieldAccounting ( )
{
    TDSCI * tdsci = TDSCI_GetTop ( ) ; //, *one = TDSCI_Pick ( 1 ) ;
    //A_Parser_Debug ( ) ;
    if ( ! GetState ( tdsci, TDSCI_POST_STRUCT ) )
    {
        if ( GetState ( tdsci, TDSCI_UNION ) )
        {
            if ( tdsci->Tdsci_Field_Size > tdsci->Tdsci_StructureUnion_Size ) tdsci->Tdsci_StructureUnion_Size = tdsci->Tdsci_Field_Size ;
            tdsci->Tdsci_Offset = 0 ;
        }
        else // not TDSCI_UNION // field may be a struct
        {
            tdsci->Tdsci_StructureUnion_Size += tdsci->Tdsci_Field_Size ;
            tdsci->Tdsci_Offset = tdsci->Tdsci_StructureUnion_Size ;
        }
        SetState ( tdsci, TDSCI_POINTER, false ) ;
    }
    //A_Parser_Debug ( ) ;
}

void
TDSCI_Init ( TypeDefStructCompileInfo * tdsci )
{
    //tdsci->State = 0 ;
    tdsci->Tdsci_InNamespace = _CSL_Namespace_InNamespaceGet ( ) ;
}

TypeDefStructCompileInfo *
TypeDefStructCompileInfo_New ( uint64 allocType )
{
    TypeDefStructCompileInfo * tdsci = ( TypeDefStructCompileInfo * ) Mem_Allocate ( sizeof (TypeDefStructCompileInfo ), allocType ) ;
    TDSCI_Init ( tdsci ) ;
    return tdsci ;
}

TypeDefStructCompileInfo *
TDSCI_Push_New ( )
{
    TypeDefStructCompileInfo *tdsci = TypeDefStructCompileInfo_New ( COMPILER_TEMP ) ;
    Stack_Push ( _CONTEXT_TDSCI_STACK, ( int64 ) tdsci ) ;
    return tdsci ;
}

TDSCI *
TDSCI_Start ( Word * word, byte* objectBitData, int64 stateFlags )
{
    Context * cntx = _Context_ ;
    TypeDefStructCompileInfo * tdsci ;
    TDSCI_STACK_INIT ( ) ;
    tdsci = TDSCI_Push_New ( ) ;
    if ( stateFlags & TDSCI_CLONE_FLAG )
    {
        tdsci->Tdsci_StructureUnion_Namespace = word ;
        tdsci->Tdsci_Offset = _Namespace_VariableValueGet ( tdsci->Tdsci_InNamespace, ( byte* ) "size" ) ; // allows for cloning - prototyping
        tdsci->Tdsci_StructureUnion_Size = tdsci->Tdsci_Offset ;
    }
    tdsci->State |= stateFlags ;
    tdsci->DataPtr = objectBitData ;
    SetState ( _Compiler_, TDSCI_PARSING, true ) ;
    Lexer_SetTokenDelimiters ( cntx->Lexer0, ( byte* ) " ,\n\r\t", COMPILER_TEMP ) ;
    if ( ! ( stateFlags & TDSCI_PRINT ) )
    {
        CSL_Lexer_SourceCodeOn ( ) ;
        CSL_InitSourceCode_WithName ( _CSL_, word ? word->Name : 0, 1 ) ;
    }
    return tdsci ;
}

void
Parse_Post_Struct_Identifiers ( )
{
    TDSCI * tdsci = TDSCI_GetTop ( ) ;
    SetState ( tdsci, TDSCI_POST_STRUCT, true ) ;
    byte * token = tdsci->TdsciToken ; //TDSCI_ReadToken ( ) ; //tdsci->TdsciToken ;
    if ( token && ( token[0] != ';' ) && ( ! String_Equal ( token, "struct" ) ) && ( ! String_Equal ( token, "union" ) ) )
        Parse_Identifier_Fields ( POST_STRUCTURE_ID ) ;
    SetState ( tdsci, TDSCI_POST_STRUCT, false ) ;
}

TDSCI *
TDSCI_Finalize ( )
{
    //Parse_Post_Struct_Identifiers ( ) ;
    TDSCI * tdsci = TDSCI_GetTop ( ), * one = TDSCI_Pick ( 1 ) ; // top was dummy //TDSCI_GetTop ( ) ;
    Namespace * ns = tdsci->Tdsci_StructureUnion_Namespace ? tdsci->Tdsci_StructureUnion_Namespace : one->Tdsci_StructureUnion_Namespace ;
    CSL_Finish_WordSourceCode ( _CSL_, ns, 1 ) ;
    if ( ! GetState ( tdsci, TDSCI_PRINT ) ) Class_Size_Set ( ns ? ns : tdsci->Tdsci_InNamespace, tdsci->Tdsci_StructureUnion_Size ) ;
    SetState ( _Compiler_, TDSCI_PARSING, false ) ;
    return 0 ; //tdsci ; // should return the initial tdsci from TDSCI_Start with the final fields filled in
}

TDSCI *
TDSCI_GetTop ( )
{
    TDSCI * tdsci ;
    return tdsci = ( TDSCI * ) Stack_Top ( _CONTEXT_TDSCI_STACK ) ;
}

TDSCI *
TDSCI_Pick ( int64 offset )
{
    TDSCI * tdsci ;
    return tdsci = ( TDSCI * ) _Stack_Pick ( _CONTEXT_TDSCI_STACK, offset ) ;
}

TDSCI *
TDSCI_Pop ( )
{
    TDSCI * tdsci ;
    return tdsci = ( TDSCI* ) Stack_Pop ( _CONTEXT_TDSCI_STACK ) ;
}

void
TDSCI_STACK_INIT ( )
{
    Stack_Init ( _CONTEXT_TDSCI_STACK ) ;
}

void
TDSCI_Print_StructNameEtc ( )
{
    TDSCI * tdsci = TDSCI_GetTop ( ) ;
    Printf ( "\n\t%16s : %s : size = %d : at %016lx",
        tdsci->Tdsci_Field_Type_Namespace->Name, tdsci->TdsciToken,
        ( int64 ) _CSL_VariableValueGet ( tdsci->Tdsci_Field_Type_Namespace->Name, ( byte* ) "size" ), //CSL_GetAndSet_ObjectByteSize ( tdsci->Tdsci_Field_Type_Namespace ),
        &tdsci->DataPtr [ tdsci->Tdsci_Offset ] ) ;
}

void
TDSCI_Print_StructField ( )
{
    TDSCI * tdsci = TDSCI_GetTop ( ) ;
    CSL_NewLine ( ) ;
    //TDSCI_DebugPrintWord ( cntx, tdsci->Tdsci_Field_Type_Namespace ) ;
    TDSCI_Print_StructNameEtc ( ) ;
    Word * word = Word_UnAlias ( tdsci->Tdsci_Field_Type_Namespace ) ;
    Object_PrintStructuredData ( & tdsci->DataPtr [ tdsci->Tdsci_Offset ], word ) ; //->W_OriginalCodeText ) ;
    //CSL_NewLine ( ) ;
}

void
TDSCI_Print_Field ( int64 t_type, int64 size )
{
#define FRMT "\n0x%016lx  %-16s%-3s  %-24s"    
#define FRMT_CHAR2 "0x%02x"    
    TDSCI * tdsci = TDSCI_GetTop ( ) ;
    byte *token = tdsci->TdsciToken, *format ; //, *format1 = "\n0x%016lx\t%12s%s\t%16s" ;
    int64 value ;
    if ( tdsci->Tdsci_Field_Type_Namespace )
    {
        if ( ( ! GetState ( tdsci, TDSCI_POINTER ) ) && ( tdsci->Tdsci_Field_Type_Namespace
            && ( tdsci->Tdsci_Field_Type_Namespace->W_ObjectAttributes & STRUCTURE_TYPE )
            && ( ! ( tdsci->Tdsci_Field_Type_Namespace->W_ObjectAttributes & T_POINTER ) ) ) )
        {
            TDSCI_Print_StructField ( ) ;
        }
        else
        {
            byte * dataPtr = & tdsci->DataPtr [ tdsci->Tdsci_Offset ] ;
            //tdsci->DataPtr = dataPtr ;
            switch ( size )
            {
                case 1:
                {
                    format = ( byte* ) FRMT " = \'%c\' : " FRMT_CHAR2 ;
                    value = * ( ( int8* ) ( dataPtr ) ) ; //( &tdsci->DataPtr [ tdsci->Tdsci_Offset ] ) ) ;
                    break ;
                }
                case 2:
                {
                    format = ( byte* ) FRMT " = 0x%04x" ;
                    value = * ( ( int16* ) ( dataPtr ) ) ; //( &tdsci->DataPtr [ tdsci->Tdsci_Offset ] ) ) ;
                    break ;
                }
                case 4:
                {
                    format = ( byte* ) FRMT " = 0x%08lx" ;
                    value = * ( ( int32* ) ( dataPtr ) ) ; //( &tdsci->DataPtr [ tdsci->Tdsci_Offset ] ) ) ;
                    break ;
                }
                default:
                case CELL:
                {
                    format = ( byte* ) FRMT " = 0x%016lx" ;
                    value = * ( ( int64* ) ( dataPtr ) ) ; //( &tdsci->DataPtr [ tdsci->Tdsci_Offset ] ) ) ;
                    break ;
                }
            }
            //byte * dataPtr = tdsci->Tdsci_Field_Object ? &tdsci->DataPtr [ tdsci->Tdsci_Field_Object->Offset ] : &tdsci->DataPtr [ tdsci->Tdsci_Offset ] ;
            //byte * dataPtr = & tdsci->DataPtr [ tdsci->Tdsci_Offset ] ;
            if ( size == 1 )
            {
                Printf ( format, dataPtr, tdsci->Tdsci_Field_Type_Namespace->Name,
                    ( GetState ( tdsci, TDSCI_POINTER ) ? " * " : "" ), token, value, value ) ;
            }
            else Printf ( format, dataPtr, tdsci->Tdsci_Field_Type_Namespace->Name,
                ( GetState ( tdsci, TDSCI_POINTER ) ? " * " : "" ), token, value ) ;

            if ( ! ( t_type & ( POST_STRUCTURE_ID | PRE_STRUCTURE_ID ) ) ) tdsci->Tdsci_Field_Size = size ;
            SetState ( tdsci, TDSCI_POINTER, false ) ;
        }
    }
}

void
TDSCI_DebugPrintWord ( Word * word )
{
    TDSCI * tdsci = TDSCI_GetTop ( ) ;
    if ( word ) Printf ( "\n%s.%s : field size = %ld : structure size = %ld : total size = %ld : dataPtr = 0x%09x : offset == %ld : at %s",
        ( word->TypeNamespace ? word->TypeNamespace->Name : word->S_ContainingNamespace->Name ),
        word->Name, tdsci->Tdsci_Field_Size, tdsci->Tdsci_StructureUnion_Size,
        tdsci->Tdsci_StructureUnion_Size, tdsci->DataPtr, tdsci->Tdsci_Offset, Context_Location ( ) ) ;
}

Boolean
Parser_Check_Do_CommentWord ( Word * word )
{
    if ( word && ( word->W_MorphismAttributes & ( COMMENT | DEBUG_WORD ) ) )
    {
        Interpreter_DoWord ( _Interpreter_, word, _Lexer_->TokenStart_ReadLineIndex, _Lexer_->SC_Index ) ;
        return true ;
    }

    else return false ;
}

Boolean
Parser_Check_Do_Debug_Token ( byte * token )
{
    Word * word = Finder_Word_FindUsing ( _Finder_, token, 0 ) ;
    return Parser_Check_Do_CommentWord ( word ) ;
}

byte *
TDSCI_ReadToken ( )
{
    Context * cntx = _Context_ ;
    TDSCI * tdsci = TDSCI_GetTop ( ) ;
    //if ( Is_DebugOn )
    {
        //A_Parser_Debug ( ) ;
        do
        {
            tdsci->TdsciToken = Lexer_ReadToken ( cntx->Lexer0 ) ;
            //if ( ! ( GetState ( tdsci, TDSCI_PRINT ) ) ) 
            DEBUG_SETUP_TOKEN ( tdsci->TdsciToken, 0 ) ;
            //if ( Is_DebugOn ) _Printf ( "%s ", tdsci->TdsciToken ) ;
        }
        while ( Parser_Check_Do_Debug_Token ( tdsci->TdsciToken ) ) ;
    }
    //else tdsci->TdsciToken = Lexer_ReadToken ( cntx->Lexer0 ) ;
    //tdsci->LineNumber = cntx->ReadLiner0->LineNumber ;
    //tdsci->Token_StartIndex = cntx->Lexer0->TokenStart_FileIndex ;
    //tdsci->Token_EndIndex = cntx->Lexer0->TokenEnd_FileIndex ;
    tdsci->NextChar = _ReadLine_pb_NextChar ( cntx->ReadLiner0 ) ;
    return tdsci->TdsciToken ;
}

Word *
_CSL_TypedefAlias ( Word * word0, byte * name, Namespace * addToNs, int64 size )
{
    TDSCI * tdsci = TDSCI_GetTop ( ) ;
    Word * word = Word_UnAlias ( word0 ), * alias = 0 ;
    if ( word && word->Definition )
    {
        if ( ! ( alias = _Finder_FindWord_InOneNamespace ( _Finder_, tdsci->Tdsci_StructureUnion_Namespace, name ) ) )
            if ( ! ( alias = _Finder_FindWord_InOneNamespace ( _Finder_, tdsci->Tdsci_InNamespace, name ) ) )
                alias = _Word_New ( name, word->W_MorphismAttributes | ALIAS, word->W_ObjectAttributes, word->W_LispAttributes, 0, addToNs, DICTIONARY ) ; // inherit type from original word
        _Word_DefinitionStore ( alias, ( block ) word->Definition ) ;
        DObject_Finish ( alias ) ;
        CSL_TypeStackReset ( ) ;
        _CSL_->LastFinished_Word = alias ;
        alias->S_CodeSize = word->S_CodeSize ;
        alias->W_AliasOf = word ;
        alias->ObjectByteSize = word->ObjectByteSize = size ;
        Class_Size_Set ( alias, size ) ;
    }
    else Exception ( USEAGE_ERROR, ABORT ) ;

    return alias ;
}

Word *
Parse_Do_IdentifierAlias ( byte * token, int64 size )
{
    TDSCI * tdsci = TDSCI_GetTop ( ) ;
    Namespace * alias = _CSL_TypedefAlias ( tdsci->Tdsci_StructureUnion_Namespace, token, _CSL_Namespace_InNamespaceGet ( ), size ) ; //tdsci->Tdsci_InNamespace ) ;
    TypeNamespace_Set ( alias, tdsci->Tdsci_StructureUnion_Namespace ) ;
    return alias ;
}

void
CSL_Parse_Error ( byte * msg, byte * token )
{
    byte * buffer = Buffer_Data_Cleared ( _CSL_->ScratchB1 ) ;
    snprintf ( ( char* ) buffer, BUFFER_IX_SIZE, "\nCSL_Parse_Error : %s : \'%s\' at %s", ( char* ) msg, token, Context_Location ( ) ) ;
    _Debugger_->Token = token ;
    _SyntaxError ( ( byte* ) buffer, 1 ) ; // else structure component size error
}

TDSCI *
_Object_Continue_PrintStructuredData ( byte * objectBits )
{
    TDSCI * tdsci = TDSCI_Start ( 0, objectBits, TDSCI_PRINT ) ;
    byte * token = TDSCI_ReadToken ( ) ; // read 'typedef' token
    if ( String_Equal ( token, "typedef" ) ) token = TDSCI_ReadToken ( ) ;
    Parse_Field ( ) ;
    tdsci = TDSCI_Finalize ( ) ;
    return tdsci ;
}

TDSCI *
Object_PrintStructuredData ( byte * objectBits, Word * typedefWord ) //byte * typedefString )
{
    TDSCI * tdsci = 0 ;
    byte * typedefString = typedefWord->W_OriginalCodeText ;
    if ( objectBits && typedefString && typedefString[0] )
    {
        Context * cntx0 = _Context_, * cntx = CSL_Context_PushNew ( _CSL_ ) ;
        cntx->State = cntx0->State ; // preserve C_SYNTAX, etc
        ReadLiner * rl = cntx->ReadLiner0 ;

        byte * b = Buffer_Data ( _CSL_->ScratchB4 ) ;
        snprintf ( b, BUFFER_IX_SIZE, "%s : <source code text>", typedefWord->Name ) ;
        rl->Filename = b ; //(byte*) "<source code text>" ; //typedefWord->W_WordData->Filename ;
        //rl->LineNumber = typedefWord->W_WordData->LineNumber ;
        Readline_Setup_OneStringInterpret ( rl, typedefString ) ; //typedefString ) ;

        tdsci = _Object_Continue_PrintStructuredData ( objectBits ) ;

        Readline_Restore_InputLine_State ( rl ) ;
        CSL_Context_PopDelete ( _CSL_ ) ;
    }
    return tdsci ;
}

#if 0

void
A_Parser_Debug ( )
{
    TDSCI * tdsci = TDSCI_GetTop ( ) ; // current -> previous
    Namespace * ins = Namespace_Find ( "_Identifier" ) ;
    if ( ins && ( ins == tdsci->Tdsci_StructureUnion_Namespace ) )
    {
        int64 size = _Namespace_VariableValueGet ( ins, ( byte* ) "size" ) ;
        if ( size || tdsci->Tdsci_StructureUnion_Size ) Printf ( "\n%s : size = %d : struct size = %d", ins->Name, size, tdsci->Tdsci_StructureUnion_Size ) ;
        //if ( ( size >= 184 ) || ( tdsci->Tdsci_StructureUnion_Size >= 184 ) )
        //    Pause ( ) ;
    }
}
#endif



