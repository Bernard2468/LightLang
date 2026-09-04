#include "common/Diagnostic.h"

#include <sstream>

using namespace std;

SourceContext::SourceContext(const string& sourceCode)
{
    string current;

    for (char character : sourceCode)
    {
        if (character == '\n')
        {
            lines.push_back(current);
            current.clear();
            continue;
        }

        // Tolerate CRLF sources so the caret is not pushed off by a stray \r.
        if (character != '\r')
        {
            current += character;
        }
    }

    lines.push_back(current);
}

bool SourceContext::hasLine(int line) const
{
    return line >= 1 && static_cast<size_t>(line) <= lines.size();
}

string SourceContext::lineText(int line) const
{
    if (!hasLine(line))
    {
        return "";
    }

    return lines[static_cast<size_t>(line) - 1];
}

string SourceContext::excerpt(int line, int column) const
{
    if (!hasLine(line))
    {
        return "";
    }

    const string& text = lines[static_cast<size_t>(line) - 1];
    string number = to_string(line);
    string gutter(number.length(), ' ');

    stringstream output;
    output << "    " << number << " | " << text << "\n";

    if (column < 1 || static_cast<size_t>(column) > text.length() + 1)
    {
        // No usable column: show the line without a caret rather than
        // pointing somewhere misleading.
        return output.str();
    }

    output << "    " << gutter << " | ";

    // Copy the leading whitespace verbatim so that a tab-indented line keeps
    // the caret aligned under the token once the terminal expands the tabs.
    for (size_t index = 0; index + 1 < static_cast<size_t>(column); index++)
    {
        output << (text[index] == '\t' ? '\t' : ' ');
    }

    output << "^";
    return output.str();
}
