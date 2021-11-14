// ┌──────────┬──────────┬───────────────┬───────────────────┐
// │ priority │ operator │ associativity │    description    │
// ├──────────┼──────────┼───────────────┼───────────────────┤
// │ 1        │ ()       │ ->            │ round bracket     │
// │          │ []       │               │ array index       │
// │          │ {}       │               │ array declaration │
// ├──────────┼──────────┼───────────────┼───────────────────┤
// │ 2        │ +        │ <-            │ plus sign         │
// │          │ -        │               │ negative sign     │
// │          │ *        │               │ expand array      │
// │          │ !        │               │ logical not       │
// │          │ ~        │               │ bitwise not       │
// ├──────────┼──────────┼───────────────┼───────────────────┤
// │ 3        │ *        │ ->            │ mul               │
// │          │ /        │               │ div               │
// ├──────────┼──────────┼───────────────┼───────────────────┤
// │ 4        │ %        │ ->            │ mod               │
// ├──────────┼──────────┼───────────────┼───────────────────┤
// │ 5        │ +        │ ->            │ add               │
// │          │ -        │               │ sub               │
// ├──────────┼──────────┼───────────────┼───────────────────┤
// │ 6        │ <<       │ ->            │ bitwise lsh       │
// │          │ >>       │               │ Bitwise rsh       │
// ├──────────┼──────────┼───────────────┼───────────────────┤
// │ 7        │ >        │ ->            │ gtr               │
// │          │ >=       │               │ geq               │
// │          │ <        │               │ lss               │
// │          │ <=       │               │ leq               │
// ├──────────┼──────────┼───────────────┼───────────────────┤
// │ 8        │ ==       │ ->            │ equ               │
// │          │ !=       │               │ neq               │
// ├──────────┼──────────┼───────────────┼───────────────────┤
// │ 9        │ &        │ ->            │ bitwise and       │
// ├──────────┼──────────┼───────────────┼───────────────────┤
// │ 10       │ ^        │ ->            │ bitwise xor       │
// ├──────────┼──────────┼───────────────┼───────────────────┤
// │ 11       │ |        │ ->            │ bitwise or        │
// ├──────────┼──────────┼───────────────┼───────────────────┤
// │ 12       │ &&       │ ->            │ logical and       │
// ├──────────┼──────────┼───────────────┼───────────────────┤
// │ 13       │ ||       │ ->            │ logical or        │
// ├──────────┼──────────┼───────────────┼───────────────────┤
// │ 14       │ =        │ <-            │ assign            │
// │          │ /=       │               │ assign after div  │
// │          │ *=       │               │ assign after mul  │
// │          │ %=       │               │ assign after mod  │
// │          │ +=       │               │ assign after add  │
// │          │ -=       │               │ assign after sub  │
// │          │ <<=      │               │ assign after lsh  │
// │          │ >>=      │               │ assign after rsh  │
// │          │ &=       │               │ assign after and  │
// │          │ ^=       │               │ assign after xor  │
// │          │ |=       │               │ assign after or   │
// ├──────────┼──────────┼───────────────┼───────────────────┤
// │ 15       │ ,        │ ->            │ comma             │
// └──────────┴──────────┴───────────────┴───────────────────┘

#define OP_BRE     0x01 // braket expression
#define OP_ARG     0x02 // argument list
#define OP_IDX     0x03 // array index
#define OP_ARR     0x04 // array declaration
#define OP_BRE_S   0x41 // start of OP_BRE
#define OP_ARG_S   0x42 // start of OP_ARG
#define OP_IDX_S   0x43 // start of OP_IDX
#define OP_ARR_S   0x44 // start of OP_ARR
#define OP_BRE_E   0x81 // end of OP_BRE
#define OP_ARG_E   0x82 // end of OP_ARG
#define OP_IDX_E   0x83 // end of OP_IDX
#define OP_ARR_E   0x84 // end of OP_ARR
#define OP_PL      0x05 // plus sign
#define OP_NL      0x06 // negative sign
#define OP_EPA     0x07 // expand array
#define OP_LNOT    0x08 // logical not
#define OP_NOT     0x09 // bitwise not
#define OP_MUL     0x0a // mul
#define OP_DIV     0x0b // div
#define OP_MOD     0x0c // mod
#define OP_ADD     0x0d // add
#define OP_SUB     0x0e // sub
#define OP_LSH     0x0f // bitwise left shift
#define OP_RSH     0x10 // bitwise right shift
#define OP_GTR     0x11 // greater than
#define OP_GEQ     0x12 // greater than or equal
#define OP_LSS     0x13 // less than
#define OP_LEQ     0x14 // less than or equal
#define OP_EQU     0x15 // equal
#define OP_NEQ     0x16 // not equal
#define OP_AND     0x17 // bitwise and
#define OP_XOR     0x18 // bitwise xor
#define OP_OR      0x19 // bitwise or
#define OP_LAND    0x1a // logical and
#define OP_LOR     0x1b // logical or
#define OP_ASS     0x1c // ass
#define OP_DIV_ASS 0x1d // assign after div
#define OP_MUL_ASS 0x1e // assign after mul
#define OP_MOD_ASS 0x1f // assign after mod
#define OP_ADD_ASS 0x20 // assign after add
#define OP_SUB_ASS 0x21 // assign after sub
#define OP_LSH_ASS 0x22 // assign after lsh
#define OP_RSH_ASS 0x23 // assign after rsh
#define OP_AND_ASS 0x24 // assign after and
#define OP_XOR_ASS 0x25 // assign after xor
#define OP_OR_ASS  0x26 // assign after or
#define OP_CME     0x27 // comma expression

#define OP_RANGE_MIN OP_BRE
#define OP_RANGE_MAX OP_CME

#ifdef __cplusplus
extern "C" {
#endif

int tok2op(int toktype);
int get_priority(int op);
int get_associativity(int op); // 0: <- 1: ->

#ifdef __cplusplus
}
#endif