#pragma once
#include "Exports.h"
namespace Jde{ struct Stopwatch; }
namespace Jde::IO::Zip::XZ
{
	JDE_XZ up<vector<char>> Read( path path )noexcept(false);
	JDE_XZ up<vector<char>> Read( std::istream& is, uint size )noexcept(false);
	//PRESET is a number 0-9 and can optionally be  followed by `e' to indicate extreme preset
	JDE_XZ void Write( path path, const vector<char>& bytes, uint32_t preset=6 )noexcept(false);
	JDE_XZ void Write( path path, string&& data, uint32_t preset=6 )noexcept(false);
	JDE_XZ uint Write( std::ostream& os, const char* pBytes, uint size, uint32_t preset=6, Stopwatch* pStopwatch=nullptr )noexcept(false);
	inline uint Write( std::ostream& os, string&& data, uint32_t preset=6 )noexcept(false){ return Write( os, data.data(), data.size(), preset ); }
	JDE_XZ up<vector<char>> Compress( str bytes, uint32_t preset=6 )noexcept(false);
	JDE_XZ bool compress();
}