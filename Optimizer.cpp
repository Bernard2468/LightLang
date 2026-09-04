#include "Optimizer.h"
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <unordered_set>

using namespace std;

Optimizer::Optimizer()
    : valueTypes(), constants()
{
}

bool Optimizer::isTemporary(const string& name) const
{
    return name.size() >= 3 && name[0] == '%' && name[1] == 't';
}

bool Optimizer::isLiteral(const string& operand) const
{
    ConstantValue value;
    return parseConstant(operand, value);
}

string Optimizer::serializeString(const string& value) const
{
    string result = "\"";

    for (char character : value)
    {
        switch (character)
        {
            case '\\': result += "\\\\"; break;
            case '"': result += "\\\""; break;
            case '\n': result += "\\n"; break;
            case '\t': result += "\\t"; break;
            case '\r': result += "\\r"; break;
            default: result += character; break;
        }
    }

    result += "\"";
    return result;
}

bool Optimizer::deserializeString(const string& text, string& value) const
{
    if (text.size() < 2 || text.front() != '"' || text.back() != '"')
    {
        return false;
    }

    value.clear();

    for (size_t index = 1; index + 1 < text.size(); index++)
    {
        char character = text[index];

        if (character != '\\')
        {
            value += character;
            continue;
        }

        if (index + 1 >= text.size() - 1)
        {
            return false;
        }

        index++;
        char escaped = text[index];

        switch (escaped)
        {
            case '\\': value += '\\'; break;
            case '"': value += '"'; break;
            case 'n': value += '\n'; break;
            case 't': value += '\t'; break;
            case 'r': value += '\r'; break;
            default:
                value += '\\';
                value += escaped;
                break;
        }
    }

    return true;
}

string Optimizer::formatFloat(double value) const
{
    ostringstream output;
    output << setprecision(15) << value;
    string text = output.str();

    if (text.find('.') == string::npos &&
        text.find('e') == string::npos &&
        text.find('E') == string::npos)
    {
        text += ".0";
    }

    return text;
}

bool Optimizer::parseConstant(const string& text, ConstantValue& value) const
{
    if (text == "true" || text == "false")
    {
        value = {ValueType::BOOL, text};
        return true;
    }

    string stringValue;

    if (deserializeString(text, stringValue))
    {
        value = {ValueType::STRING, serializeString(stringValue)};
        return true;
    }

    if (text.empty())
    {
        return false;
    }

    size_t start = 0;

    if (text[0] == '+' || text[0] == '-')
    {
        start = 1;
    }

    if (start >= text.size())
    {
        return false;
    }

    bool hasDigit = false;
    bool hasDot = false;
    bool hasExponent = false;

    for (size_t index = start; index < text.size(); index++)
    {
        char character = text[index];

        if (character >= '0' && character <= '9')
        {
            hasDigit = true;
            continue;
        }

        if (character == '.' && !hasDot && !hasExponent)
        {
            hasDot = true;
            continue;
        }

        if ((character == 'e' || character == 'E') &&
            !hasExponent && hasDigit)
        {
            hasExponent = true;
            hasDigit = false;

            if (index + 1 < text.size() &&
                (text[index + 1] == '+' || text[index + 1] == '-'))
            {
                index++;
            }

            continue;
        }

        return false;
    }

    if (!hasDigit)
    {
        return false;
    }

    try
    {
        if (hasDot || hasExponent)
        {
            size_t consumed = 0;
            double number = stod(text, &consumed);

            if (consumed != text.size() || !isfinite(number))
            {
                return false;
            }

            value = {ValueType::FLOAT, formatFloat(number)};
            return true;
        }

        size_t consumed = 0;
        long long number = stoll(text, &consumed);

        if (consumed != text.size())
        {
            return false;
        }

        value = {ValueType::INT, to_string(number)};
        return true;
    }
    catch (...)
    {
        return false;
    }
}

ValueType Optimizer::typeOfOperand(const string& operand) const
{
    auto found = valueTypes.find(operand);

    if (found != valueTypes.end())
    {
        return found->second;
    }

    ConstantValue constant;

    if (parseConstant(operand, constant))
    {
        return constant.type;
    }

    return ValueType::ERROR;
}

