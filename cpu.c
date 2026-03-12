#include "cpu.h"

CPU* cpu_init(int memory_size){
    CPU* cpu = malloc(sizeof(CPU));
    cpu->memory_handler = memory_init(memory_size);
    cpu->context = hashmap_create();

    int* ax = malloc(sizeof(int)); *ax = 0; // on doit faire comme ca sinon ils seront considérés comme des pointeurs nuls
    int* bx = malloc(sizeof(int)); *bx = 0;
    int* cx = malloc(sizeof(int)); *cx = 0;
    int* dx = malloc(sizeof(int)); *dx = 0;
    int* ip = malloc(sizeof(int)); *ip = 0;
    int* zf = malloc(sizeof(int)); *zf = 0;
    int* sf = malloc(sizeof(int)); *sf = 0;
    int* sp = malloc(sizeof(int)); *sp = 127; //Pile de taille 128
    int* bp = malloc(sizeof(int)); *bp = 127;
    int* es = malloc(sizeof(int)); *es = -1;

    hashmap_insert(cpu->context,"AX", ax);
    hashmap_insert(cpu->context,"BX", bx);
    hashmap_insert(cpu->context,"CX", cx);
    hashmap_insert(cpu->context,"DX", dx);
    hashmap_insert(cpu->context,"IP", ip);
    hashmap_insert(cpu->context,"ZF", zf);
    hashmap_insert(cpu->context,"SF", sf);
    hashmap_insert(cpu->context,"SP", sp);
    hashmap_insert(cpu->context,"BP", bp);
    hashmap_insert(cpu->context,"ES", es);

    Segment* prev = NULL;
    Segment* ss_segment = find_free_segment(cpu->memory_handler, 0, 128, &prev);
    if (!ss_segment) {
        printf("Erreur: Pas assez de place pour allouer le segment de pile SS.\n");
        free(cpu);
        return NULL;
    }
    
    int start_ss = ss_segment->start;
    create_segment(cpu->memory_handler, "SS", start_ss, 128);

    cpu->constant_pool = hashmap_create();
    return cpu;
}

void cpu_destroy(CPU* cpu){
    if (cpu == NULL) return;

    free(hashmap_get(cpu->context, "AX"));
    free(hashmap_get(cpu->context, "BX"));
    free(hashmap_get(cpu->context, "CX"));
    free(hashmap_get(cpu->context, "DX"));
    free(hashmap_get(cpu->context, "IP"));
    free(hashmap_get(cpu->context, "ZF"));
    free(hashmap_get(cpu->context, "SF"));
    free(hashmap_get(cpu->context, "SP"));
    free(hashmap_get(cpu->context, "BP"));
    free(hashmap_get(cpu->context, "ES"));
    hashmap_destroy(cpu->context);
    
    for (int i = 0; i < TABLE_SIZE; i++) {
        void* val = cpu->constant_pool->table[i].value;
        if(val){
            free(val);
        }
    }
    hashmap_destroy(cpu->constant_pool);

    Segment* cs = hashmap_get(cpu->memory_handler->allocated, "CS");
    if(cs){
        for(int i = 0; i<cs->size; i++){
            Instruction* instru = load(cpu->memory_handler,"CS",i);
            if(instru){
                free(instru->mnemonic);
                free(instru->operand1);
                free(instru->operand2);
                free(instru);
                store(cpu->memory_handler, "CS", i, NULL);
            }
        }
    }

    free_memory_handler(cpu->memory_handler);

    free(cpu);
}

void* store(MemoryHandler *handler, const char *segment_name,int pos, void *data){
    Segment* seg = hashmap_get(handler->allocated,segment_name);
    if(seg && (pos < seg->size)){ // si le segment existe dans allocated
        handler->memory[seg->start + pos] = data;
        return data;
    }else{
        printf("Segment non alloué ou position invalide\n");
        return NULL;
    }
}

