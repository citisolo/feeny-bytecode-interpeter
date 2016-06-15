#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/mman.h>
#include "utils.h"
#include "bytecode.h"
#include "vm.h"

#define add_sym(symtab, id, val) add_symbol(symtab, int_to_id(id), val)
#define lookup_sym(symtab, id) lookup_symbol(symtab, int_to_id(id))

void init(){
	constant_pool = (Symtab*) malloc(sizeof(Symtab));
	constant_pool->head = NULL;
	global_table = (Symtab*) malloc(sizeof(Symtab));
	global_table->head = NULL;
	runstack = (RuntimeStack*) malloc(sizeof(RuntimeStack));
	runstack->top = NULL;
	opstack = (OperandStack*) malloc(sizeof(OperandStack));
	opstack->top = NULL;
}

char* int_to_id(int id ){
	char* str = malloc(sizeof(int)*8+1);
	snprintf(str,sizeof(int)*8+1,"%d", id);
	return str;
}

void* allocate_obj(Type type){
	Value_t* res ;
	switch(type){
		case INT_TYPE:{
		    IntObj* obj = (IntObj*)malloc(sizeof(IntObj));
		    obj->type = INT_TYPE;
		    res = obj;
			break;
		}
		case NULL_TYPE:{
		    NullObj* obj = (NullObj*)malloc(sizeof(NullObj));
		    obj->type = NULL_TYPE;
		    res = obj;
		    break;
		}
		case SLOT:{
		    SlotObj* obj = (SlotObj*)malloc(sizeof(SlotObj));
		    obj->type = SLOT;
		    res = obj;
		    break;
		}
		case STRING:{
		    StringObj* obj = (StringObj*)malloc(sizeof(StringObj));
		    obj->type = STRING;
		    res = obj;
		    break;
		}
		case METHOD:{
		    MethodObj* obj = (MethodObj*)malloc(sizeof(MethodObj));
		    obj->type = METHOD;
		    res = obj;
		    break;
		}
		case CLASS:{
		    ClassObj* obj = (ClassObj*)malloc(sizeof(ClassObj));
		    obj->type = CLASS;
		    res = obj;
		    break;
		}
		case SYMBOL:{
		    Symbol* obj = (Symbol*)malloc(sizeof(Symbol));
		    obj->type = SYMBOL;
		    obj->next = NULL;
		    res = obj;
		    break;
		}
		case OPERAND:{
		    Operand* obj = (Operand*)malloc(sizeof(Operand));
		    obj->type = OPERAND;
		    obj->next = NULL;
		    res = obj;
		    break;
		}
		default:
		    printf("Cannot allocate for unknown type %d\n", type);
            exit(-1);
		
	}
	return res;
}

void add_symbol(Symtab* symtab, char* id, Value_t* val ){
	/*create the new symbol */
	Symbol* sym = allocate_obj(SYMBOL);
	if(sym == NULL){
		printf("Cannot allocate resources for symbol %s",id);
		exit(1);
	}
	sym->id = id;
	sym->object = val;
	sym->next = NULL;
	
	/*if list is empty add to start of list*/
	if (symtab->head == NULL){
        symtab->head = sym;	
	}else{/*else walk to end of list*/
		Symbol* node = symtab->head;
		while(node->next != NULL){
			node = node->next;
		}
		node->next = sym;
	}
	
}
		
Symbol* lookup_symbol(Symtab* symtab, char* id){
	Symbol* node = symtab->head;
    if (node == NULL){
		return NULL;	
	}else{
		while(node != NULL){
			if(strcmp(node->id, id) == 0){
				return node;
			} 
			node = node->next;
		}
	}
	return NULL;
}
	
void rs_push (RuntimeStack* stack, Record* frame){
	if(stack->top == NULL){
		stack->top = frame;
	}else{
        frame->parent = stack->top;
        stack->top = frame;	
	}
}

Record* rs_pop (RuntimeStack* stack){
	Record* rec = stack->top;
	
	if (stack->top == NULL){
		return NULL;
	}else{
	   stack->top = rec->parent;
    }
	return rec;
}

