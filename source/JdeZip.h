#pragma once
#include "./Exports.h"

namespace Jde::IO::Zip::GZip
{
#ifndef _MSC_VER
	JDE_XZ std::stringstream Read( std::istream& is )noexcept(false);
#endif
};
