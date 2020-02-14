
#include "../include/csl.h"

/*
 * csl namespaces basic understandings :
 * 0. to allow for ordered search thru lists in use ...
 * 1. Namespaces are also linked on the _Context->System0->Namespaces list with status of USING or NOT_USING
 *      and are moved the front or back of that list if status is set to USING with the 'Symbol node'
 *      This list is ordered so we can set an order to our search thru the namespaces in use. 
 *      The first word found within the ordered USING list will be used.
 */

void
Do_Namespace_WithStatus_2 ( dlnode * node, MapFunction2 nsf, int64 nsStateFlag, int64 one, int64 two )
{
    Namespace * ns = ( Namespace * ) node ;
    if ( ns->State == nsStateFlag )
    {
        nsf ( node, one, two ) ;
    }
}

void
_CSL_TreeMap ( MapSymbolFunction2 msf2, uint64 state, int64 one, int64 two )
{
    Tree_Map_Namespaces_State_2Args ( _CSL_->Namespaces->Lo_List, state, msf2, one, two ) ;
}

void
_CSL_NamespacesMap ( MapSymbolFunction2 msf2, uint64 state, int64 one, int64 two )
{
    Tree_Map_Namespaces_State_2Args ( _CSL_->Namespaces->Lo_List, state, msf2, one, two ) ;
}

// list/print namespaces

void
_CSL_ForAllNamespaces ( MapSymbolFunction2 msf2 )
{
    Printf ( ( byte* ) "\nusing :" ) ;
    _CSL_NamespacesMap ( msf2, USING, 1, 1 ) ;
    Printf ( ( byte* ) "\nnotUsing :" ) ;
    int64 usingWords = _CSL_->FindWordCount ;
    _CSL_NamespacesMap ( msf2, NOT_USING, 1, 1 ) ;
    int64 notUsingWords = _CSL_->FindWordCount ;
    _CSL_->FindWordCount = usingWords + notUsingWords ;
    CSL_WordAccounting ( ( byte* ) "_CSL_ForAllNamespaces" ) ;
}

void
Namespace_PrettyPrint ( Namespace* ns, int64 indentFlag, int64 indentLevel )
{
    if ( indentFlag )
    {
        Printf ( ( byte* ) "\n" ) ;
        while ( indentLevel -- ) Printf ( ( byte* ) "\t" ) ;
    }
    if ( ns->State & NOT_USING ) Printf ( ( byte* ) " - %s", c_gd ( ns->Name ) ) ;
    else Printf ( ( byte* ) " - %s", ns->Name ) ;
    _Context_->NsCount ++ ;
}

void
CSL_Namespace_New ( )
{
    Namespace * ns = Namespace_FindOrNew_SetUsing ( ( byte* ) DataStack_Pop ( ), _CSL_Namespace_InNamespaceGet ( ), 1 ) ;
    Namespace_Do_Namespace ( ns, 0 ) ;

}

void
_CSL_Namespace_NotUsing ( byte * name )
{
    Namespace * ns = Namespace_Find ( name ) ;
    if ( ns )
    {
        _Namespace_RemoveFromUsingList ( ns ) ;
        _Namespace_ResetFromInNamespace ( ns ) ;
    }
}

void
CSL_Namespace_NotUsing ( )
{
    byte * name = ( byte* ) DataStack_Pop ( ) ;
    _CSL_Namespace_NotUsing ( name ) ;
}

void
CSL_Namespace_UsingFirst ( )
{
    Namespace * ns = Namespace_Find ( ( byte* ) DataStack_Pop ( ) ) ;
    if ( ns ) _Namespace_AddToUsingList ( ns ) ;
}

void
CSL_Namespace_UsingLast ( )
{
    _Namespace_SetAs_UsingLast ( ( byte* ) DataStack_Pop ( ) ) ;
}

void
CSL_Namespace_SetStateAs_Using ( )
{
    byte * token = ( byte* ) DataStack_Pop ( ) ;
    Namespace * ns = Namespace_Find ( token ) ;
    _Namespace_SetState_AsUsing ( ns ) ;
}

void
CSL_Namespace_SetStateAs_NotUsing ( )
{
    byte * token = ( byte* ) DataStack_Pop ( ) ;
    Namespace * ns = Namespace_Find ( token ) ;
    _Namespace_SetStateAs_NotUsing ( ns ) ;
}

// "in"

void
CSL_PrintInNamespace ( )
{
    Printf ( ( byte* ) "\nCurrent Namespace Being Compiled : %s\n",
        _CSL_Namespace_InNamespaceGet ( )->Name ) ;
}

// list/print namespaces

