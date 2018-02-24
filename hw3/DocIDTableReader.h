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

#ifndef _HW3_DOCIDTABLEREADER_H_
#define _HW3_DOCIDTABLEREADER_H_

#include <list>      // for std::list
#include <cstdio>    // for (FILE *)

#include "./HashTableReader.h"

using std::list;

namespace hw3 {

// A DocIDTableReader (a subclass of HashTableReader) is used to
// read one of the many the embedded docid-->positions "docIDtable"
// tables within the index file.
class DocIDTableReader : protected HashTableReader {
  // This friend declaration is here so that the Test_DocIDTableReader
  // unit test fixture can access protected member variables of
  // DocIDTableReader.  See test_docidtablereader.h for details.
  friend class Test_DocIDTableReader;

 public:
  // Construct a new DocIDTableReader at a specific offset with an
  // index file.  Arguments:
  //
  // - f: an open (FILE *) for the underlying index file.  The
  //   constructed object takes ownership of the (FILE *) and will
  //   fclose() it  on destruction.
  //
  // - offset: the "docIDtable"'s byte offset within the file.
  DocIDTableReader(FILE *f, IndexFileOffset_t offset);
  ~DocIDTableReader() { }

  // Lookup a docid and get back a std::list<DocPositionOffset_t>
  // containing the positions listed for that docid.
  //
  // Arguments:
  //
  // - docid:  the docID to look for within the docIDtable.
  //
  // - ret_tr: the std::list<DocPositionOffset_t> containing the positions of
  //   the word in the document (an output parameter). Nothing is
  //   returned through this if the docID is not found.
  //
  // Returns:
  //
  // - true if the docID is found, false otherwise.
  bool LookupDocID(const DocID_t &docid, list<DocPositionOffset_t> *ret_list);

  // Returns a list of docid_element structs where
  // the num_positions element is the number of matches
  // the word has within that docID for this docIDtable.
  list<docid_element_header> GetDocIDList();
};

}  // namespace hw3

#endif  // _HW3_DOCIDTABLEREADER_H_
