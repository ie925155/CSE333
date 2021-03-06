/*
 * Copyright 2011 Steven Gribble
 *
 *  This file is part of the UW CSE 333 course project sequence
 *  (333proj).
 *
 *  333proj is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  333proj is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with 333proj.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "CSE333.h"
#include "HashTable.h"
#include "HashTable_priv.h"

// A private utility function to grow the hashtable (increase
// the number of buckets) if its load factor has become too high.
static void ResizeHashtable(HashTable ht);

// a free function that does nothing
static void LLNullFree(LLPayload_t freeme) { }
static void HTNullFree(HTValue_t freeme) { }

HashTable AllocateHashTable(HWSize_t num_buckets) {
  HashTable ht;
  HWSize_t  i;

  // defensive programming
  if (num_buckets == 0) {
    return NULL;
  }

  // allocate the hash table record
  ht = (HashTable) malloc(sizeof(HashTableRecord));
  if (ht == NULL) {
    return NULL;
  }

  // initialize the record
  ht->num_buckets = num_buckets;
  ht->num_elements = 0;
  ht->buckets =
    (LinkedList *) malloc(num_buckets * sizeof(LinkedList));
  if (ht->buckets == NULL) {
    // make sure we don't leak!
    free(ht);
    return NULL;
  }
  for (i = 0; i < num_buckets; i++) {
    ht->buckets[i] = AllocateLinkedList();
    if (ht->buckets[i] == NULL) {
      // allocating one of our bucket chain lists failed,
      // so we need to free everything we allocated so far
      // before returning NULL to indicate failure.  Since
      // we know the chains are empty, we'll pass in a
      // free function pointer that does nothing; it should
      // never be called.
      HWSize_t j;
      for (j = 0; j < i; j++) {
        FreeLinkedList(ht->buckets[j], LLNullFree);
      }
      free(ht);
      return NULL;
    }
  }

  return (HashTable) ht;
}

void FreeHashTable(HashTable table,
                   ValueFreeFnPtr value_free_function) {
  HWSize_t i;

  Verify333(table != NULL);  // be defensive

  // loop through and free the chains on each bucket
  for (i = 0; i < table->num_buckets; i++) {
    LinkedList  bl = table->buckets[i];
    HTKeyValue *nextKV;

    // pop elements off the the chain list, then free the list
    while (NumElementsInLinkedList(bl) > 0) {
      Verify333(PopLinkedList(bl, (LLPayload_t*)&nextKV));
      value_free_function(nextKV->value);
      free(nextKV);
    }
    // the chain list is empty, so we can pass in the
    // null free function to FreeLinkedList.
    FreeLinkedList(bl, LLNullFree);
  }

  // free the bucket array within the table record,
  // then free the table record itself.
  free(table->buckets);
  free(table);
}

HWSize_t NumElementsInHashTable(HashTable table) {
  Verify333(table != NULL);
  return table->num_elements;
}

HTKey_t FNVHash64(unsigned char *buffer, HWSize_t len) {
  // This code is adapted from code by Landon Curt Noll
  // and Bonelli Nicola:
  //
  // http://code.google.com/p/nicola-bonelli-repo/
  static const uint64_t FNV1_64_INIT = 0xcbf29ce484222325ULL;
  static const uint64_t FNV_64_PRIME = 0x100000001b3ULL;
  unsigned char *bp = (unsigned char *) buffer;
  unsigned char *be = bp + len;
  uint64_t hval = FNV1_64_INIT;

  /*
   * FNV-1a hash each octet of the buffer
   */
  while (bp < be) {
    /* xor the bottom with the current octet */
    hval ^= (uint64_t) * bp++;
    /* multiply by the 64 bit FNV magic prime mod 2^64 */
    hval *= FNV_64_PRIME;
  }
  /* return our new hash value */
  return hval;
}

HTKey_t FNVHashInt64(HTValue_t hashval) {
  unsigned char buf[8];
  int i;
  uint64_t hashme = (uint64_t)hashval;

  for (i = 0; i < 8; i++) {
    buf[i] = (unsigned char) (hashme & 0x00000000000000FFULL);
    hashme >>= 8;
  }
  return FNVHash64(buf, 8);
}

HWSize_t HashKeyToBucketNum(HashTable ht, HTKey_t key) {
  return key % ht->num_buckets;
}