void os_push (OperandStack* stack, Value_t* operand){
	Operand* op = (Operand*) malloc(sizeof(Operand));
	op->val = operand;
	op->next = NULL;
	
	if (stack->top == NULL){
		stack->top = op;
	}else{
		op->next = stack->top;
		stack->top = op;
	}
		
}

Value_t* os_pop (OperandStack* stack){
	
    Operand* op = stack->top;
    if ( op == NULL){
		return NULL;
	}else{
	    stack->top = op->next;	
	}
	 
	return op->val;
	
}

Value_t* os_peek(OperandStack* stack , int i){
	Operand* op = stack->top;
	if(i == 0){
		return op->val;
	}
	while(op->next != NULL && i != 0){
		op = op->next;
		i--;
	}
    return op->val;
}

Record* new_frame(Record* parent){
	Record* frame = (Record*) malloc(sizeof(Record));
	frame->parent = parent;
	return frame;
}

void add_to_constpool (int id, Value* v) {
  switch(v->tag){
  case INT_VAL:{
    IntValue* v2 = (IntValue*)v;
    IntObj* num = (IntObj*)allocate_obj(INT_TYPE);
    num->type = INT_TYPE;
    num->val = v2->value;
    char* str = int_to_id(id);
    add_symbol(constant_pool, str, num);
    
    break;
  }
  case NULL_VAL:{
    NullObj* val = (NullObj*)allocate_obj(NULL_TYPE);
    add_sym(constant_pool, id, val);
    break;
  }
  case STRING_VAL:{
    StringValue* v2 = (StringValue*)v;
    StringObj* val = (StringObj*)allocate_obj(STRING);
    val->val = v2->value;
    add_sym(constant_pool, id, val);
    
    break;
  }
  case METHOD_VAL:{
    MethodValue* v2 = (MethodValue*)v;
    
    MethodObj* val = (MethodObj*)allocate_obj(METHOD);
    StringObj* string = (StringObj*)lookup_sym(constant_pool, v2->name )->object;
    if( string != NULL){
		val->index = string;
	}else{
		printf("Method: %d is not defined", v2->name);
		exit(1);
	} 
    val->nargs = v2->nargs;
    val->nlocals = v2->nlocals;
    val->body = v2->code;
    add_sym(constant_pool, id, val);
    break;
  }
  case SLOT_VAL:{
    SlotValue* v2 = (SlotValue*)v;
    SlotObj* val = (SlotObj*)allocate_obj(SLOT);
    StringObj* string = (StringObj*)lookup_sym(constant_pool, v2->name)->object;
    if( string != NULL){
		val->index = string;
	}else{
		printf("Slot: #%d is not defined", v2->name);
		exit(1);
	} 
    add_sym(constant_pool, id, val);
    break;
  }
  case CLASS_VAL:{
    ClassValue* v2 = (ClassValue*)v;
    ClassObj* val = (ClassObj*)allocate_obj(CLASS);
    val->slots = v2->slots;
    add_sym(constant_pool, id, val);
    break;
  }
  default:
    printf("Value with unknown tag: %d\n", v->tag);
    exit(-1);
  }
}

void add_to_globtable (int id){
	Symbol* sym = lookup_sym(constant_pool, id);
	Value_t* v = sym->object;
    switch(v->type){
		case SLOT:{
			SlotObj* v2 = (SlotObj*)v;
			add_symbol(global_table, v2->index->val, v2);
			break;
		}
		case METHOD:{
			MethodObj* v2 = (MethodObj*)v;
			add_symbol(global_table, v2->index->val, v2);
			break;
		}
		default:
          printf("Value with unknown tag: %d\n", v->type);
          exit(-1);
  }	
	
}

