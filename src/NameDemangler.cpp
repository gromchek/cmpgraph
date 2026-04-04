#include "NameDemangler.h"
#include <cxxabi.h>
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <cstring>

std::string NameDemangler::demangle( const std::string &mangledName )
{
	int status = 0;
	char *demangled = abi::__cxa_demangle( mangledName.c_str(), nullptr, nullptr, &status );

	if( status != 0 || !demangled )
	{
		return mangledName;
	}

	std::string result( demangled );
	free( demangled );

	size_t pos;
	while( ( pos = result.find( " >" ) ) != std::string::npos )
	{
		result.erase( pos, 1 );
	}

	result = stripArguments( result );

	result = stripTrailingQualifiers( result );

	result = stripReturnType( result );

	return result;
}

std::string NameDemangler::stripTrailingQualifiers( const std::string &name )
{
	std::string result = name;

	const char *qualifiers[] = { "const", "volatile", "&", "&&" };

	bool changed = true;
	while( changed )
	{
		changed = false;
		while( !result.empty() && isspace( result.back() ) )
		{
			result.pop_back();
			changed = true;
		}

		for( const char *q : qualifiers )
		{
			size_t len = strlen( q );
			if( result.size() >= len && result.compare( result.size() - len, len, q ) == 0 )
			{
				result.erase( result.size() - len );
				changed = true;
			}
		}
	}

	return result;
}

std::string NameDemangler::stripArguments( const std::string &fullName )
{
	int paren_depth = 0;
	int angle_depth = 0;
	int bracket_depth = 0;

	for( size_t i = fullName.length(); i-- > 0; )
	{
		char c = fullName[i];

		if( c == ')' )
		{
			paren_depth++;
		}
		else if( c == '(' )
		{
			if( paren_depth > 0 )
			{
				paren_depth--;
			}

			if( paren_depth == 0 && angle_depth == 0 && bracket_depth == 0 )
			{
				if( i >= 8 && fullName.compare( i - 8, 8, "operator" ) == 0 )
				{
					continue;
				}

				return fullName.substr( 0, i );
			}
		}
		else if( c == '>' )
		{
			angle_depth++;
		}
		else if( c == '<' )
		{
			if( angle_depth > 0 )
			{
				angle_depth--;
			}
		}
		else if( c == ']' )
		{
			bracket_depth++;
		}
		else if( c == '[' )
		{
			if( bracket_depth > 0 )
			{
				bracket_depth--;
			}
		}
	}

	return fullName;
}

std::string NameDemangler::stripReturnType( const std::string &name )
{
	int templateDepth = 0;

	for( int i = name.length() - 1; i >= 0; --i )
	{
		if( name[i] == '>' )
		{
			templateDepth++;
		}
		else if( name[i] == '<' )
		{
			templateDepth--;
		}
		else if( name[i] == ' ' && templateDepth == 0 )
		{
			return name.substr( i + 1 );
		}
	}

	return name;
}