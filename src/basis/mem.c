
#include "../include/csl.h"

void
NBA_Show ( NamedByteArray * nba, int64 flag )
{
    NBA_PrintInfo ( nba ) ;
    if ( flag && ( _O_->Verbosity > 1 ) )
    {
        dlnode * node, *nodeNext ;
        for ( node = dllist_First ( ( dllist* ) & nba->NBA_BaList ) ; node ; node = nodeNext )
        {
            nodeNext = dlnode_Next ( node ) ;
            ByteArray * ba = Get_BA_Symbol_To_BA ( node ) ;
            MemChunk_Show ( &ba->BA_MemChunk ) ;
        }
    }
}

void
NBA_PrintInfo ( NamedByteArray * nba )
{
    byte * name = nba->NBA_Symbol.S_Name ;
    if ( _O_->Verbosity > 1 )
    {
        Printf ( "\n%-23s InUse = " INT_FRMT_9 " : Unused = " INT_FRMT_9 " : Allocations = %4d : Largest = %8d : Smallest = %8d : AllocSize = %8d : OrigAllocSize = %8d",
            name, nba->MemAllocated - nba->MemRemaining, nba->MemRemaining, nba->Allocations, nba->LargestRemaining,
            nba->SmallestRemaining, nba->NBA_DataSize, nba->OriginalSize ) ;
    }
    else if ( _O_->Verbosity > 2 )
    {
        Printf ( "\n%-23s InUse = " INT_FRMT_9 " : Unused = " INT_FRMT_9 " : Allocations = %4d : Largest = %8d : Smallest = %8d : AllocSize = %8d : OrigAllocSize = %8d : alloctype = %8lu ",
            name, nba->MemAllocated - nba->MemRemaining, nba->MemRemaining, nba->Allocations, nba->LargestRemaining,
            nba->SmallestRemaining, nba->NBA_DataSize, nba->OriginalSize, ( uint64 ) nba->NBA_AAttribute ) ;
    }
    else
    {
        Printf ( "\n%-23s InUse = " INT_FRMT_9 " : Unused = " INT_FRMT_9 " : Allocations = %4d : Largest = %8d : Smallest = %8d : AllocSize = %8d",
            name, nba->MemAllocated - nba->MemRemaining, nba->MemRemaining, nba->Allocations, nba->LargestRemaining, nba->SmallestRemaining, nba->NBA_DataSize ) ;
    }
}

void
NBA_AccountRemainingAndShow ( NamedByteArray * nba, Boolean flag )
{
    if ( nba )
    {
        byte * name = nba->NBA_Symbol.S_Name ;
        int64 largest = 0, smallest = 0 ;
        nba->MemRemaining = 0 ;
        dlnode * node, *nodeNext ;
        for ( node = dllist_First ( ( dllist* ) & nba->NBA_BaList ) ; node ; node = nodeNext )
        {
            nodeNext = dlnode_Next ( node ) ;
            ByteArray * ba = Get_BA_Symbol_To_BA ( node ) ;
            nba->MemRemaining += ba->MemRemaining ;
            if ( ! largest ) largest = ba->MemRemaining ;
            else if ( ba->MemRemaining > largest ) largest = ba->MemRemaining ;
            if ( ! smallest ) smallest = ba->MemRemaining ;
            else if ( ba->MemRemaining < smallest ) smallest = ba->MemRemaining ;
        }
        nba->LargestRemaining = largest ;
        nba->SmallestRemaining = smallest ;
        if ( flag ) NBA_PrintInfo ( nba ) ;
    }
}

void
OVT_ShowNBAs ( OpenVmTil * ovt, int64 flag )
{
    if ( ovt )
    {
        dlnode * node, *nodeNext ;
        if ( ovt->MemorySpace0 && ( node = dllist_First ( ( dllist* ) ovt->MemorySpace0->NBAs ) ) )
        {
            for ( ; node ; node = nodeNext )
            {

                nodeNext = dlnode_Next ( node ) ;
                NamedByteArray * nba = Get_NbaSymbolNode_To_NBA ( node ) ;
                NBA_Show ( nba, flag ) ;
            }
        }
        printf ( "\n" ) ;
        fflush ( stdout ) ;
    }
}

