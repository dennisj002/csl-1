
#include "../../include/csl.h"

OVT_MemSystem *_OMS_ ; // initialized by OVT_Static_MemSystem_New
OVT_StaticMemSystem * _OSMS_ ;

void
MemChunk_Account ( int64 size, int64 allocType, int64 flag )
{
    if ( allocType & ( _STATIC_ | MMAP ) )
    {
        if ( _OSMS_ )
        {
            if ( flag == MEM_ALLOC ) // OVT_StaticMemSystem
            {
                _OSMS_->Static_Allocated += size ;
                _OSMS_->Static_RemainingAllocated += size ;
            }
            else
            {
                _OSMS_->Static_Freed += size ;
                _OSMS_->Static_RemainingAllocated -= size ;
            }
        }
        //else printf ( "\nMemChunk_Account failure : _OSMS_" ), fflush ( stdout ) ;
    }
    else
    {
        if ( _OMS_ )
        {
            if ( flag == MEM_ALLOC ) // OVT_MemSystem
            {
                _OMS_->Allocated += size ;
                _OMS_->RemainingAllocated += size ;
            }
            else
            {
                _OMS_->RemainingAllocated -= size ;
                _OMS_->Freed += size ;
            }
        }
        //else printf ( "\nMemChunk_Account failure : _OMS_" ), fflush ( stdout ) ;
    }
}

void
_Munmap ( MemChunk * mchunk )
{
    int64 size = mchunk->mc_ChunkSize ;
    munmap ( mchunk, size ) ;
}

void
MMAP_Munmap ( byte * ptr, int64 size )
{
    MemChunk_Account ( size, MMAP, MEM_FREE ) ;
    munmap ( ( MemChunk * ) ptr, size ) ;
}

void
Munmap ( MemChunk * mchunk )
{
#if 0 // DEBUG    
    if ( ! ( mchunk->mc_Type & MEM_CHUNK ) )
    {
        printf ( "\nMunmap : incorrect Type" ) ;
        fflush ( stdout ) ;
    }
#endif    
    MemChunk_Account ( mchunk->mc_ChunkSize, mchunk->mc_AllocType, MEM_FREE ) ;
    _Munmap ( mchunk ) ;
}

byte*
_Mmap ( int64 size )
{
    byte * mem = ( byte* ) mmap ( NULL, size, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, - 1, 0 ) ;
    memset ( mem, 0, size ) ; // ??! is this necessary 
    return mem ;
}

byte*
Mmap ( int64 size )
{
    byte * mem = _Mmap ( size ) ;
    if ( ( mem == MAP_FAILED ) ) // MAP_FAILED == -1
    {
        perror ( "_Mem_Mmap" ) ;
        OVT_ShowMemoryAllocated ( ) ;
        //OVT_Exit ( ) ;
        CSL_FullRestart ( ) ;
    }
    return mem ;
}

byte*
MMAP_Mmap ( int64 size, int64 allocType )
{
    byte * mem = _Mmap ( size ) ;
    MemChunk_Account ( size, allocType, MEM_ALLOC ) ;
    return mem ;
}

// MemChunk struct is at the beginning of structures being allocated 

MemChunk *
_MemChunk_Allocate ( int64 size, uint64 allocType )
{
    // a MemChunk is already a part of the size being part of the ByteArray and NamedByteArray : cf. types.h
    MemChunk * mchunk = ( MemChunk * ) MMAP_Mmap ( size, allocType ) ; //Mmap ( size ) ;
    mchunk->mc_unmap = ( byte* ) mchunk ;
    mchunk->mc_ChunkSize = size ; // S_ChunkSize is the total size of the chunk including any prepended accounting structure in that total
    mchunk->mc_AllocType = allocType ;
    mchunk->mc_Data = ( byte* ) ( mchunk + 1 ) ;
    mchunk->mc_Type = MEM_CHUNK ;
    return mchunk ; 
}

