#include "FunctionMatcher.h"

#include <iostream>
#include <thread>
#include <tuple>
#include <queue>
#include <climits>
#include <algorithm>
#include <atomic>

namespace
{

static size_t levenshtein( const std::string &s1, const std::string &s2 )
{
	const size_t m = s1.size();
	const size_t n = s2.size();
	std::vector<size_t> prev( n + 1 );
	std::vector<size_t> curr( n + 1 );

	for( size_t j = 0; j <= n; ++j )
	{
		prev[j] = j;
	}

	for( size_t i = 1; i <= m; ++i )
	{
		curr[0] = i;
		for( size_t j = 1; j <= n; ++j )
		{
			if( s1[i - 1] == s2[j - 1] )
			{
				curr[j] = prev[j - 1];
			}
			else
			{
				curr[j] = 1 + std::min( { prev[j], curr[j - 1], prev[j - 1] } );
			}
		}
		std::swap( prev, curr );
	}
	return prev[n];
}

double nameSimilarity( const std::string &a, const std::string &b, double threshold )
{
	if( a.empty() || b.empty() )
	{
		return 0.0;
	}

	size_t max_len = std::max( a.size(), b.size() );
	size_t min_len = std::min( a.size(), b.size() );
	double len_diff_ratio = 1.0 - static_cast<double>( max_len - min_len ) / max_len;

	if( len_diff_ratio < threshold )
	{
		return 0.0;
	}

	size_t dist = levenshtein( a, b );
	return 1.0 - static_cast<double>( dist ) / max_len;
}

} // namespace

FunctionMatcher::FunctionMatcher( const std::vector<Function> &g1, const std::vector<Function> &g2,
								  double simThreshold ) :
	g1_( g1 ), g2_( g2 ), simThreshold_( simThreshold )
{
	std::cout << "[LOG] Building indices for G1 and G2..." << std::endl;

	for( size_t i = 0; i < g1_.size(); ++i )
	{
		nameToIdxG1_[g1_[i].name] = static_cast<int>( i );
	}

	for( size_t i = 0; i < g2_.size(); ++i )
	{
		nameToIndicesG2_[g2_[i].name].push_back( static_cast<int>( i ) );
	}

	calleeNamesG1_.resize( g1_.size() );
	callerNamesG1_.resize( g1_.size() );
	for( size_t i = 0; i < g1_.size(); ++i )
	{
		calleeNamesG1_[i].insert( g1_[i].callees.begin(), g1_[i].callees.end() );
		callerNamesG1_[i].insert( g1_[i].callers.begin(), g1_[i].callers.end() );
	}

	buildInvertedIndex();
}

void FunctionMatcher::buildInvertedIndex()
{
	for( size_t u = 0; u < g1_.size(); ++u )
	{
		for( const auto &name : g1_[u].callees )
		{
			calleeToG1Nodes_[name].insert( static_cast<int>( u ) );
		}

		for( const auto &name : g1_[u].callers )
		{
			callerToG1Nodes_[name].insert( static_cast<int>( u ) );
		}
	}
}

std::unordered_map<int, int> FunctionMatcher::runFullMatch()
{
	std::cout << "[LOG] Starting full matching process..." << std::endl;

	matchExactNamesParallel();
	matchSimilarNamesParallel();
	matchIterativeParallel();

	std::cout << "[LOG] Matching process completed. Total matches: " << match_.size() << std::endl;
	return match_;
}

