#include "RuntimeValue.h"
#include <iomanip>
#include <sstream>
#include <stdexcept>

using namespace std;

RuntimeValue::RuntimeValue()
    : type(ValueType::INT), value(0LL)
{
}

RuntimeValue::RuntimeValue(long long value)
    : type(ValueType::INT), value(value)
{
}

RuntimeValue::RuntimeValue(double value)
    : type(ValueType::FLOAT), value(value)
{
}

RuntimeValue::RuntimeValue(bool value)
    : type(ValueType::BOOL), value(value)
{
}

RuntimeValue::RuntimeValue(const string& value)
    : type(ValueType::STRING), value(value)
{
}

ValueType RuntimeValue::getType() const
{
    return type;
}

long long RuntimeValue::asInt() const
{
    if (type != ValueType::INT)
    {
        throw runtime_error("Runtime value is not an int.");
    }

    return get<long long>(value);
}

double RuntimeValue::asFloat() const
{
    if (type != ValueType::FLOAT)
    {
        throw runtime_error("Runtime value is not a float.");
    }

    return get<double>(value);
}

bool RuntimeValue::asBool() const
{
    if (type != ValueType::BOOL)
    {
        throw runtime_error("Runtime value is not a bool.");
    }

    return get<bool>(value);
}

const string& RuntimeValue::asString() const
{
    if (type != ValueType::STRING)
    {
        throw runtime_error("Runtime value is not a string.");
    }

    return get<string>(value);
}

bool RuntimeValue::isNumeric() const
{
    return type == ValueType::INT || type == ValueType::FLOAT;
}

double RuntimeValue::asNumber() const
{
    if (type == ValueType::INT)
    {
        return static_cast<double>(asInt());
    }

    if (type == ValueType::FLOAT)
    {
        return asFloat();
    }

    throw runtime_error("Runtime value is not numeric.");
}

string RuntimeValue::toString() const
{
    if (type == ValueType::INT)
    {
        return to_string(asInt());
    }

    if (type == ValueType::FLOAT)
    {
        ostringstream output;
        output << setprecision(15) << asFloat();
        string text = output.str();

        if (text.find('.') == string::npos &&
            text.find('e') == string::npos &&
            text.find('E') == string::npos)
        {
            text += ".0";
        }

        return text;
    }

    if (type == ValueType::BOOL)
    {
        return asBool() ? "true" : "false";
    }

    if (type == ValueType::STRING)
    {
        return asString();
    }

    return "<error>";
}