MemChunk *
MemChunk_AllocateAdd ( int64 size, int64 allocType )
{
    MemChunk * mchunk = ( MemChunk * ) _MemChunk_Allocate ( size, allocType ) ;
    if ( allocType & _STATIC_ ) dllist_AddNodeToHead ( _OSMS_->OVT_StaticMemList, ( dlnode* ) mchunk ) ;
    else if ( allocType & HISTORY ) dllist_AddNodeToHead ( _OSMS_->HistorySpace_MemChunkStringList, ( dlnode* ) mchunk ) ;
    else if ( allocType != MMAP ) dllist_AddNodeToHead ( _OMS_->OvtMemChunkList, ( dlnode* ) mchunk ) ;
    // !! else if ( allocType == MMAP ) !! not added to a list ; must be unmapped individually
    return mchunk ;
}

void
MemChunk_Show ( MemChunk * mchunk )
{
    Printf ( "\nMemChunk : address : 0x%08x : allocType = %8lu : size = %8d", ( uint64 ) mchunk, ( uint64 ) mchunk->mc_AllocType, ( int64 ) mchunk->mc_ChunkSize ) ;
}

void
_MemChunk_WithSymbol_Show ( MemChunk * mchunk, int64 flag )
{
    Symbol * sym = ( Symbol * ) ( mchunk + 1 ) ;
    Printf ( "\n%s : %s : 0x%lld : %d, ", ( flag == MEM_ALLOC ) ? "Alloc" : "Free",
        ( ( int64 ) ( sym->S_Name ) > 0x80000000 ) ? ( char* ) sym->S_Name : "(null)", mchunk->mc_AllocType, mchunk->mc_ChunkSize ) ;
}

void
OVT_Static_New ( )
{
    OVT_StaticMemSystem * osms = _OSMS_ = ( OVT_StaticMemSystem * ) _MemChunk_Allocate ( sizeof ( OVT_StaticMemSystem ), MMAP ) ; //_OVT_Static_AllocMem ( sizeof ( OVT_StaticMemSystem ) ) ;
    MemChunk_Account ( sizeof ( OVT_StaticMemSystem ), MMAP, MEM_ALLOC ) ; // couldn't be accounted before _OSMS_ was created
    osms->OVT_StaticMemList = _dllist_New ( MMAP ) ;
    osms->HistorySpace_MemChunkStringList = _dllist_New ( MMAP ) ;
    OVT_MemSystem * oms = _OMS_ = ( OVT_MemSystem * ) _MemChunk_Allocate ( sizeof ( OVT_MemSystem ), MMAP ) ; //_OVT_Static_AllocMem ( sizeof ( OVT_StaticMemSystem ) ) ;
    oms->OvtMemChunkList = _dllist_New ( MMAP ) ;
}

void
MMAP_Munmap_List ( dllist * list )
{
    MMAP_Munmap ( ( byte * ) & list->l_List.n_Head, sizeof ( dlnode ) ) ;
    MMAP_Munmap ( ( byte * ) & list->l_List.n_Tail, sizeof ( dlnode ) ) ;
    MMAP_Munmap ( ( byte * ) list, sizeof ( dllist ) ) ;
}

void
OVT_Static_Delete ( OVT_StaticMemSystem * osms )
{
    FreeChunkList ( _OMS_->OvtMemChunkList ) ;
    FreeChunkList ( osms->OVT_StaticMemList ) ;
    FreeChunkList ( _OSMS_->HistorySpace_MemChunkStringList ) ;
    MMAP_Munmap_List ( osms->OVT_StaticMemList ) ;
    MMAP_Munmap_List ( osms->HistorySpace_MemChunkStringList ) ;
    MMAP_Munmap_List ( _OMS_->OvtMemChunkList ) ;
    MMAP_Munmap ( ( byte * ) _OMS_, sizeof ( OVT_MemSystem ) ) ;
    MMAP_Munmap ( ( byte * ) osms, sizeof ( OVT_StaticMemSystem ) ) ;
}

