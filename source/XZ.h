#pragma once
#include "Exports.h"

namespace Jde::IO::Zip::XZ
{
	namespace fs=std::filesystem;

	JDE_XZ std::unique_ptr<std::vector<char>> Read( const fs::path& path )noexcept(false);
	//PRESET is a number 0-9 and can optionally be  followed by `e' to indicate extreme preset
	JDE_XZ void Write( const fs::path& path, const std::vector<char>& bytes, uint32_t preset=6 )noexcept(false);
	JDE_XZ void Write( std::ostream& os, const char* pBytes, uint size, uint32_t preset=6 )noexcept(false);
	JDE_XZ bool compress();
}