#include "rpckeyvaluestore.h"
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <err.h>
#include <fcntl.h>
#include <unistd.h>

// Check if a string is a number
uint8_t isnumber(char *number) {
  int i;
  for (i = 0; number[i] != 0; i++) {
    // Check if a character is a digit
    if (i == 0) {
      if (!isdigit(number[i])) {
        if (number[0] != '-') {
          return 0;
        }
      }
    } else {
      if (!isdigit(number[i])) {
        return 0;
      }
    }
  }
  return 1;
}

// Utilized MurmurHash
uint64_t hash(uint8_t *key, uint64_t size) {
  uint64_t result = 0;

  while (*key) {
    result ^= *key++;
    result ^= result >> 33;
    result *= 0xff51afd7ed558ccdL;
    result ^= result >> 33;
    result *= 0xc4ceb9fe1a85ec53L;
    result ^= result >> 33;
  }

  return result % size;
}

typedef struct LinkedListNodeObj *Node;

typedef struct LinkedListNodeObj {
  uint8_t key[32];
  uint8_t name[32];
  int64_t value;
  uint8_t flag;
  struct LinkedListNodeObj *next;
} LinkedListNodeObj;

typedef struct LinkedListObj *LinkedList;

typedef struct LinkedListObj {
  Node head;
  uint64_t num_items;
} LinkedListObj;

typedef struct KeyValueStoreObj {
  uint64_t num_keys;
  uint64_t num_lists;
  LinkedList *lists;
} KeyValueStoreObj;

// Input: key - the name of the variable
// Input: name - the value of the variable if it holds another variable name
// Input: value - the value of the variable if it holds a number
// Input: flag - the flag to determine the value type of the node
//        (1) = variable (0) = number
// Output: node - the newly created node
//
// Create a new node
Node create_node(uint8_t *key, uint8_t *name, int64_t value, uint8_t flag) {
  Node node = (LinkedListNodeObj *)malloc(sizeof(LinkedListNodeObj));
  if (node != NULL) {
    strncpy((char *)node->key, (char *)key, 31);
    if (flag == 1 && name != NULL) {
      strncpy((char *)node->name, (char *)name, 31);
    } else {
      node->value = value;
    }
    node->flag = flag;
    node->next = NULL;
  }
  return node;
}

// Input: ptr - pointer to the node to be deleted
// Output: none
//
// Delete a node
void delete_node(Node *ptr) {
  if (ptr != NULL && *ptr != NULL) {
    free(*ptr);
    *ptr = NULL;
  }
  return;
}

// Input: list - the linked list to search for a node
// Input: key - the key to search for
// Output: the node that was found
//
// Find the node with a specific key in a linked list
Node find_node(LinkedList list, uint8_t *key) {
  if (list == NULL) {
    return NULL;
  }
  Node node = list->head;
  while (node != NULL) {
    if (strcmp((char *)node->key, (char *)key) == 0) {
      return node;
    }
    node = node->next;
  }
  return node;
}

// Input: none
// Output: the newly created linked list
//
// Create a linked list
LinkedList create_linked_list() {
  LinkedList list = (LinkedListObj *)malloc(sizeof(LinkedListObj));
  if (list != NULL) {
    list->head = NULL;
    list->num_items = 0;
  }
  return list;
}

// Input: ptr - ptr to a linked list
// Output: none
//
// Delete a linked list
void delete_linked_list(LinkedList *ptr) {
  if (ptr != NULL && *ptr != NULL) {
    LinkedList list = *ptr;
    Node head = list->head;
    while (head != NULL) {
      head = head->next;
      delete_node(&head);
    }
    free(list);
    list = NULL;
  }
  return;
}

// Input: list - the linked list to search for a node
// Input: key - the key to search for
// Output: the key of the node that was found
//
// Find the node with a specific key in a linked list and return its key
uint8_t *linked_list_find_key(LinkedList list, uint8_t *key) {
  if (list == NULL) {
    return NULL;
  }
  Node node = find_node(list, key);
  if (node == NULL) {
    return NULL;
  }
  return node->key;
}

