#ifndef TYPES_H
#define TYPES_H

#include <string>

using namespace std;

enum class ValueType
{
    INT,
    FLOAT,
    BOOL,
    STRING,
    ERROR
};

string valueTypeToString(ValueType type);

#endif
