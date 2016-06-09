#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/mman.h>
#include "utils.h"
#include "bytecode.h"
#include "vm.h"

#define add_sym(symtab, id, val) add_symbol(symtab, int_to_id(id), val)
#define lookup_sym(symtab, id) lookup_symbol(symtab, int_to_id(id))

void init_constpool(){
	constant_pool = (Symtab*) malloc(sizeof(Symtab));
	constant_pool->head = NULL;
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
	
	
Value_t* lookup_symbol(Symtab* symtab, char* id){
	Symbol* node = symtab->head;
    if (node == NULL){
		return NULL;	
	}else{
		while(node != NULL){
			if(strcmp(node->id, id) == 0){
				return node->object;
			} 
			node = node->next;
		}
	}
	return NULL;
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
    StringObj* string = (StringObj*)lookup_sym(constant_pool, v2->name );
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
    StringObj* string = (StringObj*)lookup_sym(constant_pool, v2->name);
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


void run_prog (Program* p) {
  init_constpool();
  /* Add constants to symnbol table */
  Vector* vs = p->values;
  for(int i=0; i<vs->size; i++){
    add_to_constpool(i, vector_get(vs, i));
  }
  
  /* execute code */
  for(int i=0; i<p->slots->size; i++)
    printf("\n   #%d", (int)vector_get(p->slots, i));
  printf("\nEntry : #%d", p->entry);
}


void interpret_bc (Program* p) {
  printf("Interpreting Bytecode Program:\n");
  run_prog(p);
  printf("\n");
}

