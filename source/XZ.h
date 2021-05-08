#pragma once
#include "Exports.h"
#include "../../Framework/source/TypeDefs.h"
namespace Jde{ struct Stopwatch; }
namespace Jde::IO::Zip::XZ
{
#define 🚀 JDE_XZ auto
	🚀 Read( path path )noexcept(false)->up<vector<char>> ;
	🚀 Read( std::istream& is, uint size )noexcept(false)->up<vector<char>> ;
	//PRESET is a number 0-9 and can optionally be  followed by `e' to indicate extreme preset
	🚀 Write( path path, const vector<char>& bytes, uint32_t preset=6 )noexcept(false)->void;
	🚀 Write( path path, string&& data, uint32_t preset=6 )noexcept(false)->void;
	🚀 Write( std::ostream& os, const char* pBytes, uint size, uint32_t preset=6, Stopwatch* pStopwatch=nullptr )noexcept(false)->uint;
	📎 Write( std::ostream& os, string&& data, uint32_t preset=6 )noexcept(false)->uint{ return Write( os, data.data(), data.size(), preset ); }
	🚀 Compress( str bytes, uint32_t preset=6 )noexcept(false)->up<vector<char>>;
	//✈compress()->bool;
#undef 🚀
}