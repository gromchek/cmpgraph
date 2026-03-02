#include <iostream>
#include <vector>
#include "StringTransformer.h"

int main()
{
	StringTransformer transformer;

	transformer.addRule( "SE3", "SE2" );
	transformer.addRule( "SI3", "SI2" );
	transformer.addRule( "CWorldMap", "CMap" );
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

	std::vector<std::pair<std::string, std::string>> tests = {
		{ "GLWorker::GLWorker", "GLWorker__constructor" },
		{ "CSplineParticleEmitter::~CSplineParticleEmitter", "CSplineParticleEmitter__destructor" },
		{ "TSGrowableArray<CXMLAttribute>::SetCount", "TSGrowableArray__CXMLAttribute__SetCount" },
		{ "DBCache<WardenCachedModule,CWardenKey,CWardenKey>::Save", "DBCache__WardenCachedModule__Save" },
		{ "DBCache<PageTextCache_C,int,HASHKEY_INT>::AddItems", "DBCache__PageTextCache_C__AddItems" },
		{ "TSHashTable<OVERRIDE_SPELLCAST_BY_NAME_NODE,HASHKEY_UTF8I>::Ptr",
		  "TSHashTable__OVERRIDE_SPELLCAST_BY_NAME_NODE__Ptr" },
		{ "TSSimpleArray<TSExplicitList<SOUNDKITLOOKUP,-572662307>>::FatalArrayBounds",
		  "TSSimpleArray__TSExplicitList__SOUNDKITLOOKUP__FatalArrayBounds" },
		{ "TSHashTable<struct_FILEMAP,class_HASHKEY_STRI>::InternalClear", "TSHashTable__FILEMAP__InternalClear" },
		{ "Battlenet::MatchMaker::MapSpec::MapSpec", "Battlenet__MatchMaker__MapSpec__constructor" },

		{ "CDynamicString::operator=", "CDynamicString__operator_assign" },
		{ "CDynamicString::operator+=", "CDynamicString__operator_add_on_assign" },

		{ "WowTime::operator<", "WowTime__operator_lt" },
		{ "WowTime::operator>", "WowTime__operator_gt" },
		{ "WowTime::operator==", "WowTime__operator_eq" },
		{ "WowTime::operator>=", "WowTime__operator_ge" },
		{ "WowTime::operator<=", "WowTime__operator_le" },

		{ "SI3::DSP_AttachCustomDspChain", "SI2__DSP_AttachCustomDspChain" },
		{ "SI3::DSP_UpdateDsp", "SI2__DSP_UpdateDsp" },
		{ "SI3DspChainHead::DestroyChain", "SI2DspChainHead__DestroyChain" },
		{ "SE3::GetInputDriverName_Cached", "SE2__GetInputDriverName_Cached" },
		{ "SE3::CacheEventCallback", "SE2__CacheEventCallback" },
		{ "SE3CaptureCallbacks::~SE3CaptureCallbacks", "SE2CaptureCallbacks__destructor" },
		{ "TSExplicitList<SE3SoundChunk,-572662307>::TSExplicitList", "TSExplicitList__SE2SoundChunk__constructor" }

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