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

#include <stdint.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <map>
#include <string>
#include <vector>
#include <iostream>

#include "./HttpRequest.h"
#include "./HttpUtils.h"
#include "./HttpConnection.h"

using std::vector;
using std::map;
using std::string;

#define BUFFER_SIZE 1024

namespace hw4 {

bool HttpConnection::GetNextRequest(HttpRequest *request) {
  // Use "WrappedRead" to read data into the buffer_
  // instance variable.  Keep reading data until either the
  // connection drops or you see a "\r\n\r\n" that demarcates
  // the end of the request header.
  //
  // Once you've seen the request header, use ParseRequest()
  // to parse the header into the *request argument.
  //
  // Very tricky part:  clients can send back-to-back requests
  // on the same socket.  So, you need to preserve everything
  // after the "\r\n\r\n" in buffer_ for the next time the
  // caller invokes GetNextRequest()!

  size_t index;
  unsigned char buffer[BUFFER_SIZE]= {0x00};
  while((index = buffer_.find("\r\n\r\n")) == std::string::npos) {
    WrappedRead(fd_, buffer, BUFFER_SIZE);
    string tmp((const char*)buffer);
    buffer_ += tmp;
  }
  *request = ParseRequest(index);

  return true;
}

bool HttpConnection::WriteResponse(const HttpResponse &response) {
  std::string str = response.GenerateResponseString();
  int res = WrappedWrite(fd_,
                         (unsigned char *) str.c_str(),
                         str.length());
  if (res != static_cast<int>(str.length()))
    return false;
  return true;
}

HttpRequest HttpConnection::ParseRequest(size_t end) {
  HttpRequest req;
  req.URI = "/";  // by default, get "/".

  // Get the header.
  std::string str = buffer_.substr(0, end);

  // Split the header into lines.  Extract the URI from the first line
  // and store it in req.URI.  For each additional line beyond the
  // first, extract out the header name and value and store them in
  // req.headers (i.e., req.headers[headername] = headervalue).
  // You should look at HttpResponse.h for details about the HTTP header
  // format that you need to parse.
  //
  // You'll probably want to look up boost functions for (a) splitting
  // a string into lines on a "\r\n" delimiter, (b) trimming
  // whitespace from the end of a string, and (c) converting a string
  // to lowercase.
  vector<string> strs;
  boost::split(strs, str, boost::is_any_of("\r\n"), boost::token_compress_on);
  string line;
  for(size_t i = 0; i < strs.size(); i++) {
    line = strs[i];
    std::transform(line.begin(), line.end(), line.begin(), ::tolower);
    if (i == 0) {
      size_t found = line.find("/");
      size_t uri_len = line.find(" ", found) - found;
      req.URI = line.substr(found, uri_len);
    } else {
      size_t found = line.find(" ");
      string key = line.substr(0, found-1);
      string value = line.substr(found+1, line.length());
      req.headers[key] = value;
    }
  }
  // set to next request
  buffer_.erase(0, end+4);
  return req;
}

}  // namespace hw4
