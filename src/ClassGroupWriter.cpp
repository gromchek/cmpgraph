#include "ClassGroupWriter.h"
#include <fstream>
#include <map>

void ClassGroupWriter::export_grouped( const std::string &output_path, const MatchList &matches )
{
	std::map<std::string, MatchList> grouped_matches;

	for( const auto &match : matches )
	{
		const std::string &known_name = match.second;

		std::string group_name;

		size_t pos = known_name.find( "::" );
		if( pos != std::string::npos )
		{
			group_name = known_name.substr( 0, pos );
		}
		else
		{
			group_name = "unnamed namespace";
		}

		grouped_matches[group_name].push_back( match );
	}

	std::ofstream file( output_path );
	if( !file.is_open() )
	{
		return;
	}

	for( const auto &group : grouped_matches )
	{
		file << "[" << group.first << "]\n";

		for( const auto &func : group.second )
		{
			file << "- " << func.first << " " << func.second << "\n";
		}

		file << "\n";
	}
}