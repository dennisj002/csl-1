#include "../../include/csl.h"
#define OP_EQ_STORE 0
/* ------
 * 2 args
 * ----------
 * lit lit op 
 * lit var op
 * var lit op
 * var var op
 * -------
 * op args
 * -------
 * lit op op
 * op lit op
 * op var op
 * var op op
 * op op op
 * 
 * x xword [var|lit|op] op :: xword = nonvar, nonlit, nonop
 * [var|lit|op] [var|lit|op] op :: 9 combinations + x betweenWord [var|lit|op] op :: 3 combinations = 12 total combinations
 * ----------------------------
 * 1 arg
 * -----
 * [var|lit|op] op :: 3 combinations
 * 15 total total combinations
 * 
 */
// remember this is rpn code so arg 1, the first (linearly) arg, is lower on the stack (STACK_1) than arg2, the second arg, (STACK_0)
// eg:
//      : test arg1 arg2 op ;  
// stack conventions :
// ________
// | arg2 | TOS : OREG : RCX
// | arg1 | NOS : ACC  : RAX
// --------
// need to consider also REGISTER args/parameters !!
// but we still need to consider types (sizes) and type checking
// and ARM cpu adjustments

// get arg actual setup
// convert arg setup to standard arg1=ACC, arg2=OREG (RCX)
// setup machine insn parameters 

// standard locations are :: arg1=reg=ACC=RAX ; arg2=rm=OREG=RCX
// for 2 arg ops : STACK_1= arg1 : STACK_0 = arg2 

int64
_Compiler_CheckOptimize ( Compiler * compiler, int64 _specialReturn )
{
    int64 specialReturn = _specialReturn ? _specialReturn : compiler->OptimizeForcedReturn ;
    if ( ( ! specialReturn ) && GetState ( _CSL_, OPTIMIZE_ON ) )
    {
        _specialReturn = Compiler_Optimize ( compiler, CSL_WordList ( 0 ) ) ;
    }
    return _specialReturn ? _specialReturn : compiler->OptimizeForcedReturn ;
}

int64
Compiler_CheckOptimize ( Compiler * compiler, int64 specialReturn )
{
    return _Compiler_CheckOptimize ( compiler, specialReturn ) ;
}

Word *
Compiler_CheckForOpBetweenParentheses ( CompileOptimizeInfo * optInfo, dlnode * snode )
{
    Word *wordn, * rvalue = 0 ;
    for ( optInfo->node = snode ; optInfo->node ; optInfo->node = optInfo->nextNode )
    {
        optInfo->nextNode = dlnode_Next ( optInfo->node ) ;
        if ( dobject_Get_M_Slot ( ( dobject* ) optInfo->node, SCN_IN_USE_FLAG ) & SCN_IN_USE_FOR_OPTIMIZATION )
            wordn = ( Word* ) dobject_Get_M_Slot ( ( dobject* ) optInfo->node, SCN_T_WORD ) ;
        else continue ;
        //if ( ( optInfo->wordn->CAttribute & ( CATEGORY_OP | COMBINATOR ) ) && ( ! (optInfo->wordn->CAttribute & CATEGORY_PLUS_PLUS_MINUS_MINUS) ) ) rvalue = true ;
        if ( wordn->W_MorphismAttributes & ( CATEGORY_OP | COMBINATOR ) ) rvalue = optInfo->wordn ;
        else if ( wordn->W_MorphismAttributes & ( LEFT_PAREN ) ) break ;
    }
    return rvalue ;
}

// conventions :
// the 'stack' - list starts with the top being the last word processed in an rpn ordering, next on the list is the previous to last word, ...
// nb! : remember this is a rpn optimizer ; get second arg first when going right to left : arg1 arg2 op :: so op is first then arg2 then arg1
// standard locations are :: arg1=reg=ACC=RAX ; arg2=rm=OREG=RCX
// for 2 arg ops : STACK_1= arg1 : STACK_0 = arg2 
// next on this list is a 'previous' (because again this is rpn ordering; the syntactic transformation processing needs to put in rpn ordering to get here)

