#include "XZ.h"
#include <lzma.h> //https://tukaani.org/xz/
//for windows:  lib /machine:i386 /def:sqlite3.def
//https://stackoverflow.com/questions/23232864/how-to-use-lzma-sdk-in-c doesn't have right file header.
//http://newosxbook.com/src.jl?tree=listings&file=02_decompress.c
//https://github.com/kobolabs/liblzma/blob/master/doc/examples/01_compress_easy.c
#include <fstream>
#ifndef _MSC_VER
	#include <boost/interprocess/streams/bufferstream.hpp>
	#include <boost/iostreams/device/array.hpp>
	#include <boost/iostreams/stream.hpp>
	#include <boost/archive/binary_oarchive.hpp>
	#include <boost/interprocess/streams/vectorstream.hpp>
#endif

#include "../../Framework/source/Stopwatch.h"
#include <jde/Exception.h>
#include <jde/io/File.h>
#define var const auto

namespace Jde::IO::Zip
{
	using std::unique_ptr;
	using std::vector;

	void InitEncoder( lzma_stream *strm, uint32_t preset )noexcept(false);
	void InitDecoder( lzma_stream& strm )noexcept(false);

	//from 02_decompress.c
	α XZ::CoRead( path path )noexcept(false)->FunctionAwaitable//vector<char>;
	{
		return FunctionAwaitable( [path]( auto h )->Task2
		{
			TaskResult r = co_await IO::Read( path );
			auto p = r.Get<vector<char>>();//TODO deal with exception.
			co_await CoRead( move(*p) );
		});
	}
	α XZ::CoRead( vector<char>&& x )noexcept(false)->AsyncAwaitable
	{
		return AsyncAwaitable( [compressed=move(x)]()->sp<void>
		{
			auto y = XZ::Read( (uint8_t*)compressed.data(), compressed.size() );//TODO deal with exception.
			return sp<vector<char>>{ y.release() };
		});
	}

	unique_ptr<vector<char>> XZ::Read( const fs::path& path )noexcept(false)
	{
		auto pathString = path.string();
		std::ifstream file( pathString, std::ios::binary ); THROW_IF( file.fail(), "Could not open file '{}'", path.string() );

		const size_t fileSize = fs::file_size( fs::canonical(path) );
		//Stopwatch sw( format("Read '{}' - '{}K' bytes", path.string(), fileSize/(1 << 10)) );
		if( fileSize==0 )
			return unique_ptr<vector<char>>{};
		try
		{
			return Read( file, fileSize );
		}
		catch( IOException& e )
		{
			e.SetPath( path );
			throw e;
		}
	}
	α XZ::Read( uint8_t* pInput, uint size )noexcept(false)->up<vector<char>>
	{
		auto pResult = std::make_unique<vector<char>>( size );
		lzma_stream strm = LZMA_STREAM_INIT;
		InitDecoder( strm );
		strm.next_in = pInput;
		strm.avail_in = size;
		strm.next_out = reinterpret_cast<uint8_t*>( pResult->data() );
		strm.avail_out = pResult->capacity();
		for( lzma_action action = LZMA_RUN;; )
		{
			if ( strm.avail_in==0 )
				action = LZMA_FINISH;
			const lzma_ret ret = lzma_code( &strm, action );
			if( ret == LZMA_STREAM_END )
			{
				pResult->resize( pResult->size()-strm.avail_out );
				pResult->shrink_to_fit();
				break;
			}
			else if( ret == LZMA_OK && strm.avail_out == 0 )
			{
				uint originalSize = pResult->size();
				pResult->resize( originalSize+size, '\0' );
				strm.next_out = reinterpret_cast<uint8_t*>( pResult->data() + originalSize );
				strm.avail_out = pResult->size() - originalSize;
			}
			else if( ret != LZMA_OK )
			{
				lzma_end( &strm );
				switch (ret)
				{
				case LZMA_MEM_ERROR:
					THROW( "('{}')Memory allocation failed" );
				case LZMA_FORMAT_ERROR:
					THROW( "('{}')The input is not in the .xz format" );
				case LZMA_OPTIONS_ERROR:
					THROW( "('{}')Unsupported compression options" );
				case LZMA_DATA_ERROR:
					THROW( "('{}')Compressed file is corrupt" );
				case LZMA_BUF_ERROR:
					THROW( "('{}')Compressed file is truncated or otherwise corrupt" );
				default:
					THROW( "('{}')Unknown error, possibly a bug" );
				}
			}
		}
		lzma_end( &strm );
		return pResult;
	}
	α XZ::Read( std::istream& is, uint size )noexcept(false)->up<vector<char>>
	{
		std::unique_ptr<uint8_t[]> p{ new uint8_t[size] };
		is.read( (char*)p.get(), size );
		return Read( p.get(), size );
	}

