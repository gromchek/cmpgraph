#ifndef FUNCTION_MATCHER_H
#define FUNCTION_MATCHER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <functional>

struct Function
{
	std::string name;
	std::string baseName;
	std::vector<std::string> callees;
	std::vector<std::string> callers;
};

class FunctionMatcher
{
public:
	FunctionMatcher( const std::vector<Function> &g1, const std::vector<Function> &g2, double simThreshold );

	std::unordered_map<int, int> runFullMatch();

	std::vector<std::pair<int, double>> findTargetFunction( const std::string &targetName, double threshold );

private:
	const std::vector<Function> &g1_;
	const std::vector<Function> &g2_;
	double simThreshold_;

	std::unordered_map<int, int> match_;
	std::unordered_set<int> usedG1_;

	std::unordered_map<std::string, int> nameToIdxG1_;
	std::unordered_map<std::string, std::vector<int>> nameToIndicesG2_;

	std::vector<std::unordered_set<std::string>> calleeNamesG1_;
	std::vector<std::unordered_set<std::string>> callerNamesG1_;

	std::unordered_map<std::string, std::unordered_set<int>> calleeToG1Nodes_;
	std::unordered_map<std::string, std::unordered_set<int>> callerToG1Nodes_;

	mutable std::mutex mtx_;

	void buildInvertedIndex();

	void matchExactNamesParallel();
	void matchSimilarNamesParallel();
	void matchIterativeParallel();

	std::vector<int> getNeighborIndicesG2( int v ) const;

	bool isUnknownFunction( const std::string &name ) const;
	void addMatch( int idxG2, int idxG1 );

	std::pair<std::unordered_set<std::string>, std::unordered_set<std::string>> getRequiredNeighbors( int v ) const;

	std::vector<int> findStructureCandidates( const std::unordered_set<std::string> &reqCallees,
											  const std::unordered_set<std::string> &reqCallers ) const;

	double calculateStructuralScore( int idxG2, int idxG1 ) const;

	int resolveMultipleCandidates( int v, const std::vector<int> &candidates ) const;

	int countUnknownNeighbors( int v ) const;
	int countUnmatchedNeighbors( int u ) const;
};

#endif