static bool is_key_exist_delete(LinkedList ll_head, HTKey_t key,
  LLPayload_t *ppayload, bool act_delete) {
  if (NumElementsInLinkedList(ll_head) == 0) {
    return false;
  }
  bool ret = false;
  LLIter iter = LLMakeIterator(ll_head, 0UL);
  /* case 1: only one LInkedListNode
   * case 2: normal case, current has previous and next node
   * case 3: the iterator point at the end of LinkedList Node
   */
  LLPayload_t payload;
  while ((NumElementsInLinkedList(ll_head) == 1) || LLIteratorHasNext(iter) ||
    (!LLIteratorHasNext(iter) && LLIteratorHasPrev(iter))) {
    LLIteratorGetPayload(iter, &payload);
    if (((HTKeyValuePtr)payload)->key == key) {
      if (act_delete) {
        LLIteratorDelete(iter, &LLNullFree);
      }
      *ppayload = payload;
      ret = true;
      break;
    }
    if (!LLIteratorNext(iter)) {
      break;
    }
  }
  LLIteratorFree(iter);
  return ret;
}

int InsertHashTable(HashTable table,
                    HTKeyValue newkeyvalue,
                    HTKeyValue *oldkeyvalue) {
  HWSize_t insertbucket;
  LinkedList insertchain;
  int32_t ret = 0;
  Verify333(table != NULL);
  ResizeHashtable(table);

  // calculate which bucket we're inserting into,
  // grab its linked list chain
  insertbucket = HashKeyToBucketNum(table, newkeyvalue.key);
  insertchain = table->buckets[insertbucket];

  LLPayload_t payload;
  if (!is_key_exist_delete(insertchain, newkeyvalue.key, &payload, false)) {
    HTKeyValuePtr phtk = (HTKeyValuePtr) malloc(sizeof(HTKeyValue));
    phtk->key = newkeyvalue.key;
    phtk->value = newkeyvalue.value;
    AppendLinkedList(insertchain, phtk);
    table->num_elements++;
    ret = 1;
  } else {  // case of update
    oldkeyvalue->key = ((HTKeyValuePtr)payload)->key;
    oldkeyvalue->value = ((HTKeyValuePtr)payload)->value;
    ((HTKeyValuePtr)payload)->key = newkeyvalue.key;
    ((HTKeyValuePtr)payload)->value = newkeyvalue.value;
    ret = 2;
  }

  return ret;
}

int LookupHashTable(HashTable table,
                    HTKey_t key,
                    HTKeyValue *keyvalue) {
  Verify333(table != NULL);

  HWSize_t insertbucket = HashKeyToBucketNum(table, key);
  LinkedList insertchain = table->buckets[insertbucket];


  LLPayload_t payload;
  if (is_key_exist_delete(insertchain, key, &payload, false)) {
    keyvalue->key = ((HTKeyValuePtr)payload)->key;
    keyvalue->value = ((HTKeyValuePtr)payload)->value;
    return 1;
  }
  return 0;
}

int RemoveFromHashTable(HashTable table,
                        HTKey_t key,
                        HTKeyValue *keyvalue) {
  Verify333(table != NULL);

  HWSize_t insertbucket = HashKeyToBucketNum(table, key);
  LinkedList insertchain = table->buckets[insertbucket];

  LLPayload_t payload;
  if (is_key_exist_delete(insertchain, key, &payload, true)) {
    keyvalue->key = ((HTKeyValuePtr)payload)->key;
    keyvalue->value = ((HTKeyValuePtr)payload)->value;
    free(payload);
    table->num_elements--;
    return 1;
  }
  return 0;
}

HTIter HashTableMakeIterator(HashTable table) {
  HTIterRecord *iter;
  HWSize_t      i;

  Verify333(table != NULL);  // be defensive

  // malloc the iterator
  iter = (HTIterRecord *) malloc(sizeof(HTIterRecord));
  if (iter == NULL) {
    return NULL;
  }

  // if the hash table is empty, the iterator is immediately invalid,
  // since it can't point to anything.
  if (table->num_elements == 0) {
    iter->is_valid = false;
    iter->ht = table;
    iter->bucket_it = NULL;
    return iter;
  }

  // initialize the iterator.  there is at least one element in the
  // table, so find the first element and point the iterator at it.
  iter->is_valid = true;
  iter->ht = table;
  for (i = 0; i < table->num_buckets; i++) {
    if (NumElementsInLinkedList(table->buckets[i]) > 0) {
      iter->bucket_num = i;
      break;
    }
  }
  Verify333(i < table->num_buckets);  // make sure we found it.
  iter->bucket_it = LLMakeIterator(table->buckets[iter->bucket_num], 0UL);
  if (iter->bucket_it == NULL) {
    // out of memory!
    free(iter);
    return NULL;
  }
  return iter;
}

