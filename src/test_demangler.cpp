#include <iostream>
#include <vector>
#include "NameDemangler.h"

int main()
{
	NameDemangler demangler;

	// clang-format off
	std::vector<std::pair<std::string, std::string>> tests = {
        {
            "_ZN11TSHashTableIN7DBCacheI17GameObjectStats_Ci11HASHKEY_INTE11DBCACHEHASHES2_E3PtrEjRKS2_",
            "TSHashTable<DBCache<GameObjectStats_C, int, HASHKEY_INT>::DBCACHEHASH, HASHKEY_INT>::Ptr"
        },
        {
            "_ZN11TSHashTableIN7DBCacheI9NameCachey12CHashKeyGUIDE11DBCACHEHASHES2_E3NewEjRKS2_mm",
            "TSHashTable<DBCache<NameCache, unsigned long long, CHashKeyGUID>::DBCACHEHASH, CHashKeyGUID>::New"
        },
        {
            "_ZN11TSHashTableIN7DBCacheI7NPCTexti11HASHKEY_INTE11DBCACHEHASHES2_E3PtrEjRKS2_",
            "TSHashTable<DBCache<NPCText, int, HASHKEY_INT>::DBCACHEHASH, HASHKEY_INT>::Ptr"
        },
        {
            "_ZN11TSHashTableIN7DBCacheI15PageTextCache_Ci11HASHKEY_INTE11DBCACHEHASHES2_E3PtrEjRKS2_",
            "TSHashTable<DBCache<PageTextCache_C, int, HASHKEY_INT>::DBCACHEHASH, HASHKEY_INT>::Ptr"
        },
        {
            "_ZN11TSHashTableIN7DBCacheI12PetNameCachei11HASHKEY_INTE11DBCACHEHASHES2_E3PtrEjRKS2_",
            "TSHashTable<DBCache<PetNameCache, int, HASHKEY_INT>::DBCACHEHASH, HASHKEY_INT>::Ptr"
        },
        {
            "_ZN11TSHashTableIN7DBCacheI10CGPetitioni11HASHKEY_INTE11DBCACHEHASHES2_E15InternalNewNodeEjmm",
            "TSHashTable<DBCache<CGPetition, int, HASHKEY_INT>::DBCACHEHASH, HASHKEY_INT>::InternalNewNode"
        },
        {
            "_ZN11TSHashTableIN7DBCacheI14ArenaTeamCachei11HASHKEY_INTE11DBCACHEHASHES2_E3PtrEjRKS2_",
            "TSHashTable<DBCache<ArenaTeamCache, int, HASHKEY_INT>::DBCACHEHASH, HASHKEY_INT>::Ptr"
        },
        {
            "_ZN11TSHashTableIN7DBCacheI10DanceCachei11HASHKEY_INTE11DBCACHEHASHES2_E15InternalNewNodeEjmm",
            "TSHashTable<DBCache<DanceCache, int, HASHKEY_INT>::DBCACHEHASH, HASHKEY_INT>::InternalNewNode"
        },
        {
            "_ZN11TSHashTableIN7DBCacheI15CreatureStats_Ci11HASHKEY_INTE11DBCACHEHASHES2_E15InternalNewNodeEjmm",
            "TSHashTable<DBCache<CreatureStats_C, int, HASHKEY_INT>::DBCACHEHASH, HASHKEY_INT>::InternalNewNode"
        },
        {
            "_ZN11TSHashTableIN7DBCacheI12PetNameCachei11HASHKEY_INTE11DBCACHEHASHES2_E15InternalNewNodeEjmm",
            "TSHashTable<DBCache<PetNameCache, int, HASHKEY_INT>::DBCACHEHASH, HASHKEY_INT>::InternalNewNode"
        },
        {
            "_ZN11TSHashTableIN7DBCacheI7NPCTexti11HASHKEY_INTE11DBCACHEHASHES2_E15InternalNewNodeEjmm",
            "TSHashTable<DBCache<NPCText, int, HASHKEY_INT>::DBCACHEHASH, HASHKEY_INT>::InternalNewNode"
        },
        {
            "_ZN6TSListI15DBCACHECALLBACK9TSGetLinkIS0_EE5ClearEv",
            "TSList<DBCACHECALLBACK, TSGetLink<DBCACHECALLBACK>>::Clear"
        },
        {
            "_ZN11TSHashTableIN7DBCacheI10DanceCachei11HASHKEY_INTE11DBCACHEHASHES2_E3PtrEjRKS2_",
            "TSHashTable<DBCache<DanceCache, int, HASHKEY_INT>::DBCACHEHASH, HASHKEY_INT>::Ptr"
        },
        {
            "_ZN11TSHashTableIN7DBCacheI15ItemTextCache_Cy12CHashKeyGUIDE11DBCACHEHASHES2_E3NewEjRKS2_mm",
            "TSHashTable<DBCache<ItemTextCache_C, unsigned long long, CHashKeyGUID>::DBCACHEHASH, CHashKeyGUID>::New"
        },
        {
            "_ZN11CMoveSplineD1Ev",
            "CMoveSpline::~CMoveSpline"
        },
        {
            "_Z13GetGuildColorI11WowClientDBI19GuildColorBorderRecEEiT_hR9CImVector",
            "GetGuildColor<WowClientDB<GuildColorBorderRec>>"
        },
        {
            "_Z15AdvanceAndParseI13OnSizeChanged10linkObjectEbPT0_R8MemBlock",
            "AdvanceAndParse<OnSizeChanged, linkObject>"
        },
        {
            "_ZN11CWorldQueryC2Ev",
            "CWorldQuery::CWorldQuery"
        },
	};

	// clang-format on

	std::cout << "=== Test Results ===\n";
	for( const auto &[input, expected] : tests )
	{
		std::string output = demangler.demangle( input );
		bool passed = ( output == expected );

		std::cout << "Input:    " << input << "\n";
		std::cout << "Output:   " << output << "\n";
		std::cout << "Expected: " << expected << "\n";
		std::cout << "Status:   " << ( passed ? "[OK]" : "[FAIL]" ) << "\n";
		std::cout << "---------------------\n";
	}

	return 0;
}