void
OpenVmTil_Delete ( OpenVmTil * ovt )
{
    if ( ovt )
    {
        if ( ovt->Verbosity > 2 ) Printf ( "\nAll allocated, non-static memory is being freed.\nRestart : verbosity = %d.", ovt->Verbosity ) ;
        FreeChunkList ( _OMS_->OvtMemChunkList ) ;
    }
    _O_ = 0 ;
}

OpenVmTil *
_OpenVmTil_Allocate ( OpenVmTil * ovt )
{
    if ( ! _OSMS_ ) OVT_Static_New ( ) ;
    OpenVmTil_Delete ( ovt ) ; // first delete a previous ovt - it could have been corrupted by a software error
    ovt = _O_ = ( OpenVmTil* ) MemChunk_AllocateAdd ( sizeof ( OpenVmTil ), OPENVMTIL ) ; //OVT_STATIC
    ovt->OpenVmTilSpace = MemorySpace_NBA_OvtNew ( ( byte* ) "OpenVmTilSpace", (ovt->OpenVmTilSize ? ovt->OpenVmTilSize : ( 6 * K )), OPENVMTIL ) ;
    ovt->NBAs = _dllist_New ( OPENVMTIL ) ;
    ovt->MemorySpaceList = _dllist_New ( OPENVMTIL ) ;
    return ovt ;
}

void
OVT_FullRestartCompleteDelete ( )
{
    if ( _O_ && ( _O_->SigSegvs ++ < 2 ) ) OpenVmTil_Delete ( _O_ ) ;
    OVT_Static_Delete ( _OSMS_ ) ;
    _OMS_ = 0 ;
    _OSMS_ = 0 ;
    _O_ = 0 ;
}

byte *
Mem_Allocate ( int64 size, uint64 allocType )
{
    OpenVmTil * ovt = _O_ ;
    MemorySpace * ms = _O_ ? _O_->MemorySpace0 : 0 ;
    switch ( allocType )
    {
        case DICTIONARY: return _Allocate ( size, ms->DictionarySpace ) ;
        case CODE: return _Allocate ( size, ms->CodeSpace ) ;
        case OBJECT_MEM: return _Allocate ( size, ms->ObjectSpace ) ;
        case STRING_MEMORY: return _Allocate ( size, ms->StringSpace ) ;
        case LISP: return _Allocate ( size, ms->LispSpace ) ;

        case CONTEXT: return _Allocate ( size, ms->ContextSpace ) ;
        case BUFFER: return _Allocate ( size, ms->BufferSpace ) ;
        case LISP_TEMP: return _Allocate ( size, ms->LispTempSpace ) ;
        case TEMPORARY: return _Allocate ( size, ms->TempObjectSpace ) ; // used for SourceCode
        case SESSION: return _Allocate ( size, ms->SessionObjectsSpace ) ;
        case COMPILER_TEMP: return _Allocate ( size, ms->CompilerTempObjectSpace ) ;
        case WORD_RECYCLING: return _Allocate ( size, ms->WordRecylingSpace ) ;

        case T_CSL: case DATA_STACK: return _Allocate ( size, ovt->CSL_InternalSpace ) ;
        case INTERNAL_OBJECT_MEM: return _Allocate ( size, ovt->InternalObjectSpace ) ;
        case OPENVMTIL: return _Allocate ( size, ovt->OpenVmTilSpace ) ;

        case MMAP: case _STATIC_: case OVT_STATIC: case HISTORY: 
        {
            if ( allocType == HISTORY ) _OSMS_->HistoryAllocated += size ;
            return MMAP_Mmap ( size, allocType ) ;
        }

        default: CSL_Exception ( MEMORY_ALLOCATION_ERROR, 0, QUIT ) ;
    }
    return 0 ;
}

void *
sl9_malloc ( int size )
{
    return ( void* ) Mem_Allocate ( size, LISP_TEMP ) ;
}

