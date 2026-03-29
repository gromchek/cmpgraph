#include <iostream>
#include <vector>
#include "StringTransformer.h"

int main()
{
	StringTransformer transformer;

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
	transformer.addRule( "TSFixedArray_", "TSGrowableArray_" );
	transformer.addRule( "__", "_" );

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

	std::vector<std::pair<std::string, std::string>> tests = {
		{ "GLWorker::GLWorker", "GLWorker_constructor" },
		{ "CSplineParticleEmitter::~CSplineParticleEmitter", "CSplineParticleEmitter_destructor" },
		{ "TSGrowableArray<CXMLAttribute>::SetCount", "TSGrowableArray_CXMLAttribute_SetCount" },
		{ "DBCache<WardenCachedModule,CWardenKey,CWardenKey>::Save", "DBCache_WardenCachedModule_Save" },
		{ "DBCache<PageTextCache_C,int,HASHKEY_INT>::AddItems", "DBCache_PageTextCache_C_AddItems" },
		{ "TSHashTable<OVERRIDE_SPELLCAST_BY_NAME_NODE,HASHKEY_UTF8I>::Ptr",
		  "TSHashTable_OVERRIDE_SPELLCAST_BY_NAME_NODE_Ptr" },
		{ "TSSimpleArray<TSExplicitList<SOUNDKITLOOKUP,-572662307>>::FatalArrayBounds",
		  "TSSimpleArray_TSExplicitList_SOUNDKITLOOKUP_FatalArrayBounds" },
		{ "TSFixedArray<TSExplicitList<SOUNDKITLOOKUP,-572662307>>::ReallocData",
		  "TSGrowableArray_TSExplicitList_SOUNDKITLOOKUP_ReallocData" },
		{ "TSHashTable<struct_FILEMAP,class_HASHKEY_STRI>::InternalClear", "TSHashTable_FILEMAP_InternalClear" },
		{ "Battlenet::MatchMaker::MapSpec::MapSpec", "Battlenet_MatchMaker_MapSpec_constructor" },

		{ "CDynamicString::operator=", "CDynamicString_operator_assign" },
		{ "CDynamicString::operator+=", "CDynamicString_operator_add_on_assign" },

		{ "WowTime::operator<", "WowTime_operator_lt" },
		{ "WowTime::operator>", "WowTime_operator_gt" },
		{ "WowTime::operator==", "WowTime_operator_eq" },
		{ "WowTime::operator>=", "WowTime_operator_ge" },
		{ "WowTime::operator<=", "WowTime_operator_le" },

		{ "SI3::DSP_AttachCustomDspChain", "SI2_DSP_AttachCustomDspChain" },
		{ "SI3::DSP_UpdateDsp", "SI2_DSP_UpdateDsp" },
		{ "SI3DspChainHead::DestroyChain", "SI2DspChainHead_DestroyChain" },
		{ "SE3::GetInputDriverName_Cached", "SE2_GetInputDriverName_Cached" },
		{ "SE3::CacheEventCallback", "SE2_CacheEventCallback" },
		{ "SE3CaptureCallbacks::~SE3CaptureCallbacks", "SE2CaptureCallbacks_destructor" },
		{ "TSExplicitList<SE3SoundChunk,-572662307>::TSExplicitList", "TSExplicitList_SE2SoundChunk_constructor" },

		{ "__jump_table::_strcasecmp", "SStrCmp" },
		{ "__jump_table::strchr", "SStrChr" },
		{ "__jump_table::strcpy", "SStrCopy" },
		{ "__jump_table::strlen", "SStrLen" },
		{ "__jump_table::strncasecmp", "SStrCmpI" },
		{ "__jump_table::strrchr", "SStrChrR" },
		{ "__jump_table::strstr", "SStrStr" },
		{ "__jump_table::_memmove", "_memmove" },
		{ "_SErrGetErrorStr", "SErrGetErrorStr" },
		{ "_SErrGetLastError", "SErrGetLastError" },
		{ "_SErrSetLastError", "SErrSetLastError" },
		{ "_SErrSetLogCallback", "SErrSetLogCallback" },

		{ "WowClientDB2<KeyChainRec_C>::WowClientDB2", "WowClientDB_KeyChainRec_C_constructor" },
		{ "WowClientDB2<KeyChainRec_C>::GetRecordByIndex", "WowClientDB_KeyChainRec_C_GetRecordByIndex" },
		{ "WowClientDB2<KeyChainRec_C>::IterateOverCache", "WowClientDB_KeyChainRec_C_IterateOverCache" }
	};

	std::cout << "=== Test Results ===\n";
	for( const auto &[input, expected] : tests )
	{
		std::string result = transformer.transform( input );
		bool passed = ( result == expected );

		std::cout << "Input:    " << input << "\n";
		std::cout << "Output:   " << result << "\n";
		std::cout << "Expected: " << expected << "\n";
		std::cout << "Status:   " << ( passed ? "[OK]" : "[FAIL]" ) << "\n";
		std::cout << "---------------------\n";
	}

	return 0;
}