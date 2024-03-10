#pragma once
#include "Exports.h"
#include <jde/TypeDefs.h>
#include "../../Framework/source/coroutine/Awaitable.h"
#include "../../Framework/source/io/ProtoUtilities.h"

#define var const auto
namespace Jde{ struct Stopwatch; }
namespace Jde::IO::Zip
{
	using namespace Coroutine;
	namespace XZ
	{
	#define Φ JDE_XZ α
		Φ Read( const fs::path& path )ε->up<vector<char>>;
		Φ Read( std::istream& is, uint size )ε->up<vector<char>>;
		α Read( uint8_t* pInput, uint size )ε->up<vector<char>>;
		Φ CoRead( fs::path path, bool cache=false )ε->AsyncAwait;//vector<char>;
		Φ CoRead( vector<char>&& compressed )ε->TPoolAwait<vector<char>>;//vector<char>
		Ŧ ReadProto( fs::path path )ι->AsyncAwait;//vector<char>;
		Φ Write( const fs::path& path, const vector<char>& bytes, uint32_t preset=6 )ε->void;//PRESET=0-9 and can optionally be  followed by `e' to indicate extreme preset
		Φ Write( const fs::path& path, string&& data, uint32_t preset=6 )ε->void;
		Φ Write( std::ostream& os, const char* pBytes, uint size, uint32_t preset=6, Stopwatch* pStopwatch=nullptr )ε->uint;
		Ξ Write( std::ostream& os, string&& data, uint32_t preset=6 )ε->uint{ return Write( os, data.data(), data.size(), preset ); }
		Φ Compress( str bytes, uint32_t preset=6 )ε->up<vector<char>>;
	#undef Φ
	}
	Ŧ XZ::ReadProto( fs::path path_ )ι->AsyncAwait{
		return AsyncAwait{ [path=move(path_)]( HCoroutine h )->Task {
			AwaitResult t = co_await CoRead( path );
			try{
				var pBytes = t.UP<vector<char>>();
				if( !pBytes->size() ){
					fs::remove( path );
					throw IOException( path, "deleted, has 0 bytes." );
				}
				Resume( IO::Proto::Deserialize<T>(*pBytes), move(h) );
			}
			catch( IException& e ){
				Resume( move(e), move(h) );
			}
		} };
	}
	#undef var
}