// Input: list - the linked list to search for a node
// Input: key - the key to search for
// Output: the variable name value of the node that was found
//
// Find the node with a specific key in a linked list and return its variable
// name value
uint8_t *linked_list_get_item_name(LinkedList list, uint8_t *key) {
  if (list == NULL) {
    return NULL;
  }
  Node node = find_node(list, key);
  if (node == NULL) {
    return NULL;
  }
  if (node->flag == 0) {
    return NULL;
  }
  return node->name;
}

// Input: list - the linked list to search for a node
// Input: key - the key to search for
// Output: the numerical value of the node that was found
//
// Find the node with a specific key in a linked list and return its numerical
// value
int64_t linked_list_get_item_value(LinkedList list, uint8_t *key) {
  if (list == NULL) {
    return ENOENT;
  }
  Node node = find_node(list, key);
  if (node == NULL) {
    return ENOENT;
  }
  if (node->flag == 1) {
    return EFAULT;
  }
  return node->value;
}

// Input: list - the linked list to search for a node
// Input: key - the key to search for
// Output: the flag value of the node that was found
//
// Find the node with a specific key in a linked list and return its flag value
uint8_t linked_list_get_item_flag(LinkedList list, uint8_t *key) {
  if (list == NULL) {
    return ENOENT;
  }
  Node node = find_node(list, key);
  if (node == NULL) {
    return ENOENT;
  }
  return node->flag;
}

// Input: list - the linked list to insert a node
// Input: key - the key to insert
// Input: name - the variable name to insert
// Output: none
//
// Insert a node with a variable name value into a linked list
void linked_list_insert_item_name(LinkedList list, uint8_t *key,
                                  uint8_t *name) {
  Node find = find_node(list, key);
  if (find != NULL) {
    strncpy((char *)find->name, (char *)name, 31);
    find->value = 0;
    find->flag = 1;
    return;
  }
  Node head_node = list->head;
  Node node = create_node(key, name, 0, 1);
  if (node != NULL) {
    node->next = head_node;
    list->head = node;
    list->num_items++;
  }
  return;
}

// Input: list - the linked list to insert a node
// Input: key - the key to insert
// Input: value - the numerical value to insert
// Output: none
//
// Insert a node with a numerical value into a linked list
void linked_list_insert_item_value(LinkedList list, uint8_t *key,
                                   int64_t value) {
  Node find = find_node(list, key);
  if (find != NULL) {
    memset(find->name, 0, 32);
    find->value = value;
    find->flag = 0;
    return;
  }
  Node head_node = list->head;
  Node node = create_node(key, NULL, value, 0);
  if (node != NULL) {
    node->next = head_node;
    list->head = node;
    list->num_items++;
  }
  return;
}

// Input: list - the linked list to delete a node
// Input: key - the key to delete
// Output: none
//
// Delete a node with a specific key from a linked list
uint8_t linked_list_delete_item(LinkedList list, uint8_t *key) {
  if (list == NULL) {
    return ENOENT;
  }
  if (linked_list_find_key(list, key) == NULL) {
    return ENOENT;
  }
  Node head_node = list->head;
  Node node = head_node;
  if (strcmp((char *)node->key, (char *)key) == 0) {
    Node del_node = head_node;
    Node new_head = del_node->next;
    list->head = new_head;
    delete_node(&del_node);
  } else {
    while (node != NULL && node->next != NULL) {
      if (strcmp((char *)node->next->key, (char *)key) == 0) {
        Node before = node;
        Node del_node = node->next;
        before->next = del_node->next;
        node = before;
        delete_node(&del_node);
      }
      node = node->next;
    }
  }
  list->num_items--;
  return 0;
}

// Input: list - the linked list to delete all nodes
// Output: none
//
// Delete all nodes in a linked list
void linked_list_delete_all_items(LinkedList list) {
  Node node = NULL;
  if (list == NULL) {
    return;
  }
  while (list->num_items > 0) {
    node = list->head;
    list->head = list->head->next;
    node->next = NULL;
    delete_node(&node);
    list->num_items--;
  }
  return;
}

