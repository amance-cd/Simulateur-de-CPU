#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define TABLE_SIZE 128
#define TOMBSTONE ((void*)-1)

typedef struct hashentry {
    char* key; 
    void* value;
} HashEntry;

typedef struct hashmap {
    int size; // taille du hashmap
    HashEntry* table; // tableau de taille size de HashEntry
} HashMap;

unsigned long simple_hash(const char *str); // renvoie la version hashée de la clé
HashMap* hashmap_create(); // crée la HashMap
void hashmap_destroy(HashMap *map);

int hashmap_insert(HashMap *map, const char *key, void *value); // Insere un élément à partir de sa clé et sa valeur
int hashmap_remove(HashMap *map, const char *key); // Enleve un élément

void* hashmap_get(HashMap *map, const char *key); // Cherche un élément et renvoie sa valeur

void afficher_hashmap(HashMap* map); // Affiche le hashmap

#endif