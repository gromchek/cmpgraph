#pragma once

#include <string>
#include <vector>
#include <unordered_set>

class StringFilter
{
private:
	std::unordered_set<std::string> m_exactMatches;
	std::vector<std::string> m_partialMatches;

public:
	void addExact( const std::string &str );
	void addPartial( const std::string &str );
	bool match( const std::string &input ) const;
};