void
MemorySpace_Init ( MemorySpace * ms )
{
    OpenVmTil * ovt = _O_ ;
    MemorySpace * oldMs = ovt->MemorySpace0 ;

    ms->DictionarySpace = ( oldMs && oldMs->DictionarySpace ) ? oldMs->DictionarySpace : MemorySpace_NBA_New ( ms, ( byte* ) "DictionarySpace", ovt->DictionarySize, DICTIONARY ) ;
    ms->CodeSpace = ( oldMs && oldMs->CodeSpace ) ? oldMs->CodeSpace : MemorySpace_NBA_New ( ms, ( byte* ) "CodeSpace", ovt->MachineCodeSize, CODE ) ;

    ms->LispSpace = MemorySpace_NBA_New ( ms, ( byte* ) "LispSpace", ovt->LispSpaceSize, LISP ) ;
    ms->ObjectSpace = MemorySpace_NBA_New ( ms, ( byte* ) "ObjectSpace", ovt->ObjectSpaceSize, OBJECT_MEM ) ;
    ms->StringSpace = MemorySpace_NBA_New ( ms, ( byte* ) "StringSpace", ovt->StringSpaceSize, STRING_MEMORY ) ;
    ms->BufferSpace = MemorySpace_NBA_New ( ms, ( byte* ) "BufferSpace", ovt->BufferSpaceSize, BUFFER ) ;

    ms->TempObjectSpace = MemorySpace_NBA_New ( ms, ( byte* ) "TempObjectSpace", ovt->TempObjectsSize, TEMPORARY ) ;
    ms->LispTempSpace = MemorySpace_NBA_New ( ms, ( byte* ) "LispTempSpace", ovt->LispTempSize, LISP_TEMP ) ;
    //ms->LispCopySpace = MemorySpace_NBA_New ( ms, ( byte* ) "LispCopySpace", ovt->LispCopySize, LISP_COPY ) ;
    ms->CompilerTempObjectSpace = MemorySpace_NBA_New ( ms, ( byte* ) "CompilerTempObjectSpace", ovt->CompilerTempObjectsSize, COMPILER_TEMP ) ;
    ms->WordRecylingSpace = MemorySpace_NBA_New ( ms, ( byte* ) "WordRecylingSpace", ovt->WordRecylingSize, WORD_RECYCLING ) ;
    ms->SessionObjectsSpace = MemorySpace_NBA_New ( ms, ( byte* ) "SessionObjectsSpace", ovt->SessionObjectsSize, SESSION ) ;

    if ( ovt->OVT_CSL && ovt->OVT_CSL->Context0 )
    {
        NBA * cnba = ovt->OVT_CSL->Context0->ContextNba ;
        ms->ContextSpace = cnba ;
    }
    _O_CodeByteArray = ms->CodeSpace->ba_CurrentByteArray ; //init CompilerSpace ptr
    if ( _O_->Verbosity > 2 ) Printf ( "\nSystem Memory has been initialized.  " ) ;
}

void
Mem_FreeItem ( dllist * mList, byte * item )
{
    dlnode * node, *nodeNext ;
    for ( node = dllist_First ( ( dllist* ) mList ) ; node ; node = nodeNext )
    {
        MemChunk * mchunk = ( MemChunk* ) node ;
        nodeNext = dlnode_Next ( node ) ;
        if ( ( byte* ) mchunk->mc_Data == item )
        {
            _Mem_ChunkFree ( mchunk ) ;
            return ;
        }
    }
}

void
_Mem_ChunkFree ( MemChunk * mchunk )
{
    dlnode_Remove ( ( dlnode* ) mchunk ) ;
    Munmap ( mchunk ) ;
}

void
FreeChunkList ( dllist * list )
{
    dlnode * node, *nodeNext ;
    MemChunk * mchunk ;
    for ( node = dllist_First ( ( dllist* ) list ) ; node ; node = nodeNext )
    {
        nodeNext = dlnode_Next ( node ) ;
        mchunk = ( MemChunk* ) node ;
        if ( ( mchunk->mc_AllocType & HISTORY ) && ( mchunk->mc_Name ) ) 
            MemChunk_Account ( ( Strlen ( mchunk->mc_Name ) + 1 ), MMAP, MEM_FREE ) ;
        _Mem_ChunkFree ( mchunk ) ;
    }
}

