#include "hashmap.h"

unsigned long simple_hash(const char* str){
    int cle = 0;
    for (int i=0; str[i]!='\0'; i++){
        cle += str[i] - '0';
    }
    double A = (sqrt(5)-1)/2;
    unsigned long hash = (unsigned long)(floor(TABLE_SIZE * (cle * A - floor(cle * A))));
    return hash;
}

HashMap* hashmap_create(){
    
    HashMap* nouveau = (HashMap*)(malloc(sizeof(HashMap)));
    if(!nouveau) {
        printf("Erreur d'allocation\n");
        return NULL;
    }
    nouveau->size = 0;
    
    nouveau->table = (HashEntry*)(calloc(TABLE_SIZE,sizeof(HashEntry)));
    if(!nouveau->table){
        printf("Erreur d'allocation\n");
        free(nouveau);
        return NULL;
    }
    
    return nouveau;
}

int hashmap_insert(HashMap* map, const char *key, void *value) {
    unsigned long index = simple_hash(key) % TABLE_SIZE; // on retrouve l'index de l'élément
    if (map->size == TABLE_SIZE) {  // si la table est pleine
        return 1;
    }
    
    // linear probing
    while (map->table[index].key != NULL) {
        if (strcmp(map->table[index].key, key) == 0) { //clé deja existante, on remplace
            map->table[index].value = value;
            return 0;
        }
        index = (index + 1) % TABLE_SIZE;
    }
    
    map->table[index].key = strdup(key);
    map->table[index].value = value;
    map->size++;
    return 0;
}

void* hashmap_get(HashMap *map, const char *key) {
    
    unsigned long index = simple_hash(key) % TABLE_SIZE;
    
    while (map->table[index].key != NULL) {
        if (strcmp(map->table[index].key, key) == 0) {
            return map->table[index].value;
        }
        index = (index + 1) % TABLE_SIZE;
    }
    return NULL;
}

int hashmap_remove(HashMap *map, const char *key){
    unsigned long index = simple_hash(key) % TABLE_SIZE;
    unsigned long index_debut = index;
    
    while (map->table[index].key != NULL) {
        if (strcmp(map->table[index].key, key) == 0) {
            free(map->table[index].key);
            map->table[index].key = TOMBSTONE; // on remplace par la tombstone
            map->table[index].value = NULL;
            map->size--;
            return 0;
        }
        index = (index + 1) % TABLE_SIZE;

        if(index == index_debut){
            break;
        }
    }
    return 1;
}

void hashmap_destroy(HashMap *map){
    for (int i = 0; i < TABLE_SIZE; i++) {
        if (map->table[i].key != NULL && map->table[i].key != TOMBSTONE) {
            free(map->table[i].key);
        }
    }
    free(map->table);
    free(map);
}


void afficher_hashmap(HashMap* map) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        if (map->table[i].key == TOMBSTONE) {
            printf("index %d: clé = [TOMBSTONE], valeur = %p\n", i, map->table[i].value);
        } else if (map->table[i].key != NULL) {
            printf("index %d: clé = %s, valeur = %d\n", i, map->table[i].key, *(int*)(map->table[i].value));
        }
    }
}


// int main() {
//     HashMap* map = hashmap_create();
//     // if (!map) {
//     //     return 1;
//     // }
    
//     int val1 = 100;
//     hashmap_insert(map, "aaa", &val1);
//     int val2 = 200;
//     hashmap_insert(map, "bbb", &val2);
//     int val3 = 300;
//     hashmap_insert(map, "ccc", &val3);

//     afficher_hashmap(map);


//     hashmap_remove(map, "bbb"); //existe

//     printf("\naprès suppression:\n");
//     afficher_hashmap(map);

//     if (!hashmap_remove(map, "ddd") == 0) { // n'existe pas
//         printf("n'existe pas\n");
//     }

//     hashmap_destroy(map);

//     return 0;
// }