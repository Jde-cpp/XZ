#include "JdeZip.h"
#include <jde/Exception.h>
#ifdef _MSC_VER
   #include <zlib.h>
#else
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/copy.hpp>
#endif

#define var const auto
namespace Jde::IO::Zip::GZip
{
	std::stringstream Read( std::istream& is )noexcept(false)
	{
#ifdef _MSC_VER
      z_stream zs;
      zs.zalloc = Z_NULL;
      zs.zfree = Z_NULL;
      zs.opaque = Z_NULL;
      ostringstream os;
      os << is.rdbuf();
      var str = os.str();
      //is.read(
      zs.avail_in = (uInt)str.size();
      zs.next_in = (Bytef *)str.data();
      //gz_header h;
     // deflateInit2( &zs, 
      //deflateSetHeader( &zs, gz_header );

      //auto hr = deflateInit2( &zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15/*15 | 16*/, 8, Z_DEFAULT_STRATEGY ); CHECK(hr==Z_OK);      // hard to believe they don't have a macro for gzip encoding, "Add 16" is the best thing zlib can do: Add 16 to windowBits to write a simple gzip header and trailer around the compressed data instead of a zlib wrapper
      std::stringstream ss;
      auto hr = inflateInit2( &zs, MAX_WBITS | 16 ); CHECK(hr==Z_OK); 
      std::unique_ptr<char[]> output; hr=Z_BUF_ERROR;
      for( auto outputSize = str.size(); hr==Z_BUF_ERROR; outputSize*=2 ) 
      {
         output = std::unique_ptr<char[]>{ new char[outputSize] };
         zs.avail_out = (uInt)outputSize;
         zs.next_out = (Bytef *)output.get();
         var start = zs.total_out;
         hr = inflate( &zs, Z_FINISH );
         ss.write( output.get(), zs.total_out-start );
      } 
      CHECK( hr==Z_STREAM_END );
      hr = inflateEnd( &zs ); 
   //   hr = deflateEnd( &zs ); CHECK( hr==Z_OK ); CHECK( zs.avail_out>0 );
//      ss.write( output.get(), zs.total_out );
      return ss;
#else
		boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
		boost::iostreams::gzip_decompressor x;
		in.push( x );
		in.push( is );
		std::stringstream ss;
		boost::iostreams::copy( in, ss );
		return ss;
#endif
	}
}
