#ifndef VM_H
#define VM_H
#include "bytecode.h"

typedef enum {
	NULL_TYPE, 
	INT_TYPE, 
    STRING,
    SLOT,
	METHOD,
	CLASS,
	SYMBOL
}Type;


typedef struct {
	int type;
}Value_t;



typedef struct {
	int type;
	char* id;
	Value_t* object;
	struct Symbol* next;
}Symbol;

typedef struct {
	Symbol* head;
}Symtab;

Symtab* constant_pool;

void init_constpool();
void add_symbol(Symtab* symtab, char* id, Value_t* val );
Value_t* lookup_symbol(Symtab* symtab, char* id);


typedef struct {
	struct Record* next;
}Record;

typedef struct {
	Record* top;
}Stack;

void push (Stack* stack, Record* slot);
Record* pop (Stack* stack);

/* Objects */
typedef struct {
	int type;
}NullObj;



typedef struct {
	int type;
	int val;
}IntObj;

typedef struct {
	int type;
	char* val;
}StringObj;

typedef struct {
	int type;
	StringObj* index;
}SlotObj;

typedef struct {
	int type;
	StringObj* index;
	int nargs;
	int nlocals;
	Vector* body;
}MethodObj;

typedef struct {
	int type;
    Vector* slots;
}ClassObj;




/* Instructions */
void lit(Symtab* constant_pool, Value_t* val);


void* allocate_obj(Type type);

void run_prog (Program* p) ;
void interpret_bc (Program* prog);

#endif
