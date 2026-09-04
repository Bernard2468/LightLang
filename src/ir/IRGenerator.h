#ifndef IR_GENERATOR_H
#define IR_GENERATOR_H

#include <string>
#include <unordered_map>
#include <vector>
#include "frontend/AST.h"
#include "ir/IntermediateCode.h"

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

    // Jump targets of the innermost enclosing loop. 'break' goes to breakLabel
    // and 'continue' to continueLabel, which for a 'for' loop is the increment
    // rather than the condition, so the loop still advances.
    struct LoopLabels
    {
        string breakLabel;
        string continueLabel;
    };
    vector<LoopLabels> loopLabels;

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
    void generateFor(const ForStmt* statement);
    void generateBreak(const BreakStmt* statement);
    void generateContinue(const ContinueStmt* statement);
    void generateFunction(const FunctionStmt* statement);

public:
    IRGenerator();
    IntermediateCode generate(const Program& program);
};

#endif
