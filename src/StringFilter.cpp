#include "StringFilter.h"

void StringFilter::addExact( const std::string &str )
{
	m_exactMatches.insert( str );
}

void StringFilter::addPartial( const std::string &str )
{
	m_partialMatches.push_back( str );
}

bool StringFilter::match( const std::string &input ) const
{
	if( m_exactMatches.find( input ) != m_exactMatches.end() )
	{
		return true;
	}

	for( const auto &substr : m_partialMatches )
	{
		if( input.find( substr ) != std::string::npos )
		{
			return true;
		}
	}

	return false;
}