void HTIteratorFree(HTIter iter) {
  Verify333(iter != NULL);
  if (iter->bucket_it != NULL) {
    LLIteratorFree(iter->bucket_it);
    iter->bucket_it = NULL;
  }
  iter->is_valid = false;
  free(iter);
}

int HTIteratorNext(HTIter iter) {
  Verify333(iter != NULL);
  if (iter->bucket_it == NULL) {
    return 0;
  }
  int32_t ret = 0;
  if (LLIteratorHasNext(iter->bucket_it)) {
    Verify333(LLIteratorNext(iter->bucket_it) == true);
    ret = 1;
  } else {
    int i;
    for (i = iter->bucket_num+1; i < iter->ht->num_buckets; i++) {
      if (NumElementsInLinkedList(iter->ht->buckets[i]) > 0) {
        iter->bucket_num = i;
        break;
      }
    }
    LLIteratorFree(iter->bucket_it);
    iter->bucket_it = NULL;
    if (i < iter->ht->num_buckets) {
      iter->bucket_it = LLMakeIterator(iter->ht->buckets[iter->bucket_num],
        0UL);
      if (iter->bucket_it == NULL) {
        // out of memory!
        free(iter);
        return ret;
      }
      ret = 1;
    }
  }

  return ret;
}

int HTIteratorPastEnd(HTIter iter) {
  Verify333(iter != NULL);
  if (iter->bucket_it == NULL) {
    return 1;
  }
  if ((NumElementsInLinkedList(iter->ht->buckets[iter->bucket_num]) == 1) ||
    LLIteratorHasNext(iter->bucket_it) || (!LLIteratorHasNext(iter->bucket_it)
    && LLIteratorHasPrev(iter->bucket_it))) {
    return 0;
  }

  return 1;  // you might need to change this return value.
}

int HTIteratorGet(HTIter iter, HTKeyValue *keyvalue) {
  Verify333(iter != NULL);

  if (!iter->is_valid || iter->ht == NULL) {
    return 0;
  }
  LLPayload_t payload;
  LLIteratorGetPayload(iter->bucket_it, &payload);
  keyvalue->key = ((HTKeyValuePtr)payload)->key;
  keyvalue->value = ((HTKeyValuePtr)payload)->value;

  return 1;
}

int HTIteratorDelete(HTIter iter, HTKeyValue *keyvalue) {
  HTKeyValue kv;
  int res, retval;

  Verify333(iter != NULL);

  // Try to get what the iterator is pointing to.
  res = HTIteratorGet(iter, &kv);
  if (res == 0)
    return 0;

  // Advance the iterator.
  res = HTIteratorNext(iter);
  if (res == 0) {
    retval = 2;
  } else {
    retval = 1;
  }
  res = RemoveFromHashTable(iter->ht, kv.key, keyvalue);
  Verify333(res == 1);
  Verify333(kv.key == keyvalue->key);
  Verify333(kv.value == keyvalue->value);

  return retval;
}

static void ResizeHashtable(HashTable ht) {
  // Resize if the load factor is > 3.
  if (ht->num_elements < 3 * ht->num_buckets)
    return;

  // This is the resize case.  Allocate a new hashtable,
  // iterate over the old hashtable, do the surgery on
  // the old hashtable record and free up the new hashtable
  // record.
  HashTable newht = AllocateHashTable(ht->num_buckets * 9);

  // Give up if out of memory.
  if (newht == NULL)
    return;

  // Loop through the old ht with an iterator,
  // inserting into the new HT.
  HTIter it = HashTableMakeIterator(ht);
  if (it == NULL) {
    // Give up if out of memory.
    FreeHashTable(newht, &HTNullFree);
    return;
  }

  while (!HTIteratorPastEnd(it)) {
    HTKeyValue item, dummy;

    Verify333(HTIteratorGet(it, &item) == 1);
    if (InsertHashTable(newht, item, &dummy) != 1) {
      // failure, free up everything, return.
      HTIteratorFree(it);
      FreeHashTable(newht, &HTNullFree);
      return;
    }
    HTIteratorNext(it);
  }

  // Worked!  Free the iterator.
  HTIteratorFree(it);

  // Sneaky: swap the structures, then free the new table,
  // and we're done.
  {
    HashTableRecord tmp;

    tmp = *ht;
    *ht = *newht;
    *newht = tmp;
    FreeHashTable(newht, &HTNullFree);
  }

  return;
}
