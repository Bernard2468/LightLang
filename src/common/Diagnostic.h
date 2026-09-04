#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H

#include <string>
#include <vector>

using namespace std;

// Renders source excerpts for compiler diagnostics.
//
// Given the original source text, produces an excerpt such as:
//
//        3 | string z = x + y;
//          |                ^
//
// so that an error message can point at the exact offending token instead of
// naming a line number alone.
class SourceContext
{
private:
    vector<string> lines;

public:
    explicit SourceContext(const string& sourceCode);

    bool hasLine(int line) const;

    // Returns the text of a 1-based line, or an empty string when out of range.
    string lineText(int line) const;

    // Returns a multi-line excerpt for a 1-based line and column. A column of
    // zero (or out of range) omits the caret row and shows the line alone.
    // Returns an empty string when the line does not exist, so callers can
    // simply print the result unconditionally.
    string excerpt(int line, int column) const;
};

#endif
