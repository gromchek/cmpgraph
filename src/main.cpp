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

void printFunctions( const std::vector<Function> &baseFuncs, const std::vector<Function> &refFuncs,
					 const std::string &fileName )
{
	std::ofstream out( fileName );
	for( const auto &refFunc : refFuncs )
	{
		if( refFunc.baseName.rfind( "FUN_", 0 ) == 0 )
		{
			continue;
		}

		auto it = std::find_if( baseFuncs.begin(), baseFuncs.end(),
								[&refFunc]( const Function &f ) { return f.name == refFunc.name; } );
		if( it != baseFuncs.end() )
		{
			continue;
		}

		out << refFunc.baseName << ' ' << refFunc.name << '\n';
	}
}

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

	std::unordered_map<std::string, std::string> transform_cache;

	auto cached_transform = [&]( const std::string &input ) -> std::string {
		auto it = transform_cache.find( input );
		if( it != transform_cache.end() )
		{
			return it->second;
		}
		std::string result = transformer.transform( input );
		transform_cache[input] = result;
		return result;
	};

	for( const auto &item : functions_array )
	{
		Function func;
		func.baseName = item.value( "name", "" );

		if( filter.match( func.baseName ) )
		{
			continue;
		}

		func.name = cached_transform( func.baseName );
		func.name = cached_transform( demangler.demangle( func.name ) );

		if( item.contains( "called_names" ) )
		{
			for( const auto &called : item["called_names"] )
			{
				auto name = cached_transform( called.get<std::string>() );
				if( filter.match( name ) )
				{
					continue;
				}

				name = cached_transform( demangler.demangle( name ) );
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
				auto name = cached_transform( caller.get<std::string>() );
				if( filter.match( name ) )
				{
					continue;
				}

				name = cached_transform( demangler.demangle( name ) );
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
	filter.addPartial( "UpdateCamouflage" );
	filter.addPartial( "Script_CagePetByID" );
	filter.addPartial( "UserClientCommentatorGetPartyInfo" );
	filter.addPartial( "UserClientRequestPartyJoinUpdates" );
	filter.addPartial( "___tcf_" );
	filter.addPartial( "Transmog" );
	filter.addPartial( "Reforge" );
	filter.addPartial( "VoidStorage" );
	filter.addPartial( "__static_initialization_and_destruction" );
	filter.addPartial( "ClientAllAccountCriteria" );
	filter.addPartial( "ClientAuthResponse" );
	filter.addPartial( "ClientBattlePetError" );
	filter.addPartial( "ClientChallegeModeRewards" );
	filter.addPartial( "ClientChallengeModeAllMapStats" );
	filter.addPartial( "ClientCommentatorPartyInfo" );
	filter.addPartial( "RealmCache" );
	filter.addPartial( "$E" );
	filter.addPartial( "__GLOBAL__I_a" );
	filter.addPartial( "BattlePet" );
	filter.addPartial( "VoidWithdraw" );
	filter.addPartial( "VoidDeposit" );
	filter.addPartial( "Artifac" );
	filter.addPartial( "GLApplicationCommandLine" );
	filter.addPartial( "GLApplicationLoad" );
	filter.addPartial( "GLApplicationUnload" );
	filter.addPartial( "GLBaseCommand" );
	filter.addPartial( "GLBuffer" );
	filter.addPartial( "GLBufferData" );
	filter.addPartial( "GLBufferPool" );
	filter.addPartial( "GLClientStorageBufferAllocCmdType" );
	filter.addPartial( "GLCustomCommandsInit" );
	filter.addPartial( "GLDebugMipmap2D" );
	filter.addPartial( "GLDeleteCommandCmdType" );
	filter.addPartial( "GLDevice" );
	filter.addPartial( "GLDisplayGetCurrentDisplayMode" );
	filter.addPartial( "GLDisplayGetDesktopDisplayMode" );
	filter.addPartial( "GLDisplayGetDisplayModes" );
	filter.addPartial( "GLDisplayModeEqual" );
	filter.addPartial( "GLDisplayModeLessThan" );
	filter.addPartial( "GLDisplaySwitchDisplayMode" );
	filter.addPartial( "GLDisplaySwitchToDesktopMode" );
	filter.addPartial( "GLFlush" );
	filter.addPartial( "GLGetImageSize" );
	filter.addPartial( "GLGetNSViewFrame" );
	filter.addPartial( "GLLayerYUV420Buffer" );
	filter.addPartial( "GLNSViewAddExecute" );
	filter.addPartial( "GLNSViewRemoveExecute" );
	filter.addPartial( "GLNSViewUpdateExecute" );
	filter.addPartial( "GLObject" );
	filter.addPartial( "GLParamParser" );
	filter.addPartial( "GLPixelFormatAllocExecute" );
	filter.addPartial( "GLPixelFormatReleaseExecute" );
	filter.addPartial( "GLPool" );
	filter.addPartial( "GLPoolAddName" );
	filter.addPartial( "GLPoolGetNextGLName" );
	filter.addPartial( "GLPoolRemoveName" );
	filter.addPartial( "GLReclyclableObject" );
	filter.addPartial( "GLRendererInfo" );
	filter.addPartial( "GLScopedAutoreleasePool" );
	filter.addPartial( "GLStates" );
	filter.addPartial( "GLTexDeleteGL" );
	filter.addPartial( "GLTexDestroy" );
	filter.addPartial( "GLTexUnmap" );
	filter.addPartial( "GLTransform" );
	filter.addPartial( "GLVertexFormat" );
	filter.addPartial( "GLZipFile" );
	filter.addPartial( "GLZippedFile" );
	filter.addPartial( "GLAbstractWindow" );
	filter.addPartial( "_vp8_" );
	filter.addPartial( "_vp9_" );
	filter.addPartial( "_vpx_" );
	filter.addPartial( "tact::" );
	filter.addPartial( "Borg::" );
	filter.addPartial( "_Curl_" );
	filter.addPartial( "AccountObjectSetProfile" );
	filter.addPartial( "AIObstacleMgr" );
	filter.addPartial( "AgentConnection" );
	filter.addPartial( "BodyCount" );
	filter.addPartial( "CAnimKitInst" );
	filter.addPartial( "CAnimatedDisplay" );
	filter.addPartial( "CCharacterServices" );
	filter.addPartial( "CClippedTriangle" );
	filter.addPartial( "CGChallengeModeInfo" );
	filter.addPartial( "CGCinematicModelFrame" );
	filter.addPartial( "CGSpecGroup_C" );
	filter.addPartial( "CGVignetteInfo" );
	filter.addPartial( "CPerf" );
	filter.addPartial( "CScriptUnitDisplay" );
	filter.addPartial( "CSimpleBrowser" );
	filter.addPartial( "CUnitDisplayTransition_C" );
	filter.addPartial( "CVideoDecoderVPX" );
	filter.addPartial( "CWOWClientStatus" );
	filter.addPartial( "CXpcMessage" );
	filter.addPartial( "Chaser<" );
	filter.addPartial( "ClientBattlenetChallengeStart" );
	filter.addPartial( "ClientClientHandlerReporter" );
	filter.addPartial( "ClientDispelFailed" );
	filter.addPartial( "ClientGuildEventTabModified" );
	filter.addPartial( "ClientLfgPlayerInfo" );
	filter.addPartial( "ClientLogoutComplete" );
	filter.addPartial( "ClientLossOfControlAuraUpdate" );
	filter.addPartial( "ClientMessageBox" );
	filter.addPartial( "ClientMoveUpdatePitchRate" );
	filter.addPartial( "ClientServerBuckData" );
	filter.addPartial( "DebugChallengeCreateCallback" );
	filter.addPartial( "DebugClientCommands" );
	filter.addPartial( "DebugDraw" );
	filter.addPartial( "DefaultTextureCache" );
	filter.addPartial( "EffectAntiSpoof" );
	filter.addPartial( "FourCC" );
	filter.addPartial( "IMoviePlayer" );
	filter.addPartial( "::Jam" );
	filter.addPartial( "NewsDownloadTask" );
	filter.addPartial( "PCResultsIterator" );
	filter.addPartial( "PassCMAADbgShowEdges" );
	filter.addPartial( "PerfMgr" );
	filter.addPartial( "PlayerMove" );
	filter.addPartial( "PlayerMove" );
	filter.addPartial( "UserClient" );
	filter.addPartial( "ViewSettings" );

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
	transformer.addRule( "(anonymous_namespace)::", "" );
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
	transformer.addRule( "_MD5", "MD5" );
	transformer.addRegexRule( R"(\b(struct|class)_)", "" );
	transformer.addRegexRule( R"((\w+)<([^,>]+)[^>]*>)", "$1__$2" );
	transformer.addRegexRule( R"((\w+)(.*?)(::|__)\1$)", "$1$2$3constructor" );
	transformer.addRegexRule( R"((\w+)(.*?)(::|__)~\1$)", "$1$2$3destructor" );
	transformer.addRegexRule( R"((\w+)<([^<>,]+)[^<>]*>)", "$1__$2" );
	transformer.addRegexRule( R"((\w+)<([^<>,]+)[^<>]*>)", "$1__$2" );
	transformer.addRule( "TSFixedArray_", "TSGrowableArray_" );
	transformer.addRule( "TSBaseArray_", "TSGrowableArray_" );
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
	transformer.addRule( "unsigned_int", "uint32" );
	transformer.addRule( "unsigned_char", "uint8" );
	transformer.addRule( "signed_char", "int8" );
	transformer.addRule( "unsigned_short", "uint16" );
	transformer.addRule( "signed_short", "int16" );
	transformer.addRule( "signed_int", "int32" );
	transformer.addRule( "unsigned_long", "uint64" );
	transformer.addRule( "signed_long", "int64" );
	transformer.addRule( "long_long", "int64" );
	transformer.addRule( "unsigned_long_long", "uint64" );
	transformer.addRule( "unsigned_long_int", "uint64" );
	transformer.addRule( "signed_long_int", "int64" );
	transformer.addRule( "unsigned_long_long_int", "uint64" );
	transformer.addRule( "FrameTime_GetCurTimeMs", "OsGetAsyncTimeMs" );
	transformer.addRule( "FID_conflict:", "" );
	transformer.addRule( "CMovement_C", "CMovement" );
	transformer.addRule( "_zlib_uncompress@16", "_zlib_uncompress" );
	transformer.addRule( "_zlib_compress@20", "_zlib_compress" );
	transformer.addRule( "@z_inflateEnd@4", "_inflateEnd" );
	transformer.addRule( "@z_inflateInit2_@16", "_inflateInit2_" );
	transformer.addRule( "@z_inflateInit_@12", "_inflateInit_" );
	transformer.addRule( "@z_inflate@8", "_inflate" );
	transformer.addRule( "@z_inflateSetDictionary@12", "_inflateSetDictionary" );
	transformer.addRule( "@z_inflateSync@4", "_inflateSync" );
	transformer.addRule( "@z_deflateInit_@16", "_deflateInit_" );
	transformer.addRule( "@z_deflateInit2_@32", "_deflateInit2_" );
	transformer.addRule( "@z_deflate@8", "_deflate" );
	transformer.addRule( "@z_deflateEnd@4", "_deflateEnd" );
	transformer.addRule( "@zcalloc@12", "_zcalloc" );
	transformer.addRule( "_SCrcBuffer@16", "_SCrcBuffer" );
	transformer.addRule( "operator.new", "operator_new" );
	transformer.addRule( "operator.delete", "operator_delete" );
	transformer.addRule( "OpenGl_I", "OpenGl_" );
	transformer.addRule( "Blizzard_Lock_MutexLeave", "LeaveCriticalSection" );
	transformer.addRule( "Blizzard_Lock_MutexEnter", "EnterCriticalSection" );
	transformer.addRule( "Blizzard_Process_Sleep", "Sleep" );
	transformer.addRule( "Blizzard_Lock_MutexDestroy", "DeleteCriticalSection" );
	transformer.addRule( "Blizzard_Lock_MutexCreate", "InitializeCriticalSection" );
	transformer.addRule( "Blizzard_Mopaq_SFileCreateFile", "SFileCreateFile" );
	transformer.addRule( "CObjectDisplay", "CGObject_C" );


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
		auto onlyPrintFuncs = args.getBool( "onlyPrintFuncs", false );
		auto groupFile = args.get( "group", "" );

		std::cout << "=== Function Matcher Started ===" << std::endl;

		auto g1_json = load_json( pathJSONRef );
		auto g2_json = load_json( pathJSON335 );

		std::cout << "[LOG] Parsing JSON structures..." << std::endl;
		auto g1 = load_functions_from_json_with_processing( g1_json );
		auto g2 = load_functions_from_json( g2_json );
		std::cout << "[LOG] Graphs loaded. G1 size: " << g1.size() << ", G2 size: " << g2.size() << std::endl;

		if( onlyPrintFuncs )
		{
			printFunctions( g2, g1, pathJSONRef + ".formated.txt" );
			return 0;
		}

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