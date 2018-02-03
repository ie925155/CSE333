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

// Feature test macro for strtok_r (c.f., Linux Programming Interface p. 63)
#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <limits.h>

#include "libhw1/CSE333.h"
#include "memindex.h"
#include "filecrawler.h"

typedef struct {
  int32_t query_count;
  char **queries;
} QueriesInfo;

extern bool is_stop_words(HashTable tab, const char* word);
static void Usage(void);
static QueriesInfo parsingArgument(char str[], HashTable tab);

static QueriesInfo parsingArgument(char str[], HashTable tab) {
  char *delim = " ";
  char *pch;
  char *saveptr;
  QueriesInfo queriesInfo;
  char tmpStr[MAX_INPUT];
  strncpy(tmpStr, str, MAX_INPUT);
  int str_count = 0, idx = 0;
  pch = strtok_r(tmpStr, delim, &saveptr);
  while (pch != NULL) {
    str_count++;
    pch = strtok_r(NULL, delim, &saveptr);
  }
  queriesInfo.query_count = str_count;
  queriesInfo.queries = (char **) malloc(sizeof(char *) * str_count);
  pch = strtok_r(str, delim, &saveptr);
  while (pch != NULL) {
    if (tab == NULL || !is_stop_words(tab, pch)) {
      queriesInfo.queries[idx++] = pch;
    }
    pch = strtok_r(NULL, delim, &saveptr);
  }
  queriesInfo.query_count = idx;
  return queriesInfo;
}

int main(int argc, char **argv) {
  if (argc < 2)
    Usage();
  char *rootdir = argv[1], c;
  bool stop_words = false;

  while ((c = getopt(argc, argv, "s")) != EOF) {
    switch (c) {
      case 's':
        stop_words = true;
        rootdir = argv[2];
        break;
      default:
        Usage();
    }
  }

  int res;
  DocTable dt;
  MemIndex idx;
  HashTable stopwordtab = NULL;
  // Implement searchshell!  We're giving you very few hints
  // on how to do it, so you'll need to figure out an appropriate
  // decomposition into functions as well as implementing the
  // functions.  There are several major tasks you need to build:
  //
  //  - crawl from a directory provided by argv[1] to produce and index
  //  - prompt the user for a query and read the query from stdin, in a loop
  //  - split a query into words (check out strtok_r)
  //  - process a query against the index and print out the results
  //
  // When searchshell detects end-of-file on stdin (cntrl-D from the
  // keyboard), searchshell should free all dynamically allocated
  // memory and any other allocated resources and then exit.
  Verify333(CrawlFileTree(rootdir, &dt, &idx, &stopwordtab, stop_words) == 1);
  char str[256] = {0x0};
  QueriesInfo queriesInfo;
  LinkedList llres;
  LLIter llit;
  SearchResult *sr;
  char *docname;
  while (1) {
    fprintf(stdout, "%s\n", "enter query:");
    fgets(str, sizeof(str), stdin);
    str[strlen(str)-1] = '\0';
    queriesInfo = parsingArgument(str, stopwordtab);
    llres = MIProcessQuery(idx, queriesInfo.queries, queriesInfo.query_count);
    if (llres == NULL) {
      free(queriesInfo.queries);
      continue;
    }
    llit  = LLMakeIterator(llres, 0);
    while (LLIteratorHasNext(llit)) {
      LLIteratorGetPayload(llit, (LLPayload_t*) &sr);
      docname = DTLookupDocID(dt, sr->docid);
      fprintf(stdout, "%s (%u)\n", docname, sr->rank);
      LLIteratorNext(llit);
    }
    LLIteratorGetPayload(llit, (LLPayload_t*) &sr);
    docname = DTLookupDocID(dt, sr->docid);
    fprintf(stdout, "%s (%u)\n", docname, sr->rank);
    LLIteratorFree(llit);
    FreeLinkedList(llres, (LLPayloadFreeFnPtr)free);
    free(queriesInfo.queries);
  }
  return EXIT_SUCCESS;
}

static void Usage(void) {
  fprintf(stderr, "Usage: ./searchshell <docroot>\n");
  fprintf(stderr,
          "where <docroot> is an absolute or relative " \
          "path to a directory to build an index under.\n");
  exit(-1);
}
