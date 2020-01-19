#include "XZ.h"
#include <lzma.h> //https://tukaani.org/xz/
//https://stackoverflow.com/questions/23232864/how-to-use-lzma-sdk-in-c doesn't have right file header.
//http://newosxbook.com/src.jl?tree=listings&file=02_decompress.c
//https://github.com/kobolabs/liblzma/blob/master/doc/examples/01_compress_easy.c
#include <fstream>
#include "../../Framework/source/Stopwatch.h"
#include "../../Framework/source/Diagnostics.h"
#define var const auto

namespace Jde::IO::Zip
{
	using std::unique_ptr;
	using std::vector;

	void InitEncoder( lzma_stream *strm, uint32_t preset )noexcept(false);
	void InitDecoder( lzma_stream& strm )noexcept(false);
	//from 02_decompress.c
	unique_ptr<vector<char>> XZ::Read( const fs::path& path )noexcept(false)
	{
		auto pathString = path.string();
		std::ifstream file( pathString, std::ios::binary );
		if( file.fail() )
			THROW( Exception("Could not open file '{}'", path.string()) );

		lzma_stream strm = LZMA_STREAM_INIT;
		InitDecoder(strm);
		lzma_action action = LZMA_RUN;

		const size_t fileSize = fs::file_size( fs::canonical(path) );
		Stopwatch sw( fmt::format("Read '{}' - '{:n}K' bytes", path.string(), fileSize/(1 << 10)) );
		if( fileSize==0 )
			return unique_ptr<vector<char>>{};

		vector<uint8_t> inbuf; inbuf.reserve(fileSize);
		auto pResult = std::make_unique<vector<char>>(fileSize);//
		strm.next_in = nullptr;
		strm.avail_in = 0;
		strm.next_out = reinterpret_cast<uint8_t*>( pResult->data() );
		strm.avail_out = pResult->capacity();
		for(;;)
		{
			if ( strm.avail_in==0 && !file.eof() )
			{
				strm.next_in = inbuf.data();
				file.read( reinterpret_cast<char*>(inbuf.data()), inbuf.capacity() );
				strm.avail_in = file.gcount();
				if( file.eof() )
					action = LZMA_FINISH;
			}
			lzma_ret ret = lzma_code( &strm, action );
			if( ret == LZMA_STREAM_END )
			{
				pResult->resize( pResult->size()-strm.avail_out );
				pResult->shrink_to_fit(); //3551
				break;
			}
			else if( ret == LZMA_OK && strm.avail_out == 0 )
			{
				uint originalSize = 0;
				originalSize = pResult->size();
				//string txt( pResult->data(), pResult->data()+pResult->size() );
				//FileUtilities::Save( "/home/duffyj/tmp.txt", txt );
				pResult->resize( originalSize+fileSize, '\0' );
				strm.next_out = reinterpret_cast<uint8_t*>( pResult->data() + originalSize );
				strm.avail_out = pResult->size() - originalSize;
			}
			else if( ret != LZMA_OK )
			{
				lzma_end(&strm);
				switch (ret)
				{
				case LZMA_MEM_ERROR:
					THROW( IOException("('{}')Memory allocation failed", pathString) );
				case LZMA_FORMAT_ERROR:
					THROW( IOException("('{}')The input is not in the .xz format", pathString) );
				case LZMA_OPTIONS_ERROR:
					 THROW( IOException("('{}')Unsupported compression options", pathString) );
				case LZMA_DATA_ERROR:
					 THROW( IOException("('{}')Compressed file is corrupt", pathString) );
				case LZMA_BUF_ERROR:
					THROW( IOException("('{}')Compressed file is truncated or otherwise corrupt", pathString) );
				default:
					THROW( IOException("('{}')Unknown error, possibly a bug", pathString) );
				}
			}
		}
		lzma_end(&strm);
		return pResult;
	}

