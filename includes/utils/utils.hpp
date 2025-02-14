#ifndef UTILS_H
#define UTILS_H

#include <sys/stat.h>
#include <dirent.h>

#include <string>


unsigned long stringToUnsignedLong(const std::string& str);

std::string NumberToString(int number);

bool isDirectory(const std::string &path);

std::string get_http_date();

#endif