void
_Compiler_GetWordStackState ( Compiler * compiler, Word * word )
{
    CompileOptimizeInfo * optInfo ;
    int64 state = compiler->OptInfo->State ;
    compiler->OptInfo = Compiler_CompileOptimizeInfo_PushNew ( compiler ) ;
    optInfo = compiler->OptInfo ;
    CompileOptimizeInfo_Init ( optInfo, state ) ; // State : not used yet ??
    optInfo->opWord = word ;
    SetState ( _CSL_, IN_OPTIMIZER, true ) ;
    for ( optInfo->node = optInfo->wordNode = dllist_First ( ( dllist* ) _CSL_->CSL_N_M_Node_WordList ), optInfo->node = dlnode_Next ( optInfo->node ) ;
        optInfo->node ; optInfo->node = optInfo->nextNode )
    {
        optInfo->nextNode = dlnode_Next ( optInfo->node ) ;
        if ( dobject_Get_M_Slot ( ( dobject* ) optInfo->node, SCN_IN_USE_FLAG ) & SCN_IN_USE_FOR_OPTIMIZATION )
        {
            optInfo->wordn = ( Word* ) dobject_Get_M_Slot ( ( dobject* ) optInfo->node, SCN_T_WORD ) ;
            //byte * name = optInfo->wordn->Name ; //debugging
        }
        else continue ;
        if ( optInfo->wordn->W_MorphismAttributes & ( NO_OP_WORD | LEFT_PAREN ) ) continue ;
        else if ( ( optInfo->wordn->W_MorphismAttributes & ( CATEGORY_OP ) ) || ( optInfo->wordn->W_MorphismAttributes & ( RIGHT_PAREN | RIGHT_BRACKET ) ) )
        {
            if ( ( optInfo->wordn->W_MorphismAttributes & RIGHT_PAREN ) )
            {
                dlnode * nextNode = optInfo->nextNode ;
                Word * wordn = Compiler_CheckForOpBetweenParentheses ( optInfo, optInfo->node ) ;
                if ( ! wordn )
                {
                    optInfo->nextNode = nextNode ;
                    continue ;
                }
                else
                {
                    if ( optInfo->lparen2 ) optInfo->lparen1 = wordn ;
                    else optInfo->lparen2 = wordn ;
                }
            }
            if ( optInfo->wordn->Definition == CSL_DoubleQuoteMacro ) continue ;
            else if ( optInfo->wordn->W_MorphismAttributes & ( CATEGORY_OP_LOAD ) )
            {
                optInfo->rvalue ++ ; // ++ : for recursive peek constructions like @ @ and @ @ @ etc.
                continue ;
            }
            else
            {
                optInfo->NumberOfArgs ++ ;
                if ( optInfo->wordArg2 )
                {
                    optInfo->wordArg1 = optInfo->wordn ;
                    optInfo->wordArg1Node = optInfo->node ;
                    optInfo->wordArg1_Op = true ;
                }
                else
                {
                    optInfo->wordArg2 = optInfo->wordn ;
                    optInfo->wordArg2Node = optInfo->node ;
                    optInfo->wordArg2_Op = true ;
                }
                //if ( optInfo->wordn->CAttribute2 & LEFT_PAREN ) ; else break ;
                break ;
            }
        }
        else if ( IS_MORPHISM_TYPE ( optInfo->wordn ) ) //&& ( ! IS_NON_MORPHISM_TYPE(optInfo->wordn) ) )
        {
            if ( optInfo->wordArg2 ) optInfo->xBetweenArg1AndArg2 = optInfo->wordn ;
            else optInfo->wordArg2 = optInfo->wordn ;
            break ; // no optimization possible if ( ( ! optInfo->wordArg2 ) && ( IS_MORPHISM_TYPE ( optInfo->wordn ) ) )
        }
        else if ( optInfo->wordArg2 )
        {
            optInfo->NumberOfArgs ++ ;
            optInfo->wordArg1 = optInfo->wordn ;
            optInfo->wordArg1Node = optInfo->node ;
            optInfo->wordArg1_rvalue = optInfo->rvalue ? optInfo->rvalue :
                ( ( GetState ( _Context_, INFIX_MODE ) || GetState ( compiler, LISP_MODE ) )
                && ( ! ( optInfo->opWord->W_MorphismAttributes & ( CATEGORY_OP_EQUAL ) ) ) ) ; // rem : rvalue can be higher than 1 (cf. above for '@ @')
            optInfo->rvalue = false ;
            if ( optInfo->wordArg1->W_ObjectAttributes & ( CONSTANT | LITERAL ) )
            {
                optInfo->wordArg1_literal = true ;
                optInfo->Optimize_Imm = optInfo->wordArg2->W_Value ;
            }
            break ;
        }
        else // nb! : remember this is a rpn optimizer ; get second arg first when going right to left : arg1 arg2 op 
        {
            optInfo->NumberOfArgs ++ ;
            optInfo->wordArg2 = optInfo->wordn ;
            optInfo->wordArg2Node = optInfo->node ;
            optInfo->wordArg2_rvalue = optInfo->rvalue ? optInfo->rvalue :
                ( ( GetState ( _Context_, INFIX_MODE ) || GetState ( compiler, LISP_MODE ) )
                && ( ! ( optInfo->opWord->W_MorphismAttributes & ( CATEGORY_OP_STORE ) ) ) ) ; // rem : rvalue can be higher than 1 (cf. above for '@ @')
            optInfo->rvalue = false ;
            if ( optInfo->wordArg2->W_ObjectAttributes & ( CONSTANT | LITERAL ) )
            {
                optInfo->wordArg2_literal = true ;
                optInfo->Optimize_Imm = optInfo->wordArg2->W_Value ;
            }
            if ( optInfo->opWord->W_MorphismAttributes & ( CATEGORY_OP_LOAD ) && ( ! ( GetState ( _Context_, INFIX_MODE ) ) ) ) optInfo->wordArg2_rvalue ++ ;
            if ( optInfo->opWord->W_MorphismAttributes & ( CATEGORY_OP_1_ARG | CATEGORY_OP_STACK | CATEGORY_OP_LOAD ) ) break ;
        }
    }
}

int64
Compiler_Optimize ( Compiler * compiler, Word * word )
{
    if ( word )
    {
        if ( Is_DebugOn && ( _O_->Verbosity > 1 ) ) _CSL_SC_WordList_Show ( 0, 0, 0 ) ;
        _Compiler_GetWordStackState ( compiler, word ) ;
        Compiler_SetStandardPreHere_ForDebugDisassembly ( compiler ) ;
        Compiler_SetupArgsToStandardLocations ( compiler ) ;
        Setup_MachineCodeInsnParameters ( compiler, REG, REG, ACC, OREG, 0, 0 ) ;
        SetState ( _CSL_, IN_OPTIMIZER, false ) ;
        return compiler->OptInfo->rtrn ;
    }
    else return 0 ;
}

void
Compiler_SetStandardPreHere_ForDebugDisassembly ( Compiler * compiler )
{
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    if ( optInfo->wordArg1 && ( ! optInfo->wordArg1_Op ) ) SetPreHere_ForDebug ( optInfo->wordArg1->Coding ) ;
    else if ( optInfo->wordArg2 ) SetPreHere_ForDebug ( optInfo->wordArg2->Coding ) ;
}

// standard locations are :: arg1=reg=ACC=RAX ; arg2=rm=OREG=RCX
// for 2 arg ops : STACK_1= arg1 : STACK_0 = arg2 
// for 1 arg ops : STACK_0= arg1 

void
Compiler_SetupArgsToStandardLocations ( Compiler * compiler )
{
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    if ( optInfo->opWord->W_MorphismAttributes & ( CATEGORY_DUP ) ) Compile_Optimize_Dup ( compiler ) ;
    else if ( optInfo->wordArg1_literal && optInfo->wordArg2_literal ) Do_OptimizeOp2Literals ( compiler ) ;
    else if ( optInfo->wordArg2_Op || optInfo->xBetweenArg1AndArg2 ) Compiler_Optimizer_WordArg2Op_Or_xBetweenArg1AndArg2 ( compiler ) ;
    else if ( ( optInfo->NumberOfArgs == 2 ) && optInfo->wordArg2_literal && optInfo->opWord->W_OpInsnCode == CMP ) Compiler_Optimizer_Cmp_2Args_With_Arg2Literal ( compiler ) ;
    else if ( ( optInfo->NumberOfArgs == 2 ) && optInfo->wordArg2_literal ) Compiler_Optimizer_2Args_With_Arg2Literal ( compiler ) ;
    else if ( ( optInfo->NumberOfArgs == 2 ) || optInfo->wordArg1_Op ) Compiler_Optimizer_2Args_Or_WordArg1_Op ( compiler ) ;
    else if ( optInfo->NumberOfArgs == 1 ) Compiler_Optimizer_1Arg ( compiler ) ;
    else Compiler_Optimizer_0Args ( compiler ) ;
}

void
Compiler_Optimizer_Cmp_2Args_With_Arg2Literal ( Compiler * compiler )
{
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    if ( optInfo->wordArg1->W_ObjectAttributes & REGISTER_VARIABLE )
    {
        byte reg = optInfo->wordArg1->RegToUse ;
        reg |= REG_LOCK_BIT ;
        optInfo->Optimize_Reg = reg ;
        SetHere ( optInfo->wordArg1->Coding ? optInfo->wordArg1->Coding : optInfo->wordArg2->Coding, 1 ) ;
    }
    else if ( optInfo->wordArg1->StackPushRegisterCode ) _Set_To_Here_Word_StackPushRegisterCode ( optInfo->wordArg1, 1 ) ;
    compiler->OptInfo->Optimize_Imm = optInfo->wordArg2->S_Value ;
    optInfo->OptimizeFlag |= OPTIMIZE_IMM ;
}