std::vector<std::pair<int, double>> FunctionMatcher::findTargetFunction( const std::string &targetName,
																		 double threshold )
{
	std::vector<std::pair<int, double>> results;

	auto itG2 = nameToIndicesG2_.find( targetName );
	if( itG2 == nameToIndicesG2_.end() || itG2->second.empty() )
	{
		std::cerr << "[WARN] Function '" << targetName << "' not found in G2." << std::endl;
		return results;
	}

	int targetIdx = -1;
	for( int idx : itG2->second )
	{
		if( match_.count( idx ) == 0 )
		{
			targetIdx = idx;
			break;
		}
	}

	if( targetIdx == -1 )
	{
		std::cerr << "[WARN] All indices for '" << targetName << "' are already matched." << std::endl;
		return results;
	}

	auto [reqCallees, reqCallers] = getRequiredNeighbors( targetIdx );
	auto candidates = findStructureCandidates( reqCallees, reqCallers );

	for( int u : candidates )
	{
		if( usedG1_.count( u ) )
		{
			continue;
		}

		double score = calculateStructuralScore( targetIdx, u );
		if( score >= threshold )
		{
			results.emplace_back( u, score );
		}
	}

	if( results.empty() )
	{
		for( size_t u = 0; u < g1_.size(); ++u )
		{
			if( usedG1_.count( static_cast<int>( u ) ) )
			{
				continue;
			}

			double sim = nameSimilarity( g2_[targetIdx].name, g1_[u].name, threshold );
			if( sim >= threshold )
			{
				double structBonus = calculateStructuralScore( targetIdx, static_cast<int>( u ) ) * 0.5;
				results.emplace_back( static_cast<int>( u ), sim + structBonus );
			}
		}
	}

	std::sort( results.begin(), results.end(), []( const auto &a, const auto &b ) { return a.second > b.second; } );

	return results;
}

void FunctionMatcher::matchExactNamesParallel()
{
	std::cout << "[LOG] Stage 1: Matching exact names (Parallel)..." << std::endl;

	unsigned int numThreads = std::max( 2u, std::thread::hardware_concurrency() );
	std::vector<std::thread> threads;

	if( g2_.empty() )
	{
		std::cout << "[LOG] Stage 1 completed. Matches found: " << match_.size() << std::endl;
		return;
	}

	size_t blockSize = ( g2_.size() + numThreads - 1 ) / numThreads;

	std::vector<std::vector<std::pair<int, int>>> threadMatches( numThreads );

	for( unsigned int t = 0; t < numThreads; ++t )
	{
		size_t start = t * blockSize;
		size_t end = std::min( start + blockSize, g2_.size() );
		if( start >= end )
		{
			break;
		}

		threads.emplace_back( [this, start, end, t, &threadMatches]() {
			auto &local = threadMatches[t];
			for( size_t i = start; i < end; ++i )
			{
				if( isUnknownFunction( g2_[i].name ) )
				{
					continue;
				}

				auto it = nameToIdxG1_.find( g2_[i].name );
				if( it == nameToIdxG1_.end() )
				{
					continue;
				}

				local.emplace_back( static_cast<int>( i ), it->second );
			}
		} );
	}

	for( auto &th : threads )
	{
		th.join();
	}

	for( const auto &vec : threadMatches )
	{
		for( const auto &[g2idx, g1idx] : vec )
		{
			if( usedG1_.count( g1idx ) == 0 && match_.count( g2idx ) == 0 )
			{
				addMatch( g2idx, g1idx );
			}
		}
	}

	std::cout << "[LOG] Stage 1 completed. Matches found: " << match_.size() << std::endl;
}