// Input: size - the size of the hash table in a key-value store
// Output: the newly created key-value store
//
// Create a key-value store
KeyValueStore create_key_value_store(uint64_t size) {
  KeyValueStore kvstore = (KeyValueStoreObj *)malloc(sizeof(KeyValueStoreObj));
  if (kvstore != NULL) {
    kvstore->num_keys = 0;
    kvstore->num_lists = size;
    kvstore->lists = (LinkedListObj **)calloc(size, sizeof(LinkedListObj));
    for (uint64_t i = 0; i < kvstore->num_lists; i++) {
      kvstore->lists[i] = create_linked_list();
    }
  }
  return kvstore;
}

// Input: ptr - pointer to a key-value store
// Output: (0) if the key-value store was deleted successfully, EINVAL (22) if
// the pointer or contents of the key-value store do not exist
//
// Delete a key-value store
uint8_t delete_key_value_store(KeyValueStore *ptr) {
  if (ptr != NULL && *ptr != NULL) {
    KeyValueStore kvstore = *ptr;
    for (uint64_t i = 0; i < kvstore->num_lists; i++) {
      delete_linked_list(&(kvstore->lists[i]));
    }
    free(kvstore->lists);
    kvstore->lists = NULL;
    free(kvstore);
    kvstore = NULL;
    return 0;
  } else {
    return EINVAL;
  }
}

// Input: kvstore - the key-value store
// Output: the number of keys in the key-value store
//
// Get the number of keys in a key-value store
uint64_t key_value_store_num_keys(KeyValueStore kvstore) {
  if (kvstore == NULL) {
    return 0;
  }
  return kvstore->num_keys;
}

// Input: kvstore - the key-value store
// Output: the number of linked lists in the key-value store
//
// Get the number of linked lists in a key-value store
uint64_t key_value_store_num_lists(KeyValueStore kvstore) {
  if (kvstore == NULL) {
    return 0;
  }
  return kvstore->num_lists;
}

// Input: kvstore - the key-value store
// Input: key - the key to check
// Output: (0) if the key exists in the key-value store, ENOENT (2) if either
// the key-value store or the key do not exist
//
// Check if a key is in a key-vaue store
uint8_t key_value_store_key_check(KeyValueStore kvstore, uint8_t *key) {
  if (kvstore == NULL || key == NULL) {
    return ENOENT;
  }

  uint64_t index = hash(key, key_value_store_num_lists(kvstore));
  uint8_t *check = linked_list_find_key(kvstore->lists[index], key);
  if (check == NULL) {
    return ENOENT;
  }
  return 0;
}

// Input: kvstore - the key-value store
// Input: key - the key to lookup
// Output: the variable name value associated with the key
//
// Lookup a specific key in a key-value store and return its variable name
// value
uint8_t *key_value_store_key_name_lookup(KeyValueStore kvstore, uint8_t *key) {
  if (kvstore == NULL || key == NULL) {
    return NULL;
  }
  uint64_t index = hash(key, key_value_store_num_lists(kvstore));
  uint8_t *name = linked_list_get_item_name(kvstore->lists[index], key);
  return name;
}

// Input: kvstore - the key-value store
// Input: key - the key to lookup
// Output: the numerical value associated with the key
//
// Lookup a specific key in a key-value store and return its numerical value
int64_t key_value_store_key_value_lookup(KeyValueStore kvstore, uint8_t *key) {
  if (kvstore == NULL || key == NULL) {
    return ENOENT;
  }
  uint64_t index = hash(key, key_value_store_num_lists(kvstore));
  int64_t value = linked_list_get_item_value(kvstore->lists[index], key);
  return value;
}

// Input: kvstore - the key-value store
// Input: key - the key to lookup
// Output: the flag value associated with the key or ENOENT (2) if the key-value
// store or key are NULL
//
// Lookup a specific key in a key-value store and return its flag value
uint8_t key_value_store_key_flag_lookup(KeyValueStore kvstore, uint8_t *key) {
  if (kvstore == NULL || key == NULL) {
    return ENOENT;
  }
  uint64_t index = hash(key, key_value_store_num_lists(kvstore));
  uint8_t flag = linked_list_get_item_flag(kvstore->lists[index], key);
  return flag;
}

