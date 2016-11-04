#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <assert.h>
#include "utils.h"
#include "bytecode.h"
#include "vm.h"


#define add_sym(symtab, id, val) add_symbol(symtab, int_to_id(id), val)
#define lookup_sym(symtab, id) lookup_symbol(symtab, int_to_id(id))
#define INC_PC  pc = vector_get(counter.code, ++counter.index);

int GETCOUNT = 0;
int MODCOUNT = 0;
char* concat(char *s1, char *s2)
{
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);
    char *result = malloc(len1+len2+1);//+1 for the zero-terminator
    //in real code you would check for errors in malloc here
    memcpy(result, s1, len1);
    memcpy(result+len1, s2, len2+1);//+1 to copy the null-terminator
    return result;
}

void print_constpool();
void print_runstack();
void current_ins();
							
void init(){
	constant_pool = (Symtab*) malloc(sizeof(Symtab));
	constant_pool->head = NULL;
	global_table = (Symtab*) malloc(sizeof(Symtab));
	global_table->head = NULL;
	runstack = (RuntimeStack*) malloc(sizeof(RuntimeStack));
	runstack->top = NULL;
	opstack = (OperandStack*) malloc(sizeof(OperandStack));
	opstack->top = NULL;
	pc = 0;
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
		case ARRAY:{
		    ArrayObj* obj = (ArrayObj*)malloc(sizeof(ArrayObj));
		    obj->type = ARRAY;
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
		printf("Cannot allocate resources for symbol %s\n",id);
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

Value_t* lookup(Symtab* symtab, char* id) {
	Symbol* sym = lookup_symbol(symtab, id);
	if(sym == NULL){ 
		/*printf("Symbol not found");*/ 
		return NULL; 
	}
	else 
	  return sym->object;
	  
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
	frame->nargs = 0;
	frame->nlocals = 0;
	frame->type = RECORD;
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
    StringObj* string = (StringObj*)lookup(constant_pool, int_to_id(v2->name));
    if( string != NULL){
		val->index = string;
	}else{
		printf("Method: %d is not defined", v2->name);
		exit(1);
	} 
    val->nargs = v2->nargs;
    val->nlocals = v2->nlocals;
    val->body = v2->code;
    val->calls = 0;
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
    /*
    for(int it = 0; it < v2->slots->size; it++){
		Value* slot = (Value*)vector_get(v2->slots, it);
		switch(slot->tag){
			case SLOT_VAL:{
				SlotValue* s2 = (SlotValue*)slot;
				SlotObj* member = (SlotObj*)lookup(constant_pool, int_to_id(s2->name));
				vector_add(val->slots, member);
				break;
			}
			case METHOD_VAL:{
				MethodValue* m2 = (MethodValue*)slot;
				MethodObj* method = (MethodObj*) lookup(constant_pool, int_to_id(m2->name));
				vector_add(val->slots, method);
			default:
				printf("Value with unknown member tag: %d\n", slot->tag);
				exit(-1);
			
			}
		}
	}*/
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

IntObj* add (IntObj* x, IntObj* y){
	IntObj* res = allocate_obj(INT_TYPE);
	res->val = x->val + y->val;
	return res;	
}

IntObj* sub (IntObj* x, IntObj* y){
	IntObj* res = allocate_obj(INT_TYPE);
	res->val = x->val - y->val;
	return res;	
}

IntObj* mul (IntObj* x, IntObj* y){
	IntObj* res = allocate_obj(INT_TYPE);
	res->val = x->val * y->val;
	return res;	
}

IntObj* divd (IntObj* x, IntObj* y){
	IntObj* res = allocate_obj(INT_TYPE);	
	res->val = x->val / y->val;
	return res;	
}

IntObj* mod (IntObj* x, IntObj* y){
	IntObj* res = allocate_obj(INT_TYPE);
	res->val = x->val % y->val;
	return res;	
}

Value_t* eq (IntObj* x, IntObj* y){
   Value_t* res;
   if ( x->val == y->val ){
	   IntObj* i = allocate_obj(INT_TYPE);
	   i->val = 0;
	   res = i;
   }
   else{
	   res = allocate_obj(NULL_TYPE);
   }
   return res;	
}

Value_t* lt (IntObj* x, IntObj* y){
   Value_t* res;
   if ( x->val < y->val ){
	   IntObj* i = allocate_obj(INT_TYPE);
	   i->val = 0;
	   res = i;
   }
   else{
	   res = allocate_obj(NULL_TYPE);
   }
   return res;
}

Value_t* le (IntObj* x, IntObj* y){
   Value_t* res;
   if ( x->val <= y->val ){
	   IntObj* i = allocate_obj(INT_TYPE);
	   i->val = 0;
	   res = i;
   }
   else{
	   res = allocate_obj(NULL_TYPE);
   }
	return res;	
}

Value_t* ge (IntObj* x, IntObj* y){
   Value_t* res;
   if ( x->val >= y->val ){
	   IntObj* i = allocate_obj(INT_TYPE);
	   i->val = 0;
	   res = i;
   }
   else{
	   res = allocate_obj(NULL_TYPE);
   }
	return res;	
}

Value_t* gt (IntObj* x, IntObj* y){
   Value_t* res;
   if ( x->val > y->val ){
	   IntObj* i = (IntObj*)allocate_obj(INT_TYPE);
	   i->val = 0;
	   res = i;
   }
   else{
	   res = allocate_obj(NULL_TYPE);
   }
	return res;	
}

IntObj* array_length (ArrayObj* a){
	IntObj* len = allocate_obj(INT_TYPE);
	len->val = a->len;
	return len;
}

NullObj* array_set (ArrayObj* a, IntObj* i, Value_t* v){
	a->elems[i->val] = v;
	NullObj* null = allocate_obj(NULL_TYPE);
	return null;
}

Value_t* array_get (ArrayObj* a, IntObj* i){
	return a->elems[i->val];
}

void call(MethodObj* fn, Record* parent, char* name){

	Record* frame = new_frame(parent);
	frame->name = name;
	frame->nargs = fn->nargs;
	
	unsigned int size = fn->nargs + fn->nlocals;
	frame->nlocals = size;
	/* add args to frame */
	frame->locals = malloc(sizeof(Value_t*) * size);

	/* add  frame args first and then locals*/
	for (int  i = fn->nargs-1; i >=0 ; i--) {
		frame->locals[i] = os_pop(opstack);
	}

	/*save the return address*/
	if(parent != NULL){
		/*frame->ret = pc;*/
		frame->ret.code = counter.code;
		frame->ret.index = counter.index + 1;
	}
    /*set the pc to the start of the function*/
    counter.code = fn->body;
    counter.index = 0;
	pc = vector_get(counter.code, counter.index);
    ByteIns* ins;
    if (fn->calls < 1){/* resolve labels*/
		for(int j=0; j<fn->body->size; j++){ 
		  ins = vector_get(fn->body, j);
		  if(ins->tag == LABEL_OP){
			 LabelIns* ins2 = (LabelIns*)ins;
			 StringObj* obj = (StringObj*)lookup(constant_pool, int_to_id(ins2->name));
			 IntObj* address = allocate_obj(INT_TYPE);
			 address->val = j+1;

			 add_symbol(constant_pool, obj->val, address);	     	  
		  }
		}
    }
    /*push frame on stack and set frame pointer*/
	rs_push(runstack, frame);
	fp = runstack->top;
	fn->calls++;
	
}

void call_method(MethodObj* fn, Record* parent, Value_t* args[], char* name ){

	Record* frame = new_frame(parent);
	frame->name = name;
	frame->nargs = fn->nargs;
	unsigned int size = fn->nargs + fn->nlocals;
	frame->nlocals = size;
	/* add args to frame */
	frame->locals = malloc(sizeof(Value_t*) * size);
	int it = 0 ;
	/*frame->locals[it++] = reciever;*/
	for ( ; it < fn->nargs; it++) {
		frame->locals[it] = args[it];
	}

	/*save the return address*/
	if(parent != NULL){
		/*frame->ret = pc;*/
		frame->ret.code = counter.code;
		frame->ret.index = counter.index + 1;
	}
    /*set the pc to the start of the function*/
    counter.code = fn->body;
    counter.index = 0;
	pc = vector_get(counter.code, counter.index);
    ByteIns* ins;
    /*preprocess labels*/
    for(int j=0; j<fn->body->size; j++){ 
      ins = vector_get(fn->body, j);
      if(ins->tag == LABEL_OP){
	     LabelIns* ins2 = (LabelIns*)ins;
         StringObj* obj = (StringObj*)lookup(constant_pool, int_to_id(ins2->name));
		 IntObj* address = allocate_obj(INT_TYPE);
		 address->val = j+1;

		 add_symbol(constant_pool, obj->val, address);	     	  
	  }
    }
    /*push frame on stack and set frame pointer*/
	rs_push(runstack, frame);
	fp = runstack->top;
	
}

void call_builtin_slot( StringObj* message, Value_t* args[] ){
	 Value_t* res;
	 Value_t* object_ref = args[0];
	 char* lastex;
	 switch(object_ref->type){
		case INT_TYPE:{
			if(strcmp(message->val, "add") == 0){
				res = add(args[0], args[1]);
			}
			else if (strcmp(message->val, "sub") == 0){
				res = sub(args[0], args[1]);
			}
			else if (strcmp(message->val, "mul") == 0){
				res = mul(args[0], args[1]);	
			}
			else if (strcmp(message->val, "div")== 0){
				res = divd(args[0], args[1]);
			}
			else if (strcmp(message->val, "mod")== 0){
				res = mod(args[0], args[1]);
			}
			else if (strcmp(message->val, "eq")== 0){
				res = eq(args[0], args[1]);
			}
			else if (strcmp(message->val, "lt")== 0){
				res = lt(args[0], args[1]);
			}
			else if (strcmp(message->val, "le")== 0){
				res = le(args[0], args[1]);
			}
			else if (strcmp(message->val, "gt")== 0){
				res = gt(args[0], args[1]);

			}
			else if (strcmp(message->val, "ge")== 0){
				res = ge(args[0], args[1]);
			}else{
			  printf("No  operation %s defined for built-in object\n", message->val);
			  exit(-1);
			}
			break;
	      }
	    case ARRAY:{
		    ArrayObj* array = (ArrayObj*) object_ref;
			if (strcmp(message->val, "get")== 0){
				
				IntObj* arg1 = (IntObj*) args[1];
				if ( arg1->type != INT_TYPE){
					printf("Array index must be integer value\n");
					exit(-1);					
				}else if (arg1->val >= array->len || arg1->val < 0){
					printf("Array index out of bounds error\n");
					exit(-1);
				}
                res = array_get(array, arg1);

			}
			else if (strcmp(message->val, "set")== 0){
				IntObj* arg1 = args[1];
				Value_t* arg2 = args[2];
				if (arg1->type != INT_TYPE){
					printf("Array index must be integer value\n");
					exit(-1);					
				}
				NullObj* null = array_set(array, arg1, arg2);
				res = null; 
			}
			else if (strcmp(message->val, "length")== 0){
				IntObj* len = array_length(array);
				res = len;
			}
			else{
			  printf("No  operation %s defined for built-in object\n", message->val);
			  exit(-1);
			}
			break;
		  }
		  default:
			  printf("No  operation %s defined for built-in object\n", message->val);
			  exit(-1);
		  
		  }

    os_push(opstack, res);

}

void current_ins(){
	 printf("<Function:%s:%d>\n",fp->name, counter.index);
     print_ins(pc);	printf("\n");
}

void print_object(Value_t* obj){
		switch(obj->type){
		case INT_TYPE:{
			IntObj* o2 = (IntObj*)obj;
            printf("[IntObj]:\n");
            printf("val: %d\n", o2->val);
			break;
		}
		case ARRAY:{
            ArrayObj* o2 = (ArrayObj*)obj;
            printf("[ArrayObj]:\n");
            for(int i = 0; i< o2->len; i++){
				printf("[%d]",i);
				IntObj* num = allocate_obj(INT_TYPE);
				num->val = i;
				print_object(array_get(o2,num));
			}
			break;
		}
		case NULL_TYPE:{
		    printf("[NullObj]:\n");
		    break;
		}
		case SLOT:{
            SlotObj* o2 = (SlotObj*)obj;
            printf("[SlotObj]:\n");
            printf("index: ");
            print_object(o2->index);
            printf("val: ");
            if(o2->val != NULL){
				print_object(o2->val);
		    }else
		      printf("<NULL>\n");
		    break;
		}
		case STRING:{
		    StringObj* o2 = (StringObj*)obj;
		    printf("[StringObj]:\n");
		    printf("val: %s\n",o2->val);
		    break;
		}
		case METHOD:{
		    MethodObj* o2 = (MethodObj*)obj;
		    printf("[MethodObj]:\n");
		    printf("index: \n ");
		    print_object(o2->index);
		    printf("nargs: %d\n",o2->nargs);
		    printf("nlocals: %d\n", o2->nlocals);
		    break;
		}
		case CLASS:{
		    ClassObj* o2 = (ClassObj*)obj;
		    printf("[ClassObj]:\n");
		    /*printf("    parent: ");
		    print_object(o2->parent);*/
		    printf("Type: %d\n",o2->id);
		    printf("Id: %p\n",o2);
		    break;
		}
		case RECORD:{
		    Record* o2 = (Record*)obj;
		    printf("[Record]:\n");
		    printf("name: %s\n",o2->name);
		    printf("nlocals: %d\n",o2->nlocals);
		    printf("locals:\n");
		    for(int i=0; i<o2->nlocals;i++){
				printf("[%d] ",i);
				if(o2->locals[i] == NULL){
					NullObj* null = allocate_obj(NULL_TYPE);
					print_object(null);
				}else{
					print_object(o2->locals[i]);
				}
			}
			printf("\n");
		    break;
		}
		default:
		    printf("Cannot print unknown type %d\n", obj->type);
            exit(-1);
	    }
}

void print_constpool(){
	Symbol* sym = constant_pool->head;
	while(sym != NULL){
		printf("*************\n");
		printf("id: %s\n", sym->id);
		print_object(sym->object);
		sym = sym->next;
	}
	printf("*************\n");
	printf("*--END OF TABLE--*\n");
}

void print_opstack(){
	Operand* op = opstack->top;
	while(op != NULL){
		printf("-------------\n");
        print_object(op->val);
        op = op->next;
	}
	printf("-------------\n");
	printf("*--END OF STACK--*\n");
	
}

void print_runstack(){
	Record* frame = runstack->top;
	while(frame != NULL){
		printf("==============\n");
		/*print_object(frame);*/
		printf("Function:%s",frame->name);
		frame = frame->parent;
	}
	printf("==============\n");
	printf("*--END OF STACK--*\n");
}

Value_t* get_slot(ClassObj* obj, char* index){
     Value_t* val = NULL;
     for(int i=0; i<obj->slots->size;i++){
		int slotnum = (int)vector_get(obj->slots, i); 
		Value_t* s = (Value_t*)lookup(constant_pool, int_to_id(slotnum));
		
		SlotObj* s1 = (SlotObj*)s;
		if(strcmp(s1->index->val, index)==0){
			if(s->type == METHOD){
				val = (MethodObj*) s;
				goto success;
			}else{
				val = s1->val;
				goto success;
			}
		}
	 }
	 
	 printf("Slot %s not found in object\n", index);
	 exit(1); 
	 
	 
	 success:
	 return val;	
}

void exec(){
  while(1){
	  switch(pc->tag){
		  case LABEL_OP:{
			LabelIns* i = (LabelIns*)pc;
			StringObj* obj = (StringObj*)lookup(constant_pool, int_to_id(i->name));
			/*IntObj* address = allocate_obj(INT_TYPE);
			address->val = counter.index;
			add_symbol(constant_pool, obj->val, address);*/
			pc = vector_get(counter.code, ++counter.index);
			break;
		  }
		  case LIT_OP:{
			LitIns* i = (LitIns*)pc;
			Symbol* s = lookup_sym(constant_pool, i->idx);
			if (s == NULL){
				printf(" symbol '%d' not defined ", i->idx);
				exit(1);
			}
			os_push(opstack, s->object);
			pc = vector_get(counter.code, ++counter.index);
			break;
		  }
		  case PRINTF_OP:{
			PrintfIns* ins = (PrintfIns*)pc;
			StringObj* format = (StringObj*)lookup(constant_pool, int_to_id(ins->format));
			Value_t* args[ins->arity+1];
			/*args = malloc(sizeof(Value_t*) * ins->arity+1);*/
			/*for(int j=0; j< ins->arity; j++){
				args[j] = os_pop(opstack);
			}*/
			
			for(int j = ins->arity-1; j>=0; j--){
				args[j] = os_pop(opstack);
			}
			/*printf("%s %d", format->val, i->arity);*/
			char* c = format->val;
			int bufindex, argindex, charindex;
			bufindex = 0; argindex=0; charindex=0;
			size_t size = strlen(format->val);
		    unsigned int bufsize = (size * sizeof(char))+1;
			char* buf = malloc(bufsize);
			memset(buf, '\0', bufsize );
			
			while(c[charindex] != '\0'){
			  if (charindex == size-1) {
				if(c[charindex] == '~'){
					Value_t* v = args[argindex];
					switch(v->type){
						case INT_TYPE:{
						   IntObj* v2 = (IntObj*)v;
						   printf("%s%d",buf ,v2->val);
						   break;
					    }
						default:
						   printf("Format does not understand type %d\n",v->type);
						   exit(-1);
				    }
				    argindex++;
				    bufindex=0;
				    memset( buf, '\0', bufsize);/*clearbuffer*/
				}else{	
					buf[bufindex] = c[charindex];
					buf[bufindex+1] = '\0';
					printf("%s",buf);
					memset(buf, '\0', bufsize);
					bufindex=0;
					}
		      }
			  else if(c[charindex] == '~'){
					Value_t* v = args[argindex];
					switch(v->type){
						case INT_TYPE:{
						   IntObj* v2 = (IntObj*)v;
						   printf("%s%d",buf ,v2->val);
						   break;
					    }
						default:
						   printf("Format does not understand type %d\n",v->type);
						   exit(-1);
				    }
				    argindex++;
				    bufindex=0;
				    memset( buf, '\0', bufsize);/*clearbuffer*/
				    
				}
			  else{
				buf[bufindex] = c[charindex];
				bufindex++;
			   }
			  charindex++;
			}
			
			pc = vector_get(counter.code, ++counter.index);
			
			break;
		  }
		  case ARRAY_OP:{	
		    Value_t* init = os_pop(opstack);
	        IntObj* len = (IntObj*) os_pop(opstack);
			ArrayObj* arr = allocate_obj(ARRAY);
			arr->len = len->val;
			arr->elems = malloc(sizeof(Value_t*) * arr->len);
			for(int i=0;i<arr->len;i++){
				arr->elems[i] = init;
			}
			os_push(opstack, arr);
			pc = vector_get(counter.code, ++counter.index);
			break;
		  }
		  case OBJECT_OP:{
			ObjectIns* i = (ObjectIns*)pc;
			ClassObj* clss = (ClassObj*)lookup(constant_pool, int_to_id(i->class));
            /*print_runstack();*/
            clss->id = i->class;
			Vector* vec = make_vector();
            /* Get the number of slot variables */
			Value_t* slot;
			int numslotvars = 0;
		    int slotnum = (int)vector_get(clss->slots, numslotvars);
		    slot = (Value_t*) lookup(constant_pool, int_to_id(slotnum));
			while(slot->type == SLOT && numslotvars < clss->slots->size){
				++numslotvars;
				slotnum = (int)vector_get(clss->slots, numslotvars);
				slot = (Value_t*) lookup(constant_pool, int_to_id(slotnum));
			}
			

			Value_t* slotarr[numslotvars];
			for (int it = numslotvars-1; it >=0; it--){
				slotarr[it] = os_pop(opstack);
			}
			Value_t* parent = os_pop(opstack);
			
			ClassObj* instance = (ClassObj*) allocate_obj(CLASS);
			SlotObj* oldslot;
			for (int it = 0; it < clss->slots->size; it++){
				if(it < numslotvars){
					SlotObj* newslot = allocate_obj(SLOT);
					slotnum = (int)vector_get(clss->slots, it);
					oldslot = (SlotObj*) lookup(constant_pool, int_to_id(slotnum));
					
					newslot->index = oldslot->index;
					newslot->val = slotarr[it];
					unsigned long uid = instance ;
					uid += it;
					add_symbol(constant_pool, int_to_id(uid), newslot);
					vector_add(vec, uid);
				}else{
					vector_add(vec,vector_get(clss->slots,it));
				}
			}
			instance->id = i->class;
			instance->slots = vec;
			
			/*build the vector with the slots and methods */
			/*SlotObj* newslot;
			ClassObj* instance = (ClassObj*) allocate_obj(CLASS);
			for (int it = 0; it < clss->slots->size; it++){
				
				if(it < numslotvars){
				  /*vector_add(vec, slotarr[it]);
				  slotnum = (int)vector_get(clss->slots, it);
				  newslot = (SlotObj*) lookup(constant_pool, int_to_id(slotnum));
				  newslot->val = slotarr[it];
                  vector_add(vec, (int)slotnum);
				}
				else{
				  vector_add(vec,vector_get(clss->slots,it));
			    }	
				
			}
            vector_add(vec, parent);
			
			
			instance->id = i->class;
			instance->slots = vec; /*set the list of slots*/
            
            os_push(opstack, instance);
			pc = vector_get(counter.code, ++counter.index);
			break;
		  }
		  case SLOT_OP:{
			SlotIns* i = (SlotIns*)pc;
			ClassObj* obj = (ClassObj*)os_pop(opstack);
			StringObj* ref = (StringObj*) lookup(constant_pool, int_to_id(i->name));
			Value_t* val = NULL;
            
            /*search through the object to find the slot that lhs val refers to*/
			int it = 0;
			Value_t* s = NULL;
			
			val = get_slot(obj, ref->val);
			os_push(opstack, val);
			/*  
			while(it < obj->slots->size){
				int slotnum = (int)vector_get(obj->slots, it);
				s = (Value_t*)lookup(constant_pool, int_to_id(slotnum));
				
				if(s->type == SLOT){
					SlotObj* s2 = (SlotObj*)s;
					if(strcmp(s2->index->val, ref->val)==0){
						/*get the value of the slot
						val = s2->val;
						it = obj->slots->size;
					    os_push(opstack, val);
						pc = vector_get(counter.code, ++counter.index);
						goto success;
					}
			    }				
				it++;
			}
             */			
			pc = vector_get(counter.code, ++counter.index);
			break;
		  }
		  case SET_SLOT_OP:{
			SetSlotIns* i = (SetSlotIns*)pc;
			Value_t* x = os_pop(opstack);                   /*get the rhs value*/
			ClassObj* object = (ClassObj*)os_pop(opstack); /*get the reciever object*/
			StringObj* index = (StringObj*)lookup(constant_pool, int_to_id(i->name)); /*get string reference to lhs variable*/
			Value_t* val = NULL;
            
            /*search through the object to find the slot that lhs val refers to*/
			val = get_slot(object, index->val);

            os_push(opstack, val);
			pc = vector_get(counter.code, ++counter.index);
			break;
		  }
		  case CALL_SLOT_OP:{
			CallSlotIns* i = (CallSlotIns*)pc;
			/*printf("   call-slot #%d %d", i->name, i->arity);*/
			/*pc = vector_get(counter.code, ++counter.index);*/
			int nargs = i->arity-1;
			Value_t* args[i->arity];
			for (int it = nargs; it >= 1; it--){
				args[it] = os_pop(opstack);
			}
			Value_t* reciever = os_pop(opstack);
			args[0] = reciever;
			
			
			StringObj* name = (StringObj*) lookup(constant_pool, int_to_id(i->name));	
		    if (reciever->type == INT_TYPE || reciever->type == ARRAY){
				call_builtin_slot(name, args);
				pc = vector_get(counter.code, ++counter.index);
			}
			else{
			   Value_t* message = get_slot(reciever, name->val);
			   call_method(message, fp, args, name->val);
		    }
			/*
			else if (reciever->type == CLASS){
                call_method(message,fp,args,name->val);     
			}
			else{
				printf("CALL_OP Not supported\n");
				exit(0);				
			}*/
			
			break;
		  }
		  case CALL_OP:{
			CallIns* i = (CallIns*)pc;
			StringObj* fn_ref = (StringObj*) lookup(constant_pool, int_to_id(i->name));
			MethodObj* fn = (MethodObj*) lookup(global_table, fn_ref->val);
			if(fn == NULL){
				printf("Function %s is not defined\n", fn_ref->val);
				exit(1);
			}
		    call(fn, fp, fn_ref->val);
			break;
		  }
		  case SET_LOCAL_OP:{
			SetLocalIns* i = (SetLocalIns*)pc;
			fp->locals[i->idx] = opstack->top->val;
			/*fp->locals[i->idx] = os_pop(opstack);*/
			pc = vector_get(counter.code, ++counter.index);
			break;
		  }
		  case GET_LOCAL_OP:{
			GetLocalIns* i = (GetLocalIns*)pc;
			Value_t* val = fp->locals[i->idx];
			os_push(opstack, val);
			pc = vector_get(counter.code, ++counter.index);
			break;
		  }
		  case SET_GLOBAL_OP:{
			SetGlobalIns* i = (SetGlobalIns*)pc;
			Symbol* sym = lookup_sym(constant_pool, i->name);
			if(sym == NULL){
				printf("Symbol %d not found", i->name);
				exit(1);
			}
            sym->object = opstack->top->val;
            /*sym->object = os_pop(opstack);*/
            pc = vector_get(counter.code, ++counter.index);
			break;
		  }
		  case GET_GLOBAL_OP:{
			GetGlobalIns* i = (GetGlobalIns*)pc;
			Symbol* sym = lookup_sym(constant_pool, i->name);
			if(sym == NULL){
				printf("Symbol %d not found", i->name);
				exit(1);
			}
			os_push(opstack, sym->object);
			pc = vector_get(counter.code, ++counter.index);		
			break;
		  }
		  case BRANCH_OP:{
			BranchIns* i = (BranchIns*)pc;
			/*printf("   branch #%d", i->name);*/
			Value_t* br = os_pop(opstack);
			if(br->type != NULL_TYPE){
				StringObj* label = (StringObj*) lookup(constant_pool, int_to_id(i->name));
				IntObj* index = (IntObj*)lookup(constant_pool, label->val);
				counter.index = index->val;
				pc = vector_get(counter.code, counter.index);
			}else{
				pc = vector_get(counter.code, ++counter.index);
			}
			break;
		  }
		  case GOTO_OP:{
			GotoIns* i = (GotoIns*)pc;
			Symbol* sym = lookup_sym(constant_pool, i->name);
			IntObj* address;
			if (sym != NULL){
				StringObj* s = (StringObj*)sym->object;
				address = (IntObj*)lookup(constant_pool, s->val);
			}else{
				printf("label %d not found", i->name);
				exit(1);
			}
			/*pc = vector_get(fp->body, address->val);*/
			counter.index = address->val;
			pc = vector_get(counter.code, counter.index);
			break;
		  }
		  case RETURN_OP:{
			if(fp->parent == NULL){
				goto end;
			}	
			/* set the pc to ret address*/
			counter.code = fp->ret.code;
			counter.index = fp->ret.index;
			pc = vector_get(counter.code, counter.index);
			rs_pop(runstack);    /* pop frame off stack */
			fp = runstack->top;  /* set frame pointer to next frame */
			break;
		  }
		  case DROP_OP:{
			os_pop(opstack);
			pc = vector_get(counter.code, ++counter.index);
			break;
		  }
		  default:{
			printf("Unknown instruction with tag: %u\n", pc->tag);
			exit(-1);
		  }
		 }
		
	}
	end:
	  return;
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
    /*printf("\n   #%d", (int)vector_get(p->slots, i));*/
    add_to_globtable((int)vector_get(p->slots,i));
  }
  /*set the counter to the initial instruction*/
  MethodObj* fn = (MethodObj*) lookup(constant_pool, int_to_id(p->entry));
  if(fn == NULL){
		printf("Method: %d not defined\n", p->entry);
		exit(1);
  }
  else if(fn->type != METHOD){
	    printf("cannot execute %d it is not a method\n", p->entry);
		exit(1); 
  }
  
  call(fn, NULL, fn->index->val);
  exec();
  /*execute_frame(p->entry, NULL);
  printf("\nEntry : #%d", p->entry);*/
}

void interpret_bc (Program* p) {
  printf("Interpreting Bytecode Program:\n");
  run_prog(p);
  printf("\n");
}

