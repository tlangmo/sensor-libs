// ===========================================================
//
// Copyright (c) 2018 Motesque Inc.  All rights reserved.
//
// ===========================================================
#include "http_utils.h"
namespace motesque {
std::map<int, std::string> StatusCodes::codes =
{
{ 200, "OK" },
{ 201, "Created" },
{ 204, "No Content" },
{ 206, "Partial Content" },
{ 301, "Moved Permanently" },
{ 400, "Bad Request" },
{ 401, "Unauthorized" },
{ 403, "Forbidden" },
{ 404, "Not Found" },
{ 413, "Request Entity Too Large" },
{ 415, "Unsupported Media Type" },
{ 500, "Internal Server Error" },
{ 501, "Not Implemented" },
{ 503, "Service Unavailable" },
{ 408, "Request Timeout" } };

std::string to_string(int i)
{
   char buf[34];
   memset(buf,0,sizeof(buf));
   sprintf(buf,"%d",i);
   return std::string(buf);
}

std::string to_string(size_t i)
{
   char buf[34];
   memset(buf,0,sizeof(buf));
   sprintf(buf,"%zu",i);
   return std::string(buf);
}

}; // end ns
std::string& operator<<(std::string& lh, const int& in) {
    lh += motesque::to_string(in);
    return lh;
}

std::string& operator<<(std::string& lh, const std::string& in) {
    lh += in;
    return lh;
}




