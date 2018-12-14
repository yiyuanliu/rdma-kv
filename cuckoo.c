#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define MAX_KV_LEN          128

#define IS_USED(a)          ((a)->size == 0 || (a)->pointer == NULL)

// hash | pointer | size | kv checksum | checksum
typedef struct hash_entry {
    uint32_t hash;
    char* ptr;
    uint32_t size;
    uint32_t kv_checksum;
    uint32_t checksum;
} hash_entry;

int get_value(char* kv_ptr, char* key, char* value) {
    int key_len = strlen(kv_ptr);
    int value_len = strlen(kv_ptr + key_len);
    if (key_len == 0 || strcmp(key, kv_ptr) != 0) {
        return -1;
    }

    if (key_len + value_len > MAX_KV_LEN) {
        exit(-1);
    }

    strcpy(value, kv_ptr + key_len);
    return 1;
}

typedef struct hash_table {
    hash_entry* entry_list1;
    hash_entry* entry_list2;

    int n;
} hash_table;