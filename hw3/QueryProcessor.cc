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

#include <iostream>
#include <algorithm>
#include <map>

#include "./QueryProcessor.h"

extern "C" {
  #include "./libhw1/CSE333.h"
}

namespace hw3 {

QueryProcessor::QueryProcessor(list<string> indexlist, bool validate) {
  // Stash away a copy of the index list.
  indexlist_ = indexlist;
  arraylen_ = indexlist_.size();
  Verify333(arraylen_ > 0);

  // Create the arrays of DocTableReader*'s. and IndexTableReader*'s.
  dtr_array_ = new DocTableReader *[arraylen_];
  itr_array_ = new IndexTableReader *[arraylen_];

  // Populate the arrays with heap-allocated DocTableReader and
  // IndexTableReader object instances.
  list<string>::iterator idx_iterator = indexlist_.begin();
  for (HWSize_t i = 0; i < arraylen_; i++) {
    FileIndexReader fir(*idx_iterator, validate);
    dtr_array_[i] = new DocTableReader(fir.GetDocTableReader());
    itr_array_[i] = new IndexTableReader(fir.GetIndexTableReader());
    idx_iterator++;
  }
}

QueryProcessor::~QueryProcessor() {
  // Delete the heap-allocated DocTableReader and IndexTableReader
  // object instances.
  Verify333(dtr_array_ != nullptr);
  Verify333(itr_array_ != nullptr);
  for (HWSize_t i = 0; i < arraylen_; i++) {
    delete dtr_array_[i];
    delete itr_array_[i];
  }

  // Delete the arrays of DocTableReader*'s and IndexTableReader*'s.
  delete[] dtr_array_;
  delete[] itr_array_;
  dtr_array_ = nullptr;
  itr_array_ = nullptr;
}

vector<QueryProcessor::QueryResult>
QueryProcessor::ProcessQuery(const vector<string> &query) {
  Verify333(query.size() > 0);
  vector<QueryProcessor::QueryResult> finalresult;

  for (HWSize_t i = 0; i < arraylen_; i++) {
    std::map<DocID_t, QueryProcessor::QueryResult> map;
    for (size_t j = 0; j < query.size(); j++) {
      string data = query[j];
      transform(data.begin(), data.end(), data.begin(),::tolower);
      DocIDTableReader *ditr = itr_array_[i]->LookupWord(data);
      if (ditr == nullptr) {
        map.clear();
        break;
      }
      if (j == 0) {
        list<docid_element_header> docidList = ditr->GetDocIDList();
        for (auto it = docidList.begin(); it != docidList.end(); ++it) {
          docid_element_header header = *it;
          string file_name;
          bool ret = dtr_array_[i]->LookupDocID(header.docid, &file_name);
          if (ret) {
            QueryResult result;
            result.document_name = file_name;
            result.rank = header.num_positions;
            map[header.docid] = result;
          }
        }
      } else {
        for(auto it = map.begin(); it != map.end(); ) {
          DocID_t docid = it->first;
          std::list<DocPositionOffset_t> matchlist;
          if (ditr->LookupDocID(docid, &matchlist)) {
            it->second.rank += matchlist.size();
            ++it;
          } else {
            it = map.erase(it);
          }
        }
      }
      delete ditr;
    } // end of queries
    for(auto it = map.begin(); it != map.end(); it++) {
      finalresult.push_back(it->second);
    }
  }

  // Sort the final results.
  std::sort(finalresult.begin(), finalresult.end());
  return finalresult;
}

}  // namespace hw3