int64
_OVT_CalculateShowMemList ( OpenVmTil * ovt )
{
    int64 size ;
    if ( ovt )
    {
        int64 diff ;
        dlnode * node, *nodeNext ;
        if ( ovt->Verbosity > 2 ) printf ( "\nMemChunk List :: " ) ;
        for ( size = 0, node = dllist_First ( ( dllist* ) _OMS_->OvtMemChunkList ) ; node ; node = nodeNext )
        {
            nodeNext = dlnode_Next ( node ) ;
            if ( ovt->Verbosity > 2 ) MemChunk_Show ( ( MemChunk * ) node ) ;
            size += ( ( MemChunk * ) node )->mc_ChunkSize ;
        }
        for ( node = dllist_First ( ( dllist* ) & _OSMS_->HistorySpace_MemChunkStringList ) ; node ; node = nodeNext )
        {
            nodeNext = dlnode_Next ( node ) ;
            if ( ovt->Verbosity > 2 ) MemChunk_Show ( ( MemChunk * ) node ) ;
            size += ( ( MemChunk * ) node )->mc_ChunkSize ;
        }
        for ( node = dllist_First ( _OSMS_->OVT_StaticMemList ) ; node ; node = nodeNext )
        {
            nodeNext = dlnode_Next ( node ) ;
            if ( ovt->Verbosity > 2 ) MemChunk_Show ( ( MemChunk * ) node ) ;
            size += ( ( MemChunk * ) node )->mc_ChunkSize ;
        }
        diff = _OMS_->RemainingAllocated - size - _OSMS_->HistoryAllocated ;
        if ( diff ) //|| ovt->Verbosity > 2 )
        {
            printf ( "\nTotal Size = %9ld : _OMS_->RemainingAllocated = %9ld :: diff = %6ld", size, _OMS_->RemainingAllocated, diff ) ;
            fflush ( stdout ) ;
        }
        Printf ( "\nNon-Static Mem Allocated      = %9ld", ovt->TotalNbaAccountedMemAllocated ) ;
        //printf ( "\n_OS_->Mmap_RemainingMemoryAllocated                     = %9ld : <=: _OS_->Mmap_RemainingMemoryAllocated", _OS_->Mmap_RemainingMemoryAllocated ) ;
    }
    ovt->PermanentMemListRemainingAccounted = size ;

    return size ;
}

void
_OVT_CalculateAndShow_TotalNbaAccountedMemAllocated ( OpenVmTil * ovt, Boolean showFlag )
{
    if ( ovt )
    {
        dlnode * node, * nextNode, *msNode, *nextMsNode ;
        NamedByteArray * nba ;
        MemorySpace * ms ;
        //Boolean flag = 0 ;
        ovt->TotalNbaAccountedMemRemaining = 0 ;
        ovt->TotalNbaAccountedMemAllocated = 0 ;
        if ( ovt && ovt->MemorySpace0 )
        {
            for ( msNode = dllist_First ( ( dllist* ) ovt->MemorySpaceList ) ; msNode ; msNode = nextMsNode )
            {
                ms = ( MemorySpace * ) msNode ;
                nextMsNode = dlnode_Next ( msNode ) ;
                for ( node = dllist_First ( ( dllist* ) ms->NBAs ) ; node ; node = nextNode )// nb. NBAs is the NBA Symbol list
                {
                    nextNode = dlnode_Next ( node ) ;
                    nba = ( NBA* ) Get_NbaSymbolNode_To_NBA ( node ) ;
                    NBA_AccountRemainingAndShow ( nba, showFlag ) ;
                    ovt->TotalNbaAccountedMemAllocated += nba->TotalAllocSize ;
                    ovt->TotalNbaAccountedMemRemaining += nba->MemRemaining ;
                }
                for ( node = dllist_First ( ( dllist* ) ovt->NBAs ) ; node ; node = nextNode )// nb. NBAs is the NBA Symbol list
                {
                    nextNode = dlnode_Next ( node ) ;
                    nba = ( NBA* ) Get_NbaSymbolNode_To_NBA ( node ) ;
                    if ( nba ) // ?? should not need this
                    {
                        NBA_AccountRemainingAndShow ( nba, showFlag ) ;
                        ovt->TotalNbaAccountedMemAllocated += nba->TotalAllocSize ;
                        ovt->TotalNbaAccountedMemRemaining += nba->MemRemaining ;
                    }
                }
            }
        }
    }
}

