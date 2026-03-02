#ifndef CLIARGS_H
#define CLIARGS_H

#include <string>
#include <map>

class CliArgs
{
private:
	std::map<std::string, std::string> m_args;
	std::string cleanKey( const std::string &key );

public:
	CliArgs( int argc, char *argv[] );
	bool has( const std::string &key ) const;
	std::string get( const std::string &key, const std::string &defaultVal = "" ) const;
	int getInt( const std::string &key, int defaultVal = 0 ) const;
	float getFloat( const std::string &key, float defaultVal = 0.0f ) const;
	bool getBool( const std::string &key, bool defaultVal = false ) const;
};

#endif