// Input: kvstore - the key-value store
// Input: key - the key to insert
// Input: name - the variable name to insert
// Output: (0) if the key was inserted successfully or ENOENT (2) if the
// key-value store or key are NULL
//
// Insert a key in a key-value store and return a status code
uint8_t key_value_store_insert_key_name(KeyValueStore kvstore, uint8_t *key,
                                        uint8_t *name) {
  if (kvstore == NULL || key == NULL) {
    return ENOENT;
  }
  int64_t index = hash(key, key_value_store_num_lists(kvstore));
  linked_list_insert_item_name(kvstore->lists[index], key, name);
  kvstore->num_keys++;
  return 0;
}

// Input: kvstore - the key-value store
// Input: key - the key to insert
// Input: value - the numerical value to insert
// Output: (0) if the key was inserted successfully or ENOENT (2) if the 
// key-value store or key are NULL
//
// Insert a key in a key-value store and return a status code
uint8_t key_value_store_insert_key_value(KeyValueStore kvstore, uint8_t *key,
                                         int64_t value) {
  if (kvstore == NULL || key == NULL) {
    return ENOENT;
  }
  int64_t index = hash(key, key_value_store_num_lists(kvstore));
  linked_list_insert_item_value(kvstore->lists[index], key, value);
  kvstore->num_keys++;
  return 0;
}

// Input: kvstore - the key-value store
// Input: key - the key to delete
// Output: (0) if the key was deleted successfully or ENOENT (2) if the
// key-value store or key are NULL
//
// Delete a key in a key-value store and return a status code
uint8_t key_value_store_delete_key(KeyValueStore kvstore, uint8_t *key) {
  if (kvstore == NULL || key == NULL) {
    return ENOENT;
  }
  int64_t index = hash(key, key_value_store_num_lists(kvstore));
  uint8_t status = linked_list_delete_item(kvstore->lists[index], key);
  if (status == 0) {
    kvstore->num_keys--;
  }
  return status;
}

// Input: key - the key to insert
// Input: name - the variable name to insert
// Input: value - the numerical value to insert
// Input: flag - the flag value to insert
// Output: (0) if the key was inserted successfully or ENOENT (2) if the
// key-value store or key are NULL
//
// Insert a key into the log file and return a status code
uint8_t log_insert_key(uint8_t *key, uint8_t *name, int64_t value, uint8_t flag,
                       int logfd) {
  if (key == NULL) {
    return ENOENT;
  }

  FILE *fp;
  fp = fdopen(logfd, "w");

  if (fseek(fp, 0, SEEK_END) == -1) {
    warn("%s", "log.txt");
    return EINVAL;
  }

  if (flag == 1) { // Variable is a name
    if (name == NULL) {
      return ENOENT;
    }
    if (dprintf(logfd, "%s=%s\n", key, name) == -1) {
      return EINVAL;
    }
  } else if (flag == 0) { // Variable is a value
    if (dprintf(logfd, "%s=%ld\n", key, value) == -1) {
      return EINVAL;
    }
  }

  return 0;
}

// Input: kvstore - the key-value store
// Input: logfd - the file descriptor of the log file
// Output: (0) if the key-value store was dumped successfully, ENOENT (2) if
// the key-value store is NULL or EINVAL (22) if there was error in the
// key dumping process
//
// Dump the key-value store to the log file and return a status code
uint8_t log_key_value_store(KeyValueStore kvstore, int logfd) {
  if (kvstore == NULL) {
    return ENOENT;
  }

  FILE *fp;
  LinkedList list;
  Node node;

  fp = fdopen(logfd, "w");

  if (fseek(fp, 0, SEEK_END) == -1) {
    warn("%s", "log.txt");
    return EINVAL;
  }

  for (uint64_t i = 0; i < key_value_store_num_lists(kvstore); i++) {
    list = kvstore->lists[i];
    for (node = list->head; node != NULL; node = node->next) {
      if (node->flag == 1) {
        if (dprintf(logfd, "%s=%s\n", node->key, node->name) == -1) {
          return EINVAL;
        }
      } else if (node->flag == 0) {
        if (dprintf(logfd, "%s=%ld\n", node->key, node->value) == -1) {
          return EINVAL;
        }
      }
    }
  }

  return 0;
}