void
Compiler_Optimizer_2Args_With_Arg2Literal ( Compiler * compiler )
{
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    if ( optInfo->opWord->W_MorphismAttributes & ( CATEGORY_OP_OPEQUAL | CATEGORY_OP_EQUAL ) ) Compiler_Optimizer_2Args_Or_WordArg1_Op ( compiler ) ;
    else
    {
        if ( optInfo->wordArg1->W_ObjectAttributes & REGISTER_VARIABLE )
        {
            byte rm = optInfo->wordArg1->RegToUse ;
            rm |= REG_LOCK_BIT ;
            optInfo->Optimize_Rm = rm ;
            SetHere ( optInfo->wordArg1->Coding, 1 ) ; //? optInfo->wordArg1->Coding : optInfo->wordArg2->Coding, 1 ) ;
            compiler->OptInfo->Optimize_Imm = optInfo->wordArg2->S_Value ;
            optInfo->OptimizeFlag |= OPTIMIZE_IMM ;
        }
        else Compiler_Optimizer_2Args_Or_WordArg1_Op ( compiler ) ;
        //if ( optInfo->wordArg1->StackPushRegisterCode ) _Set_To_Here_Word_StackPushRegisterCode ( optInfo->wordArg1, 1 ) ;
    }
}

void
Compiler_Optimizer_2Args_Or_WordArg1_Op ( Compiler * compiler )
{
    // we know ( ! ( optInfo->wordArg2_Op || optInfo->xBetweenArg1AndArg2 ) ) because we already tested for that
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    if ( optInfo->wordArg2->W_ObjectAttributes & DOBJECT ) Compile_StackArgsToStandardRegs ( compiler ) ;
    else
    {
        Boolean rm = ( optInfo->wordArg2->W_ObjectAttributes & REGISTER_VARIABLE ) ? optInfo->wordArg2->RegToUse : OREG ;
        if ( optInfo->opWord->W_MorphismAttributes & CATEGORY_OP_OPEQUAL )
        {
            if ( GetState ( _Context_, ( C_SYNTAX | INFIX_MODE ) ) ) SetHere ( optInfo->wordArg2->Coding, 0 ) ;
            else SetHere ( optInfo->wordArg1->Coding, 0 ) ;
        }
        else
        {
            optInfo->Optimize_Reg = ( ( optInfo->wordArg1->W_ObjectAttributes & REGISTER_VARIABLE ) ? optInfo->wordArg1->RegToUse : ACC ) | REG_LOCK_BIT ;
            if ( optInfo->wordArg1->W_ObjectAttributes & REGISTER_VARIABLE ) SetHere ( optInfo->wordArg1->Coding ? optInfo->wordArg1->Coding : optInfo->wordArg2->Coding, 1 ) ;
            else if ( optInfo->wordArg1->StackPushRegisterCode ) _Set_To_Here_Word_StackPushRegisterCode ( optInfo->wordArg1, 1 ) ;
            else
            {
                if ( GetState ( _Context_, ( C_SYNTAX | INFIX_MODE ) ) )
                {
                    SetHere ( optInfo->wordArg2->Coding, 0 ) ;
                    Compiler_Word_SCHCPUSCA ( optInfo->wordArg2, 0 ) ;
                    _Compile_Stack_PopToReg ( DSP, ACC ) ;
                }
                else
                {
                    SetHere ( optInfo->wordArg1->Coding, 0 ) ;
                    Compile_StandardArg ( optInfo->wordArg1, ACC, optInfo->wordArg1_rvalue, 0, true ) ;
                }
            }
            Compile_StandardArg ( optInfo->wordArg2, rm, optInfo->wordArg2_rvalue, 0, true ) ;
            optInfo->Optimize_Rm = rm | REG_LOCK_BIT ;
        }
    }
}

void
Compiler_Optimizer_WordArg2Op_Or_xBetweenArg1AndArg2 ( Compiler * compiler )
{
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    if ( ( optInfo->wordArg2 && optInfo->wordArg2->StackPushRegisterCode ) ||
        ( optInfo->xBetweenArg1AndArg2 && optInfo->xBetweenArg1AndArg2->StackPushRegisterCode ) )
    {
        Word_Check_ReSet_To_Here_StackPushRegisterCode ( optInfo->xBetweenArg1AndArg2, 1 ) ;
        Word_Check_ReSet_To_Here_StackPushRegisterCode ( optInfo->wordArg2, 1 ) ;
        if ( ! ( optInfo->opWord->W_MorphismAttributes & ( CATEGORY_OP_1_ARG | CATEGORY_OP_OPEQUAL ) ) )
        {
            if ( ( optInfo->wordArg2_literal ) && ( optInfo->opWord->W_OpInsnCode != CMP ) )
            {
                SetHere ( optInfo->wordArg2->Coding, 1 ) ;
                optInfo->OptimizeFlag |= OPTIMIZE_IMM ;
                optInfo->Optimize_Rm = DSP ;
                //optInfo->Optimize_Reg = DSP ; // nb : cmp  reg must be RAX
                optInfo->Optimize_Mod = MEM ;
                optInfo->rtrn = 1 ;
            }
            else if ( ! ( ( optInfo->opWord->W_MorphismAttributes & ( CATEGORY_OP_EQUAL ) ) && Compile_Optimize_EqualCheck ( compiler ) ) )
#if 1          
            {
                Compiler_Word_SCHCPUSCA ( optInfo->wordArg2, 1 ) ;
                Compile_Move_Reg_To_Reg ( OREG, ACC, 0 ) ;
                _Compile_Stack_PopToReg ( DSP, ACC ) ;
                optInfo->Optimize_Reg = ACC | REG_LOCK_BIT ; // REG_LOCK_BIT : let Setup_MachineCodeInsnParameters know we have a parameter for it in case of ACC == 0
                optInfo->Optimize_Rm = OREG ;
            }
#else
                {
                    //Compiler_Word_SCHCPUSCA( optInfo->wordArg2, 1 ) ;
                    //Compile_Move_Reg_To_Reg ( OREG, ACC, 0 ) ;
                    if ( ( optInfo->wordArg2_literal ) && ( optInfo->opWord->W_OpInsnCode != CMP ) ) SetHere ( optInfo->wordArg1->Coding, 1 ) ;
                    else Word_Check_ReSet_To_Here_StackPushRegisterCode ( optInfo->wordArg1, 1 ) ;
                    _Compile_Stack_PopToReg ( DSP, OREG ) ;
                    optInfo->Optimize_Reg = OREG ;
                    optInfo->Optimize_Rm = DSP ;
                    optInfo->rtrn = 1 ;
                }
#endif            
else Word_Check_ReSet_To_Here_StackPushRegisterCode ( optInfo->wordArg2, 1 ) ; // the rest of the code will be handled in Compile_Optimize_Equal
        }
    }
    else if ( optInfo->NumberOfArgs )
    {
        if ( ( optInfo->wordArg2->W_MorphismAttributes & ( RIGHT_BRACKET ) ) && ( ( GetState ( _Context_, C_SYNTAX | INFIX_MODE ) || GetState ( compiler, LC_ARG_PARSING ) ) ) )
        {
            Compile_Move_Rm_To_Reg ( OREG, DSP, 0, 0 ) ;
            Compile_Move_Rm_To_Reg ( OREG, OREG, 0, 0 ) ;
            _Compile_Move_StackN_To_Reg ( ACC, DSP, - 1 ), optInfo->Optimize_Reg = ACC | REG_LOCK_BIT ;
            Compile_SUBI ( REG, DSP, 0, 2 * CELL, 0 ) ;
        }

        else Compile_StackArgsToStandardRegs ( compiler ) ;
    }
}

