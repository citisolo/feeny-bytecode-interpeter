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


void call(MethodObj* fn, Record* parent){

	Record* frame = new_frame(parent);
	unsigned int size = fn->nargs + fn->nlocals;
	/* add args to frame */
	frame->locals = malloc(sizeof(Value_t*) * size);

	/* add locals to frame args first and then locals*/
	int i ;
	for ( i = 0; i < fn->nargs; i++) {
		frame->locals[i] = os_pop(opstack);
	}
    for (; i < size; i++) {
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

void exec(){
  while(1){
	  switch(pc->tag){
		  case LABEL_OP:{
			LabelIns* i = (LabelIns*)pc;
			StringObj* obj = (StringObj*)lookup(constant_pool, int_to_id(i->name));
			IntObj* address = allocate_obj(INT_TYPE);
			address->val = counter.index;
			add_symbol(constant_pool, obj->val, address);
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
			for(int j=0; j< ins->arity; j++){
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
				buf[bufindex] = c[charindex];
				buf[bufindex+1] = '\0';
				printf("%s\n",buf);
				memset(buf, '\0', bufsize);
				bufindex=0;
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
						   printf("Format does not understand type %d",v->type);
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
			for(int i=0;i<arr->len;i++){
				arr->elems[i] = init;
			}
			os_push(opstack, arr);
			pc = vector_get(counter.code, ++counter.index);
			break;
		  }
		  case OBJECT_OP:{
			ObjectIns* i = (ObjectIns*)pc;
			ClassObj* class = (ClassObj*)lookup(global_table, int_to_id(i->class));

			ClassObj* instance = (ClassObj*) allocate_obj(CLASS);
			
			Vector* vec = make_vector();
			/*Pop vars off of stack */
			Value_t* slot = os_peek(opstack,0);
			while (slot != NULL && slot->type == SLOT){
               	vector_add(vec, os_pop(opstack));
				slot = os_peek(opstack, 0);
			}
			
			vector_add(vec, os_pop(opstack));/*push parent object */
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
			SlotObj* s = NULL;
			while(it < obj->slots->size){
				SlotObj* s = (SlotObj*)vector_get(obj->slots, it);
				if(strcmp(s->index->val, ref->val)==0){
					/*get the value of the slot*/
					val = s->val;
					it = obj->slots->size;
				}				
				it++;
			}			
			if(s == NULL){
				printf("Slot %s not found in object", ref->val);
				exit(1);
			}
						
			os_push(opstack, s);
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
			int it = 0;
			while(it < object->slots->size){
				SlotObj* s = (SlotObj*)vector_get(object->slots, it);
				if(strcmp(s->index->val, index->val)==0){
					s->val = x; /*store x into val*/
				    
					it = object->slots->size;
					/*val = lookup(constant_pool, s->id);/*get the value of the slot
					val = x;
					
					i = object->slots->size;*/
				}				
				it++;
			}
			if(val == NULL){
				printf("Slot %s not found in object", index->val);
				exit(1);
			}

            os_push(opstack, x);
			pc = vector_get(counter.code, ++counter.index);
			break;
		  }
		  case CALL_SLOT_OP:{
			CallSlotIns* i = (CallSlotIns*)pc;
			/*printf("   call-slot #%d %d", i->name, i->arity);*/
			pc = vector_get(counter.code, ++counter.index);
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
		    call(fn, fp);
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
			Symbol* sym = lookup_sym(global_table, i->name);
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
			Symbol* sym = lookup_sym(global_table, i->name);
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
  
  call(fn, NULL);
  exec();
  /*execute_frame(p->entry, NULL);
  printf("\nEntry : #%d", p->entry);*/
}

void interpret_bc (Program* p) {
  printf("Interpreting Bytecode Program:\n");
  run_prog(p);
  printf("\n");
}