void FunctionMatcher::matchSimilarNamesParallel()
{
	std::cout << "[LOG] Stage 2: Matching similar names (Parallel)..." << std::endl;

	std::unordered_set<int> alreadyMatchedG2;
	for( const auto &[g2idx, g1idx] : match_ )
	{
		alreadyMatchedG2.insert( g2idx );
	}

	if( g2_.empty() )
	{
		std::cout << "[LOG] Stage 2 completed. New matches found: 0" << std::endl;
		return;
	}

	unsigned int numThreads = std::max( 2u, std::thread::hardware_concurrency() );
	std::vector<std::vector<std::tuple<double, int, int>>> threadCandidates( numThreads );
	std::vector<std::thread> threads;
	size_t blockSize = ( g2_.size() + numThreads - 1 ) / numThreads;

	for( unsigned int t = 0; t < numThreads; ++t )
	{
		size_t start = t * blockSize;
		size_t end = std::min( start + blockSize, g2_.size() );
		if( start >= end )
		{
			break;
		}

		threads.emplace_back( [this, start, end, t, &threadCandidates, &alreadyMatchedG2]() {
			auto &localCandidates = threadCandidates[t];
			localCandidates.reserve( ( end - start ) * 5 );

			for( size_t i = start; i < end; ++i )
			{
				if( alreadyMatchedG2.count( static_cast<int>( i ) ) )
				{
					continue;
				}
				if( isUnknownFunction( g2_[i].name ) )
				{
					continue;
				}

				for( size_t j = 0; j < g1_.size(); ++j )
				{
					double sim = nameSimilarity( g2_[i].name, g1_[j].name, simThreshold_ );
					if( sim >= simThreshold_ )
					{
						localCandidates.emplace_back( sim, static_cast<int>( i ), static_cast<int>( j ) );
					}
				}
			}
		} );
	}

	for( auto &th : threads )
	{
		th.join();
	}

	size_t totalSize = 0;
	for( const auto &vec : threadCandidates )
	{
		totalSize += vec.size();
	}

	std::vector<std::tuple<double, int, int>> allCandidates;
	allCandidates.reserve( totalSize );
	for( auto &vec : threadCandidates )
	{
		allCandidates.insert( allCandidates.end(), std::make_move_iterator( vec.begin() ),
							  std::make_move_iterator( vec.end() ) );
	}

	std::sort( allCandidates.begin(), allCandidates.end(),
			   []( const auto &a, const auto &b ) { return std::get<0>( a ) > std::get<0>( b ); } );

	size_t newMatches = 0;
	for( const auto &[sim, g2idx, g1idx] : allCandidates )
	{
		if( match_.count( g2idx ) == 0 && usedG1_.count( g1idx ) == 0 )
		{
			addMatch( g2idx, g1idx );
			++newMatches;
		}
	}

	std::cout << "[LOG] Stage 2 completed. New matches found: " << newMatches << std::endl;
}

void FunctionMatcher::matchIterativeParallel()
{
	std::cout << "[LOG] Stage 3: Iterative structural matching (Parallel)..." << std::endl;

	std::queue<int> q;
	std::vector<bool> inQueue( g2_.size(), false );

	for( size_t i = 0; i < g2_.size(); ++i )
	{
		if( match_.count( static_cast<int>( i ) ) == 0 )
		{
			q.push( static_cast<int>( i ) );
			inQueue[i] = true;
		}
	}

	struct SearchResult
	{
		int g2_idx = -1;
		int g1_cand = -1;
		bool is_unique = false;
		std::vector<int> neighbors_to_queue;
	};

	bool changed = true;
	while( changed && !q.empty() )
	{
		changed = false;

		std::vector<int> currentBatch;
		currentBatch.reserve( q.size() );
		while( !q.empty() )
		{
			currentBatch.push_back( q.front() );
			q.pop();
		}

		std::vector<SearchResult> results( currentBatch.size() );

		unsigned int numThreads = std::max( 2u, std::thread::hardware_concurrency() );
		std::vector<std::thread> threads;
		size_t blockSize = ( currentBatch.size() + numThreads - 1 ) / numThreads;

		for( unsigned int t = 0; t < numThreads; ++t )
		{
			size_t start = t * blockSize;
			size_t end = std::min( start + blockSize, currentBatch.size() );
			if( start >= end )
			{
				break;
			}

			threads.emplace_back( [this, start, end, &currentBatch, &results]() {
				for( size_t k = start; k < end; ++k )
				{
					int v = currentBatch[k];
					auto &res = results[k];
					res.g2_idx = v;

					if( match_.count( v ) )
					{
						res.is_unique = false;
						continue;
					}

					auto [reqCallees, reqCallers] = getRequiredNeighbors( v );
					auto candidates = findStructureCandidates( reqCallees, reqCallers );

					if( candidates.empty() )
					{
						res.is_unique = false;
						continue;
					}

					if( candidates.size() == 1 )
					{
						res.g1_cand = candidates[0];
						res.is_unique = true;
						res.neighbors_to_queue = getNeighborIndicesG2( v );
					}
					else
					{
						int bestU = resolveMultipleCandidates( v, candidates );
						if( bestU != -1 )
						{
							res.g1_cand = bestU;
							res.is_unique = true;
							res.neighbors_to_queue = getNeighborIndicesG2( v );
						}
						else
						{
							res.is_unique = false;
						}
					}
				}
			} );
		}

		for( auto &th : threads )
		{
			th.join();
		}

		for( const auto &res : results )
		{
			if( !res.is_unique )
			{
				if( res.g2_idx >= 0 && !inQueue[res.g2_idx] )
				{
					q.push( res.g2_idx );
					inQueue[res.g2_idx] = true;
				}
				continue;
			}

			if( usedG1_.count( res.g1_cand ) == 0 && match_.count( res.g2_idx ) == 0 )
			{
				addMatch( res.g2_idx, res.g1_cand );
				changed = true;
				inQueue[res.g2_idx] = false;

				for( int nbIdx : res.neighbors_to_queue )
				{
					if( match_.count( nbIdx ) == 0 && !inQueue[nbIdx] )
					{
						q.push( nbIdx );
						inQueue[nbIdx] = true;
					}
				}
			}
			else
			{
				if( !inQueue[res.g2_idx] )
				{
					q.push( res.g2_idx );
					inQueue[res.g2_idx] = true;
				}
			}
		}
	}

	std::cout << "[LOG] Stage 3 completed." << std::endl;
}