void
Compiler_Optimizer_0Args ( Compiler * compiler )
{
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    if ( optInfo->opWord->W_MorphismAttributes & CATEGORY_OP_LOAD )
    {
        CompileOptimizedLoad_TOS ( ) ;
        optInfo->rtrn = OPTIMIZE_DONE ;
    }
    else if ( optInfo->opWord->W_MorphismAttributes & CATEGORY_OP_1_ARG )
    {
        if ( optInfo->wordArg2 )
        {
            if ( optInfo->wordArg2->StackPushRegisterCode ) _Set_To_Here_Word_StackPushRegisterCode ( optInfo->wordArg2, 0 ) ;
            else
            {
                _Compile_Move_StackN_To_Reg ( ACC, DSP, 0 ), optInfo->Optimize_Reg = ACC | REG_LOCK_BIT ;
                Compile_SUBI ( REG, DSP, 0, CELL, 0 ) ;
            }
        }
        else Word_Check_ReSet_To_Here_StackPushRegisterCode ( optInfo->opWord, 0 ) ;
    }

    else Compile_StackArgsToStandardRegs ( compiler ) ;
}

void
Compiler_Optimizer_1Arg ( Compiler * compiler )
{
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    // remember : CATEGORY_DUP && CATEGORY_OP_LOAD were handled already in Compiler_SetupArgsToStandardLocations.Compiler_Optimizer_0Args
    if ( optInfo->opWord->W_MorphismAttributes & ( CATEGORY_OP_LOAD ) ) Compiler_CompileOptimizedLoad ( compiler ) ;
    else if ( optInfo->opWord->W_MorphismAttributes & ( CATEGORY_PLUS_PLUS_MINUS_MINUS ) ) Compiler_CompileOptimize_IncDec ( compiler ) ;
    else if ( optInfo->wordArg2->StackPushRegisterCode ) _Set_To_Here_Word_StackPushRegisterCode ( optInfo->wordArg2, 0 ), optInfo->Optimize_Reg = ACC | REG_LOCK_BIT ;

    else _Compile_Move_StackN_To_Reg ( ACC, DSP, 0 ), optInfo->Optimize_Reg = ACC | REG_LOCK_BIT ;
}

void
Compile_StandardArg ( Word * word, Boolean reg, Boolean rvalueFlag, byte * setHere, Boolean setScaFlag )
{
    if ( setHere ) SetHere ( setHere, 1 ) ;
    if ( setScaFlag ) Compiler_Word_SCHCPUSCA ( word, 0 ) ;
    if ( rvalueFlag -- )
    {
        if ( word->W_ObjectAttributes & REGISTER_VARIABLE ) reg = word->RegToUse ;
        else Compile_GetVarLitObj_RValue_To_Reg ( word, reg, 8 ) ;
        while ( rvalueFlag -- ) Compile_Move_Rm_To_Reg ( reg, reg, 0, 0 ) ;
    }

    else _Compile_GetVarLitObj_LValue_To_Reg ( word, reg, 8 ) ;
    word->StackPushRegisterCode = Here ; // we are not pushing this but in case we are just rewriting the code in the next arg ?
}

void
Compile_StackArgsToStandardRegs ( Compiler * compiler )
{

    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    _Compile_Move_StackN_To_Reg ( OREG, DSP, 0 ), optInfo->Optimize_Rm = OREG ;
    _Compile_Move_StackN_To_Reg ( ACC, DSP, - 1 ), optInfo->Optimize_Reg = ACC | REG_LOCK_BIT ;
    Compile_SUBI ( REG, DSP, 0, 2 * CELL, 0 ) ;
    SetState ( optInfo, STACK_ARGS_TO_STANDARD_REGS, true ) ;
}

Boolean
CheckForRegisterVariable ( Compiler * compiler, Boolean reg )
{
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    if ( ( reg == OREG ) && ( optInfo->wordArg2 && ( optInfo->wordArg2->W_ObjectAttributes & REGISTER_VARIABLE ) ) ) reg = optInfo->wordArg2->RegToUse ;
    else if ( ( reg == ACC ) && ( optInfo->wordArg1 && ( optInfo->wordArg1->W_ObjectAttributes & REGISTER_VARIABLE ) ) ) reg = optInfo->wordArg1->RegToUse ;

    return reg ;
}

// if we have a parameter already set in optInfo honor it else use the default 
// adjust if needed

// standard locations are :: arg1=reg=ACC=RAX ; arg2=rm=OREG=RCX
// for 2 arg ops : STACK_1= arg1 : STACK_0 = arg2 
// for 1 arg ops : STACK_0= arg1 
// if ( optInfo->wordArg1 ) optInfo->wordArg1->RegToUse = optInfo->Optimize_Reg ;
// if ( optInfo->wordArg2 ) optInfo->wordArg2->RegToUse = optInfo->Optimize_Rm ;