void
CSL_Namespaces ( )
{
    Printf ( ( byte* ) "\nAll Namespaces : \n<list> ':' '-' <namespace>" ) ;
    _CSL_ForAllNamespaces ( ( MapSymbolFunction2 ) Symbol_NamespacePrettyPrint ) ;
    Printf ( ( byte* ) "\n" ) ;
}

int64
Word_RemoveIfStringContainsName ( Symbol * symbol, byte * name )
{
    if ( symbol && symbol->Name && strstr ( ( CString ) symbol->Name, ( CString ) name ) )
    {
        dlnode_Remove ( (dlnode*) symbol ) ;
        Word_Recycle ( (Word *) symbol ) ;
    }
    return 0 ;
}

void
_CSL_Namespaces_PurgeWordIfContainsName ( byte * name )
{
    Tree_Map_State_OneArg ( USING | NOT_USING, ( MapFunction_1 ) Word_RemoveIfStringContainsName, ( int64 ) name ) ;
}

void
CSL_Namespaces_PurgeWordIfContainsName ( )
{
   byte * name = ( byte* ) DataStack_Pop ( ) ;
   _CSL_Namespaces_PurgeWordIfContainsName ( name ) ;
}


int64
Word_RemoveIfStringEqualExactName ( Symbol * symbol, byte * name )
{
    if ( String_Equal ( symbol->Name, name ) )
    {
        dlnode_Remove ( (dlnode*) symbol ) ;
        Word_Recycle ( (Word *) symbol ) ;
    }
    return 0 ;
}

void
_CSL_Namespaces_PurgeWordExactName ( byte * name )
{
    Tree_Map_State_OneArg ( USING | NOT_USING, ( MapFunction_1 ) Word_RemoveIfStringEqualExactName, ( int64 ) name ) ;
}

void
CSL_Namespaces_PurgeWordExactName ( )
{
   byte * name = ( byte* ) DataStack_Pop ( ) ;
   _CSL_Namespaces_PurgeWordExactName ( name ) ;
}

void
Symbol_SetNonTREED ( Symbol * symbol, int64 one, int64 two )
{
    Namespace * ns = ( Namespace * ) symbol ;
    ns->State &= ~ TREED ;
}

void
Symbol_Namespaces_PrintTraverse ( Symbol * symbol, int64 containingNamespace, int64 indentLevel )
{
    Namespace * ns = ( Namespace * ) symbol ;
    if ( ns->ContainingNamespace == ( Namespace* ) containingNamespace )
    {
        if ( ! ( ns->State & TREED ) )
        {
            ns->State |= TREED ;
            Namespace_PrettyPrint ( ns, 1, indentLevel ) ;
            _Namespace_MapAny_2Args ( ( MapSymbolFunction2 ) Symbol_Namespaces_PrintTraverse, ( int64 ) ns, indentLevel + 1 ) ;
        }
    }
}

void
Symbol_Namespaces_PrintTraverseWithWords ( Symbol * symbol, int64 containingNamespace, int64 indentLevel )
{
    Namespace * ns = ( Namespace * ) symbol ;
    if ( ns->ContainingNamespace == ( Namespace* ) containingNamespace )
    {
        if ( ! ( ns->State & TREED ) )
        {
            ns->State |= TREED ;
            Namespace_PrettyPrint ( ns, 1, indentLevel ) ;
            dllist_Map1 ( ns->Lo_List, ( MapFunction1 ) _Word_Print, 0 ) ;
            _Namespace_MapAny_2Args ( ( MapSymbolFunction2 ) Symbol_Namespaces_PrintTraverseWithWords, ( int64 ) ns, indentLevel + 1 ) ;
        }
    }
}

void
CSL_Namespaces_PrettyPrintTree ( )
{
    _Context_->NsCount = 0 ;
    _Context_->WordCount = 0 ;
    //SetState ( _O_->psi_PrintStateInfo, PSI_PROMPT, false ) ;
    Printf ( ( byte* ) "\nNamespaceTree - All Namespaces : %s%s%s", c_ud ( "using" ), " : ", c_gd ( "not using" ) ) ;
    _Namespace_MapAny_2Args ( ( MapSymbolFunction2 ) Symbol_SetNonTREED, 0, 0 ) ;
    _Namespace_MapAny_2Args ( ( MapSymbolFunction2 ) Symbol_Namespaces_PrintTraverse, ( int64 ) _CSL_->Namespaces, 1 ) ;
    Printf ( ( byte* ) "\nTotal namespaces = %d :: Total words = %d\n", _Context_->NsCount, _Context_->WordCount ) ;
}

