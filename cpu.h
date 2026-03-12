#ifndef CPU_H
#define CPU_H

#include "regex.h"


#include "hashmap.h"
#include "segment.h"
#include "parser.h"


typedef struct {
    MemoryHandler* memory_handler;// Gestionnaire de memoire
    HashMap* context;// Registres (AX, BX, CX, DX)
    HashMap* constant_pool;// Table de hachage pour stocker les valeurs immédiates 
}CPU;

// Initialiser et liberer le CPU
CPU* cpu_init(int memory_size);
void cpu_destroy(CPU* cpu);

// Enresgistrer et recupérer des pointeurs dans un segment
void* store(MemoryHandler* handler, const char* segment_name, int pos, void* data);
void* load(MemoryHandler* handler, const char* segment_name, int pos);

// Allocation de données
void allocate_variables(CPU* cpu, Instruction** data_instructions, int data_count); // Alloue le segment data
void allocate_code_segment(CPU* cpu, Instruction** code_instructions, int code_count); // Allour le segmen code

void print_data_segment(CPU* cpu);

// Gestion du segment ES
int alloc_es_segment(CPU* cpu);
int free_es_segment(CPU* cpu);

// Fonctions pop et push sur le stack segment (SS)
int push_value(CPU *cpu, int value);
int pop_value(CPU *cpu, int *dest);

// Adressages

void* immediate_addressing(CPU* cpu, const char* operand);
void* register_addressing(CPU* cpu, const char* operand);
void* memory_direct_addressing(CPU* cpu, const char* operand);
void* register_indirect_addressing(CPU* cpu, const char* operand);
void* segment_override_addressing(CPU* cpu, const char* operand);

void* resolve_addressing(CPU* cpu, const char* operand); //Teste tous les addressages pour trouver le bon

// Instructions
void handle_MOV(CPU* cpu, void* src, void* dest); // Pas utilisé, version généralisée dans handle_instruction
int handle_instruction(CPU *cpu, Instruction *instr, void *src, void *dest); // Applique l'instruction en fonction du l'opération à faire
int execute_instruction(CPU *cpu, Instruction *instr); // Lance handle_instruction, récupère les pointeurs src et dest
Instruction* fetch_next_instruction(CPU *cpu); // Incrémente l'IP (Instruction Pointer), renvoie la prochaine instruction à effectuer

int run_program(CPU *cpu);

CPU* setup_test_environment(); // Environnement de test



#endif