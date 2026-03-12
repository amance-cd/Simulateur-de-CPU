#include "parser.h"

static int index_memory = 0; //index d'inserion des elements dans memory locations

Instruction *parse_data_instruction(const char *line, HashMap*memory_locations){
    
    char m[256];
    char op1[256];
    char op2[256] = ""; // on l'initialise car il peut être vide
    
    if((sscanf(line, "%255s %255s %255[^\n]", m, op1, op2)==3)){

        Instruction* i = malloc(sizeof(Instruction));
        i -> mnemonic = NULL;
        i ->operand1 = NULL;
        i->operand2 = NULL;

        int* pointer = malloc(sizeof(int)); *pointer = index_memory;

        hashmap_insert(memory_locations,m,(void*)pointer);
        // On calcule le nb d'éléments de l'op2
        int size_op2 = 1;
        for(int i=0; op2[i]; i++){
            if(op2[i] == ','){
                size_op2++;
            }
        }
        index_memory+=size_op2;

        i->mnemonic = strdup(m);
        i->operand1 = strdup(op1);
        i->operand2 = strdup(op2);

        return i;

    }else{
        printf("Erreur de format sur la ligne\n");
        return NULL;
    }
}

Instruction *parse_code_instruction(const char *line, HashMap* labels, int code_count) {
    char label[256];
    char m[256];
    char op1[256];
    char op2[256] = "";

    if (strchr(line, ':')) { // si il y a un label
        Instruction* i = malloc(sizeof(Instruction));
        i->mnemonic = NULL;
        i->operand1 = NULL;
        i->operand2 = NULL;

        if (sscanf(line, "%255[^:]: %255s %255[^,], %255[^\n]", label, m, op1, op2) == 4){ // de la forme label: m op1,op2
            i->mnemonic = strdup(m);
            i->operand1 = strdup(op1);
            i->operand2 = strdup(op2);
        } else if (sscanf(line, "%255[^:]: %255s %255[^\n]", label, m, op1) == 3){ // de la forme label: m op1
            i->mnemonic = strdup(m);
            i->operand1 = strdup(op1);
            i->operand2 = strdup("");
        } else if (sscanf(line, "%255[^:]:", label) == 1) { // de la forme label:
            free(i);
            int* pointer = malloc(sizeof(int));
            *pointer = code_count;
            hashmap_insert(labels, label, (void*)pointer);
            return NULL;
        } else {
            printf("Erreur de format, ligne : %d\n", code_count);
            free(i);
            return NULL;
        }

        int* pointer = malloc(sizeof(int));
        *pointer = code_count;
        hashmap_insert(labels, label, (void*)pointer);

        return i;
    } else { // si il n'y a pas de label
        if (sscanf(line, "%255s %255[^,], %255[^\n]", m, op1, op2) == 3) {
            Instruction* i = malloc(sizeof(Instruction));
            i->mnemonic = strdup(m);
            i->operand1 = strdup(op1);
            i->operand2 = strdup(op2);
            return i;
        } else if (sscanf(line, "%255s %255[^\n]", m, op1) == 2) {
            Instruction* i = malloc(sizeof(Instruction));
            i->mnemonic = strdup(m);
            i->operand1 = strdup(op1);
            i->operand2 = strdup("");
            return i;
        } else if (sscanf(line, "%255s", m) == 1) {
            Instruction* i = malloc(sizeof(Instruction));
            i->mnemonic = strdup(m);
            i->operand1 = strdup("");
            i->operand2 = strdup("");
            return i;
        } else {
            printf("Erreur de format, ligne : %d\n", code_count);
            return NULL;
        }
    }
}