void execute_frame(int fn_index, Record* parent){
	Symbol* fs = lookup_sym(constant_pool, fn_index);
	if(fs == NULL){
		printf("Method: %d not defined", fn_index);
		exit(1);
	}
	MethodObj* fn = (MethodObj*)fs->object;
	if(fn->type != METHOD){
	    printf("cannot execute %d it is not a method", fn_index);
		exit(1);
	}
	
	Record* frame = new_frame(parent);
	/* add args to frame */
	for (int i = 0; i < fn->nargs; i++) {
		frame->args[i] =(SlotObj*)os_pop(opstack);
	}
	/* add locals to frame */
    for (int i = 0; i < fn->nlocals; i++) {
		frame->locals[i] =(SlotObj*)os_pop(opstack);
	}
	
	for(int i=0; i<fn->body->size; i++){
		ByteIns* ins = vector_get(fn->body, i);
		switch(ins->tag){
		  case LABEL_OP:{
			LabelIns* i = (LabelIns*)ins;
			printf("label #%d", i->name);
			break;
		  }
		  case LIT_OP:{
			LitIns* i = (LitIns*)ins;
			Symbol* s = lookup_sym(constant_pool, i->idx);
			if (s == NULL){
				printf(" symbol '%d' not defined ", i->idx);
				exit(1);
			}
			os_push(opstack, s->object);
			break;
		  }
		  case PRINTF_OP:{
			PrintfIns* i = (PrintfIns*)ins;
			printf("   printf #%d %d", i->format, i->arity);
			break;
		  }
		  case ARRAY_OP:{
			printf("   array");
			break;
		  }
		  case OBJECT_OP:{
			ObjectIns* i = (ObjectIns*)ins;
			printf("   object #%d", i->class);
			break;
		  }
		  case SLOT_OP:{
			SlotIns* i = (SlotIns*)ins;
			printf("   slot #%d", i->name);
			break;
		  }
		  case SET_SLOT_OP:{
			SetSlotIns* i = (SetSlotIns*)ins;
			printf("   set-slot #%d", i->name);
			break;
		  }
		  case CALL_SLOT_OP:{
			CallSlotIns* i = (CallSlotIns*)ins;
			printf("   call-slot #%d %d", i->name, i->arity);
			break;
		  }
		  case CALL_OP:{
			CallIns* i = (CallIns*)ins;
			printf("   call #%d %d", i->name, i->arity);
			break;
		  }
		  case SET_LOCAL_OP:{
			SetLocalIns* i = (SetLocalIns*)ins;
			printf("   set local %d", i->idx);
			break;
		  }
		  case GET_LOCAL_OP:{
			GetLocalIns* i = (GetLocalIns*)ins;
			printf("   get local %d", i->idx);
			break;
		  }
		  case SET_GLOBAL_OP:{
			SetGlobalIns* i = (SetGlobalIns*)ins;
			printf("   set global #%d", i->name);
			break;
		  }
		  case GET_GLOBAL_OP:{
			GetGlobalIns* i = (GetGlobalIns*)ins;
			printf("   get global #%d", i->name);
			break;
		  }
		  case BRANCH_OP:{
			BranchIns* i = (BranchIns*)ins;
			printf("   branch #%d", i->name);
			break;
		  }
		  case GOTO_OP:{
			GotoIns* i = (GotoIns*)ins;
			printf("   goto #%d", i->name);
			break;
		  }
		  case RETURN_OP:{
			printf("   return");
			break;
		  }
		  case DROP_OP:{
			printf("   drop");
			break;
		  }
		  default:{
			printf("Unknown instruction with tag: %u\n", ins->tag);
			exit(-1);
		  }
		 }
	}
}

void run_prog (Program* p) {
  init();
  /* Add constants to symnbol table */
  Vector* vs = p->values;
  for(int i=0; i<vs->size; i++){
    add_to_constpool(i, vector_get(vs, i));
  }
  
  /* add global symbols */
  for(int i=0; i<p->slots->size; i++){
    printf("\n   #%d", (int)vector_get(p->slots, i));
    add_to_globtable((int)vector_get(p->slots,i));
  }
  execute_frame(p->entry, NULL);
  printf("\nEntry : #%d", p->entry);
}

void interpret_bc (Program* p) {
  printf("Interpreting Bytecode Program:\n");
  run_prog(p);
  printf("\n");
}

