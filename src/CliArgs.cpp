#include "CliArgs.h"
#include <algorithm>
#include <stdexcept>

std::string CliArgs::cleanKey( const std::string &key )
{
	size_t start = key.find_first_not_of( '-' );
	if( start != std::string::npos )
	{
		return key.substr( start );
	}
	return key;
}

CliArgs::CliArgs( int argc, char *argv[] )
{
	for( int i = 1; i < argc; ++i )
	{
		std::string arg = argv[i];
		if( !arg.empty() && arg[0] == '-' )
		{
			std::string key = cleanKey( arg );
			std::string value;
			size_t eqPos = arg.find( '=' );

			if( eqPos != std::string::npos )
			{
				key = cleanKey( arg.substr( 0, eqPos ) );
				value = arg.substr( eqPos + 1 );
			}
			else
			{
				if( i + 1 < argc && argv[i + 1][0] != '-' )
				{
					value = argv[i + 1];
					i++;
				}
			}
			m_args[key] = value;
		}
	}
}

bool CliArgs::has( const std::string &key ) const
{
	return m_args.find( key ) != m_args.end();
}

std::string CliArgs::get( const std::string &key, const std::string &defaultVal ) const
{
	auto it = m_args.find( key );
	if( it != m_args.end() )
	{
		return it->second;
	}
	return defaultVal;
}

int CliArgs::getInt( const std::string &key, int defaultVal ) const
{
	auto it = m_args.find( key );
	if( it != m_args.end() && !it->second.empty() )
	{
		try
		{
			return std::stoi( it->second );
		}
		catch( ... )
		{
			return defaultVal;
		}
	}
	return defaultVal;
}

float CliArgs::getFloat( const std::string &key, float defaultVal ) const
{
	auto it = m_args.find( key );
	if( it != m_args.end() && !it->second.empty() )
	{
		try
		{
			return std::stof( it->second );
		}
		catch( ... )
		{
			return defaultVal;
		}
	}
	return defaultVal;
}

bool CliArgs::getBool( const std::string &key, bool defaultVal ) const
{
	auto it = m_args.find( key );
	if( it != m_args.end() )
	{
		std::string val = it->second;
		if( val.empty() )
		{
			return true;
		}

		std::transform( val.begin(), val.end(), val.begin(), ::tolower );
		if( val == "true" || val == "1" || val == "yes" )
		{
			return true;
		}
		if( val == "false" || val == "0" || val == "no" )
		{
			return false;
		}
	}
	return defaultVal;
}