void* load(MemoryHandler *handler, const char *segment_name,int pos){
    Segment* seg = hashmap_get(handler->allocated,segment_name);
    if(seg && (pos < seg->size)){
        return handler->memory[seg->start + pos];
    }else{
        printf("Segment non alloué ou position invalide\n");
        return NULL;
    }
}

void allocate_variables(CPU *cpu, Instruction** data_instructions, int data_count){
    int total_size = 0;
    for (int i = 0; i < data_count; i++) {
        char* val = data_instructions[i]->operand2;
        for (int j = 0; val[j]; j++) {
            if (val[j] == ',') total_size++;
        }
        total_size++;
    }

    Segment* prev = NULL;
    Segment* libre = find_free_segment(cpu->memory_handler, 0, total_size, &prev);
    if (!libre) {
        printf("Aucune place disponible dans la memoire\n");
        return;
    }

    int start = libre->start;
    char* segment_name = "DS";
    if (create_segment(cpu->memory_handler, segment_name, start, total_size) != 0) {
        printf("Erreur d'allocation du segment '%s'\n", segment_name);
        return;
    }

    int offset = 0;
    for (int i = 0; i < data_count; i++) {
        Instruction* instru = data_instructions[i];
        char* val = instru->operand2;

        char* copy = strdup(val);
        char* token = strtok(copy, ",");

        while (token) {
            int* valeur = malloc(sizeof(int));
            *valeur = atoi(token);
            store(cpu->memory_handler, segment_name, offset, valeur);  // Utilise le bon nom du segment
            offset++;
            token = strtok(NULL, ",");
        }
        free(copy);
    }
}

void print_data_segment(CPU *cpu){
    Segment* segment = hashmap_get(cpu->memory_handler->allocated, "DS");
    if (segment == NULL) {
        printf("Segment DS pas alloué\n");
        return;
    }
    printf("Segment DS :\n");
    for (int i = 0; i < segment->size; i++) {
        int* value = (int*)load(cpu->memory_handler, "DS", i);
        if (value != NULL) {
            printf("Index %d: %d\n", i, *value);
        }
    }
}

void* immediate_addressing(CPU* cpu, const char* operand){

    regex_t regex;
    int reti = regcomp(&regex, "^[1-9]*[0-9]$", 0); // de la forme 26
    reti = regexec(&regex, operand,0, NULL, 0);
    regfree(&regex);

    if(reti){
        return NULL;
    }

    void* constant_value = hashmap_get(cpu->constant_pool, operand);
    if (constant_value != NULL) {
        return constant_value; // si la valeur existe deja
    }
    // sinon on l'ajoute dans la pool
    int* value = malloc(sizeof(int));
    *value = atoi(operand);
    hashmap_insert(cpu->constant_pool, operand, value);
    return value;
}

void *register_addressing(CPU *cpu, const char *operand){
    regex_t regex;
    int reti = regcomp(&regex, "^[A-D]X$", 0); // de la forme CX
    reti = regexec(&regex, operand,0, NULL, 0);
    regfree(&regex);

    if(reti){
        return NULL;
    }

    void* retour = hashmap_get(cpu->context,operand);
    return retour; // NULL si le register n'existe pas
}

void *memory_direct_addressing(CPU *cpu, const char *operand){
    regex_t regex;
    int reti = regcomp(&regex, "^\\[([0-9]+)\\]$", REG_EXTENDED); // de la forme [23]
    reti = regexec(&regex, operand, 0, NULL, 0);
    regfree(&regex);
    
    if (reti != 0) {
        return NULL;
    }

    int index;
    sscanf(operand, "[%d]", &index);

    if (index >= cpu->memory_handler->total_size) {
        printf("Index trop grand\n");
        return NULL;
    }

    return load(cpu->memory_handler, "DS", index);
}