ParserResult* parse(const char *filename){
    
    FILE* file = fopen(filename,"r");
    if(!file){
        printf("Erreur d'ouvrture du fichier.\n");
        return NULL;
    }

    ParserResult* pr = malloc(sizeof(ParserResult));
    pr->data_instructions = NULL;
    pr->code_instructions = NULL;
	pr->code_count = 0;
	pr->data_count = 0;
	pr->labels = hashmap_create();
	pr->memory_locations= hashmap_create();
    
    Instruction* i = NULL;
    char buff[1024];
    int line_type = -1; // 0 si on est en .DATA et 1 si on est en .CODE -1 sinon
    
    while(fgets(buff,sizeof(buff),file)){

        // Ignorer les lignes vides ou ne contenant que des espaces
        int is_empty = 1;
        for(int j = 0; buff[j]; j++){
            if(buff[j] != ' ' && buff[j] != '\t' && buff[j] != '\n' && buff[j] != '\r'){
                is_empty = 0;
                break;
            }
        }
        if(is_empty) continue;
        
        if(strncmp(buff, ".DATA", 5) == 0){
            line_type = 0;
            continue;
        }
        if(strncmp(buff, ".CODE", 5) == 0){
            line_type = 1;
            continue;
        }
        if(line_type == -1){
            continue;
        }
        if(line_type == 0){
            i = parse_data_instruction(buff, pr->memory_locations);
            if (i) {
                pr->data_instructions = realloc(pr->data_instructions, sizeof(Instruction*) * (pr->data_count + 1));
                pr->data_instructions[pr->data_count] = i;
                pr->data_count++;
            } else {
                printf("Erreur de format sur une ligne de données\n");
            }        
		}else if(line_type == 1){
			i = parse_code_instruction(buff, pr->labels, pr->code_count);
			if(i){
                pr->code_instructions = realloc(pr->code_instructions, sizeof(Instruction*) * (pr->code_count + 1));
			    pr->code_instructions[pr->code_count] = i;
			    pr->code_count++;
            }
		}
    }

    fclose(file);
    return pr;

}

void free_parser_result(ParserResult* result){
    
    for(int i = 0; i<result->data_count; i++){
        Instruction* instru = result->data_instructions[i];
        free(instru->mnemonic);
        free(instru->operand1);
        free(instru->operand2);
        free(instru);
    }
    free(result->data_instructions);

    for(int i = 0; i<result->code_count; i++){
        Instruction* instru = result->code_instructions[i];
        free(instru->mnemonic);
        free(instru->operand1);
        free(instru->operand2);
        free(instru);
    }

    free(result->code_instructions);

    for (int i = 0; i < TABLE_SIZE; i++) {
        void* lab = result->labels->table[i].value;
        if(lab){
            free(lab);
        }
        void* mem = result->memory_locations->table[i].value;
        if(mem){
            free(mem);
        }
    }
    hashmap_destroy(result->labels);
    hashmap_destroy(result->memory_locations);

    free(result);
}

// ------------ Cette partie ne marche pas, le remplacement est donc fait par la fonction resolve_constants ----------

char *trim(char *str) {
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') {
        str++;
    }

    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }
    return str;
}

int search_and_replace(char **str, HashMap *values) { // Ne marche pas
    if (!str || !*str || !values) return 0;

    int replaced = 0;
    char *input = *str;

    // Iterate through all keys in the hashmap
    for (int i = 0; i < values->size; i++) {
        if (values->table[i].key && values->table[i].key != (void *)-1) {
            char *key = values->table[i].key;
            int value = *(int*)(long)values->table[i].value; // !!!! IL NE FAUT PAS CAST EN INT MAIS DEREFERENCER LE POINTEUR !!!!

            // Find potential substring match
            char *substr = strstr(input, key);
            if (substr) {
                // Construct replacement buffer
                char replacement[64];
                snprintf(replacement, sizeof(replacement), "%d", value);

                // Calculate lengths
                int key_len = strlen(key);
                int repl_len = strlen(replacement);
                //int remain_len = strlen(substr + key_len);

                // Create new string
                char *new_str = (char *)malloc(strlen(input) - key_len + repl_len + 1);
                strncpy(new_str, input, substr - input);
                new_str[substr - input] = '\0';
                strcat(new_str, replacement);
                strcat(new_str, substr + key_len);

                // Free and update original string
                free(input);
                *str = new_str;
                input = new_str;

                replaced = 1;
            }
        }
    }

    // Trim the final string
    if (replaced) {
        char *trimmed = trim(input);
        if (trimmed != input) {
            memmove(input, trimmed, strlen(trimmed) + 1);
        }
    }

    return replaced;
}


int resolve_constants_avec_search_and_replace(ParserResult *result){
    for (int n = 0; n < result->code_count; n++){
        Instruction *i = result->code_instructions[n];

        if (i->operand1) { 
            search_and_replace(&i->operand1, result->labels);
            search_and_replace(&i->operand1, result->memory_locations);
        }
        if (i->operand2) {
            search_and_replace(&i->operand2, result->labels);
            search_and_replace(&i->operand2, result->memory_locations);

        }
    }
    return 1;
}

// -----------------------------------------------------------------------------------------