std::vector<int> FunctionMatcher::getNeighborIndicesG2( int v ) const
{
	std::unordered_set<int> seen;
	std::vector<int> neighbors;

	auto addNeighbors = [&]( const std::vector<std::string> &names ) {
		for( const auto &name : names )
		{
			auto it = nameToIndicesG2_.find( name );
			if( it != nameToIndicesG2_.end() )
			{
				for( int idx : it->second )
				{
					if( seen.insert( idx ).second )
					{
						neighbors.push_back( idx );
					}
				}
			}
		}
	};

	addNeighbors( g2_[v].callees );
	addNeighbors( g2_[v].callers );
	return neighbors;
}

bool FunctionMatcher::isUnknownFunction( const std::string &name ) const
{
	return name.rfind( "FUN_", 0 ) == 0;
}

void FunctionMatcher::addMatch( int idxG2, int idxG1 )
{
	match_[idxG2] = idxG1;
	usedG1_.insert( idxG1 );
}

std::pair<std::unordered_set<std::string>, std::unordered_set<std::string>>
FunctionMatcher::getRequiredNeighbors( int v ) const
{
	std::unordered_set<std::string> reqCallees;
	std::unordered_set<std::string> reqCallers;

	auto collect = [&]( const std::vector<std::string> &names, std::unordered_set<std::string> &out ) {
		for( const auto &name : names )
		{
			auto it = nameToIndicesG2_.find( name );
			if( it == nameToIndicesG2_.end() )
			{
				continue;
			}

			for( int idx : it->second )
			{
				auto mIt = match_.find( idx );
				if( mIt != match_.end() )
				{
					out.insert( g1_.at( mIt->second ).name );
					break;
				}
			}
		}
	};

	collect( g2_[v].callees, reqCallees );
	collect( g2_[v].callers, reqCallers );
	return { reqCallees, reqCallers };
}

std::vector<int> FunctionMatcher::findStructureCandidates( const std::unordered_set<std::string> &reqCallees,
														   const std::unordered_set<std::string> &reqCallers ) const
{
	if( reqCallees.empty() && reqCallers.empty() )
	{
		return {};
	}

	std::unordered_set<int> candidateSet;
	bool initialized = false;

	auto intersect = [&]( const std::unordered_set<std::string> &names,
						  const std::unordered_map<std::string, std::unordered_set<int>> &index ) {
		for( const auto &name : names )
		{
			auto it = index.find( name );
			std::unordered_set<int> withThis;
			if( it != index.end() )
			{
				withThis = it->second;
			}

			if( !initialized )
			{
				candidateSet = std::move( withThis );
				initialized = true;
			}
			else
			{
				std::unordered_set<int> intersection;
				for( int u : candidateSet )
				{
					if( withThis.count( u ) )
					{
						intersection.insert( u );
					}
				}
				candidateSet = std::move( intersection );
			}

			if( candidateSet.empty() )
			{
				return;
			}
		}
	};

	intersect( reqCallees, calleeToG1Nodes_ );
	intersect( reqCallers, callerToG1Nodes_ );

	return std::vector<int>( candidateSet.begin(), candidateSet.end() );
}