void
OVT_CalculateAndShow_TotalNbaAccountedMemAllocated ( OpenVmTil * ovt, int64 flag )
{
    _OVT_CalculateAndShow_TotalNbaAccountedMemAllocated ( ovt, flag ) ;
    if ( _CSL_ && _DataStack_ ) // so we can use this function anywhere
    {
        int64 dsu = DataStack_Depth ( ) * sizeof (int64 ) ;
        int64 dsa = ( STACK_SIZE * sizeof (int64 ) ) - dsu ;
        Printf ( "\nData Stack              InUse = %9d : Unused = %9d", dsu, dsa ) ;
    }
    printf ( "\nTotal Accounted Mem     InUse = %9ld : Unused = %9ld",
        ovt->TotalNbaAccountedMemAllocated - ovt->TotalNbaAccountedMemRemaining, ovt->TotalNbaAccountedMemRemaining ) ;
    fflush ( stdout ) ;
}

void
_OVT_ShowMemoryAllocated ( OpenVmTil * ovt )
{
    Boolean vf = ( ovt->Verbosity > 1 ) ;
    if ( ! vf ) Printf ( c_gu ( "\nIncrease the verbosity setting to 2 or more for more info here. ( Eg. : verbosity 2 = )" ) ) ;
    OVT_CalculateAndShow_TotalNbaAccountedMemAllocated ( ovt, 1 ) ;
    _OVT_CalculateShowMemList ( ovt ) ;
    int64 leak = abs ( ovt->PermanentMemListRemainingAccounted - _OMS_->RemainingAllocated ) - _OSMS_->HistoryAllocated ; //( _OMS_->Allocated - _OMS_->Freed ) ) ; //- _OS_->HistoryAllocation ; //sizeof (OVT_Static ) ;
    int64 memDiff2 = _OMS_->RemainingAllocated - ovt->PermanentMemListRemainingAccounted - _OSMS_->HistoryAllocated ;
    byte * memDiff2s = ( byte* ) "\nCurrent Unaccounted Diff (leak?) = %9d : <=: _OS_->Mmap_RemainingMemoryAllocated - ovt->PermanentMemListRemainingAccounted" ;
    byte * leaks = ( byte* ) "\nleak?                            = %9d : <=  ( mmap_TotalMemAllocated - mmap_TotalMemFreed ) - ( _OS_->TotalMemAllocated - _OS_->TotalMemFreed ) - _OS_->HistoryAllocation -_OS_->StaticAllocation" ;
    if ( memDiff2 ) Printf ( c_ad ( memDiff2s ), memDiff2 ) ;
    else if ( vf ) Printf ( c_ud ( memDiff2s ), memDiff2 ) ;
    if ( leak ) Printf ( c_ad ( leaks ), leak ) ;
    else if ( vf ) Printf ( c_ud ( leaks ), leak ) ;
    if ( memDiff2 || leak || vf )
    {
        Printf ( "\nTotalNbaAccountedMemAllocated      = %9d : <=: ovt->TotalNbaAccountedMemAllocated", ovt->TotalNbaAccountedMemAllocated ) ;
        Printf ( "\nMem Used - Categorized  = %9d : <=: ovt->TotalNbaAccountedMemAllocated - ovt->TotalNbaAccountedMemRemaining", ovt->TotalNbaAccountedMemAllocated - ovt->TotalNbaAccountedMemRemaining ) ; //+ ovt->UnaccountedMem ) ) ;
        Printf ( "\nTotalNbaAccountedMemRemaining      = %9d : <=: ovt->TotalNbaAccountedMemRemaining", ovt->TotalNbaAccountedMemRemaining ) ;
        Printf ( "\nMmap_RemainingMemoryAllocated      = %9d : <=: _OS_->Mmap_RemainingMemoryAllocated", _OMS_->RemainingAllocated ) ;
        Printf ( "\nPermanentMemListRemainingAccounted = %9d : <=: ovt->PermanentMemListRemainingAccounted", ovt->PermanentMemListRemainingAccounted ) ; //+ ovt->UnaccountedMem ) ) ;
        Printf ( "\nTotal Mem Remaining = %9d : <=: _OS_->TotalMemAllocated - _OS_->TotalMemFreed", _OMS_->Allocated - _OMS_->Freed ) ; //+ ovt->UnaccountedMem ) ) ;
        Printf ( "\nTotal Mem Allocated = %9d : <=: _OS_->TotalMemAllocated", _OSMS_->Static_Allocated ) ; //+ ovt->UnaccountedMem ) ) ;
        Printf ( "\nTotal Mem Freed     = %9d : <=: _OS_->TotalMemFreed", _OMS_->Freed ) ; //+ ovt->UnaccountedMem ) ) ;
        Printf ( "\nTotal Mem Remaining = %9d : <=: _OS_->TotalMemAllocated - _OS_->TotalMemFreed", _OMS_->Allocated - _OMS_->Freed ) ; //+ ovt->UnaccountedMem ) ) ;
        Printf ( "\nOVT_MmapAllocated   = %9d", _OMS_->Allocated ) ;
    }
    Printf ( "\nHistoryAllocation             = %9d", _OSMS_->HistoryAllocated ) ;
    Printf ( "\nTotal Memory leaks            = %9d", leak ) ;

    Printf ( "\nNBA ReAllocations             = %9d", _O_->ReAllocations ) ;
    int64 wordSize = ( sizeof ( Word ) + sizeof ( WordData ) ) ;
    Printf ( "\nRecycledWordCount             = %9d : %-5d x %3d bytes", _O_->MemorySpace0->RecycledWordCount * wordSize, _O_->MemorySpace0->RecycledWordCount, wordSize ) ;
    Printf ( "\nWordsInRecycling              = %9d : %-5d x %3d bytes\n", _O_->MemorySpace0->WordsInRecycling * wordSize, _O_->MemorySpace0->WordsInRecycling, wordSize ) ;
    Buffer_PrintBuffers ( ) ;
}