void
CSL_Namespaces_PrettyPrintTreeWithWords ( )
{
    _Context_->NsCount = 0 ;
    _Context_->WordCount = 0 ;
    //SetState ( _O_->psi_PrintStateInfo, PSI_PROMPT, false ) ;
    Printf ( ( byte* ) "%s%s%s%s%s%s%s", "\nNamespaceTree - All Namespaces : ", "using", " : ", c_gd ( "not using" ), " :: ", "with", c_ud ( " : words" ) ) ;
    _Namespace_MapAny_2Args ( ( MapSymbolFunction2 ) Symbol_SetNonTREED, 0, 0 ) ;
    _Namespace_MapAny_2Args ( ( MapSymbolFunction2 ) Symbol_Namespaces_PrintTraverseWithWords, ( int64 ) _CSL_->Namespaces, 1 ) ;
    Printf ( ( byte* ) "\nTotal namespaces = %d :: Total words = %d\n", _Context_->NsCount, _Context_->WordCount ) ;
}

void
_Namespace_Symbol_Print ( Symbol * symbol, int64 printFlag, int64 str )
{
    char buffer [128] ;
    Namespace * ns = ( Namespace * ) symbol ;
    sprintf ( buffer, "%s ", ns->Name ) ;
    if ( printFlag )
    {
        Printf ( ( byte* ) "%s", buffer ) ;
    }
    else strcat ( ( char* ) str, buffer ) ;
}

// prints all the namespaces on the SearchList
// 'using'

byte *
_CSL_UsingToString ( )
{
    byte * b = Buffer_Data ( _CSL_->ScratchB1 ) ;
    strcpy ( ( char* ) b, "" ) ;
    Tree_Map_Namespaces_State_2Args ( _CSL_->Namespaces->Lo_List, USING, ( MapSymbolFunction2 ) _Namespace_Symbol_Print, 0, ( int64 ) b ) ;
    b = String_New ( ( byte* ) b, TEMPORARY ) ;
    return b ;
}

void
CSL_Using ( )
{
    Printf ( ( byte* ) "\nUsing Namespaces :> " ) ;
    Tree_Map_Namespaces_State_2Args ( _CSL_->Namespaces->Lo_List, USING, ( MapSymbolFunction2 ) _Namespace_Symbol_Print, 1, 0 ) ;
    Printf ( ( byte* ) "\n" ) ;
}

// this namespace is will be taken out of the system

void
CSL_NonCompilingNs_Clear ( Compiler * compiler )
{
    if ( compiler->NonCompilingNs )
    {
        _Namespace_RemoveFromUsingListAndClear (compiler->NonCompilingNs) ;
        compiler->NonCompilingNs = 0 ;
    }
}

Word *
_CSL_VariableGet ( Namespace * ns, byte * name )
{
    ns = Word_UnAlias ( ns ) ;
    Word * word = _Finder_FindWord_InOneNamespace ( _Finder_, ns, name ) ;
    return word ;
}

int64
_CSL_VariableValueGet ( byte* nameSpace, byte * name )
{
    return _Namespace_VariableValueGet ( Namespace_Find ( nameSpace ), name ) ;
}

void
_CSL_RemoveNamespaceFromUsingListAndClear ( byte * name )
{
    _Namespace_RemoveFromUsingListAndClear ( Namespace_Find ( name ) ) ;
}

//keep the stack intack just remove the namespace from the Namespaces list and set them as not using

void
Namespace_RemoveNamespacesStack ( Stack * stack )
{
    if ( stack )
    {
        int64 n = Stack_Depth ( stack ) ;
        while ( n > 0 )
        {
            Namespace * ns = ( Namespace* ) _Stack_Pop ( stack ) ; 
            if ( ns )
            {
                if ( ns == _CSL_->InNamespace ) _CSL_->InNamespace = 0 ;
                if ( _Finder_ && ( ns == _Finder_->QualifyingNamespace ) ) Finder_SetQualifyingNamespace ( _Context_->Finder0, 0 ) ;
                _Namespace_SetState ( ns, NOT_USING ) ;
                dlnode_Remove ( ( dlnode* ) ns ) ;
            }
            n -- ;
        }
    }
}

void
Namespace_RemoveAndClearNamespacesStack ( Stack * stack )
{
    if ( stack )
    {
        int64 n ;
        for ( n = Stack_Depth ( stack ) ; n ; n -- )
        {
            Namespace * ns = ( Namespace* ) Stack_Pop ( stack ) ;
            if ( ns ) _Namespace_RemoveFromUsingListAndClear ( ns ) ;
        }
        Stack_Init ( stack ) ;
    }
}

#if 0
void
Namespace_NamespacesStack_PrintWords ( Stack * stack )
{
    if ( stack )
    {
        int64 n ;
        for ( n = Stack_Depth ( stack ) ; n > 0 ; n -- )
        {
            Namespace * ns = stack->StackPointer [n] ; //( Namespace* ) Stack_Pop ( stack ) ;
            //if ( ns ) _Namespace_RemoveFromUsingListAndClear ( ns ) ;
            n -- ;
        }
    }
}
#endif

