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
	#define 🚪 JDE_XZ auto
		🚪 Read( path path )noexcept(false)->up<vector<char>>;
		🚪 Read( std::istream& is, uint size )noexcept(false)->up<vector<char>>;
		α Read( uint8_t* pInput, uint size )noexcept(false)->up<vector<char>>;
		🚪 CoRead( path path )noexcept(false)->FunctionAwaitable;//vector<char>;
		🚪 CoRead( vector<char>&& compressed )noexcept(false)->AsyncAwaitable;//vector<char>
		ⓣ ReadProto( fs::path path )noexcept->AWrapper;//vector<char>;
		🚪 Write( path path, const vector<char>& bytes, uint32_t preset=6 )noexcept(false)->void;//PRESET=0-9 and can optionally be  followed by `e' to indicate extreme preset
		🚪 Write( path path, string&& data, uint32_t preset=6 )noexcept(false)->void;
		🚪 Write( std::ostream& os, const char* pBytes, uint size, uint32_t preset=6, Stopwatch* pStopwatch=nullptr )noexcept(false)->uint;
		Ξ Write( std::ostream& os, string&& data, uint32_t preset=6 )noexcept(false)->uint{ return Write( os, data.data(), data.size(), preset ); }
		🚪 Compress( str bytes, uint32_t preset=6 )noexcept(false)->up<vector<char>>;
	#undef 🚪
	}
	ⓣ XZ::ReadProto( fs::path path )noexcept->AWrapper
	{
		return AWrapper{ [path2=move(path)]( HCoroutine h )->Task2
		{
			TaskResult t = co_await CoRead( path2 );
			try
			{
				var pBytes = t.Get<vector<char>>();
				if( !pBytes->size() )
				{
					fs::remove( path2 );
					THROWX( IOException(path2, "deleted, has 0 bytes.") );
				}
				h.promise().get_return_object().SetResult( sp<T>(IO::Proto::Deserialize<T>(*pBytes).release()) );
			}
			catch( const std::exception& e )
			{
				h.promise().get_return_object().SetResult( std::make_exception_ptr(e) );
			}
			h.resume();
		} };
	}
	#undef var
}