void *register_indirect_addressing(CPU *cpu, const char *operand){
    regex_t regex;
    int reti = regcomp(&regex, "^\\[[A-D]X\\]$", REG_EXTENDED); // de la forme [BX]
    reti = regexec(&regex, operand, 0, NULL, 0);
    regfree(&regex);

    if (reti != 0) {
        return NULL;
    }

    // extraire le nom des crochets
    char reg_name[3]; // il ya 2 caracteres et \0
    sscanf(operand, "[%2s]", reg_name); // lis les 2 lettres entre []

    int* address = (int*)hashmap_get(cpu->context, reg_name);
    if (address == NULL) {
        printf("Registre %s introuvable\n", reg_name);
        return NULL;
    }

    if (*address >= cpu->memory_handler->total_size) {
        printf("Index trop grand (%d)\n", *address);
        return NULL;
    }

    return load(cpu->memory_handler, "DS", *address); // plus sûr que memory[*address]
}

void* segment_override_addressing(CPU* cpu, const char* operand){

    regex_t regex;
    int reti = regcomp(&regex, "^\\[([A-Za-z]{2}):([A-D]X)\\]$", REG_EXTENDED); // de la forme [**:*X]
    reti = regexec(&regex, operand, 0, NULL, 0);
    regfree(&regex);

    if (reti != 0) {
        return NULL;
    }

    char seg[3];
    char reg[3];

    sscanf(operand, "[%2s:%2s]", seg, reg);

    int* registre = (int*)hashmap_get(cpu->context,reg);

    int index = *registre;
    if (index < 0 || index >= 256) {
        printf("index invalide\n");
        return NULL;
    }

    return load(cpu->memory_handler,seg,index);

}

void handle_MOV(CPU* cpu, void* src, void* dest){
   (void)cpu;
   *(int*)dest = *(int*)src; 
}

void *resolve_addressing(CPU *cpu, const char *operand) {
    void* retour;

    retour = immediate_addressing(cpu, operand);
    if (retour) {
        return retour;
    }

    retour = register_addressing(cpu, operand);
    if (retour) {
        return retour;
    }

    retour = memory_direct_addressing(cpu, operand);
    if (retour) {
        return retour;
    }

    retour = register_indirect_addressing(cpu, operand);
    if (retour) {
        return retour;
    }

    retour = segment_override_addressing(cpu, operand);
    if (retour) {
        return retour;
    }

    printf("Addressing non réussi\n");
    return NULL;
}

void allocate_code_segment(CPU *cpu, Instruction** code_instructions, int code_count){
    Segment* prev = NULL;
    Segment* libre = find_free_segment(cpu->memory_handler, 0, code_count, &prev);
    if (!libre) {
        printf("Aucune place disponible dans la memoire\n");
        return;
    }
    int start = libre->start;
    char* segment_name = "CS";
    if (create_segment(cpu->memory_handler, segment_name, start, code_count) != 0) {
        printf("Erreur d'allocation du segment '%s'\n", segment_name);
        return;
    }

    for (int i = 0; i < code_count; i++) {
        Instruction* instru = code_instructions[i];
        Instruction* code_instr_copy = malloc(sizeof(Instruction));
        
        code_instr_copy->mnemonic = strdup(instru->mnemonic);
        if (instru->operand1) {
            code_instr_copy->operand1 = strdup(instru->operand1);
        }
        if (instru->operand2) {
            code_instr_copy->operand2 = strdup(instru->operand2);
        }

        store(cpu->memory_handler, segment_name, i, code_instr_copy);
    }

    int* ip = (int*)hashmap_get(cpu->context, "IP");
    *ip = 0;
}

int alloc_es_segment(CPU *cpu){
    int* size = hashmap_get(cpu->context,"AX");
    int* strat = hashmap_get(cpu->context,"BX");

    int index = find_free_address_strategy(cpu->memory_handler, *size, *strat);

    int* zf = hashmap_get(cpu->context,"ZF");

    if(index==-1){
        *zf = 1;
        return 1;
    }

    *zf = 0;

    create_segment(cpu->memory_handler,"ES",index,*size);
    int* es = hashmap_get(cpu->context,"ES");
    *es = index;

    return 0;
}

