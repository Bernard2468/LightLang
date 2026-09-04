#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include <string>
#include <unordered_map>
#include "IntermediateCode.h"
#include "Types.h"

using namespace std;

class Optimizer
{
private:
    struct ConstantValue
    {
        ValueType type;
        string text;
    };

    unordered_map<string, ValueType> valueTypes;
    unordered_map<string, ConstantValue> constants;

    bool isTemporary(const string& name) const;
    bool isLiteral(const string& operand) const;
    bool parseConstant(const string& text, ConstantValue& value) const;
    string serializeString(const string& value) const;
    bool deserializeString(const string& text, string& value) const;
    string formatFloat(double value) const;

    ValueType typeOfOperand(const string& operand) const;
    ValueType inferUnaryType(const string& operation,
                             const string& operand) const;
    ValueType inferBinaryType(const string& operation,
                              const string& left,
                              const string& right) const;

    string resolveOperand(const string& operand) const;
    bool getConstant(const string& operand, ConstantValue& value) const;
    ConstantValue convertForDestination(const ConstantValue& value,
                                        ValueType destinationType) const;

    bool foldUnary(const string& operation,
                   const ConstantValue& operand,
                   ConstantValue& result) const;
    bool foldBinary(const string& operation,
                    const ConstantValue& left,
                    const ConstantValue& right,
                    ConstantValue& result) const;

    bool simplifyBinary(const string& operation,
                        const string& left,
                        const string& right,
                        ValueType resultType,
                        string& replacement) const;

    IntermediateCode propagateFoldAndSimplify(const IntermediateCode& input);
    IntermediateCode removeUnreferencedLabels(const IntermediateCode& input) const;
    IntermediateCode removeUnreachableCode(const IntermediateCode& input) const;
    IntermediateCode removeRedundantGotos(const IntermediateCode& input) const;
    IntermediateCode removeDeadConstantTemporaries(const IntermediateCode& input) const;

public:
    Optimizer();
    IntermediateCode optimize(const IntermediateCode& input);
};

#endif