ValueType Optimizer::inferUnaryType(const string& operation,
                                    const string& operand) const
{
    if (operation == "!")
    {
        return ValueType::BOOL;
    }

    if (operation == "+" || operation == "-")
    {
        return typeOfOperand(operand);
    }

    return ValueType::ERROR;
}

ValueType Optimizer::inferBinaryType(const string& operation,
                                     const string& left,
                                     const string& right) const
{
    ValueType leftType = typeOfOperand(left);
    ValueType rightType = typeOfOperand(right);

    if (operation == "&&" || operation == "||" ||
        operation == "==" || operation == "!=" ||
        operation == ">" || operation == ">=" ||
        operation == "<" || operation == "<=")
    {
        return ValueType::BOOL;
    }

    if (operation == "%")
    {
        return ValueType::INT;
    }

    if (operation == "+" &&
        leftType == ValueType::STRING && rightType == ValueType::STRING)
    {
        return ValueType::STRING;
    }

    if (leftType == ValueType::FLOAT || rightType == ValueType::FLOAT)
    {
        return ValueType::FLOAT;
    }

    if (leftType == ValueType::INT && rightType == ValueType::INT)
    {
        return ValueType::INT;
    }

    return ValueType::ERROR;
}

string Optimizer::resolveOperand(const string& operand) const
{
    auto found = constants.find(operand);

    if (found != constants.end())
    {
        return found->second.text;
    }

    return operand;
}

bool Optimizer::getConstant(const string& operand, ConstantValue& value) const
{
    auto found = constants.find(operand);

    if (found != constants.end())
    {
        value = found->second;
        return true;
    }

    return parseConstant(operand, value);
}

Optimizer::ConstantValue Optimizer::convertForDestination(
    const ConstantValue& value,
    ValueType destinationType) const
{
    if (destinationType == ValueType::FLOAT && value.type == ValueType::INT)
    {
        try
        {
            double converted = stod(value.text);
            return {ValueType::FLOAT, formatFloat(converted)};
        }
        catch (...)
        {
            return value;
        }
    }

    return value;
}