int free_es_segment(CPU* cpu){
    
    Segment* es_seg = hashmap_get(cpu->memory_handler->allocated,"ES");
    
    for(int i=0; i<es_seg->size; i++){
        void* p = load(cpu->memory_handler,"ES",i);
        if(p){
            free(p);
        }
    }

    remove_segment(cpu->memory_handler,"ES");

    int* es = hashmap_get(cpu->context,"ES");
    *es = -1;

    return 0;

}

int handle_instruction(CPU *cpu, Instruction *instr, void *src, void *dest){
    char* m = instr->mnemonic;
    if(!strcmp(m,"MOV")){
        *(int*)dest = *(int*)src;
        return 0;
    }if(!strcmp(m,"ADD")){
        *(int*)dest = *(int*)dest + *(int*)src;
        return 0;
    }if(!strcmp(m,"CMP")){
        int result = *(int*)dest - *(int*)src;
        int* zf = hashmap_get(cpu->context,"ZF");
        int* sf = hashmap_get(cpu->context,"SF");
        *zf = 0;
        *sf = 0;
        if(result == 0){
            *zf = 1;
        }
        if(result < 0){
            *sf = 1;
        }
        return 0;
    }if(!strcmp(m,"JMP")){
        int* ip = hashmap_get(cpu->context,"IP");
        *ip = *(int*)src;
        return 0;
    }if(!strcmp(m,"JZ")){
        int* zf = hashmap_get(cpu->context,"ZF");
        if(*zf == 1){
            int* ip = hashmap_get(cpu->context,"IP");
            *ip = *(int*)src; 
        }
        return 0;
    }if(!strcmp(m,"JNZ")){
        int* zf = hashmap_get(cpu->context,"ZF");
        if(*zf == 0){
            int* ip = hashmap_get(cpu->context,"IP");
            *ip = *(int*)src; 
        }
        return 0;
    }if(!strcmp(m,"HALT")){
        int* ip = hashmap_get(cpu->context,"IP");
        *ip = -1;
        return 0;
    }
    
    if(!strcmp(m, "PUSH")){
        int* value;
        if(src){
            value = (int*)src;
        }else{
            value = hashmap_get(cpu->context, "AX");  // AX par défaut
        }
        return push_value(cpu, *value);
    }
    
    if(!strcmp(m, "POP")){
        int* dest_reg;
        if(src){
            dest_reg = (int*)src;
        } else {
            dest_reg = hashmap_get(cpu->context, "AX");
        }
        return pop_value(cpu, dest_reg);
    
    }if(!strcmp(m,"ALLOC")){
        return alloc_es_segment(cpu);
    }

    if(!strcmp(m,"FREE")){
        return free_es_segment(cpu);
    }

    printf("expression %s non reconnue\n",m);
    return -1;


}

int execute_instruction(CPU *cpu, Instruction *instr){
    
    if(strcmp(instr->operand2,"")){ // De la forme m op1 op2
        void* dest = hashmap_get(cpu->context,instr->operand1);
        void* src = resolve_addressing(cpu,instr->operand2);
        if(dest && src){
            return handle_instruction(cpu,instr,src,dest);
        }
        printf("Operand non reconnu\n");
        return -1;
    }else if(strcmp(instr->operand1,"")){ // De la forme m op1
        void* src = resolve_addressing(cpu,instr->operand1);
        if(src){
            return handle_instruction(cpu,instr,src,NULL);
        }
        printf("Operand non reconnu\n");
        return -1;
    }else{ // de la forme mnemonic, comme HALT
        return handle_instruction(cpu,instr,NULL,NULL);
    }
    
}