void
FreeNba_BaNode ( NamedByteArray * nba, dlnode * node )
{
    ByteArray * ba = Get_BA_Symbol_To_BA ( node ) ;
    MemChunk* mchunk = ( MemChunk* ) ba ; //( ( Symbol * ) node )->S_Value ;
    int64 size = ba->BA_DataSize ; //mchunk->S_ChunkSize ;
    dlnode_Remove ( node ) ; // remove BA_Symbol from nba->NBA_BaList cf. _NamedByteArray_AddNewByteArray
    _Mem_ChunkFree ( mchunk ) ;
    nba->MemRemaining -= size ;
    nba->MemAllocated -= size ;
    nba->TotalAllocSize -= size ;
}

void
FreeNba_BaList ( NamedByteArray * nba )
{
    dllist * list = & nba->NBA_BaList ;
    dlnode * node, *nodeNext ;
    for ( node = dllist_First ( ( dllist* ) list ) ; node ; node = nodeNext )
    {
        nodeNext = dlnode_Next ( node ) ;
        FreeNba_BaNode ( nba, node ) ;
    }
}

void
NBA_FreeChunkType ( Symbol * s, uint64 allocType, int64 exactFlag )
{
    NamedByteArray * nba = Get_NbaSymbolNode_To_NBA ( s ) ;
    if ( exactFlag )
    {
        if ( nba->NBA_AAttribute != allocType ) return ;
    }
    else if ( ! ( nba->NBA_AAttribute & allocType ) ) return ;
    FreeNba_BaList ( nba ) ;
    nba->MemRemaining = 0 ;
    nba->MemAllocated = 0 ;
    _NamedByteArray_AddNewByteArray ( nba, nba->NBA_DataSize ) ;
}

MemorySpace *
MemorySpace_Find ( byte * name )
{
    MemorySpace * ms = ( MemorySpace* ) _DLList_FindName_InOneNamespaceList ( _O_->MemorySpaceList, name ) ;
    return ms ;
}

MemorySpace *
MemorySpace_StaticMem_Allocate ( )
{
    MemorySpace *ms = ( MemorySpace* ) Mem_Allocate ( sizeof ( MemorySpace ), OPENVMTIL ) ; //mmap_AllocMem ( sizeof ( MemorySpace ) ) ;
    ms->NBAs = _dllist_New ( OPENVMTIL ) ;
    return ms ;
}

void
_MemorySpace_Do_MemorySpace ( OpenVmTil * ovt, MemorySpace * ms )
{
    ovt->MemorySpace0 = ms ;
    if ( ovt->OVT_CSL && ovt->OVT_CSL->Context0 )
    {
        NBA * cnba = ovt->OVT_CSL->Context0->ContextNba ;
        ovt->MemorySpace0->ContextSpace = cnba ;
    }
}

MemorySpace *
MemorySpace_New ( OpenVmTil * ovt, byte * name )
{
    MemorySpace * ms = MemorySpace_Find ( name ) ;
    if ( ! ms )
    {
        ms = MemorySpace_StaticMem_Allocate ( ) ;
        MemorySpace_Init ( ms ) ;
        ( ( Symbol* ) ms )->S_Name = name ;
        dllist_AddNodeToHead ( ovt->MemorySpaceList, ( dlnode* ) ms ) ;
        _MemorySpace_Do_MemorySpace ( ovt, ms ) ;
    }
    return ms ;
}

// not so useful now maybe future developments will change things

