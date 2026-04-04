#pragma once

#include <string>

class NameDemangler
{
public:
	std::string demangle( const std::string &mangledName );

private:
	std::string stripArguments( const std::string &fullName );
	std::string stripTrailingQualifiers( const std::string &name );
	std::string stripReturnType( const std::string &name );
};