Instruction* fetch_next_instruction(CPU *cpu){
    
    int* ip = (int*) hashmap_get(cpu->context, "IP");
    if (ip == NULL) {
        printf("IP introuvable\n");
        return NULL;
    }

    Segment* code_segment = hashmap_get(cpu->memory_handler->allocated, "CS");
    if (code_segment == NULL) {
        printf("CS non alloué\n");
        return NULL;
    }

    if (*ip < 0 || *ip >= code_segment->size) {
        return NULL; // programme fini (-1) avec halt ou fini
    }

    Instruction* instruction = (Instruction*)load(cpu->memory_handler, "CS", *ip);

    (*ip)++;

    return instruction;

}

int run_program(CPU *cpu){
    
    printf("Etat initial du CPU:\n");

    printf("Registres:\n");
    afficher_hashmap(cpu->context);

    Instruction* instru;
    afficher_hashmap(cpu->memory_handler->allocated);

    while (1) {
        char entry;
        printf("n pour continuer, q pour quitter : ");
        scanf(" %c", &entry);  // espace avant %c pour ignorer '\n'

        if (entry == 'q') break;
        if (entry != 'n') continue;

        instru = fetch_next_instruction(cpu);
        if (!instru) {
            printf("Fin du programme ou instruction invalide.\n");
            break;
        }

        printf("\nInstruction : %s %s %s\n", instru->mnemonic ? instru->mnemonic : "", instru->operand1 ? instru->operand1 : "", instru->operand2 ? instru->operand2 : "");

        execute_instruction(cpu, instru);

        printf("Après instruction:\n");
        afficher_hashmap(cpu->context);
        printf("\n");
    }

    printf("\nEtat final des registres :\n");
    afficher_hashmap(cpu->context);

    return 0;
}

int push_value(CPU *cpu, int value){
    int* sp = hashmap_get(cpu->context, "SP");

    if(*sp <= 0){
        printf("Pile pleine\n");
        return -1;
    }

    (*sp)-=1;

    int* val = malloc(sizeof(int));
    *val = value;
    store(cpu->memory_handler,"SS", *sp, val);
    
    return 0;
}

int pop_value(CPU *cpu, int *dest){
    int* sp = hashmap_get(cpu->context, "SP");
    int* bp = hashmap_get(cpu->context, "BP");
    if(*sp >= *bp){
        printf("Pile vide\n");
        return -1;
    }

    int* val = (int*)load(cpu->memory_handler,"SS",*sp);
    *dest = *val;
    free(val);
    store(cpu->memory_handler, "SS", *sp, NULL);

    (*sp)++;

    return 0;

}

CPU* setup_test_environment(){

    // Initialiser le CPU
    CPU* cpu = cpu_init (1024);
    if(!cpu){
        printf("Error: CPU initialization failed\n");
        return NULL;
    }

    // Initialiser les registres avec des valeurs specifiques
    int* ax = (int*)hashmap_get(cpu->context, "AX");
    int* bx = (int*)hashmap_get(cpu->context, "BX");
    int* cx = (int*)hashmap_get(cpu->context, "CX");
    int* dx = (int*)hashmap_get(cpu->context, "DX");

    *ax = 3;
    *bx = 6;
    *cx = 100;
    *dx = 0;
    
    //Creer et initialiser le segment de donnees
    if(!hashmap_get(cpu->memory_handler->allocated, "DS")){
        create_segment(cpu->memory_handler, "DS", 0, 20);
        // Initialiser le segment de données avec des valeurs de test
        for(int i=0; i<10; i ++){
            int* value = (int*)malloc(sizeof(int));
            *value = i*10 + 5; // Valeurs 5, 15, 25, 35...
            store(cpu->memory_handler, "DS", i, value);
        }
    }

    printf("Test environment initialized\n") ;
    return cpu ;
}


// -------- Fonctions de test ----------------

// int main2() {
//     CPU* cpu = setup_test_environment();
//     void* source;