void
Setup_MachineCodeInsnParameters ( Compiler * compiler, Boolean direction, Boolean mod, Boolean reg, Boolean rm, int64 disp, Boolean forceSet )
{
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    if ( ! optInfo->rtrn ) //!= OPTIMIZE_DONE )
    {
        if ( ( optInfo->opWord->W_MorphismAttributes & CATEGORY_OP_1_ARG ) && ( ! ( optInfo->wordArg2->W_ObjectAttributes & REGISTER_VARIABLE ) ) ) rm = ACC ;
        if ( ( optInfo->Optimize_Rm & REG_LOCK_BIT ) && ( ! forceSet ) ) optInfo->Optimize_Rm = ( optInfo->Optimize_Rm & 0xf ) ;
        else optInfo->Optimize_Rm = CheckForRegisterVariable ( compiler, rm ) ;
        if ( optInfo->opWord->W_MorphismAttributes & BIT_SHIFT ) optInfo->Optimize_Rm = RAX ;

        if ( ( optInfo->Optimize_Reg & REG_LOCK_BIT ) && ( ! forceSet ) ) optInfo->Optimize_Reg = ( optInfo->Optimize_Reg & 0xf ) ;
        else optInfo->Optimize_Reg = CheckForRegisterVariable ( compiler, reg ) ; // register variables override REG_ON_BIT

        optInfo->Optimize_Mod = mod ;
        optInfo->Optimize_Dest_RegOrMem = direction ;
        optInfo->Optimize_Disp = disp ;

        if ( ( optInfo->wordArg2 && optInfo->wordArg2->W_ObjectAttributes & REGISTER_VARIABLE )
            || ( optInfo->wordArg1 && optInfo->wordArg1->W_ObjectAttributes & REGISTER_VARIABLE ) ) optInfo->OptimizeFlag |= OPTIMIZE_REGISTER ;

        // standard arg arrangement
        if ( optInfo->wordArg1 ) optInfo->wordArg1->RegToUse = optInfo->Optimize_Reg ;
        if ( optInfo->wordArg2 ) optInfo->wordArg2->RegToUse = optInfo->Optimize_Rm ;

        Compiler_Word_SCHCPUSCA ( optInfo->opWord, 1 ) ;

        optInfo->rtrn = 1 ;
    }
}

void
Word_Set_StackPushRegisterCode_To_Here ( Word * word )
{
    word->StackPushRegisterCode = Here ;
}

void
_Set_To_Here_Word_StackPushRegisterCode ( Word * word, Boolean setDebugFlag )
{
    SetHere ( word->StackPushRegisterCode, setDebugFlag ) ;
}

Boolean
Word_Check_ReSet_To_Here_StackPushRegisterCode ( Word * word, Boolean setDebugFlag )
{
    if ( word && word->StackPushRegisterCode )
    {
        _Set_To_Here_Word_StackPushRegisterCode ( word, setDebugFlag ) ;
        return true ;
    }
    else return false ;
}

// nb! : assumes that reg 0

Word *
Compile_Optimize_EqualCheck ( Compiler * compiler )
{
    int64 depth = List_Depth ( compiler->OptimizeInfoList ) ;
    if ( depth )
    {
        CompileOptimizeInfo * coi = ( COI * ) List_Pick ( compiler->OptimizeInfoList, 1 ) ;
        Word * word ;
        dlnode * node, *nextNode ;
        if ( coi && ( coi->NumberOfArgs == 2 ) )
        {
            for ( node = coi->wordArg1Node ; node && ( nextNode = dlnode_Next ( node ) ) ; node = nextNode )
            {
                word = ( Word* ) dobject_Get_M_Slot ( ( dobject* ) nextNode, SCN_T_WORD ) ;
                if ( word->W_ObjectAttributes & ( LOCAL_VARIABLE | PARAMETER_VARIABLE ) )
                {
                    if ( word->W_ObjectAttributes & REGISTER_VARIABLE ) return compiler->OptInfo->wordArg0_ForOpEqual = word ;
                    break ;
                }
            }
        }
    }
    return 0 ;
}

void
Do_OptimizeOp2Literals ( Compiler * compiler )
{
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;

    if ( optInfo->wordArg1->Coding ) SetHere ( optInfo->wordArg1->Coding, 1 ) ;
    int64 value ;
    // a little tricky here ...
    // ?? maybe we should setup a special compiler stack and use it here ... ??
    DataStack_Push ( ( int64 ) * optInfo_0_two->W_PtrToValue ) ;
    DataStack_Push ( ( int64 ) * optInfo_0_one->W_PtrToValue ) ;
    SetState ( compiler, COMPILE_MODE, false ) ;
    SetState ( _CSL_, OPTIMIZE_ON, false ) ; //prevent recursion here
    //Word_Run ( optInfo_0_zero ) ;
    Block_Eval ( optInfo_0_zero->Definition ) ; // no type checking
    SetState ( _CSL_, OPTIMIZE_ON, true ) ; // restore state ; OPTIMIZE_ON had to be true/on else we wouldn't have entered _Compiler_CheckOptimize
    SetState ( compiler, COMPILE_MODE, true ) ;
    value = DataStack_Pop ( ) ;
    SetHere ( optInfo_0_two->Coding, 0 ) ;
    Compiler_Word_SCHCPUSCA ( optInfo_0_two, 1 ) ;
    Compile_MoveImm_To_Reg ( ACC, value, CELL ) ;
    _Word_CompileAndRecord_PushReg ( optInfo->opWord, ACC, true ) ; // this is helpful in future optimizations looking for StackPushRegisterCode
    optInfo->rtrn = OPTIMIZE_DONE ;
}

void
Compiler_CompileOptimize_IncDec ( Compiler * compiler )
{
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    Boolean reg ;
    if ( GetState ( _Context_, C_SYNTAX ) || ( ! optInfo->wordArg2_rvalue ) )
    {
        GetRmDispImm ( optInfo, optInfo->wordArg2, - 1 ) ;
        if ( ! ( optInfo->OptimizeFlag & OPTIMIZE_IMM ) )
        {
            if ( optInfo->wordArg2->W_ObjectAttributes & REGISTER_VARIABLE ) reg = optInfo->wordArg2->RegToUse ;
            else reg = optInfo->Optimize_Rm ;
            SetHere ( optInfo->wordArg2->Coding, 0 ) ;
            if ( optInfo->wordArg2->W_ObjectAttributes & ( NAMESPACE_VARIABLE ) ) _Compile_Move_Literal_Immediate_To_Reg ( reg, ( int64 ) optInfo->wordArg2->W_PtrToValue, 0 ) ;
            Compiler_Word_SCHCPUSCA ( optInfo->opWord, 1 ) ;
            _Compile_Group5 ( optInfo->opWord->W_OpInsnCode, ( optInfo->wordArg2->W_ObjectAttributes & REGISTER_VARIABLE ) ? REG : MEM, reg, 0, optInfo->Optimize_Disp, 0 ) ;
            optInfo->rtrn = OPTIMIZE_DONE ;
        }
    }
    else Word_Check_ReSet_To_Here_StackPushRegisterCode ( optInfo->wordArg2, 0 ) ; //if ( optInfo->wordArg2->StackPushRegisterCode )
}

// '@' : fetch or load

