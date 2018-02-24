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

#include <stdint.h>     // for uint32_t, etc.
#include <string>       // for std::string.
#include <sstream>      // for std::stringstream.

#include "./fileindexutil.h"   // for FileDup().

extern "C" {
  #include "libhw1/HashTable.h"  // for libhw1/hashtable.h's FNVHash64().
  #include "libhw1/CSE333.h"
}

#include "./IndexTableReader.h"

namespace hw3 {

// The constructor for IndexTableReader calls the constructor of
// HashTableReader(), its superclass. The superclass takes care of
// taking ownership of f and using it to extract and cache the number
// of buckets within the table.
IndexTableReader::IndexTableReader(FILE *f, IndexFileOffset_t offset)
  : HashTableReader(f, offset) { }

DocIDTableReader *IndexTableReader::LookupWord(const std::string &word) {
  // Calculate the FNVHash64 of the word.  Use word.c_str() to get a
  // C-style (char *) to pass to FNVHash64, and word.lengt() to figure
  // out how many characters are in the string.
  HTKey_t wordhash = FNVHash64((unsigned char *) word.c_str(),
                                word.length());

  // Get back the list of "element" offsets for this word hash.
  auto elements = LookupElementPositions(wordhash);

  // If the list is empty, we're done; return nullptr.
  if (elements.size() == 0)
    return nullptr;

  // Iterate through the elements.
  for (auto it = elements.begin(); it != elements.end(); it++) {
    IndexFileOffset_t next_offset = *it;

    // Slurp the header information out of the "element" field;
    // specifically, extract the "word length" field and the "docID
    // table length" fields, converting from network to host order.
    worddocset_header header;
    // MISSING:


    // If the "word length" field doesn't match the length of the word
    // we're looking up, use continue to skip to the next element.
    if (header.word_len != word.length())
      continue;

    // We might have a match for the word. Read the word itself, using
    // the "<<" operator to feed a std::stringstream characters read
    // using fread().
    std::stringstream ss;
    for (int i = 0; i < header.word_len; i++) {
      // MISSING:
    }

    // Use ss.str() to extract a std::string from the stringstream,
    // and use the std::string's "compare()" method to see if the word
    // we read from the "element" matches our "word" parameter.
    if (word.compare(ss.str()) == 0) {
      // If it matches, use "new" to heap-allocate and manufacture a
      // DocIDTableReader.  Be sure to use FileDup() to pass a
      // duplicated (FILE *) as the first argument to the
      // DocIDTableReader constructor, since we want the manufactured
      // DocIDTableReader to have its own (FILE *) handle
      //
      // return the new'd (DocIDTableReader *) to the caller.
      IndexFileOffset_t docidtableoffset = next_offset + 6 + header.word_len;
      DocIDTableReader *ditr = new DocIDTableReader(FileDup(file_),
                                                    docidtableoffset);
      return ditr;
    }
  }
  return nullptr;
}

}  // namespace hw3