//     // Immediate
//     source = resolve_addressing(cpu, "35");
//     void* dest = hashmap_get(cpu->context, "AX");
//     handle_MOV(cpu, source, dest);
//     printf("AX (immédiat) = %d\n", *(int *)dest);

//     // Register Direct
//     source = resolve_addressing(cpu, "AX");
//     dest = hashmap_get(cpu->context, "BX");
//     handle_MOV(cpu, source, dest);
//     printf("BX (register direct) = %d\n", *(int *)dest);

//     // Memory direct
//     source = resolve_addressing(cpu, "[0]");
//     dest = hashmap_get(cpu->context, "CX");
//     if (source && dest) {
//         handle_MOV(cpu, source, dest);
//         printf("CX (mémoire directe) = %d\n", *(int *)dest);
//     } else {
//         printf("Erreur: Source ou destination invalide pour MOV mémoire directe\n");
//     }

//     source = resolve_addressing(cpu, "[CX]");
//     dest = hashmap_get(cpu->context, "DX");
//     if (source && dest) {
//         handle_MOV(cpu, source, dest);
//         printf("DX (indirect) = %d\n", *(int *)dest);
//     } else {
//         printf("Erreur: Source ou destination invalide pour MOV indirect de registre\n");
//     }
//     cpu_destroy(cpu);
//     free_parser_result(result);
//     return 0;
// }


// int main3() {
//     CPU* cpu = setup_test_environment();

//     // Exemple de programme à exécuter
//     Instruction* code[] = {
//         &(Instruction){"MOV", "AX", "10"},
//         &(Instruction){"ADD", "AX", "15"},
//         &(Instruction){"CMP", "AX", "25"},
//         &(Instruction){"JZ", "6", NULL},
//         &(Instruction){"MOV", "BX", "100"},
//         &(Instruction){"HALT", NULL, NULL},
//         &(Instruction){"MOV", "BX", "999"},  // Ne sera atteint que si CMP == 0
//         &(Instruction){"HALT", NULL, NULL},
//     };

//     int code_count = sizeof(code) / sizeof(code[0]);

//     allocate_code_segment(cpu, code, code_count);

//     run_program(cpu);

//     // tout free

//     Segment* code_segment = hashmap_get(cpu->memory_handler->allocated, "CS");
//     if (code_segment != NULL) {
//         // Libérer chaque instruction
//         for (int i = 0; i < code_count; i++) {
//             Instruction* instru = (Instruction*)load(cpu->memory_handler, "CS", i);
//             if (instru != NULL) {
//                 free(instru->mnemonic);
//                 if (instru->operand1) free(instru->operand1);
//                 if (instru->operand2) free(instru->operand2);
//                 free(instru);
//             }
//         }
//     }
//     cpu_destroy(cpu);
//     return 0;
// }

// int main() {
//     CPU* cpu = cpu_init(256); // petite mémoire pour test

//     // On initialise les registres
//     int* ax = hashmap_get(cpu->context, "AX");
//     int* bx = hashmap_get(cpu->context, "BX");
//     int* cx = hashmap_get(cpu->context, "CX");
//     *ax = 42;
//     *bx = 99;
//     *cx = 0;

//     // PUSH AX, PUSH BX, POP CX, POP (vers AX par défaut)
//     Instruction* code[] = {
//         &(Instruction){"PUSH", "AX", NULL},
//         &(Instruction){"PUSH", "BX", NULL},
//         &(Instruction){"POP", "CX", NULL},
//         &(Instruction){"POP", NULL, NULL},
//         &(Instruction){"HALT", NULL, NULL}
//     };

//     int code_count = sizeof(code) / sizeof(code[0]);

//     allocate_code_segment(cpu, code, code_count);

//     run_program(cpu);

//     // Résultats
//     printf("Registre AX: %d\n", *ax); 
//     printf("Registre BX: %d\n", *bx); 
//     printf("Registre CX: %d\n", *cx); 


//     cpu_destroy(cpu);
//     return 0;
// }