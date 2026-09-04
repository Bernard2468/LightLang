#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include <string>
#include <unordered_map>
#include <vector>
#include "AST.h"
#include "SymbolTable.h"
#include "Types.h"

using namespace std;

struct FunctionSymbol
{
    string name;
    ValueType returnType;
    vector<ValueType> parameterTypes;
    vector<string> parameterNames;
    int declarationLine;
};

class SemanticAnalyzer
{
private:
    SymbolTable symbolTable;
    vector<string> errors;
    unordered_map<string, FunctionSymbol> functions;
    vector<FunctionSymbol> functionDeclarations;
    string currentFunctionName;
    ValueType currentFunctionReturnType;

    void reportError(int line, const string& message);
    ValueType typeFromName(const string& typeName) const;
    ValueType typeFromLiteral(TokenType literalType) const;
    bool isNumeric(ValueType type) const;
    bool isAssignable(ValueType destination, ValueType source) const;
    ValueType commonNumericType(ValueType left, ValueType right) const;

    void collectFunctionSignatures(const Program& program);
    void analyzeFunction(const FunctionStmt* statement);
    bool statementAlwaysReturns(const Stmt* statement) const;

    void analyzeStatement(const Stmt* statement);
    void analyzeVariableDeclaration(const VarDeclStmt* statement);
    void analyzeAssignment(const AssignmentStmt* statement);
    void analyzePrint(const PrintStmt* statement);
    void analyzeReturn(const ReturnStmt* statement);
    void analyzeBlock(const BlockStmt* statement);
    void analyzeIf(const IfStmt* statement);
    void analyzeWhile(const WhileStmt* statement);

    ValueType analyzeExpression(const Expr* expression);
    ValueType analyzeLiteral(const LiteralExpr* expression);
    ValueType analyzeVariable(const VariableExpr* expression);
    ValueType analyzeCall(const CallExpr* expression);
    ValueType analyzeUnary(const UnaryExpr* expression);
    ValueType analyzeBinary(const BinaryExpr* expression);
    ValueType analyzeGrouping(const GroupingExpr* expression);

public:
    SemanticAnalyzer();

    bool analyze(const Program& program);
    const vector<string>& getErrors() const;
    const SymbolTable& getSymbolTable() const;
    const vector<FunctionSymbol>& getFunctionDeclarations() const;
};

#endif
