#include "InlineAwareMatcher.h"

#include <algorithm>
#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>

InlineAwareMatcher::InlineAwareMatcher( const std::vector<Function> &g1, const std::vector<Function> &g2, int maxDepth,
										double scoreThreshold ) :
	g1_( g1 ), g2_( g2 ), maxDepth_( maxDepth ), scoreThreshold_( scoreThreshold )
{
	for( size_t i = 0; i < g1_.size(); ++i )
	{
		nameToIdxG1_[g1_[i].name] = static_cast<int>( i );
	}

	for( size_t i = 0; i < g2_.size(); ++i )
	{
		nameToIndicesG2_[g2_[i].name].push_back( static_cast<int>( i ) );
	}

	buildG1ReachSets();
}

std::unordered_map<int, int> InlineAwareMatcher::run( const std::unordered_map<int, int> &existingMatch )
{
	std::cout << "[InlineAware] Starting inline-aware matching (maxDepth=" << maxDepth_ << ")..." << std::endl;

	std::unordered_set<int> usedG1;
	for( const auto &[g2i, g1i] : existingMatch )
	{
		usedG1.insert( g1i );
	}

	using Candidate = std::tuple<double, int, int>;

	const unsigned int numThreads = std::max( 2u, std::thread::hardware_concurrency() );
	std::vector<std::vector<Candidate>> threadCandidates( numThreads );
	const size_t blockSize = ( g2_.size() + numThreads - 1 ) / numThreads;

	std::vector<std::thread> threads;
	threads.reserve( numThreads );

	for( unsigned int t = 0; t < numThreads; ++t )
	{
		const size_t start = t * blockSize;
		const size_t end = std::min( start + blockSize, g2_.size() );
		if( start >= end )
		{
			break;
		}

		threads.emplace_back( [this, start, end, t, &existingMatch, &usedG1, &threadCandidates]() {
			auto &local = threadCandidates[t];

			for( size_t vi = start; vi < end; ++vi )
			{
				const int v = static_cast<int>( vi );

				if( existingMatch.count( v ) )
				{
					continue;
				}

				const auto reachCalleeV = bfsTranslated_G2( v, true, maxDepth_, existingMatch );
				const auto reachCallerV = bfsTranslated_G2( v, false, maxDepth_, existingMatch );

				if( reachCalleeV.empty() && reachCallerV.empty() )
				{
					continue;
				}

				for( size_t ui = 0; ui < g1_.size(); ++ui )
				{
					const int u = static_cast<int>( ui );

					if( usedG1.count( u ) )
					{
						continue;
					}

					const double score = computeScore( v, u, existingMatch );
					if( score >= scoreThreshold_ )
					{
						local.emplace_back( score, v, u );
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

	std::vector<Candidate> candidates;
	candidates.reserve( totalSize );
	for( auto &vec : threadCandidates )
	{
		candidates.insert( candidates.end(), std::make_move_iterator( vec.begin() ),
						   std::make_move_iterator( vec.end() ) );
	}

	std::sort( candidates.begin(), candidates.end(),
			   []( const Candidate &a, const Candidate &b ) { return std::get<0>( a ) > std::get<0>( b ); } );

	std::unordered_map<int, int> newMatches;
	std::unordered_set<int> takenG2;

	for( const auto &[score, g2i, g1i] : candidates )
	{
		if( takenG2.count( g2i ) || usedG1.count( g1i ) )
		{
			continue;
		}

		newMatches[g2i] = g1i;
		takenG2.insert( g2i );
		usedG1.insert( g1i );
	}

	std::cout << "[InlineAware] Done. New matches: " << newMatches.size() << std::endl;
	return newMatches;
}

double InlineAwareMatcher::computeScore( int g2Idx, int g1Idx, const std::unordered_map<int, int> &match ) const
{
	const auto &g1Callees = reachCalleeG1_[g1Idx];
	const auto &g1Callers = reachCallerG1_[g1Idx];

	auto g2Callees = bfsTranslated_G2( g2Idx, true, maxDepth_, match );
	auto g2Callers = bfsTranslated_G2( g2Idx, false, maxDepth_, match );

	double calleeScore = overlapScore( g2Callees, g1Callees );
	double callerScore = overlapScore( g2Callers, g1Callers );

	bool hasCallees = !g2Callees.empty() || !g1Callees.empty();
	bool hasCallers = !g2Callers.empty() || !g1Callers.empty();

	if( hasCallees && hasCallers )
	{
		return ( calleeScore + callerScore ) / 2.0;
	}
	else if( hasCallees )
	{
		return calleeScore;
	}
	else if( hasCallers )
	{
		return callerScore;
	}

	return 0.0;
}

void InlineAwareMatcher::buildG1ReachSets()
{
	reachCalleeG1_.resize( g1_.size() );
	reachCallerG1_.resize( g1_.size() );

	const unsigned int numThreads = std::max( 2u, std::thread::hardware_concurrency() );
	const size_t blockSize = ( g1_.size() + numThreads - 1 ) / numThreads;

	std::vector<std::thread> threads;
	threads.reserve( numThreads );

	for( unsigned int t = 0; t < numThreads; ++t )
	{
		const size_t start = t * blockSize;
		const size_t end = std::min( start + blockSize, g1_.size() );
		if( start >= end )
		{
			break;
		}

		threads.emplace_back( [this, start, end]() {
			for( size_t i = start; i < end; ++i )
			{
				reachCalleeG1_[i] = bfsNames_G1( static_cast<int>( i ), /*useCallees=*/true, maxDepth_ );
				reachCallerG1_[i] = bfsNames_G1( static_cast<int>( i ), /*useCallees=*/false, maxDepth_ );
			}
		} );
	}

	for( auto &th : threads )
	{
		th.join();
	}
}

std::unordered_set<std::string> InlineAwareMatcher::bfsNames_G1( int start, bool useCallees, int depth ) const
{
	std::unordered_set<std::string> result;
	std::unordered_set<int> visited;
	std::queue<std::pair<int, int>> q;
	q.push( { start, 0 } );
	visited.insert( start );

	while( !q.empty() )
	{
		auto [node, d] = q.front();
		q.pop();

		if( d >= depth )
		{
			continue;
		}

		const std::vector<std::string> &neighbors = useCallees ? g1_[node].callees : g1_[node].callers;

		for( const std::string &name : neighbors )
		{
			auto it = nameToIdxG1_.find( name );
			if( it == nameToIdxG1_.end() )
			{
				continue;
			}

			int nIdx = it->second;
			result.insert( name );

			if( visited.insert( nIdx ).second )
			{
				q.push( { nIdx, d + 1 } );
			}
		}
	}

	return result;
}

std::unordered_set<std::string> InlineAwareMatcher::bfsTranslated_G2( int start, bool useCallees, int depth,
																	  const std::unordered_map<int, int> &match ) const
{
	std::unordered_set<std::string> result;
	std::unordered_set<int> visited;
	std::queue<std::pair<int, int>> q;
	q.push( { start, 0 } );
	visited.insert( start );

	while( !q.empty() )
	{
		auto [node, d] = q.front();
		q.pop();

		if( d >= depth )
		{
			continue;
		}

		const std::vector<std::string> &neighbors = useCallees ? g2_[node].callees : g2_[node].callers;

		for( const std::string &name : neighbors )
		{
			auto it = nameToIndicesG2_.find( name );
			if( it == nameToIndicesG2_.end() )
			{
				continue;
			}

			for( int nIdx : it->second )
			{
				auto mIt = match.find( nIdx );
				if( mIt != match.end() )
				{
					result.insert( g1_[mIt->second].name );
				}

				if( visited.insert( nIdx ).second )
				{
					q.push( { nIdx, d + 1 } );
				}
			}
		}
	}

	return result;
}

double InlineAwareMatcher::overlapScore( const std::unordered_set<std::string> &a,
										 const std::unordered_set<std::string> &b )
{
	if( a.empty() || b.empty() )
	{
		return 0.0;
	}

	int common = 0;
	const auto &smaller = ( a.size() <= b.size() ) ? a : b;
	const auto &larger = ( a.size() <= b.size() ) ? b : a;

	for( const auto &name : smaller )
	{
		if( larger.count( name ) )
		{
			++common;
		}
	}

	return static_cast<double>( common ) / static_cast<double>( smaller.size() );
}

bool InlineAwareMatcher::isUnknown( const std::string &name ) const
{
	return name.rfind( "FUN_", 0 ) == 0;
}