	auto XZ::Compress( str bytes, uint32_t preset )noexcept(false)->up<vector<char>>
	{
#ifdef _MSC_VER
		std::stringstream os;
		var count = XZ::Write( os, bytes.data(), bytes.size(), preset );
		var str = os.str();
		return make_unique<vector<char>>( str.begin(), str.end() );
#else
		auto pCompressed = make_unique<vector<char>>( bytes.size() );
		boost::interprocess::bufferstream os{ pCompressed->data(), bytes.size() };
		var count = XZ::Write( os, bytes.data(), bytes.size(), preset );
		pCompressed->resize( count );
		return pCompressed;
#endif
	}
	//https://github.com/kobolabs/liblzma/blob/master/doc/examples/01_compress_easy.c
	void XZ::Write( const fs::path& path, string&& bytes, uint32_t preset )noexcept(false)
	{
		var pathName = path.string();
		const char* pszName = pathName.c_str();
		if( bytes.size()==0 )
			THROW( IOException("sent in 0 bytes for '{}'", pszName) );
		Stopwatch sw( fmt::format("XZ::Write( {}, {}k)", pszName, bytes.size()/(1 << 10)) );
		std::ofstream os{ path, std::ios::binary };
		try
		{
			Write( os, bytes.data(), bytes.size(), preset );
		}
		catch( IOException& e )
		{
			e.SetPath( path );
			throw e;
		}
	}
	void XZ::Write( const fs::path& path, const std::vector<char>& bytes, uint32_t preset )noexcept(false)
	{
		//DBG( "XZ::Write({},{:n},{}) Memory - {:n}M", path.string(), bytes.size(), preset, Diagnostics::GetMemorySize()/(1 << 20) );
		//Stopwatch sw( fmt::format("XZ::Write({},{}k,{})"sv, path.string(), bytes.size()/(1 << 10), preset) );
		std::ofstream os{ path, std::ios::binary };
		Write( os, bytes.data(), bytes.size(), preset );
		//DBG( "XZ::Write({},{:n},{}) Memory - {:n}M", path.string(), bytes.size(), preset, Diagnostics::GetMemorySize()/(1 << 20) );
	}

	uint XZ::Write( std::ostream& os, const char* pBytes, uint size, uint32_t preset, Stopwatch* pStopwatch )noexcept(false)
	{
		//auto pPrefix = pStopwatch ? make_shared<Stopwatch>( pStopwatch, "Prefix", "" ) : sp<Stopwatch>{};
		//auto pCalc = pStopwatch ? make_shared<Stopwatch>( pStopwatch, "Calc", "", false ) : sp<Stopwatch>{};
		//auto pWrite = pStopwatch ? make_shared<Stopwatch>( pStopwatch, "Write", "", false ) : sp<Stopwatch>{};

		lzma_stream strm = LZMA_STREAM_INIT;
		InitEncoder( &strm, preset );

		var outputSize = std::min( size, static_cast<uint>(1 << 30) );
		unique_ptr<uint8_t[]> outbuf( new uint8_t[outputSize] );
		strm.next_in = reinterpret_cast<const uint8_t*>( pBytes );
		strm.avail_in = size;
		const lzma_action action = LZMA_FINISH;
		strm.next_out = outbuf.get();
		strm.avail_out = outputSize;
		uint totalWriteSize{0};

		for(;;)
		{
			//if( pCalc ) pCalc->UnPause();
			const lzma_ret ret = lzma_code( &strm, action );
			//if( pCalc ) pCalc->Pause();
			switch( ret )
			{
			case LZMA_STREAM_END:
			case LZMA_OK:
				if( strm.avail_out == 0 || ret == LZMA_STREAM_END )
				{
					var writeSize = outputSize - strm.avail_out;
					os.write( reinterpret_cast<char*>(outbuf.get()), writeSize ); THROW_IFX( os.fail(), IOException(errno, "write") );
					totalWriteSize+=writeSize;
					strm.next_out = outbuf.get();
					strm.avail_out = outputSize;
				}
				break;
			case LZMA_MEM_ERROR:
				THROW( "Memory allocation failed '{}'", ret );
				break;
			case LZMA_DATA_ERROR:
				THROW( "File size limits exceeded '{}'", ret );
				break;
			default:
				THROW( "Unknown error, possibly a bug '{}'", ret );
				break;
			};
			if( ret == LZMA_STREAM_END )
				break;
		}
		lzma_end( &strm );
		return totalWriteSize;
	}

#pragma region Init
	void InitEncoder( lzma_stream *strm, uint32_t preset )noexcept(false)
	{
		lzma_ret ret = lzma_easy_encoder(strm, preset, LZMA_CHECK_CRC64);// Initialize the encoder using a preset. Set the integrity to check to CRC64, which is the default in the xz command line tool. If the .xz file needs to be decompressed with XZ Embedded, use LZMA_CHECK_CRC32 instead.
		if( ret == LZMA_OK )
			return;

		switch (ret)// Something went wrong. The possible errors are documented in lzma/container.h (src/liblzma/api/lzma/container.h in the source package or e.g. /usr/include/lzma/container.h depending on the install prefix).
		{
		case LZMA_MEM_ERROR:
			THROW( "Memory allocation failed" );
			break;
		case LZMA_OPTIONS_ERROR:
			THROW( "Specified preset is not supported" );
			break;
		case LZMA_UNSUPPORTED_CHECK:
			THROW( "Specified integrity check is not supported" );
			break;
		default:
			THROW( "Unknown error, possibly a bug" );
			break;
		}
	}
	void InitDecoder( lzma_stream& strm )noexcept(false)
	{
		lzma_ret ret = lzma_stream_decoder( &strm, UINT64_MAX, LZMA_CONCATENATED );
		if( ret != LZMA_OK )
		{
			switch (ret)
			{
			case LZMA_MEM_ERROR:
				THROW( "Memory allocation failed" );
			case LZMA_OPTIONS_ERROR:
				THROW( "Unsupported decompressor flags" );
			default:
				THROW( "Unknown error, possibly a bug" );
			}
		}
	}

#pragma endregion
}