void
MemorySpace_Delete ( byte * name )
{
    NamedByteArray *nba ;
    dlnode * node, *nodeNext ;
    MemorySpace * ms = MemorySpace_Find ( name ) ;
    if ( ms && ( node = dllist_First ( ( dllist* ) ms->NBAs ) ) )
    {
        MemorySpace * defaultMs = MemorySpace_Find ( "DefaultMemorySpace" ) ;
        defaultMs->WordsInRecycling = ms->WordsInRecycling ;
        defaultMs->RecycledWordCount = ms->RecycledWordCount ;
        for ( ; node ; node = nodeNext )
        {
            nodeNext = dlnode_Next ( node ) ;
            nba = Get_NbaSymbolNode_To_NBA ( node ) ;
            _OVT_MemList_FreeNBAMemory ( nba, 0, 1 ) ;
        }
        dlnode_Remove ( ( dlnode * ) ms ) ;
        _MemorySpace_Do_MemorySpace ( _O_, defaultMs ) ;
    }
}

void
CSL_MemorySpace_Delete ( )
{
    MemorySpace_Delete ( ( byte* ) DataStack_Pop ( ) ) ;
}

void
CSL_MemorySpace_New ( )
{
    byte * name = ( byte* ) DataStack_Pop ( ) ;
    MemorySpace * ms = MemorySpace_New ( _O_, name ) ;
}

ByteArray *
_ByteArray_AppendSpace_MakeSure ( ByteArray * ba, int64 size ) // size in bytes
{
    NamedByteArray * nba = ba->OurNBA ;
    if ( nba )
    {
        while ( ba->MemRemaining < size )
        {
            int64 largestRemaining = 0 ;
            // check the other bas in the nba list to see if any have enough remaining
            {
                dlnode * node, *nodeNext ;
                for ( node = dllist_First ( ( dllist* ) & nba->NBA_BaList ) ; node ; node = nodeNext )
                {
                    if ( node == nodeNext ) break ; // ?? TODO : should need this
                    nodeNext = dlnode_Next ( node ) ;
                    ba = Get_BA_Symbol_To_BA ( node ) ;
                    if ( ba )
                    {
                        if ( ba->MemRemaining > largestRemaining ) largestRemaining = ba->MemRemaining ;
                        if ( ba->MemRemaining >= size ) goto done ;
                    }
                }
            }
            _O_->ReAllocations ++ ;
#if 0     // 15 * M       
            nba->NBA_DataSize = nba->NBA_DataSize > ( 100 * K ) ? nba->NBA_DataSize : ( 100 * K ) ;
            nba->NBA_DataSize *= ( ++ nba->Allocations ) ;
            nba->NBA_DataSize += size ;
#elif 0  // 14 * M           
            nba->NBA_DataSize += ( ( ( ++ nba->Allocations ) * ( 100 * K ) ) + size ) ;
#elif 0  // 16 * M          
            nba->NBA_DataSize = ( ( ++ nba->Allocations ) * ( nba->NBA_DataSize ? nba->NBA_DataSize : ( 100 * K ) ) + size ) ;
#else   // 13 * M 
            size = ( size > ( 100 * K ) ) ? size : ( 100 * K ) ;
            nba->NBA_DataSize += ( ( ( ++ nba->Allocations ) * size ) + ( 100 * K ) ) ;
            //nba->NBA_DataSize =  ((nba->NBA_DataSize > (100 * K) ? nba->NBA_DataSize : (100 * K) ) * (++nba->Allocations) ) + size ;
#endif            
            if ( _O_->Verbosity > 1 )
                NBA_PrintInfo ( nba ) ;
            ba = _NamedByteArray_AddNewByteArray ( nba, nba->NBA_DataSize ) ;
        }
    }
    else Error_Abort ( "_ByteArray_AppendSpace_MakeSure", ( byte* ) "\n_ByteArray_AppendSpace_MakeSure : no nba?!\n" ) ;
done:
    nba->ba_CurrentByteArray = ba ;
    return ba ;
}

//macros.h :  _Allocate( size, nba ) _ByteArray_AppendSpace ( nba->ba_CurrentByteArray, size ) 