// Vérifie si l'operand est de la forme [...];
int is_bracketed_operand(const char *operand, char *out_operand) {
    if (operand[0] == '[' && operand[strlen(operand) - 1] == ']') {
        strncpy(out_operand, operand + 1, strlen(operand) - 2);
        out_operand[strlen(operand) - 2] = '\0';
        return 1;
    }
    return 0;
}

// Version si search and replace ne marche pas
void resolve_constants(ParserResult *result){
    for (int i = 0; i < result->code_count; i++) {
        Instruction *instr = result->code_instructions[i];

        if (instr->operand1) {
            int *addr = NULL;
            char inner_operand[64];

            // Vérifier si operand1 est sous forme [x]
            if (is_bracketed_operand(instr->operand1, inner_operand)) {
                // si c'est entre crochets chercher la valeur de x
                addr = hashmap_get(result->memory_locations, inner_operand);
                if (!addr) addr = hashmap_get(result->labels, inner_operand);

                if (addr) {
                    // remplacer [x] par [valeur]
                    char new_op[64];
                    snprintf(new_op, sizeof(new_op), "[%d]", *addr);
                    free(instr->operand1);
                    instr->operand1 = strdup(new_op);
                }
            } else {
                // de la forme x
                addr = hashmap_get(result->labels, instr->operand1);
                if (!addr) addr = hashmap_get(result->memory_locations, instr->operand1);

                if (addr) {
                    // remplacer x par valeur
                    char new_op[64];
                    snprintf(new_op, sizeof(new_op), "%d", *addr);
                    free(instr->operand1);
                    instr->operand1 = strdup(new_op);
                }
            }
        }

        // pareil pour l'op2
        if (instr->operand2) {
            int *addr = NULL;
            char inner_operand[64];


            if (is_bracketed_operand(instr->operand2, inner_operand)){

                addr = hashmap_get(result->memory_locations, inner_operand);
                if (!addr) addr = hashmap_get(result->labels, inner_operand);

                if (addr) {
                    char new_op[64];
                    snprintf(new_op, sizeof(new_op), "[%d]", *addr);
                    free(instr->operand2);
                    instr->operand2 = strdup(new_op);
                }
            } else {
                addr = hashmap_get(result->labels, instr->operand2);
                if (!addr) addr = hashmap_get(result->memory_locations, instr->operand2);

                if (addr) {
                    char new_op[64];
                    snprintf(new_op, sizeof(new_op), "%d", *addr);
                    free(instr->operand2);
                    instr->operand2 = strdup(new_op);
                }
            }
        }
    }
}


// Pour les tests : 

// int main(int argc, char *argv[]) {
//     if (argc != 2) {
//         printf("Usage: %s <fichier.asm>\n", argv[0]);
//         return EXIT_FAILURE;
//     }

//     const char *filename = argv[1];
//     ParserResult *result = parse(filename);

//     if (!result) {
//         printf("Erreur lors de l'analyse du fichier.\n");
//         return EXIT_FAILURE;
//     }

//     printf("=== DATA INSTRUCTIONS ===\n");
//     for (int i = 0; i < result->data_count; i++) {
//         Instruction *instr = result->data_instructions[i];
//         printf("%s %s %s\n", instr->mnemonic, instr->operand1, instr->operand2);
//     }

//     printf("\n=== CODE INSTRUCTIONS AVANT RESOLUTION DES CONSTANTES ===\n");
//     for (int i = 0; i < result->code_count; i++) {
//         Instruction *instr = result->code_instructions[i];
//         printf("%s %s %s\n", instr->mnemonic, instr->operand1, instr->operand2);
//     }

//     // Résolution des constantes dans les instructions de code
//     printf("\n=== CODE INSTRUCTIONS APRES RESOLUTION DES CONSTANTES ===\n");
//     resolve_constants(result);  // Appel de la fonction resolve_constants pour résoudre les labels et les adresses

//     // Affichage des instructions après la résolution des constantes
//     for (int i = 0; i < result->code_count; i++) {
//         Instruction *instr = result->code_instructions[i];
//         printf("%s %s %s\n", instr->mnemonic, instr->operand1, instr->operand2);
//     }

//     printf("\n=== LABELS ===\n");
//     afficher_hashmap(result->labels);

//     printf("\n=== MEMORY LOCATIONS ===\n");
//     afficher_hashmap(result->memory_locations);

//     free_parser_result(result);

//     return EXIT_SUCCESS;
// }
