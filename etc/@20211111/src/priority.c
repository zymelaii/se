#include <se/priority.h>
#include <se/token.h>

static int _priority_map[OP_RANGE_MAX - OP_RANGE_MIN + 1] =
{
     1, // OP_BRE     braket expression
     1, // OP_ARG     argument list
     1, // OP_IDX     array index
     1, // OP_ARR     array declaration
     2, // OP_PL      plus sign
     2, // OP_NL      negative sign
     2, // OP_EPA     expand array
     2, // OP_LNOT    logical not
     2, // OP_NOT     bitwise not
     3, // OP_MUL     mul
     3, // OP_DIV     div
     4, // OP_MOD     mod
     5, // OP_ADD     add
     5, // OP_SUB     sub
     6, // OP_LSH     bitwise left shift
     6, // OP_RSH     bitwise right shift
     7, // OP_GTR     greater than
     7, // OP_GEQ     greater than or equal
     7, // OP_LSS     less than
     7, // OP_LEQ     less than or equal
     8, // OP_EQU     equal
     8, // OP_NEQ     not equal
     9, // OP_AND     bitwise and
    10, // OP_XOR     bitwise xor
    11, // OP_OR      bitwise or
    12, // OP_LAND    logical and
    13, // OP_LOR     logical or
    14, // OP_ASS     ass
    14, // OP_DIV_ASS assign after div
    14, // OP_MUL_ASS assign after mul
    14, // OP_MOD_ASS assign after mod
    14, // OP_ADD_ASS assign after add
    14, // OP_SUB_ASS assign after sub
    14, // OP_LSH_ASS assign after lsh
    14, // OP_RSH_ASS assign after rsh
    14, // OP_AND_ASS assign after and
    14, // OP_XOR_ASS assign after xor
    14, // OP_OR_ASS  assign after or
    15, // OP_CME     comma expression
};

int tok2op(int toktype)
{
    switch (toktype)
    {
        case T_OP_BRACKET0L: return OP_BRE_S;
        case T_OP_BRACKET1L: return OP_IDX_S;
        case T_OP_BRACKET2L: return OP_ARR_S;
        case T_OP_BRACKET0R: return OP_BRE_E;
        case T_OP_BRACKET1R: return OP_IDX_E;
        case T_OP_BRACKET2R: return OP_ARR_E;
        case T_OP_NOT:       return OP_LNOT;
        case T_OP_NOR:       return OP_NOT;
        case T_OP_MUL:       return OP_MUL;
        case T_OP_DIV:       return OP_DIV;
        case T_OP_MOD:       return OP_MOD;
        case T_OP_ADD:       return OP_ADD;
        case T_OP_SUB:       return OP_SUB;
        case T_OP_LSH:       return OP_LSH;
        case T_OP_RSH:       return OP_RSH;
        case T_OP_GTR:       return OP_GTR;
        case T_OP_GEQ:       return OP_GEQ;
        case T_OP_LSS:       return OP_LSS;
        case T_OP_LEQ:       return OP_LEQ;
        case T_OP_EQU:       return OP_EQU;
        case T_OP_NEQ:       return OP_NEQ;
        case T_OP_AND:       return OP_AND;
        case T_OP_XOR:       return OP_XOR;
        case T_OP_OR:        return OP_OR;
        case T_OP_CMP_AND:   return OP_LAND;
        case T_OP_CMP_OR:    return OP_LOR;
        case T_OP_ASS:       return OP_ASS;
        case T_OP_DIV_ASS:   return OP_DIV_ASS;
        case T_OP_MUL_ASS:   return OP_MUL_ASS;
        case T_OP_MOD_ASS:   return OP_MOD_ASS;
        case T_OP_ADD_ASS:   return OP_ADD_ASS;
        case T_OP_SUB_ASS:   return OP_SUB_ASS;
        case T_OP_LSH_ASS:   return OP_LSH_ASS;
        case T_OP_RSH_ASS:   return OP_RSH_ASS;
        case T_OP_AND_ASS:   return OP_AND_ASS;
        case T_OP_XOR_ASS:   return OP_XOR_ASS;
        case T_OP_OR_ASS:    return OP_OR_ASS;
        case T_OP_COMMA:     return OP_CME;
        default: return 0;
    }
}

int get_priority(int op)
{
    op = op & 0xff;
	return op >= OP_RANGE_MIN && op <= OP_RANGE_MAX
		? _priority_map[op - OP_RANGE_MIN] : -1;
}

int get_associativity(int op)
{
    int priority = get_priority(op);
    return priority != 2 && priority != 14;
}