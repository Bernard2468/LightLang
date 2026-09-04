#ifndef RUNTIME_VALUE_H
#define RUNTIME_VALUE_H

#include <string>
#include <variant>
#include "common/Types.h"

using namespace std;

class RuntimeValue
{
private:
    ValueType type;
    variant<long long, double, bool, string> value;

public:
    RuntimeValue();
    explicit RuntimeValue(long long value);
    explicit RuntimeValue(double value);
    explicit RuntimeValue(bool value);
    explicit RuntimeValue(const string& value);

    ValueType getType() const;
    long long asInt() const;
    double asFloat() const;
    bool asBool() const;
    const string& asString() const;
    bool isNumeric() const;
    double asNumber() const;
    string toString() const;
};

#endif