void
Compiler_CompileOptimizedLoad ( Compiler * compiler )
{
    Context * cntx = _Context_ ;
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    Word * wordArg2 ;
    //Compiler_Word_SCHCPUSCA ( optInfo->opWord, 0 ) ;
    if ( optInfo->wordArg2 )
    {
        if ( optInfo->wordArg2->StackPushRegisterCode )
        {
            if ( optInfo->wordArg2->W_ObjectAttributes & DOBJECT )
            {
                _Set_To_Here_Word_StackPushRegisterCode ( optInfo->wordArg2, 0 ) ;
                Compile_Move_Rm_To_Reg ( ACC, ACC, 0, 0 ) ;
            }
            else
            {
                if ( ( optInfo->wordArg2->W_ObjectAttributes & OBJECT_FIELD ) && cntx->BaseObject )
                {
                    wordArg2 = cntx->BaseObject ;
                    Compile_StandardArg ( wordArg2, ACC, optInfo->wordArg2_rvalue, wordArg2->Coding, true ) ;
                }
                    //SetHere ( optInfo->wordArg2->Coding, 0 ) ;
                else Compile_StandardArg ( optInfo->wordArg2, ACC, optInfo->wordArg2_rvalue, optInfo->wordArg2->Coding, true ) ;
            }
            _Word_CompileAndRecord_PushReg ( optInfo->wordArg2, ACC, true ) ;
            optInfo->opWord->StackPushRegisterCode = optInfo->wordArg2->StackPushRegisterCode ; // for Compiler_RemoveLocalFrame ??
        }
        else CompileOptimizedLoad_TOS ( ) ;
    }
    else CompileOptimizedLoad_TOS ( ) ;
    Word_SetSourceCoding ( optInfo->opWord, 0 ) ; // we don't need to see its source code
    optInfo->rtrn = OPTIMIZE_DONE ;
}

void
Compile_Optimize_Dup ( Compiler * compiler )
{
    CompileOptimizeInfo * optInfo = compiler->OptInfo ;
    Compiler_Word_SCHCPUSCA ( optInfo->opWord, 0 ) ;
    if ( optInfo->wordArg2 && optInfo->wordArg2->StackPushRegisterCode )
    {
        _Set_To_Here_Word_StackPushRegisterCode ( optInfo->wordArg2, 0 ) ;
        Compile_ADDI ( REG, DSP, 0, 2 * CELL, 0 ) ;
        _Compile_Move_Reg_To_StackN ( DSP, 0, ACC ) ;
        _Compile_Move_Reg_To_StackN ( DSP, - 1, ACC ) ;
    }
    else
    {
        Compile_Move_Rm_To_Reg ( ACC, DSP, 0, 0 ) ;
        Compile_ADDI ( REG, DSP, 0, sizeof (int64 ), 0 ) ;
        Compile_Move_Reg_To_Rm ( DSP, ACC, 0, 0 ) ;
    }
    optInfo->rtrn = OPTIMIZE_DONE ;
}

// +=, -=, *=, /=, etc.

void
Compile_X_OpEqual ( Compiler * compiler, block op )
{
    Word * zero = CSL_WordList ( 0 ) ;
    uint8 valueReg = ACC ;
    if ( Is_DebugOn ) _CSL_SC_WordList_Show ( 0, 0, 0 ) ;
    _Compiler_GetWordStackState ( compiler, zero ) ;
    CompileOptimizeInfo * optInfo = compiler->OptInfo ; // nb. after _Compiler_GetOptimizeState
    Compiler_SetStandardPreHere_ForDebugDisassembly ( compiler ) ;
    //if ( Is_DebugOn ) Compiler_ShowOptimizeArgs ( compiler ) ;
    if ( ( ! optInfo->NumberOfArgs ) || optInfo->xBetweenArg1AndArg2 ) SyntaxError ( 1 ) ; // this case may be handled better than with SyntaxError
    if ( optInfo->NumberOfArgs == 2 )
    {
        Compiler_SetupArgsToStandardLocations ( compiler ) ;
        if ( ! ( optInfo->wordArg1->W_ObjectAttributes & REGISTER_VARIABLE ) ) /// what about if ( optInfo->wordArg2->CAttribute & REGISTER_VARIABLE ) ???
        {
            Compile_StandardArg ( optInfo->wordArg1, OREG2, 0, 0, true ) ; //nb! lvalue
            Compile_StandardArg ( optInfo->wordArg1, ACC, 1, 0, false ) ; //nb! rvalue
            Compile_StandardArg ( optInfo->wordArg2, OREG, 1, 0, true ) ; //nb! rvalue
        }
        else
        {
            valueReg = optInfo->wordArg1->RegToUse ;
            Compile_StandardArg ( optInfo->wordArg1, OREG2, 0, 0, true ) ; //nb! lvalue
            Compile_StandardArg ( optInfo->wordArg1, ACC, 1, 0, false ) ; //nb! rvalue
            Compile_StandardArg ( optInfo->wordArg2, OREG, 1, 0, true ) ; //nb! rvalue
        }
    }
    else if ( compiler->LHS_Word )
    {
        if ( optInfo->lparen2 && optInfo->lparen2->StackPushRegisterCode )
        {
            SetHere ( optInfo->lparen2->StackPushRegisterCode, 1 ) ;
            Compile_Move_Reg_To_Reg ( OREG, ACC, 0 ) ;
        }
        Compile_StandardArg ( compiler->LHS_Word, OREG2, 0, 0, true ) ; //nb! lvalue
        Compile_StandardArg ( compiler->LHS_Word, ACC, 1, 0, false ) ; //nb! rvalue
        if ( ! optInfo->lparen2 ) Compile_StandardArg ( optInfo->wordArg2, OREG, 1, 0, true ) ; //nb! rvalue
        valueReg = ACC ;
    }
    else SyntaxError ( 1 ) ;
    //else args should be set up to standard locations by Compiler_SetupArgsToStandardLocations and Setup_MachineCodeInsnParameters
    Setup_MachineCodeInsnParameters ( compiler, REG, REG, ACC, OREG, 0, 1 ) ;
    Compiler_Word_SCHCPUSCA ( optInfo->opWord, 1 ) ;
    compiler->OptimizeForcedReturn = 1 ;
    Block_Eval ( op ) ;
    compiler->OptimizeForcedReturn = 0 ;
    Word_Check_ReSet_To_Here_StackPushRegisterCode ( optInfo->opWord, 0 ) ;
    if ( ( ! optInfo->wordArg1 ) || ( ! ( optInfo->wordArg1->W_ObjectAttributes & REGISTER_VARIABLE ) ) )
    {
        Compiler_Word_SCHCPUSCA ( optInfo->opWord, 0 ) ;
        Compile_Move_Reg_To_Rm ( OREG2, valueReg, 0, 0 ) ; //optInfo->Optimize_Reg, 0 ) ; //ACC, 0 ) ;
    }
}

// this function probably could be improved/optimized 