// Input: kvstore - the key-value store
// Input: filename - the name of the file to dump to
// Output: (0) if the key-value store was dumped successfully, ENOENT (2) if
// the key-value store or the filename is NULL, EEXIST (14) if the file exists
// or EINVAL (22) if there was error in the key dumping process
//
// Dump the key-value store to a file and return a status code
uint8_t dump_key_value_store(KeyValueStore kvstore, char *filename) {
  if (kvstore == NULL || filename == NULL) {
    return ENOENT;
  }

  int fd;
  FILE *fp;
  LinkedList list;
  Node node;

  if ((fd = open(filename, O_WRONLY | O_CREAT,
                 0644)) == -1) { // Open file filename
    warn("%s", filename);
    return EEXIST;
  }

  fp = fdopen(fd, "w");

  for (uint64_t i = 0; i < key_value_store_num_lists(kvstore); i++) {
    list = kvstore->lists[i];
    for (node = list->head; node != NULL; node = node->next) {
      if (node->flag == 1) {
        if (dprintf(fd, "%s=%s\n", node->key, node->name) == -1) {
          return EINVAL;
        }
      } else if (node->flag == 0) {
        if (dprintf(fd, "%s=%ld\n", node->key, node->value) == -1) {
          return EINVAL;
        }
      }
    }
  }

  if (close(fd) == -1) {
    warn("%s", filename);
    return EINVAL;
  }

  return 0;
}

// Input: kvstore - the key-value store
// Input: logfd - the file descriptor of the log file
// Output: (0) if the key-value store was loaded successfully or EINVAL (22) if
// the log file could not be opened or an invalid variable was encountered
//
// Load the key-value store from the log file and return a status code
uint8_t load_log(KeyValueStore kvstore, int logfd) {
  if (kvstore == NULL) {
    return EINVAL;
  }

  FILE *fp;
  uint8_t key[32];
  uint8_t name[32];
  int64_t value;
  char *ptr;

  if ((fp = fdopen(logfd, "r")) == NULL) {
    return EINVAL;
  }

  while (fscanf(fp, "%[^'=']=%s\n", key, name) != EOF) {
    if (isnumber((char *)key)) {
      return EINVAL;
    }

    for (uint8_t i = 0; key[i] != 0; i++) {
      // Check if a character is a valid character
      if (i == 0) {
        if (isdigit(key[i])) {
          return EINVAL;
        }
        if (!isalpha(key[i])) {
          return EINVAL;
        }
      } else {
        if (!isdigit(key[i])) {
          if (!isalpha(key[i])) {
            if (key[i] != '_') {
              return EINVAL;
            }
          }
        }
      }
    }

    if (isnumber((char *)name)) {
      value = strtol((char *)name, &ptr, 10);
      key_value_store_insert_key_value(kvstore, key, value);
    } else {
      for (uint8_t i = 0; name[i] != 0; i++) {
        // Check if a character is a valid character
        if (i == 0) {
          if (isdigit(key[i])) {
            return EINVAL;
          }
          if (!isalpha(name[i])) {
            if (name[i] != '~') {
              return EINVAL;
            }
          }
        } else {
          if (!isdigit(name[i])) {
            if (!isalpha(name[i])) {
              if (name[i] != '_') {
                return EINVAL;
              }
            }
          }
        }
      }

      if (name[0] == '~') {
        key_value_store_delete_key(kvstore, key);
      } else {
        key_value_store_insert_key_name(kvstore, key, name);
      }
    }

    memset(key, 0, 32);
    memset(name, 0, 32);
  }

  return 0;
}

