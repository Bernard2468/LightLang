#ifndef IR_GENERATOR_H
#define IR_GENERATOR_H

#include <string>
#include <unordered_map>
#include <vector>
#include "AST.h"
#include "IntermediateCode.h"

using namespace std;

class IRGenerator
{
private:
    IntermediateCode code;
    int temporaryCounter;
    int labelCounter;
    unordered_map<string, int> variableCounters;
    vector<unordered_map<string, string>> scopes;
    unordered_map<string, string> functionReturnTypes;
    string currentFunctionReturnType;

    string newTemporary();
    string newLabel();
    void enterScope();
    void exitScope();
    string declareVariable(const string& sourceName);
    string resolveVariable(const string& sourceName) const;

    string quoteString(const string& value) const;
    string generateLiteral(const LiteralExpr* expression) const;
    string generateExpression(const Expr* expression);

    void generateStatement(const Stmt* statement);
    void generateVariableDeclaration(const VarDeclStmt* statement);
    void generateAssignment(const AssignmentStmt* statement);
    void generatePrint(const PrintStmt* statement);
    void generateReturn(const ReturnStmt* statement);
    void generateBlock(const BlockStmt* statement);
    void generateIf(const IfStmt* statement);
    void generateWhile(const WhileStmt* statement);
    void generateFunction(const FunctionStmt* statement);

public:
    IRGenerator();
    IntermediateCode generate(const Program& program);
};

#endif