double FunctionMatcher::calculateStructuralScore( int idxG2, int idxG1 ) const
{
	const auto &fG2 = g2_[idxG2];

	auto translatedNeighborNames = [&]( const std::vector<std::string> &neighbors ) {
		std::unordered_set<std::string> translated;
		for( const auto &name : neighbors )
		{
			auto it = nameToIndicesG2_.find( name );
			if( it == nameToIndicesG2_.end() )
			{
				continue;
			}
			for( int idx : it->second )
			{
				auto mIt = match_.find( idx );
				if( mIt != match_.end() )
				{
					translated.insert( g1_.at( mIt->second ).name );
					break;
				}
			}
		}
		return translated;
	};

	auto translatedCallees = translatedNeighborNames( fG2.callees );
	auto translatedCallers = translatedNeighborNames( fG2.callers );

	auto intersectionSize = []( const std::unordered_set<std::string> &translated,
								const std::unordered_set<std::string> &g1Names ) {
		int count = 0;
		for( const auto &name : translated )
		{
			if( g1Names.count( name ) )
			{
				++count;
			}
		}
		return count;
	};

	int commonCallees = intersectionSize( translatedCallees, calleeNamesG1_[idxG1] );
	int commonCallers = intersectionSize( translatedCallers, callerNamesG1_[idxG1] );

	int totalTranslated = static_cast<int>( translatedCallees.size() + translatedCallers.size() );
	if( totalTranslated == 0 )
	{
		return 0.0;
	}

	return static_cast<double>( commonCallees + commonCallers ) / totalTranslated;
}

int FunctionMatcher::resolveMultipleCandidates( int v, const std::vector<int> &candidates ) const
{
	int unknownV = countUnknownNeighbors( v );

	int bestU = -1;
	int bestScore = INT_MAX;
	std::vector<int> bestCandidates;

	for( int u : candidates )
	{
		if( usedG1_.count( u ) )
		{
			continue;
		}

		int unmatchedU = countUnmatchedNeighbors( u );

		int score = std::abs( static_cast<int>( g2_[v].callees.size() ) - static_cast<int>( g1_[u].callees.size() ) ) +
					std::abs( static_cast<int>( g2_[v].callers.size() ) - static_cast<int>( g1_[u].callers.size() ) ) +
					std::abs( unknownV - unmatchedU );

		if( score < bestScore )
		{
			bestScore = score;
			bestU = u;
			bestCandidates = { u };
		}
		else if( score == bestScore )
		{
			bestCandidates.push_back( u );
		}
	}

	return ( bestCandidates.size() == 1 ) ? bestU : -1;
}

int FunctionMatcher::countUnknownNeighbors( int v ) const
{
	int count = 0;

	auto countFn = [&]( const std::vector<std::string> &names ) {
		for( const auto &name : names )
		{
			bool matched = false;
			auto it = nameToIndicesG2_.find( name );
			if( it != nameToIndicesG2_.end() )
			{
				for( int idx : it->second )
				{
					if( match_.count( idx ) )
					{
						matched = true;
						break;
					}
				}
			}
			if( !matched )
			{
				++count;
			}
		}
	};

	countFn( g2_[v].callees );
	countFn( g2_[v].callers );
	return count;
}

int FunctionMatcher::countUnmatchedNeighbors( int u ) const
{
	int count = 0;

	auto countFn = [&]( const std::vector<std::string> &names ) {
		for( const auto &name : names )
		{
			auto it = nameToIdxG1_.find( name );
			if( it != nameToIdxG1_.end() && usedG1_.count( it->second ) == 0 )
			{
				++count;
			}
		}
	};

	countFn( g1_[u].callees );
	countFn( g1_[u].callers );
	return count;
}