#include <iostream>
#include "json.hpp"
#include "CliArgs.h"
#include <fstream>
#include "StringTransformer.h"
#include "FunctionMatcher.h"
#include "StringFilter.h"
#include "InlineAwareMatcher.h"

StringTransformer transformer;
StringFilter filter;

using json = nlohmann::json;

json load_json( const std::string &filepath )
{
	std::cout << "[LOG] Loading JSON file: " << filepath << "..." << std::endl;
	std::ifstream file( filepath );
	if( !file.is_open() )
	{
		throw std::runtime_error( "File not found: " + filepath );
	}
	json data;
	file >> data;
	if( !data.is_array() && !data.is_object() )
	{
		throw std::runtime_error( "JSON root must be an array or object: " + filepath );
	}
	return data;
}

std::string removePrefix( const std::string &str )
{
	if( str.rfind( "FUN_", 0 ) == 0 )
	{
		return str.substr( 4 );
	}
	return str;
}

std::vector<Function> load_functions_from_json( const json &data )
{
	std::vector<Function> functions;

	json functions_array = data;
	if( data.is_object() && data.contains( "functions" ) )
	{
		functions_array = data["functions"];
	}

	functions.reserve( functions_array.size() );

	for( const auto &item : functions_array )
	{
		Function func;
		func.baseName = item.value( "name", "" );

		if( filter.match( func.baseName ) )
		{
			continue;
		}

		func.name = transformer.transform( func.baseName );

		if( item.contains( "called_names" ) )
		{
			for( const auto &called : item["called_names"] )
			{
				const auto name = transformer.transform( called.get<std::string>() );
				if( filter.match( name ) )
				{
					continue;
				}
				func.callees.push_back( name );
			}
		}

		if( item.contains( "caller_names" ) )
		{
			for( const auto &caller : item["caller_names"] )
			{
				const auto name = transformer.transform( caller.get<std::string>() );
				if( filter.match( name ) )
				{
					continue;
				}
				func.callers.push_back( name );
			}
		}

		functions.push_back( std::move( func ) );
	}

	return functions;
}

int main( int argc, char *argv[] )
{
	filter.addPartial( "std::" );
	filter.addPartial( "CGReforge" );
	filter.addPartial( "blz::" );

	transformer.addRule( "__jump_table::_strcasecmp", "SStrCmp" );
	transformer.addRule( "__jump_table::strchr", "SStrChr" );
	transformer.addRule( "__jump_table::strcpy", "SStrCopy" );
	transformer.addRule( "__jump_table::strlen", "SStrLen" );
	transformer.addRule( "__jump_table::strncasecmp", "SStrCmpI" );
	transformer.addRule( "__jump_table::strrchr", "SStrChrR" );
	transformer.addRule( "__jump_table::strstr", "SStrStr" );
	transformer.addRule( "__jump_table::", "" );
	transformer.addRule( "_SErr", "SErr" );
	transformer.addRule( "SE3", "SE2" );
	transformer.addRule( "SI3", "SI2" );
	transformer.addRule( "CWorldMap", "CMap" );
	transformer.addRule( "WowClientDB2", "WowClientDB" );
	transformer.addRule( "::", "__" );
	transformer.addRegexRule( R"(\b(struct|class)_)", "" );
	transformer.addRegexRule( R"((\w+)<([^,>]+)[^>]*>)", "$1__$2" );
	transformer.addRegexRule( R"((\w+)(.*?)(::|__)\1$)", "$1$2$3constructor" );
	transformer.addRegexRule( R"((\w+)(.*?)(::|__)~\1$)", "$1$2$3destructor" );
	transformer.addRegexRule( R"((\w+)<([^<>,]+)[^<>]*>)", "$1__$2" );
	transformer.addRegexRule( R"((\w+)<([^<>,]+)[^<>]*>)", "$1__$2" );

	transformer.addRule( []( const std::string &input ) {
		static const std::vector<std::pair<std::string, std::string>> op_map = {
			{ "()", "call" },	{ "*=", "mul_assign" }, { "+=", "add_on_assign" },
			{ "++", "inc" },	{ "<=", "le" },			{ "==", "eq" },
			{ ">=", "ge" },		{ "!=", "ne" },			{ "<<", "lshift" },
			{ ">>", "rshift" }, { "-", "sub" },			{ "*", "mul" },
			{ "+", "add" },		{ "<", "lt" },			{ ">", "gt" },
			{ "=", "assign" }
		};

		const std::string keyword = "operator";
		size_t pos = input.find( keyword );

		if( pos == std::string::npos )
		{
			return input;
		}

		std::string suffix = input.substr( pos + keyword.length() );

		for( const auto &[op, name] : op_map )
		{
			if( !suffix.rfind( op, 0 ) )
			{
				return input.substr( 0, pos + keyword.length() ) + "_" + name;
			}
		}

		return input;
	} );

	CliArgs args( argc, argv );
	try
	{
		auto pathJSON335 = args.get( "base" );
		auto pathJSONRef = args.get( "ref" );
		auto threshold = args.getFloat( "t", 0.8f );
		auto output = args.get( "o", "result.txt" );
		auto checkInline = args.getBool( "checkInline", false );
		std::cout << "=== Function Matcher Started ===" << std::endl;

		auto g1_json = load_json( pathJSONRef );
		auto g2_json = load_json( pathJSON335 );

		std::cout << "[LOG] Parsing JSON structures..." << std::endl;
		auto g1 = load_functions_from_json( g1_json );
		auto g2 = load_functions_from_json( g2_json );
		std::cout << "[LOG] Graphs loaded. G1 size: " << g1.size() << ", G2 size: " << g2.size() << std::endl;

		FunctionMatcher matcher( g1, g2, threshold );
		auto result = matcher.runFullMatch();

		if( checkInline )
		{
			InlineAwareMatcher iam( g1, g2, 3, threshold );
			auto extraMatches = iam.run( result );

			result.insert( extraMatches.begin(), extraMatches.end() );
		}

		std::cout << "\n=== Matching Results ===" << std::endl;
		std::cout << "Total pairs matched: " << result.size() << std::endl;


		std::ofstream file( output );
		if( !file.is_open() )
		{
			return 0;
		}

		std::vector<std::pair<std::string, std::string>> sortedPairs;
		sortedPairs.reserve( result.size() );

		for( const auto &p : result )
		{
			auto unkName = g2[p.first].name;
			auto knownName = g1[p.second].baseName;

			if( unkName.rfind( "FUN_", 0 ) == 0 && knownName.rfind( "FUN_", 0 ) != 0 )
			{
				sortedPairs.emplace_back( removePrefix( unkName ), knownName );
			}
		}

		std::sort( sortedPairs.begin(), sortedPairs.end(),
				   []( const auto &a, const auto &b ) { return a.first < b.first; } );

		for( const auto &item : sortedPairs )
		{
			file << "- " << item.first << " " << item.second << std::endl;
		}
	}
	catch( const std::exception &e )
	{
		std::cerr << "[ERROR] " << e.what() << std::endl;
		return 1;
	}
	return 0;
}