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
#include <string>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <memory>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

extern "C" {
  #include "libhw2/fileparser.h"
}

#include "./HttpUtils.h"
#include "./FileReader.h"

namespace hw4 {

bool FileReader::ReadFile(std::string *str) {
  std::string fullfile = basedir_ + "/" + fname_;

  // Read the file into memory, and store the file contents in the
  // output parameter "str."  Be careful to handle binary data
  // correctly; i.e., you probably want to use the two-argument
  // constructor to std::string (the one that includes a length as a
  // second argument).
  //
  // You might find ::ReadFile() from HW2's fileparser.h useful
  // here.  Be careful, though; remember that it uses malloc to
  // allocate memory, so you'll need to use free() to free up that
  // memory.  Alternatively, you can use a unique_ptr with a malloc/free
  // deleter to automatically manage this for you; see the comment in
  // HttpUtils.h above the MallocDeleter class for details.

  int fd;
  size_t left_to_read;
  ssize_t numread;
  struct stat filestat;
  stat(fullfile.c_str(), &filestat);
  if (!S_ISREG(filestat.st_mode)) {
    return false;
  }

  fd = open(fullfile.c_str(), O_RDONLY);
  if (fd == -1) {
    fprintf(stderr, "%s err=%d\n", __func__, errno);
    return false;
  }
  std::unique_ptr<char, MallocDeleter<char>> buf((char*)malloc(filestat.st_size+1));

  left_to_read = filestat.st_size;
  numread = 0;
  while (left_to_read > 0) {
    numread = read(fd, buf.get()+filestat.st_size-left_to_read, left_to_read);
    if (numread == -1) {
      fprintf(stderr, "%s read file content error %d\n", __func__, errno);
      close(fd);
      return false;
    } else if (numread == 0) {
      break;
    }
    left_to_read -= numread;
  }
  close(fd);
  buf.get()[filestat.st_size] = '\0';

  *str = buf.get();
  return true;
}

}  // namespace hw4
