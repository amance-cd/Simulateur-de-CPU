#ifndef PARSER_H
#define PARSER_H



#include "hashmap.h"

typedef struct {
    char *mnemonic ; // Instruction mnemonic (ou nom de variable pour .DATA)
    char *operand1 ; // Premier operande (ou type pour .DATA)
    char *operand2 ; // Second operande (ou initialisation pour .DATA)
} Instruction ;

typedef struct {
    Instruction ** data_instructions ; // Tableau d’instructions .DATA
    int data_count ; // Nombre d’instructions .DATA
    Instruction ** code_instructions ; // Tableau d’instructions .CODE
    int code_count ; // Nombre d’instructions .CODE
    HashMap * labels ; // labels -> indices dans code_instructions
    HashMap * memory_locations ; // noms de variables -> adresse memoire
} ParserResult ;

Instruction *parse_data_instruction(const char *line, HashMap*memory_locations); // Traite une instruction .DATA
Instruction *parse_code_instruction(const char *line, HashMap* labels, int code_count); // Traite une instruction .CODE
ParserResult* parse(const char *filename); // Traite tout un fichier asm et renvoie un ParserResult

// Partie non utilisée, car search and replace ne marche pas
char *trim(char *str);
int search_and_replace(char **str, HashMap *values);
int resolve_constants_avec_search_and_replace(ParserResult *result);
// ---------------------------------------------------------

void resolve_constants(ParserResult *result); // Remplace les variables et labels par leur valeurs ou leur index

void free_parser_result(ParserResult *result); // Libere la mémoire

#endif