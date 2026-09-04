#include "Types.h"

using namespace std;

string valueTypeToString(ValueType type)
{
    switch (type)
    {
        case ValueType::INT:
            return "int";
        case ValueType::FLOAT:
            return "float";
        case ValueType::BOOL:
            return "bool";
        case ValueType::STRING:
            return "string";
        case ValueType::ERROR:
            return "<error>";
    }

    return "<error>";
}
