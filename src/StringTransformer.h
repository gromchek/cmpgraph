#pragma once

#include <string>
#include <vector>
#include <functional>

using TransformRule = std::function<std::string( const std::string & )>;

class StringTransformer
{
private:
	std::vector<TransformRule> rules;

public:
	void addRule( TransformRule rule );
	void addRule( const std::string &from, const std::string &to );
	void addRegexRule( const std::string &pattern, const std::string &fmt );
	std::string transform( std::string input ) const;
};