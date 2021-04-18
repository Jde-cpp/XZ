#pragma once
#include "Exports.h"
namespace Jde{ struct Stopwatch; }

namespace Jde::IO::Zip::XZ
{
	constexpr uint y = 42;
	namespace fs=std::filesystem;

	JDE_XZ std::unique_ptr<std::vector<char>> Read( const fs::path& path )noexcept(false);
	JDE_XZ unique_ptr<vector<char>> Read( std::istream& is, uint size )noexcept(false);
	//PRESET is a number 0-9 and can optionally be  followed by `e' to indicate extreme preset
	JDE_XZ void Write( const fs::path& path, const std::vector<char>& bytes, uint32_t preset=6 )noexcept(false);
	JDE_XZ void Write( const fs::path& path, const string& bytes, uint32_t preset=6 )noexcept(false);
	JDE_XZ uint Write( std::ostream& os, const char* pBytes, uint size, uint32_t preset=6, Stopwatch* pStopwatch=nullptr )noexcept(false);
	inline uint Write( std::ostream& os, string&& data, uint32_t preset=6 )noexcept(false){ return Write( os, data.data(), data.size(), preset ); }
	JDE_XZ up<vector<char>> Compress( const string& bytes, uint32_t preset=6 )noexcept(false);
	JDE_XZ bool compress();
}