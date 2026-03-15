#pragma once

#include <string>
#include <vector>
#include <utility>

class ClassGroupWriter
{
public:
	using MatchPair = std::pair<std::string, std::string>;
	using MatchList = std::vector<MatchPair>;

	void export_grouped( const std::string &output_path, const MatchList &matches );
};