byte *
_ByteArray_AppendSpace ( ByteArray * ba, int64 size ) // size in bytes
{
    while ( ba->MemRemaining < size )
    {
        ba = _ByteArray_AppendSpace_MakeSure ( ba, size ) ;
    }
    ba->StartIndex = ba->EndIndex ; // move index to end of the last append
    ba->EndIndex += size ;
    if ( ba->OurNBA ) ba->OurNBA->MemRemaining -= size ; //nb. debugger->StepInstructionBA doesn't have an nba
    ba->MemRemaining -= size ;
    return ba->StartIndex ;
}

NamedByteArray *
MemorySpace_NBA_New ( MemorySpace * memSpace, byte * name, int64 size, int64 allocType )
{
    NamedByteArray *nba = NamedByteArray_New ( name, size, allocType ) ;
    dllist_AddNodeToHead ( memSpace->NBAs, ( dlnode* ) & nba->NBA_Symbol ) ;
    return nba ;
}

NamedByteArray *
MemorySpace_NBA_OvtNew ( byte * name, int64 size, int64 allocType )
{
    OpenVmTil * ovt = _O_ ;
    NamedByteArray *nba = NamedByteArray_New ( name, size, allocType ) ;
    dllist_AddNodeToHead ( ovt->NBAs, ( dlnode* ) & nba->NBA_Symbol ) ;
    return nba ;
}

void
OVT_FreeTempMem ( )
{
    OVT_MemListFree_CompilerTempObjects ( ) ;
    OVT_MemListFree_TempObjects ( ) ;
    OVT_MemListFree_Objects ( ) ;
    OVT_MemListFree_LispTemp ( ) ;
}

NamedByteArray *
_OVT_Find_NBA ( byte * name )
{
    // needs a Word_Find that can be called before everything is initialized
    Symbol * s = DLList_FindName_InOneNamespaceList ( _O_->MemorySpace0->NBAs, ( byte * ) name ) ;
    if ( s ) return Get_NbaSymbolNode_To_NBA ( s ) ;
    else return 0 ;
}

// fuzzy still but haven't yet needed to adjust this one

void
_OVT_MemList_FreeNBAMemory ( NamedByteArray *nba, uint64 moreThan, int64 always )
{
    if ( nba && ( always || ( nba->MemAllocated > ( nba->MemInitial + moreThan ) ) ) ) // this logic is fuzzy ?? what is wanted is a way to fine tune mem allocation 
    {
        dlnode * node, *nodeNext ;
        int64 size, flag ;
        for ( flag = 0, node = dllist_First ( ( dllist* ) & nba->NBA_BaList ) ; node ; node = nodeNext )
        {
            nodeNext = dlnode_Next ( node ) ;
            ByteArray * ba = Get_BA_Symbol_To_BA ( node ) ;
            if ( ba )
            {
                FreeNba_BaNode ( nba, node ) ;
                nba->NumberOfByteArrays -- ;
                if ( ! nodeNext )
                {
                    nba->MemAllocated = 0 ;
                    nba->MemRemaining = 0 ;
                    nba->NBA_DataSize = nba->OriginalSize ;
                    _NamedByteArray_AddNewByteArray ( nba, nba->OriginalSize ) ;
                }
            }
        }
        nba->InitFreedRandMarker = rand ( ) ;
    }
}

void
NamedByteArray_Delete ( NamedByteArray * nba, Boolean reinitFlag )
{
    ByteArray * ba ;
    dlnode * node, *nodeNext ;
    if ( nba )
    {
        dlnode_Remove ( ( dlnode* ) & nba->NBA_Symbol ) ;
        for ( node = dllist_First ( ( dllist* ) & nba->NBA_BaList ) ; node ; node = nodeNext )
        {
            nodeNext = dlnode_Next ( node ) ;
            ba = Get_BA_Symbol_To_BA ( node ) ;
            _Mem_ChunkFree ( ( MemChunk * ) ba ) ;
        }
        if ( ! reinitFlag ) _Mem_ChunkFree ( ( MemChunk * ) nba ) ; // mchunk )

        else _NamedByteArray_Init ( nba, nba->NBA_MemChunk.mc_Name, nba->NBA_DataSize, nba->NBA_AAttribute ) ;
    }
}