void
Compile_X_Equal ( Compiler * compiler, int64 op, int lvalueSize )
{
    Word * zero = CSL_WordList ( 0 ) ;
    if ( Is_DebugOn ) _CSL_SC_WordList_Show ( 0, 0, 0 ) ;
    _Compiler_GetWordStackState ( compiler, zero ) ;
    CompileOptimizeInfo * optInfo = compiler->OptInfo ; // nb. after _Compiler_GetOptimizeState
    Compiler_SetStandardPreHere_ForDebugDisassembly ( compiler ) ;
    Compiler_SetupArgsToStandardLocations ( compiler ) ;
    //if ( Is_DebugOn ) Compiler_ShowOptimizeArgs ( compiler ) ;
    if ( op == STORE )
    {
        CompileOptimizeInfo * optInfo = compiler->OptInfo ;
        Boolean reg = OREG, rm = ACC ;
        if ( optInfo->wordArg2 && ( optInfo->wordArg2->W_ObjectAttributes & REGISTER_VARIABLE ) )
        {
            reg = optInfo->wordArg2->RegToUse ;
            if ( optInfo->wordArg1_literal )
            {
                SetHere ( optInfo->wordArg1->Coding, 0 ) ;
                Compiler_Word_SCHCPUSCA ( optInfo->opWord, 1 ) ;
                Compile_MoveImm_To_Reg ( reg, optInfo->wordArg1->W_Value, lvalueSize ) ;
            }
            else
            {
                rm = optInfo->wordArg1->RegToUse ;
                if ( optInfo->wordArg1->W_ObjectAttributes & REGISTER_VARIABLE ) SetHere ( optInfo->wordArg1->Coding, 0 ) ;
                else SetHere ( optInfo->wordArg2->Coding, 0 ) ;
                Compiler_Word_SCHCPUSCA ( optInfo->opWord, 1 ) ;
                if ( reg != rm ) Compile_Move_Reg_To_Reg ( reg, rm, 0 ) ;
            }
            return ;
        }
        Compiler_Word_SCHCPUSCA ( optInfo->opWord, 0 ) ;
        Compile_Move_Reg_To_Rm ( reg, rm, 0, lvalueSize ) ; //optInfo->wordArg1->ObjectSize ) ; // dst = reg ; src = rm
        //Compile_Move_Reg_To_Rm ( optInfo->Optimize_Reg, optInfo->Optimize_Rm, 0, 0 ) ; //optInfo->wordArg1->ObjectSize ) ; // dst = reg ; src = rm
    }
    else if ( op == EQUAL )
    {
        Boolean dstReg, srcReg ;
        if ( optInfo->wordArg1 && ( optInfo->wordArg1->W_ObjectAttributes & REGISTER_VARIABLE ) ) dstReg = optInfo->wordArg1->RegToUse ;
        else dstReg = ACC ;
        if ( ( optInfo->wordArg2 && ( optInfo->wordArg2->W_ObjectAttributes & REGISTER_VARIABLE ) ) || optInfo->wordArg2_Op )
        {
            srcReg = ( optInfo->wordArg2->RegToUse != ACC ) ? optInfo->wordArg2->RegToUse : OREG ;
        }
        else srcReg = OREG ;
        if ( optInfo->wordArg2_literal )
        {
            SetHere ( optInfo->wordArg2->Coding, 0 ) ; // old 
            Compiler_Word_SCHCPUSCA ( optInfo->opWord, 1 ) ;
            if ( optInfo->wordArg1 && ( optInfo->wordArg1->W_ObjectAttributes & REGISTER_VARIABLE ) )
            {
                Compile_MoveImm_To_Reg ( dstReg, optInfo->wordArg2->W_Value, lvalueSize ) ;
                return ;
            }
            else
            {
                //Word_Check_ReSet_To_Here_StackPushRegisterCode ( optInfo->wordArg1, true ) ;
                Compile_MoveImm_To_Reg ( srcReg, optInfo->wordArg2->W_Value, lvalueSize ) ;
            }
        }
        else
        {
            if ( optInfo->wordArg2 && ( optInfo->wordArg2->W_ObjectAttributes & REGISTER_VARIABLE ) )
            {
                SetHere ( optInfo->wordArg2->Coding, 0 ) ;
                Compiler_Word_SCHCPUSCA ( optInfo->wordArg2, 1 ) ;
            }
        }
        Compiler_Word_SCHCPUSCA ( optInfo->opWord, 1 ) ;
        if ( ( optInfo->NumberOfArgs == 1 ) && optInfo->wordArg2_Op )
        {
            Word * word = compiler->OptInfo->wordArg0_ForOpEqual ;
            if ( word )
            {
                Compile_Move_Reg_To_Reg ( word->RegToUse, optInfo->wordArg2->RegToUse, 0 ) ; // & 0xf turn off REG_ON_BIT
                if ( word->StackPushRegisterCode )
                {
                    byte * src = word->StackPushRegisterCode + STACK_PUSH_REGISTER_CODE_SIZE ;
                    BI_Block_Copy ( 0, word->StackPushRegisterCode, src, Here - src ) ;
                }
                compiler->OptInfo->wordArg0_ForOpEqual = 0 ;
                return ;
            }
        }
        if ( optInfo->wordArg1 && ( optInfo->wordArg1->W_ObjectAttributes & REGISTER_VARIABLE ) ) Compile_Move_Reg_To_Reg ( dstReg, srcReg, 0 ) ;
        else Compile_Move_Reg_To_Rm ( dstReg, srcReg, 0, lvalueSize ) ;
    }
    else if ( ! optInfo->rtrn ) Setup_MachineCodeInsnParameters ( compiler, REG, REG, ACC, OREG, 0, 0 ) ;
    SetState ( _CSL_, IN_OPTIMIZER, false ) ;
}
// skip back WordStack words for the args of an op parameter in GetOptimizeState

void
PeepHole_Optimize_ForStackPopToReg ( )
{
    if ( GetState ( _CSL_, OPTIMIZE_ON ) )
    {
        byte * here = _O_CodeByteArray->EndIndex ;
        byte add_r14_0x8__mov_r14_rax__mov_rax_r14__sub_r14_0x8 [ ] = { 0x49, 0x83, 0xc6, 0x08, 0x49, 0x89, 0x06, 0x49, 0x8b, 0x06, 0x49, 0x83, 0xee, 0x08 } ;
        if ( ! memcmp ( add_r14_0x8__mov_r14_rax__mov_rax_r14__sub_r14_0x8, here - 14, 14 ) )
        {
            CSL_AdjustDbgSourceCodeAddress ( Here, Here - 14 ) ;
            _ByteArray_UnAppendSpace ( _O_CodeByteArray, 14 ) ;
        }
    }
}

