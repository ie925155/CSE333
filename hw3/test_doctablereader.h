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

#ifndef _HW3_TEST_DOCTABLEREADER_H_
#define _HW3_TEST_DOCTABLEREADER_H_

#include <unistd.h>
#include <iostream>
#include <list>
#include "gtest/gtest.h"

#include "./filelayout.h"
#include "./DocTableReader.h"
#include "./fileindexwriter.h"

namespace hw3 {

class Test_DocTableReader : public ::testing::Test {
 protected:
  Test_DocTableReader() {
    // Do set-up work for each test here.
  }

  virtual ~Test_DocTableReader() {
    // Do clean-up work for each test here.
  }

  virtual void SetUp() {
    // Code here will be called after the constructor and
    // right before each test.
  }

  virtual void TearDown() {
    // Code here will be called after each test and
    // right before the destructor.
  }

  static void SetUpTestCase() {
    // Code here will be called once for the entire
    // text fixture.  Note it is a static member function
    // (i.e., a class method, not an object instance method).
  }

  static void TearDownTestCase() {
    // Code here will be called once for the entire
    // text fixture.  Note it is a static member function
    // (i.e., a class method, not an object instance method).
  }

  // These relay is so that the unit test can access the
  // protected member functions of DocTableReader.
  std::list<IndexFileOffset_t> LookupElementPositions(DocTableReader *dtr,
                                             DocID_t hashval) {
    return dtr->LookupElementPositions(hashval);
  }

  // Objects declared here can be used by all tests in
  // the test case.
};  // class Test_DocTableReader

}  // namespace hw3

#endif  // _HW3_TEST_DOCTABLEREADER_H_
