#ifndef VM_H
#define VM_H
#include "bytecode.h"

typedef enum {
	NULL_TYPE, 
	INT_TYPE, 
    STRING,
    SLOT,
	METHOD,
	ARRAY,
	CLASS,
	SYMBOL,
	OPERAND
}Type;

typedef struct Symbol Symbol;
typedef struct Record Record;
typedef struct Operand Operand;

typedef struct {
	int type;
}Value_t;

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
	Value_t* val;
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


typedef struct  {
	int type;
	int len;
	Value_t** elems;
	
}ArrayObj;

struct Symbol{
	int type;
	char* id;
	Value_t* object;
	Symbol* next;
};

typedef struct {
	Symbol* head;
}Symtab;

Symtab* constant_pool;
Symtab* global_table;

void init_constpool();
void add_symbol(Symtab* symtab, char* id, Value_t* val );
Symbol* lookup_symbol(Symtab* symtab, char* id);


struct Record {
	Value_t** args;
	Value_t** locals;
	Record* parent;
	struct {
		Vector* code;
	    int index;
	}ret;
};

typedef struct {
	Record* top;
}RuntimeStack;

RuntimeStack* runstack;

void rs_push (RuntimeStack* stack, Record* slot);
Record* rs_pop (RuntimeStack* stack);
Record* new_frame( Record* parent); 

struct Operand{
	Value_t* val;
	struct Operand* next;
};

typedef struct {
	Operand* top;
}OperandStack;

OperandStack* opstack;

void os_push (OperandStack* stack, Value_t* slot);
Value_t* os_pop (OperandStack* stack);
Value_t* os_peek(OperandStack* stack, int i);

ByteIns* pc;
Record* fp;

typedef struct {
	Vector* code;
	int index;
} PC;

PC counter;


/* Instructions */
/*void LIT(int i);
void ARRAY();
void PLIT(char* format, int n);
void SETLOCAL(int i);
void GETLOCAL(int i);
void SETGLOBAL(int i);
void GETGLOBAL(int i);
void DROP();

void OBJECT(int i);
void GETSLOT(int i);
void CALLSLOT(int i, int n);

void LABEL(int i);
void BRANCH(int i);
void GOTO(int i);
void RETURN();
void CALL(int i, int n);*/


void* allocate_obj(Type type);

void run_prog (Program* p) ;
void interpret_bc (Program* prog);

#endif