// 'mem'

void
OVT_ShowMemoryAllocated ( )
{
    _OVT_ShowMemoryAllocated ( _O_ ) ;
}

void
_OVT_CheckCodeSpaceForRoom ( int64 memDesired )
{
    _O_CodeByteArray = _ByteArray_AppendSpace_MakeSure ( _O_CodeByteArray, memDesired ) ;
}

void
OVT_CheckCodeSpaceForRoom ( )
{
    _OVT_CheckCodeSpaceForRoom ( 4 * K ) ;
}

// only check a first node, if no first node the list is empty

byte *
DLList_CheckRecycledForAllocation ( dllist * list, int64 size )
{
    DLNode * node = 0 ;
    if ( list ) node = ( DLNode* ) dllist_First ( ( dllist* ) list ) ;
    if ( node )
    {
        dlnode_Remove ( ( dlnode* ) node ) ; // necessary else we destroy the list!
        Mem_Clear ( ( byte* ) node, size ) ;
        node->n_DLNode.n_InUseFlag = N_IN_USE ;
        return ( byte* ) node ;
    }
    else return 0 ;
}

void
OVT_RecyclingAccounting ( int64 flag )
{
    if ( flag == OVT_RA_RECYCLED )
    {
        _O_->MemorySpace0->RecycledWordCount ++ ;
        _O_->MemorySpace0->WordsInRecycling -- ;
    }
    else if ( flag == OVT_RA_ADDED ) _O_->MemorySpace0->WordsInRecycling ++ ;

}

void
OVT_Recycle ( dlnode * anode )
{
    if ( anode ) dllist_AddNodeToTail ( _O_->RecycledWordList, anode ) ;
    OVT_RecyclingAccounting ( OVT_RA_ADDED ) ;
}

// put a word on the recycling list

void
Word_Recycle ( Word * w )
{
    OVT_Recycle ( ( dlnode * ) w ) ;
    w->W_ObjectAttributes &= ~ ( RECYCLABLE_COPY | RECYCLABLE_LOCAL ) ;
}

void
dbg_new ( Word * w )
{
    if ( ( uint64 ) w == 0x7ffff4928020 )
        Printf ( "\n got it at %s", Context_Location ( ) ) ;
}

void
_CheckRecycleWord ( Node * node )
{
    Word * w = ( Word * ) node ;
    if ( w && ( w->W_ObjectAttributes & ( RECYCLABLE_COPY | RECYCLABLE_LOCAL ) ) )
    {
        if ( _O_->Verbosity > 2 ) _Printf ( "\n_CheckRecycleWord : recycling : %s%s%s",
            w->S_ContainingNamespace ? w->S_ContainingNamespace->Name : ( byte* ) "", w->S_ContainingNamespace ? ( byte* ) "." : ( byte* ) "", w->Name ) ; //, Pause () ;
        Word_Recycle ( w ) ;
    }
}

// check a compiler word list for recycleable words and add them to the recycled word list : _O_->MemorySpace0->RecycledWordList

void
DLList_Recycle_NamespaceList ( dllist * list )
{
    dllist_Map ( list, ( MapFunction0 ) _CheckRecycleWord ) ;
}