	//bool compress();
	//https://github.com/kobolabs/liblzma/blob/master/doc/examples/01_compress_easy.c
	void XZ::Write( const fs::path& path, const string& bytes, uint32_t preset )noexcept(false)
	{
		if( bytes.size()==0 )
			THROW( IOException("sent in 0 bytes for '{}'", path.string()) );
		Stopwatch sw( fmt::format("XZ::Write({},{:n}k)", path.string(), bytes.size()/(1 << 10)) );
		std::ofstream os{path};
		Write( os, bytes.data(), bytes.size(), preset );
	}
	void XZ::Write( const fs::path& path, const std::vector<char>& bytes, uint32_t preset )noexcept(false)
	{
		//DBG( "XZ::Write({},{:n},{}) Memory - {:n}M", path.string(), bytes.size(), preset, Diagnostics::GetMemorySize()/(1 << 20) );
		Stopwatch sw( fmt::format("XZ::Write({},{:n}k,{})", path.string(), bytes.size()/(1 << 10), preset) );
		std::ofstream os{path};
		Write( os, bytes.data(), bytes.size(), preset );
		//DBG( "XZ::Write({},{:n},{}) Memory - {:n}M", path.string(), bytes.size(), preset, Diagnostics::GetMemorySize()/(1 << 20) );
	}

	void XZ::Write( std::ostream& os, const char* pBytes, uint size, uint32_t preset, Stopwatch* pStopwatch )noexcept(false)
	{
		//auto pPrefix = pStopwatch ? make_shared<Stopwatch>( pStopwatch, "Prefix", "" ) : sp<Stopwatch>{};
		//auto pCalc = pStopwatch ? make_shared<Stopwatch>( pStopwatch, "Calc", "", false ) : sp<Stopwatch>{};
		//auto pWrite = pStopwatch ? make_shared<Stopwatch>( pStopwatch, "Write", "", false ) : sp<Stopwatch>{};

		lzma_stream strm = LZMA_STREAM_INIT;
		InitEncoder( &strm, preset );

		var outputSize = std::min( size, static_cast<uint>(1 << 30) );
		shared_ptr<uint8_t[]> outbuf( new uint8_t[outputSize] );
		strm.next_in = reinterpret_cast<const uint8_t*>( pBytes );
		strm.avail_in = size;
		const lzma_action action = LZMA_FINISH;
		strm.next_out = outbuf.get();
		strm.avail_out = outputSize;
		//if( pPrefix ) pPrefix->Finish();

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
					//if( pWrite ) pWrite->UnPause();
					os.write( reinterpret_cast<char*>(outbuf.get()), writeSize );
					//if( pWrite ) pWrite->Pause();
					if( os.fail() )
						THROW( IOException("Could not write stream '{}'", std::strerror(errno)) );
					strm.next_out = outbuf.get();
					strm.avail_out = outputSize;
				}
				break;
			case LZMA_MEM_ERROR:
				THROW( Exception(fmt::format("Memory allocation failed '{}'", ret)) );
				break;
			case LZMA_DATA_ERROR:
				THROW( Exception(fmt::format("File size limits exceeded '{}'", ret)) );
				break;
			default:
				THROW( Exception(fmt::format("Unknown error, possibly a bug '{}'", ret)) );
				break;
			};
			if( ret == LZMA_STREAM_END )
				break;
		}
		lzma_end( &strm );
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
			THROW( Exception("Memory allocation failed") );
			break;
		case LZMA_OPTIONS_ERROR:
			THROW( Exception("Specified preset is not supported") );
			break;
		case LZMA_UNSUPPORTED_CHECK:
			THROW( Exception("Specified integrity check is not supported") );
			break;
		default:
			THROW( Exception("Unknown error, possibly a bug") );
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
				THROW( Exception("Memory allocation failed") );
			case LZMA_OPTIONS_ERROR:
				THROW( Exception("Unsupported decompressor flags") );
			default:
				THROW( Exception("Unknown error, possibly a bug") );
			}
		}
	}

#pragma endregion
}