void
OVT_MemList_DeleteNBAMemory ( byte * name, Boolean reinitFlag )
{
    NamedByteArray *nba = _OVT_Find_NBA ( name ) ;
    NamedByteArray_Delete ( nba, reinitFlag ) ;
}

void
OVT_MemList_FreeNBAMemory ( byte * name, uint64 moreThan, int64 always )
{
    NamedByteArray *nba = _OVT_Find_NBA ( name ) ;
    _OVT_MemList_FreeNBAMemory ( nba, moreThan, always ) ;
}

void
OVT_MemListFree_ContextMemory ( )
{
    OVT_MemList_FreeNBAMemory ( ( byte* ) "ContextSpace", 1 * M, 1 ) ;
}

void
OVT_MemListFree_TempObjects ( )
{
    OVT_MemList_FreeNBAMemory ( ( byte* ) "TempObjectSpace", 1 * M, 1 ) ;
}

void
OVT_MemListFree_Objects ( )
{
    OVT_MemList_FreeNBAMemory ( ( byte* ) "ObjectSpace", 1 * M, 1 ) ;
}

void
_OVT_MemListFree_LispSpace ( )
{
    OVT_MemList_FreeNBAMemory ( ( byte* ) "LispSpace", 1 * M, 1 ) ;
}

void
OVT_MemListFree_LispSpace ( )
{
    _LC_Delete ( _LC_ ) ;
}

void
_OVT_MemListFree_WordRecyclingSpace ( )
{
    OVT_MemList_FreeNBAMemory ( ( byte* ) "WordRecylingSpace", 0, 1 ) ;
}

void
_OVT_MemListFree_CompilerTempObjectSpace ( )
{
    OVT_MemList_FreeNBAMemory ( ( byte* ) "CompilerTempObjectSpace", 0, 1 ) ;
}

void
OVT_MemListFree_CompilerTempObjects ( )
{
    _OVT_MemListFree_CompilerTempObjectSpace ( ) ;
}

void
OVT_MemListFree_LispTemp ( )
{
    OVT_MemList_FreeNBAMemory ( ( byte* ) "LispTempSpace", 2 * M, 1 ) ;
    if ( _LC_ )
    {
        _dllist_Init ( _LC_->LispTempNamespace->W_List ) ;
        LC_Init_Variables ( _LC_ ) ;
    }
}

void
OVT_MemListFree_Session ( )
{
    OVT_MemList_FreeNBAMemory ( ( byte* ) "SessionObjectsSpace", 2 * M, 1 ) ;
}

void
OVT_MemListFree_Buffers ( )
{
    OVT_MemList_FreeNBAMemory ( ( byte* ) "BufferSpace", 2 * M, 0 ) ;
}

void
OVT_MemListFree_HistorySpace ( )
{
    OVT_MemList_FreeNBAMemory ( ( byte* ) "HistorySpace", 1 * M, 0 ) ;
}

void
_OVT_MemListFree_CSLInternal ( )
{
    OVT_MemList_FreeNBAMemory ( ( byte* ) "CSLInternalSpace", 0, 1 ) ;
}

void
_NBAsMemList_FreeTypes ( int64 allocType, int64 exactFlag )
{
    dllist_Map2 ( _O_->MemorySpace0->NBAs, ( MapFunction2 ) NBA_FreeChunkType, allocType, exactFlag ) ;
}

void
NBAsMemList_FreeExactType ( int64 allocType )
{
    _NBAsMemList_FreeTypes ( allocType, 1 ) ;
}

void
NBAsMemList_FreeVariousTypes ( int64 allocType )
{
    _NBAsMemList_FreeTypes ( allocType, 0 ) ;
}