void
PeepHole_Optimize ( )
{
    if ( GetState ( _CSL_, OPTIMIZE_ON ) )
    {
        byte * here = _O_CodeByteArray->EndIndex ;
        byte sub_r14_0x8__add_r14_0x8 [ ] = { 0x49, 0x83, 0xee, 0x08, 0x49, 0x83, 0xc6, 0x08 } ;
        byte add_r14_0x8__mov_r14_rax__mov_rax_r14__sub_r14_0x8 [ ] = { 0x49, 0x83, 0xc6, 0x08, 0x49, 0x89, 0x06, 0x49, 0x8b, 0x06, 0x49, 0x83, 0xee, 0x08 } ;
        //byte add_r14_0x8__mov_r14_rdi__mov_rax_r14__sub_r14_0x8 [ ] = { 0x49, 0x83, 0xc6, 0x08, 0x49, 0x89, 0x3e, 0x49, 0x8b, 0x06, 0x49, 0x83, 0xee, 0x08 } ;
        //byte add_esi_04__mov_tos_eax_sub_esi_04 [ ] = { 0x83, 0xc6, 0x04, 0x89, 0x06, 0x83, 0xee, 0x04 } ;
        //byte mov_eax_tos_sub_esi_04_test_eax_eax [ ] = { 0x89, 0x06, 0x83, 0xee, 0x04, 0x85, 0xc0 } ;
        byte mov_r14_rax__mov_rax_r14 [] = { 0x49, 0x89, 0x06, 0x49, 0x8b, 0x06 } ;
        byte add_rax_0x0 [] = { 0x48, 0x81, 0xc0, 0x00, 0x00, 0x00, 0x00 } ;
        byte test_rax_rax_test_rax_rax [] = { 0x48, 0x85, 0xc0, 0x48, 0x85, 0xc0 } ;
        if ( ! memcmp ( sub_r14_0x8__add_r14_0x8, here - 8, 8 ) )
        {
            CSL_AdjustDbgSourceCodeAddress ( Here, Here - 8 ) ;
            _ByteArray_UnAppendSpace ( _O_CodeByteArray, 8 ) ;
        }
        else if ( ! memcmp ( add_r14_0x8__mov_r14_rax__mov_rax_r14__sub_r14_0x8, here - 14, 14 ) )
        {
            CSL_AdjustDbgSourceCodeAddress ( Here, Here - 14 ) ;
            _ByteArray_UnAppendSpace ( _O_CodeByteArray, 14 ) ;
        }
        else if ( ! memcmp ( mov_r14_rax__mov_rax_r14, here - 6, 6 ) )
        {
            CSL_AdjustDbgSourceCodeAddress ( Here, Here - 6 ) ;
            _ByteArray_UnAppendSpace ( _O_CodeByteArray, 6 ) ;
        }
        else if ( ! memcmp ( add_rax_0x0, here - 7, 7 ) )
        {
            CSL_AdjustDbgSourceCodeAddress ( Here, Here - 7 ) ;
            _ByteArray_UnAppendSpace ( _O_CodeByteArray, 7 ) ;
        }
        else if ( ! memcmp ( test_rax_rax_test_rax_rax, here - 6, 6 ) )
        {
            CSL_AdjustDbgSourceCodeAddress ( Here, Here - 6 ) ;
            _ByteArray_UnAppendSpace ( _O_CodeByteArray, 6 ) ;
        }
#if 0        
        else if ( ! memcmp ( add_r14_0x8__mov_r14_rdi__mov_rax_r14__sub_r14_0x8, here - 14, 14 ) )
        {
            _ByteArray_UnAppendSpace ( _O_CodeByteArray, 14 ) ;
        }
            // this occurs one time at startup in _assertStkChk : change it where it is caused and eliminate testing every instruction !! 
        else if ( ! memcmp ( mov_eax_tos_sub_esi_04_test_eax_eax, here - 7, 7 ) )
        {

            _ByteArray_UnAppendSpace ( _O_CodeByteArray, 7 ) ;
            Compile_TEST_Reg_To_Reg ( ACC, ACC ) ;
        }
#endif        
    }
}

void
GetRmDispImm ( CompileOptimizeInfo * optInfo, Word * word, int64 suggestedReg )
{
    if ( suggestedReg == - 1 ) suggestedReg = ACC ;
    optInfo->Optimize_Reg = suggestedReg ;
    if ( word->W_ObjectAttributes & REGISTER_VARIABLE )
    {
        optInfo->OptimizeFlag |= OPTIMIZE_REGISTER ;
        optInfo->Optimize_Dest_RegOrMem = REG ;
        optInfo->Optimize_Mod = REG ;
        //optInfo->Optimize_Rm = optInfo->Optimize_Reg = ( suggestedReg != - 1 ) ? suggestedReg : word->RegToUse ;
        optInfo->Optimize_Rm = optInfo->Optimize_Reg = word->RegToUse ;
    }
    else if ( word->W_ObjectAttributes & LOCAL_VARIABLE )
    {
        optInfo->Optimize_Rm = FP ;
        optInfo->Optimize_Disp = LocalVar_Disp ( word ) ;
        optInfo->OptimizeFlag |= OPTIMIZE_RM ;
    }
    else if ( word->W_ObjectAttributes & PARAMETER_VARIABLE )
    {
        optInfo->Optimize_Rm = FP ;
        optInfo->Optimize_Disp = ParameterVar_Disp ( word ) ;
        optInfo->OptimizeFlag |= OPTIMIZE_RM ;
    }
    else if ( word->W_ObjectAttributes & ( LITERAL | CONSTANT ) )
    {
        optInfo->Optimize_Imm = ( int64 ) word->W_Value ;
        optInfo->OptimizeFlag |= OPTIMIZE_IMM ;
    }
    else if ( word->W_ObjectAttributes & ( NAMESPACE_VARIABLE ) )
    {
        if ( suggestedReg == - 1 ) suggestedReg = ACC ;
        //_Compile_Move_Literal_Immediate_To_Reg ( suggestedReg, ( int64 ) word->W_PtrToValue ) ; // ?? should this be here ??
        optInfo->Optimize_Rm = suggestedReg ;
        optInfo->OptimizeFlag |= OPTIMIZE_RM ;
    }
    else //if ( word->CAttribute & CATEGORY_OP_1_ARG )
    {
        optInfo->Optimize_Rm = DSP ;
        optInfo->OptimizeFlag |= OPTIMIZE_RM ;
    }
}

void
CompileOptimizedLoad_TOS ( )
{
    Compile_Move_Rm_To_Reg ( ACC, DSP, 0, 0 ) ;
    Compile_Move_Rm_To_Reg ( ACC, ACC, 0, 0 ) ;
    Compile_Move_Reg_To_Rm ( DSP, ACC, 0, 0 ) ;
}

