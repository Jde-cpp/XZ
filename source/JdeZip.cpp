#include "JdeZip.h"
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/copy.hpp>

namespace Jde::IO::Zip::GZip
{
	std::stringstream Read( std::istream& is )noexcept(false)
	{
		auto p = make_unique<vector<char>>();
		boost::iostreams::filtering_streambuf< boost::iostreams::input> in;
		boost::iostreams::gzip_decompressor x;
		in.push( x );
		in.push( is );
		std::stringstream xyz;
		boost::iostreams::copy( in, xyz );
		return xyz;
	}
}