bool Optimizer::foldUnary(const string& operation,
                          const ConstantValue& operand,
                          ConstantValue& result) const
{
    if (operation == "!" && operand.type == ValueType::BOOL)
    {
        result = {ValueType::BOOL,
                  operand.text == "true" ? "false" : "true"};
        return true;
    }

    if (operation == "+" &&
        (operand.type == ValueType::INT || operand.type == ValueType::FLOAT))
    {
        result = operand;
        return true;
    }

    if (operation != "-" ||
        (operand.type != ValueType::INT && operand.type != ValueType::FLOAT))
    {
        return false;
    }

    try
    {
        if (operand.type == ValueType::INT)
        {
            long long number = stoll(operand.text);

            if (number == numeric_limits<long long>::min())
            {
                return false;
            }

            result = {ValueType::INT, to_string(-number)};
            return true;
        }

        double number = stod(operand.text);
        double folded = -number;

        if (!isfinite(folded))
        {
            return false;
        }

        result = {ValueType::FLOAT, formatFloat(folded)};
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool Optimizer::foldBinary(const string& operation,
                           const ConstantValue& left,
                           const ConstantValue& right,
                           ConstantValue& result) const
{
    if (operation == "+" &&
        left.type == ValueType::STRING && right.type == ValueType::STRING)
    {
        string leftValue;
        string rightValue;

        if (!deserializeString(left.text, leftValue) ||
            !deserializeString(right.text, rightValue))
        {
            return false;
        }

        result = {ValueType::STRING, serializeString(leftValue + rightValue)};
        return true;
    }

    if ((operation == "&&" || operation == "||") &&
        left.type == ValueType::BOOL && right.type == ValueType::BOOL)
    {
        bool leftValue = left.text == "true";
        bool rightValue = right.text == "true";
        bool folded = operation == "&&"
                          ? leftValue && rightValue
                          : leftValue || rightValue;
        result = {ValueType::BOOL, folded ? "true" : "false"};
        return true;
    }

    if ((operation == "==" || operation == "!=") &&
        left.type == ValueType::BOOL && right.type == ValueType::BOOL)
    {
        bool equal = left.text == right.text;
        bool folded = operation == "==" ? equal : !equal;
        result = {ValueType::BOOL, folded ? "true" : "false"};
        return true;
    }

    if ((operation == "==" || operation == "!=") &&
        left.type == ValueType::STRING && right.type == ValueType::STRING)
    {
        string leftValue;
        string rightValue;

        if (!deserializeString(left.text, leftValue) ||
            !deserializeString(right.text, rightValue))
        {
            return false;
        }

        bool equal = leftValue == rightValue;
        bool folded = operation == "==" ? equal : !equal;
        result = {ValueType::BOOL, folded ? "true" : "false"};
        return true;
    }

    bool leftNumeric = left.type == ValueType::INT || left.type == ValueType::FLOAT;
    bool rightNumeric = right.type == ValueType::INT || right.type == ValueType::FLOAT;

    if (!leftNumeric || !rightNumeric)
    {
        return false;
    }

    bool floating = left.type == ValueType::FLOAT || right.type == ValueType::FLOAT;

    try
    {
        if (!floating &&
            (operation == "+" || operation == "-" || operation == "*" ||
             operation == "/" || operation == "%"))
        {
            long long a = stoll(left.text);
            long long b = stoll(right.text);
            long long folded = 0;
            long long minimum = numeric_limits<long long>::min();
            long long maximum = numeric_limits<long long>::max();

            if (operation == "+")
            {
                if ((b > 0 && a > maximum - b) ||
                    (b < 0 && a < minimum - b))
                {
                    return false;
                }
                folded = a + b;
            }
            else if (operation == "-")
            {
                if ((b < 0 && a > maximum + b) ||
                    (b > 0 && a < minimum + b))
                {
                    return false;
                }
                folded = a - b;
            }
            else if (operation == "*")
            {
                if (a != 0 && b != 0)
                {
                    if (a == -1 && b == minimum)
                    {
                        return false;
                    }
                    if (b == -1 && a == minimum)
                    {
                        return false;
                    }

                    if (a > 0)
                    {
                        if ((b > 0 && a > maximum / b) ||
                            (b < 0 && b < minimum / a))
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if ((b > 0 && a < minimum / b) ||
                            (b < 0 && a < maximum / b))
                        {
                            return false;
                        }
                    }
                }
                folded = a * b;
            }
            else if (operation == "/")
            {
                if (b == 0 || (a == minimum && b == -1))
                {
                    return false;
                }
                folded = a / b;
            }
            else
            {
                if (b == 0 || (a == minimum && b == -1))
                {
                    return false;
                }
                folded = a % b;
            }

            result = {ValueType::INT, to_string(folded)};
            return true;
        }

        double a = stod(left.text);
        double b = stod(right.text);

        if (operation == "+" || operation == "-" ||
            operation == "*" || operation == "/")
        {
            if (operation == "/" && b == 0.0)
            {
                return false;
            }

            double folded = 0.0;

            if (operation == "+") folded = a + b;
            if (operation == "-") folded = a - b;
            if (operation == "*") folded = a * b;
            if (operation == "/") folded = a / b;

            if (!isfinite(folded))
            {
                return false;
            }

            result = {ValueType::FLOAT, formatFloat(folded)};
            return true;
        }

        bool folded = false;

        if (operation == ">") folded = a > b;
        else if (operation == ">=") folded = a >= b;
        else if (operation == "<") folded = a < b;
        else if (operation == "<=") folded = a <= b;
        else if (operation == "==") folded = a == b;
        else if (operation == "!=") folded = a != b;
        else return false;

        result = {ValueType::BOOL, folded ? "true" : "false"};
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool Optimizer::simplifyBinary(const string& operation,
                               const string& left,
                               const string& right,
                               ValueType resultType,
                               string& replacement) const
{
    ConstantValue leftConstant;
    ConstantValue rightConstant;
    bool leftIsConstant = getConstant(left, leftConstant);
    bool rightIsConstant = getConstant(right, rightConstant);

    auto numericZero = [](const ConstantValue& value)
    {
        return (value.type == ValueType::INT && value.text == "0") ||
               (value.type == ValueType::FLOAT && stod(value.text) == 0.0);
    };

    auto numericOne = [](const ConstantValue& value)
    {
        return (value.type == ValueType::INT && value.text == "1") ||
               (value.type == ValueType::FLOAT && stod(value.text) == 1.0);
    };

    if (operation == "+")
    {
        if (rightIsConstant && numericZero(rightConstant) &&
            resultType != ValueType::STRING)
        {
            replacement = left;
            return true;
        }

        if (leftIsConstant && numericZero(leftConstant) &&
            resultType != ValueType::STRING)
        {
            replacement = right;
            return true;
        }
    }

    if (operation == "-" && rightIsConstant && numericZero(rightConstant))
    {
        replacement = left;
        return true;
    }

    if (operation == "*")
    {
        if (rightIsConstant && numericOne(rightConstant))
        {
            replacement = left;
            return true;
        }

        if (leftIsConstant && numericOne(leftConstant))
        {
            replacement = right;
            return true;
        }

        if ((rightIsConstant && numericZero(rightConstant)) ||
            (leftIsConstant && numericZero(leftConstant)))
        {
            replacement = resultType == ValueType::FLOAT ? "0.0" : "0";
            return true;
        }
    }

    if (operation == "/" && rightIsConstant && numericOne(rightConstant))
    {
        replacement = left;
        return true;
    }

    if (operation == "%" && rightIsConstant &&
        rightConstant.type == ValueType::INT && rightConstant.text == "1")
    {
        replacement = "0";
        return true;
    }

    if (operation == "&&")
    {
        if (rightIsConstant && rightConstant.type == ValueType::BOOL &&
            rightConstant.text == "true")
        {
            replacement = left;
            return true;
        }

        if (leftIsConstant && leftConstant.type == ValueType::BOOL &&
            leftConstant.text == "true")
        {
            replacement = right;
            return true;
        }

        if ((rightIsConstant && rightConstant.type == ValueType::BOOL &&
             rightConstant.text == "false") ||
            (leftIsConstant && leftConstant.type == ValueType::BOOL &&
             leftConstant.text == "false"))
        {
            replacement = "false";
            return true;
        }
    }

    if (operation == "||")
    {
        if (rightIsConstant && rightConstant.type == ValueType::BOOL &&
            rightConstant.text == "false")
        {
            replacement = left;
            return true;
        }

        if (leftIsConstant && leftConstant.type == ValueType::BOOL &&
            leftConstant.text == "false")
        {
            replacement = right;
            return true;
        }

        if ((rightIsConstant && rightConstant.type == ValueType::BOOL &&
             rightConstant.text == "true") ||
            (leftIsConstant && leftConstant.type == ValueType::BOOL &&
             leftConstant.text == "true"))
        {
            replacement = "true";
            return true;
        }
    }

    return false;
}

IntermediateCode Optimizer::propagateFoldAndSimplify(const IntermediateCode& input)
{
    valueTypes.clear();
    constants.clear();
    IntermediateCode output;

    for (const IRInstruction& original : input.getInstructions())
    {
        IRInstruction instruction = original;

        if (instruction.opcode == IROpCode::DECLARE)
        {
            ValueType type = ValueType::ERROR;

            if (instruction.dataType == "int") type = ValueType::INT;
            else if (instruction.dataType == "float") type = ValueType::FLOAT;
            else if (instruction.dataType == "bool") type = ValueType::BOOL;
            else if (instruction.dataType == "string") type = ValueType::STRING;

            valueTypes[instruction.destination] = type;
            constants.erase(instruction.destination);
            output.emit(instruction);
            continue;
        }

        if (instruction.opcode == IROpCode::ASSIGN)
        {
            instruction.operand1 = resolveOperand(instruction.operand1);
            ConstantValue constant;

            if (getConstant(instruction.operand1, constant))
            {
                ValueType destinationType = typeOfOperand(instruction.destination);
                ConstantValue converted = convertForDestination(constant,
                                                                 destinationType);
                instruction.operand1 = converted.text;
                constants[instruction.destination] = converted;

                if (isTemporary(instruction.destination))
                {
                    valueTypes[instruction.destination] = converted.type;
                }
            }
            else
            {
                constants.erase(instruction.destination);

                if (isTemporary(instruction.destination))
                {
                    valueTypes[instruction.destination] = typeOfOperand(instruction.operand1);
                }
            }

            output.emit(instruction);
            continue;
        }

        if (instruction.opcode == IROpCode::UNARY)
        {
            instruction.operand1 = resolveOperand(instruction.operand1);
            ValueType resultType = inferUnaryType(instruction.operation,
                                                  instruction.operand1);
            valueTypes[instruction.destination] = resultType;

            ConstantValue operand;
            ConstantValue folded;

            if (getConstant(instruction.operand1, operand) &&
                foldUnary(instruction.operation, operand, folded))
            {
                output.emit({IROpCode::ASSIGN,
                             instruction.destination,
                             folded.text,
                             "",
                             "",
                             "",
                             "",
                             instruction.sourceLine});
                constants[instruction.destination] = folded;
            }
            else if (instruction.operation == "+")
            {
                output.emit({IROpCode::ASSIGN,
                             instruction.destination,
                             instruction.operand1,
                             "",
                             "",
                             "",
                             "",
                             instruction.sourceLine});
                constants.erase(instruction.destination);
            }
            else
            {
                constants.erase(instruction.destination);
                output.emit(instruction);
            }

            continue;
        }

        if (instruction.opcode == IROpCode::BINARY)
        {
            instruction.operand1 = resolveOperand(instruction.operand1);
            instruction.operand2 = resolveOperand(instruction.operand2);
            ValueType resultType = inferBinaryType(instruction.operation,
                                                   instruction.operand1,
                                                   instruction.operand2);
            valueTypes[instruction.destination] = resultType;

            ConstantValue left;
            ConstantValue right;
            ConstantValue folded;
            string replacement;

            if (getConstant(instruction.operand1, left) &&
                getConstant(instruction.operand2, right) &&
                foldBinary(instruction.operation, left, right, folded))
            {
                output.emit({IROpCode::ASSIGN,
                             instruction.destination,
                             folded.text,
                             "",
                             "",
                             "",
                             "",
                             instruction.sourceLine});
                constants[instruction.destination] = folded;
            }
            else if (simplifyBinary(instruction.operation,
                                    instruction.operand1,
                                    instruction.operand2,
                                    resultType,
                                    replacement))
            {
                ConstantValue replacementConstant;
                string resolvedReplacement = resolveOperand(replacement);

                output.emit({IROpCode::ASSIGN,
                             instruction.destination,
                             resolvedReplacement,
                             "",
                             "",
                             "",
                             "",
                             instruction.sourceLine});

                if (getConstant(resolvedReplacement, replacementConstant))
                {
                    ConstantValue converted = convertForDestination(replacementConstant,
                                                                     resultType);
                    constants[instruction.destination] = converted;
                }
                else
                {
                    constants.erase(instruction.destination);
                }
            }
            else
            {
                constants.erase(instruction.destination);
                output.emit(instruction);
            }

            continue;
        }

        if (instruction.opcode == IROpCode::PARAM)
        {
            ValueType type = ValueType::ERROR;
            if (instruction.dataType == "int") type = ValueType::INT;
            else if (instruction.dataType == "float") type = ValueType::FLOAT;
            else if (instruction.dataType == "bool") type = ValueType::BOOL;
            else if (instruction.dataType == "string") type = ValueType::STRING;
            valueTypes[instruction.destination] = type;
            constants.erase(instruction.destination);
            output.emit(instruction);
            continue;
        }

        if (instruction.opcode == IROpCode::ARG)
        {
            instruction.operand1 = resolveOperand(instruction.operand1);
            output.emit(instruction);
            continue;
        }

        if (instruction.opcode == IROpCode::CALL)
        {
            constants.clear();
            ValueType type = ValueType::ERROR;
            if (instruction.dataType == "int") type = ValueType::INT;
            else if (instruction.dataType == "float") type = ValueType::FLOAT;
            else if (instruction.dataType == "bool") type = ValueType::BOOL;
            else if (instruction.dataType == "string") type = ValueType::STRING;
            valueTypes[instruction.destination] = type;
            constants.erase(instruction.destination);
            output.emit(instruction);
            continue;
        }

        if (instruction.opcode == IROpCode::RETURN)
        {
            instruction.operand1 = resolveOperand(instruction.operand1);
            output.emit(instruction);
            constants.clear();
            continue;
        }

        if (instruction.opcode == IROpCode::FUNCTION_BEGIN ||
            instruction.opcode == IROpCode::FUNCTION_END)
        {
            constants.clear();
            output.emit(instruction);
            continue;
        }

        if (instruction.opcode == IROpCode::PRINT)
        {
            instruction.operand1 = resolveOperand(instruction.operand1);
            output.emit(instruction);
            continue;
        }

        if (instruction.opcode == IROpCode::IF_FALSE_GOTO)
        {
            instruction.operand1 = resolveOperand(instruction.operand1);
            ConstantValue condition;

            if (getConstant(instruction.operand1, condition) &&
                condition.type == ValueType::BOOL)
            {
                if (condition.text == "false")
                {
                    output.emit({IROpCode::GOTO,
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 instruction.label,
                                 instruction.sourceLine});
                }
            }
            else
            {
                output.emit(instruction);
            }

            constants.clear();
            continue;
        }

        if (instruction.opcode == IROpCode::GOTO ||
            instruction.opcode == IROpCode::LABEL)
        {
            constants.clear();
            output.emit(instruction);
            continue;
        }

        output.emit(instruction);
    }

    return output;
}

IntermediateCode Optimizer::removeUnreferencedLabels(const IntermediateCode& input) const
{
    unordered_set<string> referencedLabels;

    for (const IRInstruction& instruction : input.getInstructions())
    {
        if (instruction.opcode == IROpCode::GOTO ||
            instruction.opcode == IROpCode::IF_FALSE_GOTO)
        {
            referencedLabels.insert(instruction.label);
        }
    }

    IntermediateCode output;

    for (const IRInstruction& instruction : input.getInstructions())
    {
        if (instruction.opcode == IROpCode::LABEL &&
            referencedLabels.find(instruction.label) == referencedLabels.end())
        {
            continue;
        }

        output.emit(instruction);
    }

    return output;
}

IntermediateCode Optimizer::removeUnreachableCode(const IntermediateCode& input) const
{
    IntermediateCode output;
    bool unreachable = false;

    for (const IRInstruction& instruction : input.getInstructions())
    {
        if (unreachable)
        {
            if (instruction.opcode == IROpCode::LABEL)
            {
                unreachable = false;
                output.emit(instruction);
            }
            continue;
        }

        output.emit(instruction);

        if (instruction.opcode == IROpCode::GOTO)
        {
            unreachable = true;
        }
    }

    return output;
}

IntermediateCode Optimizer::removeRedundantGotos(const IntermediateCode& input) const
{
    const vector<IRInstruction>& instructions = input.getInstructions();
    IntermediateCode output;

    for (size_t index = 0; index < instructions.size(); index++)
    {
        const IRInstruction& instruction = instructions[index];

        if (instruction.opcode == IROpCode::GOTO &&
            index + 1 < instructions.size() &&
            instructions[index + 1].opcode == IROpCode::LABEL &&
            instruction.label == instructions[index + 1].label)
        {
            continue;
        }

        output.emit(instruction);
    }

    return output;
}

IntermediateCode Optimizer::removeDeadConstantTemporaries(
    const IntermediateCode& input) const
{
    vector<IRInstruction> instructions = input.getInstructions();
    bool changed = true;

    while (changed)
    {
        changed = false;
        unordered_set<string> used;

        for (const IRInstruction& instruction : instructions)
        {
            if (!instruction.operand1.empty() && !isLiteral(instruction.operand1))
            {
                used.insert(instruction.operand1);
            }

            if (!instruction.operand2.empty() && !isLiteral(instruction.operand2))
            {
                used.insert(instruction.operand2);
            }
        }

        vector<IRInstruction> next;

        for (const IRInstruction& instruction : instructions)
        {
            bool removable = instruction.opcode == IROpCode::ASSIGN &&
                             isTemporary(instruction.destination) &&
                             used.find(instruction.destination) == used.end() &&
                             isLiteral(instruction.operand1);

            if (removable)
            {
                changed = true;
                continue;
            }

            next.push_back(instruction);
        }

        instructions = next;
    }

    IntermediateCode output;

    for (const IRInstruction& instruction : instructions)
    {
        output.emit(instruction);
    }

    return output;
}

IntermediateCode Optimizer::optimize(const IntermediateCode& input)
{
    IntermediateCode optimized = propagateFoldAndSimplify(input);
    optimized = removeUnreferencedLabels(optimized);
    optimized = removeUnreachableCode(optimized);
    optimized = removeRedundantGotos(optimized);
    optimized = removeUnreferencedLabels(optimized);
    optimized = removeDeadConstantTemporaries(optimized);
    return optimized;
}
