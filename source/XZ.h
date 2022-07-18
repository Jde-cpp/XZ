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
		Φ Read( path path )noexcept(false)->up<vector<char>>;
		Φ Read( std::istream& is, uint size )noexcept(false)->up<vector<char>>;
		α Read( uint8_t* pInput, uint size )noexcept(false)->up<vector<char>>;
		Φ CoRead( fs::path path, bool cache=false )noexcept(false)->AsyncAwait;//vector<char>;
		Φ CoRead( vector<char>&& compressed )noexcept(false)->TPoolAwait<vector<char>>;//vector<char>
		Ŧ ReadProto( fs::path path )noexcept->AsyncAwait;//vector<char>;
		Φ Write( path path, const vector<char>& bytes, uint32_t preset=6 )noexcept(false)->void;//PRESET=0-9 and can optionally be  followed by `e' to indicate extreme preset
		Φ Write( path path, string&& data, uint32_t preset=6 )noexcept(false)->void;
		Φ Write( std::ostream& os, const char* pBytes, uint size, uint32_t preset=6, Stopwatch* pStopwatch=nullptr )noexcept(false)->uint;
		Ξ Write( std::ostream& os, string&& data, uint32_t preset=6 )noexcept(false)->uint{ return Write( os, data.data(), data.size(), preset ); }
		Φ Compress( str bytes, uint32_t preset=6 )noexcept(false)->up<vector<char>>;
	#undef Φ
	}
	Ŧ XZ::ReadProto( fs::path path_ )noexcept->AsyncAwait
	{
		return AsyncAwait{ [path=move(path_)]( HCoroutine h )->Task
		{
			AwaitResult t = co_await CoRead( path );
			try
			{
				var pBytes = t.UP<vector<char>>();
				if( !pBytes->size() )
				{
					fs::remove( path );
					throw IOException( path, "deleted, has 0 bytes." );
				}
				h.promise().get_return_object().SetResult( IO::Proto::Deserialize<T>(*pBytes) );
			}
			catch( IException& e )
			{
				h.promise().get_return_object().SetResult( move(e) );
			}
			h.resume();
		} };
	}
	#undef var
}