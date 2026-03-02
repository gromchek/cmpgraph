#include "StringTransformer.h"
#include <regex>

void StringTransformer::addRule( TransformRule rule )
{
	rules.push_back( std::move( rule ) );
}

void StringTransformer::addRule( const std::string &from, const std::string &to )
{
	rules.push_back( [from = std::move( from ), to = std::move( to )]( const std::string &input ) {
		std::string result = input;
		size_t pos = 0;
		while( ( pos = result.find( from, pos ) ) != std::string::npos )
		{
			result.replace( pos, from.size(), to );
			pos += to.size();
		}
		return result;
	} );
}

void StringTransformer::addRegexRule( const std::string &pattern, const std::string &fmt )
{
	rules.push_back( [pattern, fmt]( const std::string &input ) {
		return std::regex_replace( input, std::regex( pattern ), fmt );
	} );
}

std::string StringTransformer::transform( std::string input ) const
{
	std::string current = std::move( input );
	for( const auto &rule : rules )
	{
		current = rule( current );
	}
	return current;
}