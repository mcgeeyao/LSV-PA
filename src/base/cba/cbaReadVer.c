/**CFile****************************************************************

  FileName    [cbaReadVer.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchical word-level netlist.]

  Synopsis    [BLIF writer.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 29, 2014.]

  Revision    [$Id: cbaReadVer.c,v 1.00 2014/11/29 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cba.h"
#include "cbaPrs.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// Verilog keywords
typedef enum { 
    PRS_VER_NONE = 0,  // 0:  unused
    PRS_VER_MODULE,    // 1:  module
    PRS_VER_INOUT,     // 2:  inout
    PRS_VER_INPUT,     // 3:  input
    PRS_VER_OUTPUT,    // 4:  output
    PRS_VER_WIRE,      // 5:  wire
    PRS_VER_ASSIGN,    // 6:  assign
    PRS_VER_REG,       // 7:  reg
    PRS_VER_ALWAYS,    // 8:  always
    PRS_VER_DEFPARAM,  // 9:  always
    PRS_VER_BEGIN,     // 10: begin
    PRS_VER_END,       // 11: end
    PRS_VER_ENDMODULE, // 12: endmodule
    PRS_VER_UNKNOWN    // 13: unknown
} Cba_VerType_t;

const char * s_VerTypes[PRS_VER_UNKNOWN+1] = {
    NULL,              // 0:  unused
    "module",          // 1:  module
    "inout",           // 2:  inout
    "input",           // 3:  input
    "output",          // 4:  output
    "wire",            // 5:  wire
    "assign",          // 6:  assign
    "reg",             // 7:  reg
    "always",          // 8:  always
    "defparam",        // 9:  defparam
    "begin",           // 10: begin
    "end",             // 11: end
    "endmodule",       // 12: endmodule
    NULL               // 13: unknown 
};

static inline void Prs_NtkAddVerilogDirectives( Prs_Man_t * p )
{
    int i;
    for ( i = 1; s_VerTypes[i]; i++ )
        Abc_NamStrFindOrAdd( p->pStrs, (char *)s_VerTypes[i],   NULL );
    assert( Abc_NamObjNumMax(p->pStrs) == i );
}


// character recognition 
static inline int Prs_CharIsSpace( char c )   { return (c == ' ' || c == '\t' || c == '\r' || c == '\n');                           }
static inline int Prs_CharIsDigit( char c )   { return (c >= '0' && c <= '9');                                                      }
static inline int Prs_CharIsDigitB( char c )  { return (c == '0' || c == '1'  || c == 'x' || c == 'z');                             }
static inline int Prs_CharIsDigitH( char c )  { return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');  }
static inline int Prs_CharIsChar( char c )    { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');                            }
static inline int Prs_CharIsSymb1( char c )   { return Prs_CharIsChar(c) || c == '_';                                               }
static inline int Prs_CharIsSymb2( char c )   { return Prs_CharIsSymb1(c) || Prs_CharIsDigit(c) || c == '$';                        }

static inline int Prs_ManIsChar( Prs_Man_t * p, char c )    { return p->pCur[0] == c;                        }
static inline int Prs_ManIsChar1( Prs_Man_t * p, char c )   { return p->pCur[1] == c;                        }
static inline int Prs_ManIsDigit( Prs_Man_t * p )           { return Prs_CharIsDigit(*p->pCur);              }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

// collect predefined modules names
const char * s_KnownModules[100] = {
    NULL,     // CBA_OBJ_NONE = 0,  // 0:  unused
    NULL,     // CBA_OBJ_PI,        // 1:  input
    NULL,     // CBA_OBJ_PO,        // 2:  output
    NULL,     // CBA_OBJ_BI,        // 3:  box input
    NULL,     // CBA_OBJ_BO,        // 4:  box output
    NULL,     // CBA_OBJ_BOX,       // 5:  box
 
    "const0", // CBA_BOX_C0,   
    "const1", // CBA_BOX_C1,   
    "constX", // CBA_BOX_CX,   
    "constZ", // CBA_BOX_CZ,   
    "buf",    // CBA_BOX_BUF,  
    "not",    // CBA_BOX_INV,  
    "and",    // CBA_BOX_AND,  
    "nand",   // CBA_BOX_NAND, 
    "or",     // CBA_BOX_OR,   
    "nor",    // CBA_BOX_NOR,  
    "xor",    // CBA_BOX_XOR,  
    "xnor",   // CBA_BOX_XNOR, 
    "sharp",  // CBA_BOX_SHARP,
    "mux",    // CBA_BOX_MUX,  
    "maj",    // CBA_BOX_MAJ,  

    "VERIFIC_",
    "add_",                  
    "mult_",                 
    "div_",                  
    "mod_",                  
    "rem_",                  
    "shift_left_",           
    "shift_right_",          
    "rotate_left_",          
    "rotate_right_",         
    "reduce_and_",           
    "reduce_or_",            
    "reduce_xor_",           
    "reduce_nand_",          
    "reduce_nor_",           
    "reduce_xnor_",          
    "LessThan_",             
    "Mux_",                  
    "Select_",               
    "Decoder_",              
    "EnabledDecoder_",       
    "PrioSelect_",           
    "DualPortRam_",          
    "ReadPort_",             
    "WritePort_",            
    "ClockedWritePort_",     
    "lut",                   
    "and_",                  
    "or_",                   
    "xor_",                  
    "nand_",                 
    "nor_",                  
    "xnor_",                 
    "buf_",                  
    "inv_",                  
    "tri_",                  
    "sub_",                  
    "unary_minus_",          
    "equal_",                
    "not_equal_",            
    "mux_",                  
    "wide_mux_",             
    "wide_select_",          
    "wide_dff_",             
    "wide_dlatch_",          
    "wide_dffrs_",           
    "wide_dlatchrs_",        
    "wide_prio_select_",     
    "pow_",                  
    "PrioEncoder_",          
    "abs",                   
    NULL
};

// check if it is a known module
static inline int Prs_ManIsKnownModule( Prs_Man_t * p, char * pName )
{
    int i;
    for ( i = CBA_BOX_C0; s_KnownModules[i]; i++ )
        if ( !strncmp(pName, s_KnownModules[i], strlen(s_KnownModules[i])) )
            return i;
    return 0;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

// skips Verilog comments (returns 1 if some comments were skipped)
static inline int Prs_ManUtilSkipComments( Prs_Man_t * p )
{
    if ( !Prs_ManIsChar(p, '/') )
        return 0;
    if ( Prs_ManIsChar1(p, '/') )
    {
        for ( p->pCur += 2; p->pCur < p->pLimit; p->pCur++ )
            if ( Prs_ManIsChar(p, '\n') )
                { p->pCur++; return 1; }
    }
    else if ( Prs_ManIsChar1(p, '*') )
    {
        for ( p->pCur += 2; p->pCur < p->pLimit; p->pCur++ )
            if ( Prs_ManIsChar(p, '*') && Prs_ManIsChar1(p, '/') )
                { p->pCur++; p->pCur++; return 1; }
    }
    return 0;
}
static inline int Prs_ManUtilSkipName( Prs_Man_t * p )
{
    if ( !Prs_ManIsChar(p, '\\') )
        return 0;
    for ( p->pCur++; p->pCur < p->pLimit; p->pCur++ )
        if ( Prs_ManIsChar(p, ' ') )
            { p->pCur++; return 1; }
    return 0;
}

// skip any number of spaces and comments
static inline int Prs_ManUtilSkipSpaces( Prs_Man_t * p )
{
    while ( p->pCur < p->pLimit )
    {
        while ( Prs_CharIsSpace(*p->pCur) ) 
            p->pCur++;
        if ( !*p->pCur )
            return Prs_ManErrorSet(p, "Unexpectedly reached end-of-file.", 1);
        if ( !Prs_ManUtilSkipComments(p) )
            return 0;
    }
    return Prs_ManErrorSet(p, "Unexpectedly reached end-of-file.", 1);
}
// skip everything including comments until the given char
static inline int Prs_ManUtilSkipUntil( Prs_Man_t * p, char c )
{
    while ( p->pCur < p->pLimit )
    {
        if ( Prs_ManIsChar(p, c) )
            return 1;
        if ( Prs_ManUtilSkipComments(p) )
            continue;
        if ( Prs_ManUtilSkipName(p) )
            continue;
        p->pCur++;
    }
    return 0;
}
// skip everything including comments until the given word
static inline int Prs_ManUtilSkipUntilWord( Prs_Man_t * p, char * pWord )
{
    char * pPlace = strstr( p->pCur, pWord );
    if ( pPlace == NULL )  return 1;
    p->pCur = pPlace + strlen(pWord);
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Prs_ManReadName( Prs_Man_t * p )
{
    char * pStart = p->pCur;
    if ( Prs_ManIsChar(p, '\\') ) // escaped name
    {
        pStart = ++p->pCur;
        while ( !Prs_ManIsChar(p, ' ') ) 
            p->pCur++;
    }    
    else if ( Prs_CharIsSymb1(*p->pCur) ) // simple name
    {
        p->pCur++;
        while ( Prs_CharIsSymb2(*p->pCur) ) 
            p->pCur++;
    }
    else 
        return 0;
    return Abc_NamStrFindOrAddLim( p->pStrs, pStart, p->pCur, NULL );
}
static inline int Prs_ManReadNameList( Prs_Man_t * p, Vec_Int_t * vTemp, char LastSymb )
{
    Vec_IntClear( vTemp );
    while ( 1 )
    {
        int Item = Prs_ManReadName(p);
        if ( Item == 0 )                    return Prs_ManErrorSet(p, "Cannot read name in the list.", 0);
        Vec_IntPush( vTemp, Item );
        if ( Prs_ManIsChar(p, LastSymb) )   break;
        if ( !Prs_ManIsChar(p, ',') )       return Prs_ManErrorSet(p, "Expecting comma in the list.", 0);
        p->pCur++;
        if ( Prs_ManUtilSkipSpaces(p) )     return 0;
    }
    return 1;
}
static inline int Prs_ManReadConstant( Prs_Man_t * p )
{
    char * pStart = p->pCur;
    assert( Prs_ManIsDigit(p) );
    while ( Prs_ManIsDigit(p) ) 
        p->pCur++;
    if ( !Prs_ManIsChar(p, '\'') )          return Prs_ManErrorSet(p, "Cannot read constant.", 0);
    p->pCur++;
    if ( Prs_ManIsChar(p, 'b') )
    {
        p->pCur++;
        while ( Prs_CharIsDigitB(*p->pCur) ) 
            p->pCur++;
    }
    else if ( Prs_ManIsChar(p, 'h') )
    {
        p->pCur++;
        while ( Prs_CharIsDigitH(*p->pCur) ) 
            p->pCur++;
    }
    else if ( Prs_ManIsChar(p, 'd') )
    {
        p->pCur++;
        while ( Prs_ManIsDigit(p) ) 
            p->pCur++;
    }
    else                                    return Prs_ManErrorSet(p, "Cannot read radix of constant.", 0);
    return Abc_NamStrFindOrAddLim( p->pStrs, pStart, p->pCur, NULL );
}
static inline int Prs_ManReadRange( Prs_Man_t * p )
{
    assert( Prs_ManIsChar(p, '[') );
    Vec_StrClear( &p->vCover );
    Vec_StrPush( &p->vCover, *p->pCur++ );
    if ( Prs_ManUtilSkipSpaces(p) )         return 0;
    if ( !Prs_ManIsDigit(p) )               return Prs_ManErrorSet(p, "Cannot read digit in range specification.", 0);
    while ( Prs_ManIsDigit(p) )
        Vec_StrPush( &p->vCover, *p->pCur++ );
    if ( Prs_ManUtilSkipSpaces(p) )         return 0;
    if ( Prs_ManIsChar(p, ':') )
    {
        Vec_StrPush( &p->vCover, *p->pCur++ );
        if ( Prs_ManUtilSkipSpaces(p) )     return 0;
        if ( !Prs_ManIsDigit(p) )           return Prs_ManErrorSet(p, "Cannot read digit in range specification.", 0);
        while ( Prs_ManIsDigit(p) )
            Vec_StrPush( &p->vCover, *p->pCur++ );
        if ( Prs_ManUtilSkipSpaces(p) )     return 0;
    }
    if ( !Prs_ManIsChar(p, ']') )           return Prs_ManErrorSet(p, "Cannot read closing brace in range specification.", 0);
    Vec_StrPush( &p->vCover, *p->pCur++ );
    Vec_StrPush( &p->vCover, '\0' );
    return Abc_NamStrFindOrAdd( p->pStrs, Vec_StrArray(&p->vCover), NULL );
}
static inline int Prs_ManReadConcat( Prs_Man_t * p, Vec_Int_t * vTemp2 )
{
    extern int Prs_ManReadSignalList( Prs_Man_t * p, Vec_Int_t * vTemp, char LastSymb, int fAddForm );
    assert( Prs_ManIsChar(p, '{') );
    p->pCur++;
    if ( !Prs_ManReadSignalList( p, vTemp2, '}', 0 ) ) return 0;
    // check final
    assert( Prs_ManIsChar(p, '}') );
    p->pCur++;
    // return special case
    assert( Vec_IntSize(vTemp2) > 0 );
    if ( Vec_IntSize(vTemp2) == 1 )
        return Vec_IntEntry(vTemp2, 0);
    return Abc_Var2Lit2( Prs_NtkAddConcat(p->pNtk, vTemp2), CBA_PRS_CONCAT );
}
static inline int Prs_ManReadSignal( Prs_Man_t * p )
{
    int Item;
    if ( Prs_ManUtilSkipSpaces(p) )         return 0;
    if ( Prs_ManIsDigit(p) )
    {
        Item = Prs_ManReadConstant(p);
        if ( Item == 0 )                    return 0;
        if ( Prs_ManUtilSkipSpaces(p) )     return 0;
        return Abc_Var2Lit2( Item, CBA_PRS_CONST );
    }
    if ( Prs_ManIsChar(p, '{') )
    {
        if ( p->fUsingTemp2 )               return Prs_ManErrorSet(p, "Cannot read nested concatenations.", 0);
        p->fUsingTemp2 = 1;
        Item = Prs_ManReadConcat(p, &p->vTemp2);
        p->fUsingTemp2 = 0;
        if ( Item == 0 )                    return 0;
        if ( Prs_ManUtilSkipSpaces(p) )     return 0;
        return Item;
    }
    else
    {
        Item = Prs_ManReadName( p );
        if ( Item == 0 )                    return 0;    // was        return 1;                
        if ( Prs_ManUtilSkipSpaces(p) )     return 0;
        if ( Prs_ManIsChar(p, '[') )
        {
            int Range = Prs_ManReadRange(p);
            if ( Range == 0 )               return 0;
            if ( Prs_ManUtilSkipSpaces(p) ) return 0;
            return Abc_Var2Lit2( Prs_NtkAddSlice(p->pNtk, Item, Range), CBA_PRS_SLICE );
        }
        return Abc_Var2Lit2( Item, CBA_PRS_NAME );
    }
}
int Prs_ManReadSignalList( Prs_Man_t * p, Vec_Int_t * vTemp, char LastSymb, int fAddForm )
{
    Vec_IntClear( vTemp );
    while ( 1 )
    {
        int Item = Prs_ManReadSignal(p);
        if ( Item == 0 )                    return Prs_ManErrorSet(p, "Cannot read signal in the list.", 0);
        if ( fAddForm )
            Vec_IntPush( vTemp, 0 );
        Vec_IntPush( vTemp, Item );
        if ( Prs_ManIsChar(p, LastSymb) )   break;
        if ( !Prs_ManIsChar(p, ',') )       return Prs_ManErrorSet(p, "Expecting comma in the list.", 0);
        p->pCur++;
    }
    return 1;
}
static inline int Prs_ManReadSignalList2( Prs_Man_t * p, Vec_Int_t * vTemp )
{
    int FormId, ActItem;
    Vec_IntClear( vTemp );
    assert( Prs_ManIsChar(p, '.') );
    while ( Prs_ManIsChar(p, '.') )
    {
        p->pCur++;
        FormId = Prs_ManReadName( p );
        if ( FormId == 0 )                  return Prs_ManErrorSet(p, "Cannot read formal name of the instance.", 0);
        if ( !Prs_ManIsChar(p, '(') )       return Prs_ManErrorSet(p, "Cannot read \"(\" in the instance.", 0);
        p->pCur++;
        if ( Prs_ManUtilSkipSpaces(p) )     return 0;
        ActItem = Prs_ManReadSignal( p );
        if ( ActItem == 0 )                 return Prs_ManErrorSet(p, "Cannot read actual name of the instance.", 0);
        if ( !Prs_ManIsChar(p, ')') )       return Prs_ManErrorSet(p, "Cannot read \")\" in the instance.", 0);
        p->pCur++;
        Vec_IntPushTwo( vTemp, FormId, ActItem );
        if ( Prs_ManUtilSkipSpaces(p) )     return 0;
        if ( Prs_ManIsChar(p, ')') )        break;
        if ( !Prs_ManIsChar(p, ',') )       return Prs_ManErrorSet(p, "Expecting comma in the instance.", 0);
        p->pCur++;
        if ( Prs_ManUtilSkipSpaces(p) )     return 0;
    }
    assert( Vec_IntSize(vTemp) > 0 );
    assert( Vec_IntSize(vTemp) % 2 == 0 );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Prs_ManReadDeclaration( Prs_Man_t * p, int Type )
{
    int i, Sig, RangeId = 0;
    Vec_Int_t * vSigs[4]  = { &p->pNtk->vInouts,  &p->pNtk->vInputs,  &p->pNtk->vOutputs,  &p->pNtk->vWires };
    Vec_Int_t * vSigsR[4] = { &p->pNtk->vInoutsR, &p->pNtk->vInputsR, &p->pNtk->vOutputsR, &p->pNtk->vWiresR };
    assert( Type >= PRS_VER_INOUT && Type <= PRS_VER_WIRE );
    if ( Prs_ManUtilSkipSpaces(p) )                                   return 0;
    if ( Prs_ManIsChar(p, '[') && !(RangeId = Prs_ManReadRange(p)) )  return 0;
    if ( !Prs_ManReadNameList( p, &p->vTemp, ';' ) )                  return 0;
    Vec_IntForEachEntry( &p->vTemp, Sig, i )
    {
        Vec_IntPush( vSigs[Type - PRS_VER_INOUT], Sig );
        Vec_IntPush( vSigsR[Type - PRS_VER_INOUT], RangeId );
    }
    return 1;
}
static inline int Prs_ManReadAssign( Prs_Man_t * p )
{
    int OutItem, InItem, fCompl = 0, Oper = 0;
    // read output name
    OutItem = Prs_ManReadSignal( p );
    if ( OutItem == 0 )                     return Prs_ManErrorSet(p, "Cannot read output in assign-statement.", 0);
    if ( !Prs_ManIsChar(p, '=') )           return Prs_ManErrorSet(p, "Expecting \"=\" in assign-statement.", 0);
    p->pCur++;
    if ( Prs_ManUtilSkipSpaces(p) )         return 0;
    if ( Prs_ManIsChar(p, '~') ) 
    { 
        fCompl = 1; 
        p->pCur++; 
    }
    // read first name
    InItem = Prs_ManReadSignal( p );
    if ( InItem == 0 )                      return Prs_ManErrorSet(p, "Cannot read first input name in the assign-statement.", 0);
    Vec_IntClear( &p->vTemp );
    Vec_IntPush( &p->vTemp, 0 );
    Vec_IntPush( &p->vTemp, InItem );
    // check unary operator
    if ( Prs_ManIsChar(p, ';') )
    {
        Vec_IntPush( &p->vTemp, 0 );
        Vec_IntPush( &p->vTemp, OutItem );
        Oper = fCompl ? CBA_BOX_INV : CBA_BOX_BUF;
        Prs_NtkAddBox( p->pNtk, Oper, 0, &p->vTemp );
        return 1;
    }
    if ( Prs_ManIsChar(p, '&') ) 
        Oper = CBA_BOX_AND;
    else if ( Prs_ManIsChar(p, '|') ) 
        Oper = CBA_BOX_OR;
    else if ( Prs_ManIsChar(p, '^') ) 
        Oper = fCompl ? CBA_BOX_XNOR : CBA_BOX_XOR;
    else if ( Prs_ManIsChar(p, '?') ) 
        Oper = CBA_BOX_MUX;
    else                                    return Prs_ManErrorSet(p, "Unrecognized operator in the assign-statement.", 0);
    p->pCur++; 
    // read second name
    InItem = Prs_ManReadSignal( p );
    if ( InItem == 0 )                      return Prs_ManErrorSet(p, "Cannot read second input name in the assign-statement.", 0);
    Vec_IntPush( &p->vTemp, 0 );
    Vec_IntPush( &p->vTemp, InItem );
    // read third argument
    if ( Oper == CBA_BOX_MUX )
    {
        assert( fCompl == 0 ); 
        if ( !Prs_ManIsChar(p, ':') )       return Prs_ManErrorSet(p, "Expected colon in the MUX assignment.", 0);
        p->pCur++; 
        // read third name
        InItem = Prs_ManReadSignal( p );
        if ( InItem == 0 )                  return Prs_ManErrorSet(p, "Cannot read third input name in the assign-statement.", 0);
        Vec_IntPush( &p->vTemp, 0 );
        Vec_IntPush( &p->vTemp, InItem );
        if ( !Prs_ManIsChar(p, ';') )       return Prs_ManErrorSet(p, "Expected semicolon at the end of the assign-statement.", 0);
    }
    // write binary operator
    Vec_IntPush( &p->vTemp, 0 );
    Vec_IntPush( &p->vTemp, OutItem );
    Prs_NtkAddBox( p->pNtk, Oper, 0, &p->vTemp );
    return 1;
}
static inline int Prs_ManReadInstance( Prs_Man_t * p, int Func )
{
    int InstId, Status;
/*
    static Counter = 0;
    if ( ++Counter == 7 )
    {
        int s=0;
    }
*/
    if ( Prs_ManUtilSkipSpaces(p) )         return 0;
    if ( (InstId = Prs_ManReadName(p)) )
        if (Prs_ManUtilSkipSpaces(p))       return 0;
    if ( !Prs_ManIsChar(p, '(') )           return Prs_ManErrorSet(p, "Expecting \"(\" in module instantiation.", 0);
    p->pCur++;
    if ( Prs_ManUtilSkipSpaces(p) )         return 0;
    if ( Prs_ManIsChar(p, '.') ) // box
        Status = Prs_ManReadSignalList2(p, &p->vTemp);
    else  // node
    {
        //char * s = Abc_NamStr(p->pStrs, Func);
        // translate elementary gate
        int iFuncNew = Prs_ManIsKnownModule(p, Abc_NamStr(p->pStrs, Func));
        if ( iFuncNew == 0 )                return Prs_ManErrorSet(p, "Cannot find elementary gate.", 0);
        Func = iFuncNew;
        Status = Prs_ManReadSignalList( p, &p->vTemp, ')', 1 );
    }
    if ( Status == 0 )                      return 0;
    assert( Prs_ManIsChar(p, ')') );
    p->pCur++;
    if ( Prs_ManUtilSkipSpaces(p) )         return 0;
    if ( !Prs_ManIsChar(p, ';') )           return Prs_ManErrorSet(p, "Expecting semicolon in the instance.", 0);
    // add box 
    Prs_NtkAddBox( p->pNtk, Func, InstId, &p->vTemp );
    return 1;
}
static inline int Prs_ManReadArguments( Prs_Man_t * p )
{
    int iRange = 0, iType = -1;
    Vec_Int_t * vSigs[3]  = { &p->pNtk->vInouts,  &p->pNtk->vInputs,  &p->pNtk->vOutputs };
    Vec_Int_t * vSigsR[3] = { &p->pNtk->vInoutsR, &p->pNtk->vInputsR, &p->pNtk->vOutputsR };
    assert( Prs_ManIsChar(p, '(') );
    p->pCur++;
    if ( Prs_ManUtilSkipSpaces(p) )             return 0;
    while ( 1 )
    {
        int iName = Prs_ManReadName( p );
        if ( iName == 0 )                       return 0;
        if ( Prs_ManUtilSkipSpaces(p) )         return 0;
        if ( iName >= PRS_VER_INOUT && iName <= PRS_VER_OUTPUT ) // declaration
        {
            iType = iName;
            if ( Prs_ManIsChar(p, '[') )
            {
                iRange = Prs_ManReadRange(p);
                if ( iRange == 0 )              return 0;
                if ( Prs_ManUtilSkipSpaces(p) ) return 0;
            }
        }
        if ( iType > 0 )
        {
            Vec_IntPush( vSigs[iType - PRS_VER_INOUT], iName );
            Vec_IntPush( vSigsR[iType - PRS_VER_INOUT], iRange );
        }
        Vec_IntPush( &p->pNtk->vOrder, iName );
        if ( Prs_ManIsChar(p, ')') )
            break;
        if ( !Prs_ManIsChar(p, ',') )           return Prs_ManErrorSet(p, "Expecting comma in the instance.", 0);
        p->pCur++;
        if ( Prs_ManUtilSkipSpaces(p) )         return 0;
    }
    // check final
    assert( Prs_ManIsChar(p, ')') );
    return 1;
}
// this procedure can return:
// 0 = reached end-of-file; 1 = successfully parsed; 2 = recognized as primitive; 3 = failed and skipped; 4 = error (failed and could not skip)
static inline int Prs_ManReadModule( Prs_Man_t * p )
{
    int iToken, Status;
    if ( p->pNtk != NULL )                  return Prs_ManErrorSet(p, "Parsing previous module is unfinished.", 4);
    if ( Prs_ManUtilSkipSpaces(p) )
    { 
        Prs_ManErrorClear( p );       
        return 0; 
    }
    // read keyword
    iToken = Prs_ManReadName( p );
    if ( iToken != PRS_VER_MODULE )         return Prs_ManErrorSet(p, "Cannot read \"module\" keyword.", 4);
    if ( Prs_ManUtilSkipSpaces(p) )         return 4;
    // read module name
    iToken = Prs_ManReadName( p );
    if ( iToken == 0 )                      return Prs_ManErrorSet(p, "Cannot read module name.", 4);
    if ( Prs_ManIsKnownModule(p, Abc_NamStr(p->pStrs, iToken)) )
    {
        if ( Prs_ManUtilSkipUntilWord( p, "endmodule" ) ) return Prs_ManErrorSet(p, "Cannot find \"endmodule\" keyword.", 4);
        //printf( "Warning! Skipped known module \"%s\".\n", Abc_NamStr(p->pStrs, iToken) );
        Vec_IntPush( &p->vKnown, iToken );
        return 2;
    }
    Prs_ManInitializeNtk( p, iToken, 1 );
    // skip arguments
    if ( Prs_ManUtilSkipSpaces(p) )         return 4;
    if ( !Prs_ManIsChar(p, '(') )           return Prs_ManErrorSet(p, "Cannot find \"(\" in the argument declaration.", 4);
    if ( !Prs_ManReadArguments(p) )         return 4;
    assert( *p->pCur == ')' );
    p->pCur++;
    if ( Prs_ManUtilSkipSpaces(p) )         return 4;
    // read declarations and instances
    while ( Prs_ManIsChar(p, ';') )
    {
        p->pCur++;
        if ( Prs_ManUtilSkipSpaces(p) )     return 4;
        iToken = Prs_ManReadName( p );
        if ( iToken == PRS_VER_ENDMODULE )
        {
            Vec_IntPush( &p->vSucceeded, p->pNtk->iModuleName );
            Prs_ManFinalizeNtk( p );
            return 1;
        }
        if ( iToken >= PRS_VER_INOUT && iToken <= PRS_VER_WIRE ) // declaration
            Status = Prs_ManReadDeclaration( p, iToken );
        else if ( iToken == PRS_VER_REG || iToken == PRS_VER_DEFPARAM ) // unsupported keywords
            Status = Prs_ManUtilSkipUntil( p, ';' );
        else // read instance
        {
            if ( iToken == PRS_VER_ASSIGN )
                Status = Prs_ManReadAssign( p );
            else
                Status = Prs_ManReadInstance( p, iToken );
            if ( Status == 0 )
            {
                if ( Prs_ManUtilSkipUntilWord( p, "endmodule" ) ) return Prs_ManErrorSet(p, "Cannot find \"endmodule\" keyword.", 4);
                //printf( "Warning! Failed to parse \"%s\". Adding module \"%s\" as blackbox.\n", 
                //    Abc_NamStr(p->pStrs, iToken), Abc_NamStr(p->pStrs, p->pNtk->iModuleName) );
                Vec_IntPush( &p->vFailed, p->pNtk->iModuleName );
                // cleanup
                Vec_IntErase( &p->pNtk->vWires );
                Vec_IntErase( &p->pNtk->vWiresR );
                Vec_IntErase( &p->pNtk->vSlices );
                Vec_IntErase( &p->pNtk->vConcats );
                Vec_IntErase( &p->pNtk->vBoxes );
                Vec_IntErase( &p->pNtk->vObjs );
                p->fUsingTemp2 = 0;
                // add
                Prs_ManFinalizeNtk( p );
                Prs_ManErrorClear( p );
                return 3;
            }
        }
        if ( !Status )                      return 4;
        if ( Prs_ManUtilSkipSpaces(p) )     return 4;
    }
    return Prs_ManErrorSet(p, "Cannot find \";\" in the module definition.", 4);
}
static inline int Prs_ManReadDesign( Prs_Man_t * p )
{
    while ( 1 )
    {
        int RetValue = Prs_ManReadModule( p );
        if ( RetValue == 0 ) // end of file
            break;
        if ( RetValue == 1 ) // successfully parsed
            continue;
        if ( RetValue == 2 ) // recognized as primitive
            continue;
        if ( RetValue == 3 ) // failed and skipped
            continue;
        if ( RetValue == 4 ) // error
            return 0;
        assert( 0 );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Prs_ManPrintModules( Prs_Man_t * p )
{
    char * pName; int i; 
    printf( "Succeeded parsing %d models:\n", Vec_IntSize(&p->vSucceeded) );
    Prs_ManForEachNameVec( &p->vSucceeded, p, pName, i )
        printf( " %s", pName );
    printf( "\n" );
    printf( "Skipped %d known models:\n", Vec_IntSize(&p->vKnown) );
    Prs_ManForEachNameVec( &p->vKnown, p, pName, i )
        printf( " %s", pName );
    printf( "\n" );
    printf( "Skipped %d failed models:\n", Vec_IntSize(&p->vFailed) );
    Prs_ManForEachNameVec( &p->vFailed, p, pName, i )
        printf( " %s", pName );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Prs_ManReadVerilog( char * pFileName )
{
    Vec_Ptr_t * vPrs = NULL;
    Prs_Man_t * p = Prs_ManAlloc( pFileName );
    if ( p == NULL )
        return NULL;
    Prs_NtkAddVerilogDirectives( p );
    Prs_ManReadDesign( p );
    //Prs_ManPrintModules( p );
    if ( Prs_ManErrorPrint(p) )
        ABC_SWAP( Vec_Ptr_t *, vPrs, p->vNtks );
    Prs_ManFree( p );
    return vPrs;
}

void Prs_ManReadVerilogTest( char * pFileName )
{
    abctime clk = Abc_Clock();
    extern void Prs_ManWriteVerilog( char * pFileName, Vec_Ptr_t * p );
    Vec_Ptr_t * vPrs = Prs_ManReadVerilog( "c/hie/dump/1/netlist_1.v" );
//    Vec_Ptr_t * vPrs = Prs_ManReadVerilog( "aga/me/me_wide.v" );
//    Vec_Ptr_t * vPrs = Prs_ManReadVerilog( "aga/ray/ray_wide.v" );
    if ( !vPrs ) return;
    printf( "Finished reading %d networks. ", Vec_PtrSize(vPrs) );
    printf( "NameIDs = %d. ", Abc_NamObjNumMax(Prs_ManNameMan(vPrs)) );
    printf( "Memory = %.2f MB. ", 1.0*Prs_ManMemory(vPrs)/(1<<20) );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    Prs_ManWriteVerilog( "c/hie/dump/1/netlist_1_out_new.v", vPrs );
//    Prs_ManWriteVerilog( "aga/me/me_wide_out.v", vPrs );
//    Prs_ManWriteVerilog( "aga/ray/ray_wide_out.v", vPrs );
//    Abc_NamPrint( p->pStrs );
    Prs_ManVecFree( vPrs );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

