/*
 * Copyright 2012 Steven Gribble
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
#include <string.h>

#include "doctable.h"
#include "libhw1/CSE333.h"
#include "libhw1/HashTable.h"

void ValueFree(HTValue_t value);

void ValueFree(HTValue_t value) {
  Verify333(value != NULL);
  free(value);
}

// This structure represents a DocTable; it contains two hash tables, one
// mapping from docid to docname, and one mapping from docname to docid.
typedef struct doctablerecord_st {
  HashTable docid_to_docname;  // mapping docid to docname
  HashTable docname_to_docid;  // mapping docname to docid
  DocID_t max_id;              // max docID allocated so far
} DocTableRecord;

DocTable AllocateDocTable(void) {
  DocTableRecord *dt = (DocTableRecord *) malloc(sizeof(DocTableRecord));
  Verify333(dt != NULL);

  dt->docid_to_docname = AllocateHashTable(1024);
  dt->docname_to_docid = AllocateHashTable(1024);
  dt->max_id = 0;

  Verify333(dt->docid_to_docname != NULL);
  Verify333(dt->docname_to_docid != NULL);
  return dt;
}

void FreeDocTable(DocTable table) {
  Verify333(table != NULL);

  // STEP 1.
  FreeHashTable(table->docid_to_docname, &ValueFree);
  FreeHashTable(table->docname_to_docid, &ValueFree);
  free(table);
}

HWSize_t DTNumDocsInDocTable(DocTable table) {
  Verify333(table != NULL);
  return NumElementsInHashTable(table->docid_to_docname);
}

DocID_t DTRegisterDocumentName(DocTable table, char *docname) {
  // Allocate space for the docid.
  char       *doccopy;
  DocID_t   *docid = (DocID_t *) malloc(sizeof(DocID_t));
  DocID_t   res;
  HTKeyValue kv, oldkv;

  // Make a copy of the docname.
  doccopy = (char *) malloc(1+strlen(docname));
  if (doccopy != NULL) {
    strncpy(doccopy, docname, 1+strlen(docname));
  }
  Verify333(table != NULL);
  Verify333(doccopy != NULL);
  Verify333(docid != NULL);

  // Check to see if the document already exists; if so,
  // free up the malloc'ed space and return the existing docid

  // STEP 2.
  if ((res = DTLookupDocumentName(table, docname)) != 0) {
    free(doccopy);
    free(docid);
    return res;
  };

  // allocate the next docID
  table->max_id += 1;
  res = *docid = table->max_id;

  // STEP 3.
  // Set up the key/value for the docid_to_docname mapping, and
  // do the insert.
  kv.key = *docid;
  kv.value = (HTValue_t) doccopy;
  Verify333(InsertHashTable(table->docid_to_docname, kv, &oldkv) != 0);

  // STEP 4.
  // Set up the key/value for the docname_to_docid mapping, and
  // do the insert.
  kv.key = FNVHash64((unsigned char *)docname, strlen(docname));
  kv.value = (HTValue_t) docid;
  Verify333(InsertHashTable(table->docname_to_docid, kv, &oldkv) != 0);

  return res;
}

DocID_t DTLookupDocumentName(DocTable table, char *docname) {
  HTKey_t key;
  HTKeyValue kv;
  int res;

  Verify333(table != NULL);
  Verify333(docname != NULL);

  // STEP 5.
  // Lookup the FNVHash64() or the docname in the
  // docname_to_docid table within dt, and return
  // either "0" if the docname isn't found or the
  // docID if it is.
  key = FNVHash64((unsigned char *)docname, strlen(docname));
  res = LookupHashTable(table->docname_to_docid, key, &kv);
  return (res == 1) ? (*(DocID_t *)kv.value) : 0;
}

char *DTLookupDocID(DocTable table, DocID_t docid) {
  HTKeyValue kv;
  int res;

  Verify333(table != NULL);
  Verify333(docid != 0);

  // STEP 6.
  // Lookup the docid in the docid_to_docname table,
  // and either return the string (i.e., the (char *)
  // saved in the value field for that key) or
  // NULL if the key isn't in the table.
  res = LookupHashTable(table->docid_to_docname, docid, &kv);
  return (res == 1) ? (char *)kv.value : NULL;
}

HashTable DTGetDocidTable(DocTable table) {
  Verify333(table != NULL);
  return table->docid_to_docname;
}
