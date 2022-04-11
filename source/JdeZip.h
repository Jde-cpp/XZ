#pragma once
#include "./Exports.h"

namespace Jde::IO::Zip::GZip
{
	JDE_XZ std::stringstream Read( std::istream& is )noexcept(false);
};
