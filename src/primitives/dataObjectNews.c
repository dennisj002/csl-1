
#include "../include/csl.h"

void
CSL_Class_New ( void )
{
    byte * name = ( byte* ) DataStack_Pop ( ) ;
    DataObject_New (CLASS, 0, name, 0, 0, 0, 0, 0, 0, 0, 0, - 1 ) ;
}

void //Word *
CSL_Class_Value_New ( )
{
    byte * name = ( byte* ) DataStack_Pop ( ) ;
    Word * word = DataObject_New (OBJECT, 0, name, 0, 0, 0, 0, 0, 0, 0, 0, - 1 ) ;
}

void
CSL_Class_Clone ( void )
{
    byte * name = ( byte* ) DataStack_Pop ( ) ;
    DataObject_New (CLASS_CLONE, 0, name, 0, 0, 0, 0, 0, 0, 0, 0, - 1 ) ;
}

void
CSL_DObject_New ( )
{
    // clone DObject -- create an object with DObject as it's prototype (but not necessarily as it's namespace)
    DataObject_New (DOBJECT, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, - 1 ) ;
}

Namespace *
CSL_Type_New ( )
{
    byte * name ;
    Namespace * ns = 0 ;
    CSL_Token ( ) ;
    name = ( byte* ) DataStack_Pop ( ) ;
    if ( ( String_Equal ( "struct", name ) ) || ( String_Equal ( "class", name ) ) )
    {
        CSL_Token ( ) ;
        name = ( byte* ) DataStack_Pop ( ) ;
    }
    ns = DataObject_New (C_TYPE, 0, name, 0, 0, 0, 0, 0, 0, 0, 0, - 1 ) ;
    return ns ;
}

int64
Type_Create ()
{
    int64 size = 0 ;
    byte * token = Lexer_ReadToken ( _Lexer_ ) ; // 
    if ( token [ 0 ] == '{' )  size = CSL_Parse_Typedef_Field (0, 0, 0) ; //Namespace_ActivateAsPrimary ( ( byte* ) "C_Typedef" ) ;
    return size ;
}

void
CSL_Typedef ( )
{
    DataObject_New (C_TYPEDEF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, - 1 ) ; //--> _CSL_TypeDef ( ) ;
}


