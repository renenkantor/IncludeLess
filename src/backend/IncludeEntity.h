#ifndef INCLUDE_ENTITY_H
#define INCLUDE_ENTITY_H 

#include <string>
#include <regex>

class IncludeEntity
{
public:
    bool              m_is_useless;
    const std::smatch m_match;
    const std::string m_str;
    const int line;
    IncludeEntity(bool useless, const std::smatch &i_match, const int line) : m_is_useless(useless), m_match(i_match), m_str(i_match.str()), line(line) {}
};

#endif
