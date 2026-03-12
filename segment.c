#include "hashmap.h"
#include "segment.h"
#include <stdlib.h>
#include <string.h>

// Initialisation du gestionnaire mémoire
MemoryHandler* memory_init(int size){
    MemoryHandler* mh = malloc(sizeof(MemoryHandler));
    Segment* s = malloc(sizeof(Segment));

    s->size = size;
    s->start = 0;
    s->next = NULL;

    mh->memory = (void **)calloc(size, sizeof(void*));
    mh->total_size = size;
    mh->free_list = s;
    mh->allocated = hashmap_create();

    return mh;
}

// Trouver un segment libre qui contient la zone [start, start+size[
    Segment* find_free_segment(MemoryHandler* handler, int start, int size, Segment** prev){
        (void)start;
        *prev = NULL;
        Segment* curr = handler->free_list;
    
        while (curr != NULL) {
            if (curr->size >= size){ 
                return curr;
            }
            *prev = curr;
            curr = curr->next;
        }
        return NULL;
    }
    

// Créer un segment mémoire
int create_segment(MemoryHandler *handler, const char *name, int start, int size){
    Segment* prev = NULL;
    Segment* seg = find_free_segment(handler, start, size, &prev);

    if (!seg) {
        return 1; // Segment non disponible
    }

    // Allouer le nouveau segment
    Segment* new_seg = malloc(sizeof(Segment));
    new_seg->start = start;
    new_seg->size = size;
    new_seg->next = NULL;

    hashmap_insert(handler->allocated, name, new_seg);

    // Ajuster la free_list
    if (seg->start == start) {
        if (seg->size == size) {
            // Segment utilisé entièrement
            if (prev) {
                prev->next = seg->next;
            } else {
                handler->free_list = seg->next;
            }
            free(seg);
        } else {
            // Avancer le début du segment
            seg->start += size;
            seg->size -= size;
        }
    } else {
        if ((start + size) == (seg->start + seg->size)) {
            // Retirer à la fin du segment
            seg->size = start - seg->start;
        } else {
            // Couper le segment en deux
            Segment* tail = malloc(sizeof(Segment));
            tail->start = start + size;
            tail->size = (seg->start + seg->size) - (start + size);
            tail->next = seg->next;

            seg->size = start - seg->start;
            seg->next = tail;
        }
    }

    return 0;
}

int remove_segment(MemoryHandler* handler, const char* name) {
    if (!handler || !name) return 1;

    // Récupérer le segment dans la hashmap
    Segment* seg = (Segment*) hashmap_get(handler->allocated, name);
    if (!seg) {
        printf("Segment '%s' introuvable\n", name);
        return 1;
    }

    // Sauvegarder les infos avant de libérer le segment
    int start = seg->start;
    int size = seg->size;

    // Libérer le segment alloué dans create_segment
    free(seg);

    // Retirer de la hashmap (la valeur est déjà libérée)
    hashmap_remove(handler->allocated, name);

    // Créer un nouveau segment à insérer dans la free list
    Segment* new_seg = malloc(sizeof(Segment));
    new_seg->start = start;
    new_seg->size = size;
    new_seg->next = NULL;

    // Réinsertion triée dans la free list
    Segment *prev = NULL, *curr = handler->free_list;
    while (curr && curr->start < new_seg->start) {
        prev = curr;
        curr = curr->next;
    }

    // Fusion avec le précédent si adjacent
    if (prev && (prev->start + prev->size == new_seg->start)) {
        prev->size += new_seg->size;
        free(new_seg);
        new_seg = prev;
    } else {
        new_seg->next = curr;
        if (prev) {
            prev->next = new_seg;
        } else {
            handler->free_list = new_seg;
        }
    }

    // Fusion avec le suivant si adjacent
    if (curr && (new_seg->start + new_seg->size == curr->start)) {
        new_seg->size += curr->size;
        new_seg->next = curr->next;
        free(curr);
    }

    return 0;
}



void free_memory_handler(MemoryHandler* handler) {
    
    for (int i = 0; i < handler->total_size; i++) {
        if (handler->memory[i]) {
            free(handler->memory[i]);
        }
    }
    free(handler->memory);

    Segment* seg = handler->free_list;
    while (seg) {
        Segment* temp = seg;
        seg = seg->next;
        free(temp);
    }

    for (int i = 0; i < TABLE_SIZE; i++) {
        HashEntry* entry = &handler->allocated->table[i];
        if (entry->key && entry->key != TOMBSTONE) {
            free(entry->value);
        }
    }

    hashmap_destroy(handler->allocated);

    free(handler);
}

// Affiche la liste des segments libres
void afficher_free_list(Segment* head){
    printf("=== FREE LIST ===\n");
    Segment* current = head;
    while (current) {
        printf("[start: %d, size: %d] -> ", current->start, current->size);
        current = current->next;
    }
    printf("NULL\n");
}

// Affiche les segments alloués
void afficher_allocated(HashMap* map){
    printf("=== ALLOCATED SEGMENTS ===\n");
    for (int i = 0; i < TABLE_SIZE; i++) {
        HashEntry* entry = &map->table[i];
        if (entry->key && entry->key != TOMBSTONE) {
            Segment* seg = (Segment*)entry->value;
            printf("%s: [start: %d, size: %d]\n", entry->key, seg->start, seg->size);
        }
    }
}

int find_free_address_strategy(MemoryHandler *handler, int size, int strategy){

    Segment* result = NULL; // Le segment final
    int best_fit_diff = __INT_MAX__;
    int worst_fit_diff = -1;

    Segment* tmp = handler->free_list;
    
    while(tmp){
        if(tmp->size >= size){
            if(strategy==0){ // first fit
                result = tmp;
                break;
            }else if(strategy==1 && (tmp->size - size) < best_fit_diff){ // best fit
                best_fit_diff = tmp->size - size;
                result = tmp;
            }else if(strategy==2 && (tmp->size - size) > worst_fit_diff){ // worst fit
                worst_fit_diff = tmp->size - size;
                result = tmp;
            }
        }
        tmp = tmp->next;
    }

    if(!result) return -1;
    return result->start;

}

// int main() {
//     // Initialisation du gestionnaire mémoire
//     MemoryHandler* handler = memory_init(100);

//     printf("\n>>> Création des segments A, B, C...\n");
//     create_segment(handler, "A", 0, 20);
//     create_segment(handler, "B", 20, 30);
//     create_segment(handler, "C", 50, 20);

//     afficher_allocated(handler->allocated);
//     afficher_free_list(handler->free_list);

//     printf("\n>>> Suppression du segment 'B'...\n");
//     remove_segment(handler, "B");

//     afficher_allocated(handler->allocated);
//     afficher_free_list(handler->free_list);

//     printf("\n>>> Suppression du segment 'A'...\n");
//     remove_segment(handler, "A");

//     afficher_allocated(handler->allocated);
//     afficher_free_list(handler->free_list);

//     printf("\n>>> Suppression du segment 'C'...\n");
//     remove_segment(handler, "C");

//     afficher_allocated(handler->allocated);
//     afficher_free_list(handler->free_list);

//     printf("\n>>> Libération de la mémoire...\n");
//     free_memory_handler(handler);

//     return 0;
// }
