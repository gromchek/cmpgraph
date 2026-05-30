#include <iostream>
#include "json.hpp"
#include "CliArgs.h"
#include <fstream>
#include "StringTransformer.h"
#include "FunctionMatcher.h"
#include "StringFilter.h"
#include "InlineAwareMatcher.h"
#include "ClassGroupWriter.h"
#include "NameDemangler.h"
#include <optional>

StringTransformer transformer;
StringFilter filter;
NameDemangler demangler;

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
		func.name = item.value( "name", "" );

		if( item.contains( "called_names" ) )
		{
			for( const auto &called : item["called_names"] )
			{
				func.callees.push_back( called.get<std::string>() );
			}
		}

		if( item.contains( "caller_names" ) )
		{
			for( const auto &caller : item["caller_names"] )
			{
				func.callers.push_back( caller.get<std::string>() );
			}
		}

		functions.push_back( std::move( func ) );
	}

	return functions;
}

std::vector<Function> load_functions_from_json_with_processing( const json &data )
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
		func.name = transformer.transform( demangler.demangle( func.name ) );

		if( item.contains( "called_names" ) )
		{
			for( const auto &called : item["called_names"] )
			{
				auto name = transformer.transform( called.get<std::string>() );
				if( filter.match( name ) )
				{
					continue;
				}

				name = transformer.transform( demangler.demangle( name ) );
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
				auto name = transformer.transform( caller.get<std::string>() );
				if( filter.match( name ) )
				{
					continue;
				}

				name = transformer.transform( demangler.demangle( name ) );
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
	filter.addPartial( "std_" );
	filter.addPartial( "CGReforge" );
	filter.addPartial( "blz::" );
	filter.addPartial( "blz_" );
	filter.addPartial( "CGResearchFrame" );
	filter.addPartial( "TSSimpleArray" );
	filter.addPartial( "CPetBattleScene" );
	filter.addPartial( "MemLineNo" );
	filter.addPartial( "MemFileName" );
	filter.addPartial( "CheckArrayBounds" );
	filter.addPartial( "::CliPut" );
	filter.addPartial( "JamCli" );
	filter.addPartial( "AnimData_" );
	filter.addPartial( "CGTransmogrify" );
	filter.addPartial( "UserRouterClientAuthSession" );
	filter.addPartial( "UserGuildSetGuildMaster" );
	filter.addPartial( "UserClientWorldPortResponse" );
	filter.addPartial( "UserClientSendMail" );
	filter.addPartial( "UserClientDBQueryBulk" );
	filter.addPartial( "UserClientCommentatorStartWargame" );
	filter.addPartial( "MacClient" );
	filter.addPartial( "JamAllAchievements" );
	filter.addPartial( "GlobalGuildDemoteMember" );
	filter.addPartial( "GlobalGuildOfficerRemoveMember" );
	filter.addPartial( "GlobalGuildPromoteMember" );
	filter.addPartial( "GLMipmap" );
	filter.addPartial( "GLFrameSerialization" );
	filter.addPartial( "GLFrameReplayer" );
	filter.addPartial( "GLBatchViewer" );
	filter.addPartial( "GLBatch" );
	filter.addPartial( "ClientRealmResponseAdapter" );
	filter.addPartial( "ClientPhaseShiftChange" );
	filter.addPartial( "CGGlyphInfo" );
	filter.addPartial( "CActor" );
	filter.addPartial( "CActorManager" );
	filter.addPartial( "CActorMapGrid" );
	filter.addPartial( "CActorScene" );
	filter.addPartial( "CActorUnit" );
	filter.addPartial( "CAnimKitManager" );
	filter.addPartial( "CBoneGenerator" );
	filter.addPartial( "CClientLiquidEnvironment" );
	filter.addPartial( "CDataRecycler" );
	filter.addPartial( "CFlyout" );
	filter.addPartial( "CGBlackMarketInfo" );
	filter.addPartial( "CGCUFProfiles" );
	filter.addPartial( "CGEncounterJournal" );
	filter.addPartial( "CGPetBattleInfo" );
	filter.addPartial( "CGSpecializationInfo" );
	filter.addPartial( "CGScenarioPOIFrame" );
	filter.addPartial( "CGVoidStorageInfo" );
	filter.addPartial( "CMinigameTicTacToe_C" );
	filter.addPartial( "CWowMouseMac" );
	filter.addPartial( "CachedFrameBuffer" );
	filter.addPartial( "CliChat" );
	filter.addPartial( "ClientAllAchievementData" );
	filter.addPartial( "ClientBattlefield" );
	filter.addPartial( "ClientClearQuestCompletedBits" );
	filter.addPartial( "ClientGuildRewardList" );
	filter.addPartial( "ClientHotfixNotifyBlob" );
	filter.addPartial( "ClientItemPushResult" );
	filter.addPartial( "ClientLFGJoinResult" );
	filter.addPartial( "ClientLFGUpdateStatus" );
	filter.addPartial( "ClientLFGuildRecruits" );
	filter.addPartial( "ClientLearnedSpells" );
	filter.addPartial( "ClientMapObjEvents" );
	filter.addPartial( "ClientPetBattleFinalRound" );
	filter.addPartial( "ClientPetBattleRoundResult" );
	filter.addPartial( "ClientRaidMarkersChanged" );
	filter.addPartial( "ClientRealmResponseAdapter" );
	filter.addPartial( "ClientSceneObjectEvent" );
	filter.addPartial( "CliParse" );
	filter.addPartial( "GLCommand" );
	filter.addPartial( "GLLayerMovieFrameSettings" );
	filter.addPartial( "GLObjectManager" );
	filter.addPartial( "GLOcclusionQuery" );
	filter.addPartial( "GLPixelShader" );
	filter.addPartial( "GLRenderbuffer" );
	filter.addPartial( "GLTexture" );
	filter.addPartial( "GLVertexArray" );
	filter.addPartial( "GLVertexShader" );
	filter.addPartial( "GLView" );
	filter.addPartial( "GLWindow" );
	filter.addPartial( "GLWorker" );
	filter.addPartial( "GeneralFunction" );
	filter.addPartial( "GlobalGuild" );
	filter.addPartial( "Mac::" );
	filter.addPartial( "MainApp" );
	filter.addPartial( "PathDB" );
	filter.addPartial( "PlayerBattlefield" );
	filter.addPartial( "PlayerBlackMarketOpen" );
	filter.addPartial( "PlayerCli" );
	filter.addPartial( "PlayerInitiateTrade" );
	filter.addPartial( "PlayerLearnTalents" );
	filter.addPartial( "PlayerMoveWaterWalkAck" );
	filter.addPartial( "PlayerQueryQuestCompletionNPCs" );
	filter.addPartial( "PlayerRequestCategoryCooldowns" );
	filter.addPartial( "PlayerSaveCUFProfiles" );
	filter.addPartial( "PlayerSetSelection" );
	filter.addPartial( "SSAO" );
	filter.addPartial( "UserClientCommentatorStartWargame" );
	filter.addPartial( "UserClientDFSearchJoin" );
	filter.addPartial( "UserClientDFSetRoles" );
	filter.addPartial( "UserClientDFTeleport" );
	filter.addPartial( "UserClientGuildInviteByName" );
	filter.addPartial( "UserClientSendMail" );
	filter.addPartial( "UserClientSetActionButton" );
	filter.addPartial( "UserClientSupportTicketSubmitComplaint" );
	filter.addPartial( "UserClientWorldPortResponse" );
	filter.addPartial( "UserRouterClientAuthSession" );
	filter.addPartial( "CGPetJournalInfo" );
	filter.addPartial( "CGSceneObject_C" );
	filter.addPartial( "CGSpellActivationOverlay" );
	filter.addPartial( "CScriptActor" );
	filter.addPartial( "CScriptEvent" );
	filter.addPartial( "ChatWindowInfo" );
	filter.addPartial( "ClampCursorInternal" );
	filter.addPartial( "ClientAccountInfoResponse" );
	filter.addPartial( "ClientAllGuildAchievements" );
	filter.addPartial( "ClientBattlePetUpdates" );
	filter.addPartial( "ClientBattlePetXP" );
	filter.addPartial( "ClientBattlegroundPlayerPositions" );
	filter.addPartial( "ClientCategoryCooldown" );
	filter.addPartial( "ClientChallengeModeComplete" );
	filter.addPartial( "ClientEnumCharactersResult" );
	filter.addPartial( "ClientGameEventDebugLog" );
	filter.addPartial( "ClientGuildBankQueryResults" );
	filter.addPartial( "ClientEnumCharactersResult" );
	filter.addPartial( "ClientGameEventDebugLog" );
	filter.addPartial( "ClientGuildBankQueryResults" );
	filter.addPartial( "ClientGuildEventLogQueryResults" );
	filter.addPartial( "ClientGuildMoved" );
	filter.addPartial( "ClientGuildRanks" );
	filter.addPartial( "ClientInstanceInfo" );
	filter.addPartial( "ClientLFGuildBrowse" );
	filter.addPartial( "ClientLFGuildPost" );
	filter.addPartial( "ClientMoveSetCompoundState" );
	filter.addPartial( "ClientMoveSplineSetSwimSpeed" );
	filter.addPartial( "ClientPVPLogData" );
	filter.addPartial( "ClientPartyInvite" );
	filter.addPartial( "ClientPetAdded" );
	filter.addPartial( "ClientPetBattleReplacementsMade" );
	filter.addPartial( "ClientQuestCompletionNPCResponse" );
	filter.addPartial( "ClientRequestCemeteryListResponse" );
	filter.addPartial( "ClientResetAreaTrigger" );
	filter.addPartial( "ClientServerPerf" );
	filter.addPartial( "ClientVendorInventory" );
	filter.addPartial( "ClientVoidStorageContents" );
	filter.addPartial( "ClientVoidStorageTransferChanges" );
	filter.addPartial( "GLFence" );
	filter.addPartial( "GLFramebuffer" );
	filter.addPartial( "GLGLSLProgram" );
	filter.addPartial( "GLGenCommandCmdType" );
	filter.addPartial( "GLLayerGameFrame" );
	filter.addPartial( "GLPixelGLSLShader" );
	filter.addPartial( "GLShader" );
	filter.addPartial( "GLShaderCompiler" );
	filter.addPartial( "GLVertexGLSLShader" );
	filter.addPartial( "GameAllocator" );
	filter.addPartial( "JamGuildMemberSetNoteData" );
	filter.addPartial( "JamOfsConst" );
	filter.addPartial( "JamPartyMemberState" );
	filter.addPartial( "JamPetBattleFinalRound" );
	filter.addPartial( "PacketPipe_CliSvr" );
	filter.addPartial( "Physics::" );
	filter.addPartial( "PlayerMoveCharmTeleportCheat" );
	filter.addPartial( "PlayerRequestCemeteryList" );
	filter.addPartial( "PlayerSetDifficultyID" );
	filter.addPartial( "ScriptUtil" );
	filter.addPartial( "JamGuildBankItemInfo" );
	filter.addPartial( "KinematicGroup" );
	filter.addPartial( "HostDebugRepData" );
	filter.addPartial( "GLVariableArray" );
	filter.addPartial( "GLQuickFormat" );
	filter.addPartial( "GLLayerMovieFrame" );
	filter.addPartial( "GLContext" );
	filter.addPartial( "FrameBufferCache" );
	filter.addPartial( "EffectUnconscious" );
	filter.addPartial( "ClientSupercededSpells" );
	filter.addPartial( "ClientStartElapsedTimers" );
	filter.addPartial( "ClientSetupCurrency" );
	filter.addPartial( "ClientDBReply" );
	filter.addPartial( "ClientDumpRideTicketsResponse" );
	filter.addPartial( "ClientGuildAchievementMembers" );
	filter.addPartial( "ClientGuildBankLogQueryResults" );
	filter.addPartial( "ClientGuildBankTextQueryResult" );
	filter.addPartial( "ClientGuildCriteriaUpdate" );
	filter.addPartial( "ClientGuildKnownRecipes" );
	filter.addPartial( "ClientGuildPermissionsQueryResults" );
	filter.addPartial( "ClientInitialSetup" );
	filter.addPartial( "ClientLFGSearchResults" );
	filter.addPartial( "ClientLootItemList" );
	filter.addPartial( "ClientLootResponse" );
	filter.addPartial( "ClientMasterLootCandidateList" );
	filter.addPartial( "ClientMoveSplineDisableCollision" );
	filter.addPartial( "ClientMoveUpdateRemoveMovementForce" );
	filter.addPartial( "ClientPrintNotification" );
	filter.addPartial( "ClientServerInfoResponse" );
	filter.addPartial( "RangedMemManager" );
	filter.addPartial( "GarrisonInfo" );
	filter.addPartial( "GLObjectMule" );
	filter.addPartial( "GGCP" );
	filter.addPartial( "FileNotification" );
	filter.addPartial( "Archaeology" );


	
	transformer.addRule( "__jump_table::_strcasecmp", "SStrCmp" );
	transformer.addRule( "__jump_table::strchr", "SStrChr" );
	transformer.addRule( "__jump_table::strcpy", "SStrCopy" );
	transformer.addRule( "__jump_table::strlen", "SStrLen" );
	transformer.addRule( "__jump_table::strncasecmp", "SStrCmpI" );
	transformer.addRule( "__jump_table::strrchr", "SStrChrR" );
	transformer.addRule( "__jump_table::strstr", "SStrStr" );
	transformer.addRule( "__jump_table::_gl", "gl" );
	transformer.addRule( "__jump_table::", "" );
	transformer.addRule( "_SErr", "SErr" );
	transformer.addRule( "NTempest::", "" );
	transformer.addRule( "SE3", "SE2" );
	transformer.addRule( "SI3", "SI2" );
	transformer.addRule( "BattlenetUI::Script", "Script" );
	transformer.addRule( "CGCommentator::Script", "Script" );
	transformer.addRule( "CWorldMap", "CMap" );
	transformer.addRule( "WowClientDB2", "WowClientDB" );
	transformer.addRule( "CUnitDisplay", "CGUnit_C" );
	transformer.addRule( "::", "__" );
	transformer.addRule( "_ptr_", "_" );
	transformer.addRule( "___", "__" );
	transformer.addRegexRule( R"(\b(struct|class)_)", "" );
	transformer.addRegexRule( R"((\w+)<([^,>]+)[^>]*>)", "$1__$2" );
	transformer.addRegexRule( R"((\w+)(.*?)(::|__)\1$)", "$1$2$3constructor" );
	transformer.addRegexRule( R"((\w+)(.*?)(::|__)~\1$)", "$1$2$3destructor" );
	transformer.addRegexRule( R"((\w+)<([^<>,]+)[^<>]*>)", "$1__$2" );
	transformer.addRegexRule( R"((\w+)<([^<>,]+)[^<>]*>)", "$1__$2" );
	transformer.addRule( "TSFixedArray_", "TSGrowableArray_" );
	transformer.addRule( "`scalar_deleting_destructor'", "destructor" );
	transformer.addRule( "`vector_deleting_destructor'", "destructor" );
	transformer.addRule( "__", "_" );
	transformer.addRule( ">_", "_" );
	transformer.addRule( "*", "" );
	transformer.addRule( "_lua", "lua" );
	transformer.addRule( "unsigned int", "uint32" );
	transformer.addRule( "unsigned char", "uint8" );
	transformer.addRule( "signed char", "int8" );
	transformer.addRule( "unsigned short", "uint16" );
	transformer.addRule( "signed short", "int16" );
	transformer.addRule( "signed int", "int32" );
	transformer.addRule( "unsigned long", "uint64" );
	transformer.addRule( "signed long", "int64" );
	transformer.addRule( "long long", "int64" );
	transformer.addRule( "unsigned long long", "uint64" );
	transformer.addRule( "unsigned long int", "uint64" );
	transformer.addRule( "signed long int", "int64" );
	transformer.addRule( "unsigned long long int", "uint64" );
	transformer.addRule( "FrameTime_GetCurTimeMs", "OsGetAsyncTimeMs" );
	transformer.addRule( "FID_conflict:", "" );
	transformer.addRule( "CMovement_C", "CMovement" );


	transformer.addRule( []( const std::string &input ) {
		static const std::vector<std::pair<std::string, std::string>> op_map = {
			{ "()", "call" },	{ "*=", "mul_assign" }, { "+=", "add_on_assign" },
			{ "++", "inc" },	{ "<=", "le" },			{ "==", "eq" },
			{ ">=", "ge" },		{ "!=", "ne" },			{ "<<", "lshift" },
			{ ">>", "rshift" }, { "-", "sub" },			{ "*", "mul" },
			{ "+", "add" },		{ "<", "lt" },			{ ">", "gt" },
			{ "=", "assign" },	{ "[]", "subscript" },
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
		auto groupFile = args.get( "group", "" );

		std::cout << "=== Function Matcher Started ===" << std::endl;

		auto g1_json = load_json( pathJSONRef );
		auto g2_json = load_json( pathJSON335 );

		std::cout << "[LOG] Parsing JSON structures..." << std::endl;
		auto g1 = load_functions_from_json_with_processing( g1_json );
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

		if( !groupFile.empty() )
		{
			ClassGroupWriter groupWriter;
			groupWriter.export_grouped( groupFile, sortedPairs );
		}
	}
	catch( const std::exception &e )
	{
		std::cerr << "[ERROR] " << e.what() << std::endl;
		return 1;
	}
	return 0;
}