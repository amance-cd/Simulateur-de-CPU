#ifndef SEGMENT_H
#define SEGMENT_H

#include "hashmap.h"

typedef struct segment {
    int start; // Position du début du segment dans la mémoire
    int size; // Taille du segment
    struct segment *next; // Prochain dans la liste chaînée
} Segment;

typedef struct memoryHandler {
    void** memory; // Tableau de pointeurs vers la mémoire allouée
    int total_size; // Taille totale de la mémoire
    Segment* free_list; // Liste chainée des segments libres
    HashMap* allocated; // Table de hachage (nom, segment)
} MemoryHandler;

MemoryHandler* memory_init(int size); // Crée et initialise le memory handler
void free_memory_handler(MemoryHandler* handler); // Libere le memory handler

int create_segment(MemoryHandler *handler, const char *name, int start, int size); // crée un segment à l'index start de taille size
int remove_segment(MemoryHandler* handler, const char* name); // Supprime un segment par son nom

void afficher_free_list(Segment* head);
void afficher_allocated(HashMap* map);

Segment* find_free_segment(MemoryHandler* handler,int start, int size, Segment** prev); // Renvoie le 1er segment à partir de start qui a la size nécessaire
int find_free_address_strategy(MemoryHandler *handler, int size, int strategy); // Renvoie l'index d'un segment libre avec la size nécessaire en fonction de la stratégie demandée

// Stratégies : 
// 0 : First fit - On trouve le premier segment qui marche
// 1 : Best fit - On trouve le segment qui marche et dont la taille est minimale
// 2 : Worst fit - On trouve le segment qui marche et dont la taille est maximale

#endif