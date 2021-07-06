#ifndef _MSC_VER
#include "JdeZip.h"
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/copy.hpp>

namespace Jde::IO::Zip::GZip
{
	std::stringstream Read( std::istream& is )noexcept(false)
	{
		boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
		boost::iostreams::gzip_decompressor x;
		in.push( x );
		in.push( is );
		std::stringstream xyz;
		boost::iostreams::copy( in, xyz );
		return xyz;
	}
}
#endif