// Input: kvstore - the key-value store
// Input: filename - the name of the file to load from
// Output: (0) if the key-value store was loaded successfully or EINVAL (22) if
// the load file could not be opened or an invalid variable was encountered
//
// Load the key-value store from the load file and return a status code
uint8_t load_key_value_store(KeyValueStore kvstore, char *filename) {
  if (kvstore == NULL || filename == NULL) {
    return EINVAL;
  }

  int fd;
  FILE *fp;
  uint8_t key[32];
  uint8_t name[32];
  int64_t value;
  char *ptr;

  if ((fd = open(filename, O_RDONLY, 0)) == -1) { // Open file filename
    warn("%s", filename);
    return EINVAL;
  }

  if ((fp = fdopen(fd, "r")) == NULL) {
    return EINVAL;
  }

  while (fscanf(fp, "%[^'=']=%s\n", key, name) != EOF) {
    if (isnumber((char *)key)) {
      return EINVAL;
    }

    for (uint8_t i = 0; key[i] != 0; i++) {
      // Check if a character is a valid character
      if (i == 0) {
        if (isdigit(key[i])) {
          return EINVAL;
        }
        if (!isalpha(key[i])) {
          return EINVAL;
        }
      } else {
        if (!isdigit(key[i])) {
          if (!isalpha(key[i])) {
            if (key[i] != '_') {
              return EINVAL;
            }
          }
        }
      }
    }

    if (isnumber((char *)name)) {
      value = strtol((char *)name, &ptr, 10);
      key_value_store_insert_key_value(kvstore, key, value);
    } else {
      for (uint8_t i = 0; name[i] != 0; i++) {
        // Check if a character is a valid character
        if (i == 0) {
          if (isdigit(key[i])) {
            return EINVAL;
          }
          if (!isalpha(name[i])) {
            if (name[i] != '~') {
              return EINVAL;
            }
          }
        } else {
          if (!isdigit(name[i])) {
            if (!isalpha(name[i])) {
              if (name[i] != '_') {
                return EINVAL;
              }
            }
          }
        }
      }

      if (name[0] == '~') {
        key_value_store_delete_key(kvstore, key);
      } else {
        key_value_store_insert_key_name(kvstore, key, name);
      }
    }

    memset(key, 0, 32);
    memset(name, 0, 32);
  }

  if (close(fd) == -1) {
    warn("%s", filename);
    return EINVAL;
  }

  return 0;
}

// Input: key - the key to delete
// Input: logfd - the file descriptor of the log file
// Output: (0) if the key was deleted successfully, ENOENT (2) if the key does
// not exist or EINVAL (22) if the log file could not be opened or an error was
// encountered during the key deletion process
//
// Delete a key from the log file and return a status code
uint8_t log_delete_key(uint8_t *key, int logfd) {
  if (key == NULL) {
    return ENOENT;
  }

  FILE *fp;
  fp = fdopen(logfd, "w");

  if (fseek(fp, 0, SEEK_END) == -1) {
    warn("%s", "log.txt");
    return EINVAL;
  }

  if (dprintf(logfd, "%s=%s\n", key, "~") == -1) {
    return EINVAL;
  }

  return 0;
}

// Input: dirfd - the file descriptor of the directory that holds the log file
// Input: logfdptr - pointer to the file descriptor of the log file
// Output: (0) if the key was deleted successfully or EINVAL (22) if the log
// file could not be opened or error was encountered during the key deletion process
//
// Clear the log file and return a status code
uint8_t clear_log(int dirfd, int *logfdptr) {
  if (unlinkat(dirfd, "log.txt", 0) == -1) {
    return EINVAL;
  }

  int fd;

  if ((fd = openat(dirfd, "log.txt", O_CREAT | O_EXCL, 0644)) == -1) {
    return EINVAL;
  }

  logfdptr = &fd;

  return 0;
}

// Input: kvstore - the key-value store
// Output: (0) if the key was deleted successfully or EINVAL (22) if the
// key-value store could not be opened or error was encountered during the key
// deletion process
//
// Clear the key-value store and return a status code
uint8_t clear_key_value_store(KeyValueStore kvstore) {
  if (kvstore == NULL) {
    return EINVAL;
  }

  uint8_t *key;
  LinkedList list;
  Node node;

  for (uint64_t i = 0; i < key_value_store_num_lists(kvstore); i++) {
    list = kvstore->lists[i];
    for (node = list->head; node != NULL; node = node->next) {
      key = node->key;
      key_value_store_delete_key(kvstore, key);
    }
  }

  return 0;
}
