#ifndef __RPCKEYVALUESTORE_H__
#define __RPCKEYVALUESTORE_H__

#include <cstdint>

uint8_t isnumber(char *number);

typedef struct KeyValueStoreObj *KeyValueStore;

KeyValueStore create_key_value_store(uint64_t size);

uint8_t delete_key_value_store(KeyValueStore *ptr);

uint64_t key_value_store_num_keys(KeyValueStore kvstore);

uint64_t key_value_store_num_lists(KeyValueStore kvstore);

uint8_t key_value_store_key_check(KeyValueStore kvstore, uint8_t *key);

uint8_t *key_value_store_key_name_lookup(KeyValueStore kvstore, uint8_t *key);

int64_t key_value_store_key_value_lookup(KeyValueStore kvstore, uint8_t *key);

uint8_t key_value_store_key_flag_lookup(KeyValueStore kvstore, uint8_t *key);

uint8_t key_value_store_insert_key_name(KeyValueStore kvstore, uint8_t *key, uint8_t *name);

uint8_t key_value_store_insert_key_value(KeyValueStore kvstore, uint8_t *key, int64_t value);

uint8_t key_value_store_delete_key(KeyValueStore kvstore, uint8_t *key);

uint8_t log_insert_key(uint8_t *key, uint8_t *name, int64_t value, uint8_t flag, int fd);

uint8_t log_key_value_store(KeyValueStore kvstore, int fd);

uint8_t dump_key_value_store(KeyValueStore kvstore, char *filename);

uint8_t load_log(KeyValueStore kvstore, int fd);

uint8_t load_key_value_store(KeyValueStore kvstore, char *filename);

uint8_t log_delete_key(uint8_t *key, int logfd);

uint8_t clear_log(int dirfd, int *logfd);

uint8_t clear_key_value_store(KeyValueStore kvstore);

#endif
