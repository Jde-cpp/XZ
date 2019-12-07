#include "XZ.h"
#include <lzma.h> //https://tukaani.org/xz/
///home/duffyj/code/libraries/7z/C/#include <lzma/Alloc.h>
//https://stackoverflow.com/questions/23232864/how-to-use-lzma-sdk-in-c doesn't have right file header.
//http://newosxbook.com/src.jl?tree=listings&file=02_decompress.c
//https://github.com/kobolabs/liblzma/blob/master/doc/examples/01_compress_easy.c
#include <fstream>
//#include "../../File.h"
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
					throw "Memory allocation failed";
				case LZMA_FORMAT_ERROR:
					throw "The input is not in the .xz format";
				case LZMA_OPTIONS_ERROR:
					throw  "Unsupported compression options";
				case LZMA_DATA_ERROR:
					throw  "Compressed file is corrupt";
				case LZMA_BUF_ERROR:
					THROW( IOException("Compressed file '{}' is truncated or otherwise corrupt", pathString) );
				default:
					throw "Unknown error, possibly a bug";
				}
			}
		}
		lzma_end(&strm);
		return pResult;
	}

	//bool compress();
	//https://github.com/kobolabs/liblzma/blob/master/doc/examples/01_compress_easy.c
	void XZ::Write( const fs::path& path, const std::vector<char>& bytes, uint32_t preset )noexcept(false)
	{
		//uint32_t preset = 6;
		//compress();
		DBG( "XZ::Write({},{},{})", path.string(), bytes.size(), preset );
		lzma_stream strm LZMA_STREAM_INIT;
		InitEncoder( &strm, preset );

		uint8_t outbuf[512]={'\0'};
		strm.next_in = reinterpret_cast<const uint8_t*>( bytes.data() );
		strm.avail_in = bytes.size();
		const lzma_action action = LZMA_FINISH;
		strm.next_out = outbuf;
		strm.avail_out = sizeof( outbuf );

		fs::create_directories( path.parent_path() );
		std::ofstream os;
		os.exceptions( std::ifstream::failbit | std::ifstream::badbit );
		try
		{
			os.open( path, std::ios::binary );
			for(;;)
			{
				const lzma_ret ret = lzma_code( &strm, action );
				switch( ret )
				{
				case LZMA_STREAM_END:
				case LZMA_OK:
					if( strm.avail_out == 0 || ret == LZMA_STREAM_END )
					{
						var writeSize = sizeof(outbuf) - strm.avail_out;
						os.write( reinterpret_cast<char*>(outbuf), writeSize );
						if( os.fail() )
							THROW( Exception("Could not write file '{}'", path.string()) );
						strm.next_out = outbuf;
						strm.avail_out = sizeof( outbuf );
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
		}
		catch ( const std::ifstream::failure& e )
		{
			lzma_end(&strm);
			THROW( IOException("Error opening '{}' - '{}'", path.string(), e.what()) );
		}
		lzma_end(&strm);
		TRACE( "XZ::Wrote({},{},{})", path.string(), bytes.size(), preset );
	}

	void XZ::Write( std::ostream& os, const char* pBytes, uint size, uint32_t preset )noexcept(false)
	{
		lzma_stream strm = LZMA_STREAM_INIT;
		InitEncoder( &strm, preset );

		uint8_t outbuf[4096]={'\0'};
		strm.next_in = reinterpret_cast<const uint8_t*>( pBytes );
		strm.avail_in = size;
		const lzma_action action = LZMA_FINISH;
		strm.next_out = outbuf;
		strm.avail_out = sizeof(outbuf);

		for(;;)
		{
			const lzma_ret ret = lzma_code( &strm, action );
			switch( ret )
			{
			case LZMA_STREAM_END:
			case LZMA_OK:
				if( strm.avail_out == 0 || ret == LZMA_STREAM_END )
				{
					var writeSize = sizeof(outbuf) - strm.avail_out;
					os.write( reinterpret_cast<char*>(outbuf), writeSize );
					if( os.fail() )
						THROW( IOException("Could not write file '{}'", std::strerror(errno)) );
					strm.next_out = outbuf;
					strm.avail_out = sizeof( outbuf );
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
		// Initialize the encoder using a preset. Set the integrity to check to CRC64, which is the default in the xz command line tool. If the .xz file needs to be decompressed with XZ Embedded, use LZMA_CHECK_CRC32 instead.
		lzma_ret ret = lzma_easy_encoder(strm, preset, LZMA_CHECK_CRC64);
		if( ret == LZMA_OK )
			return;

		// Something went wrong. The possible errors are documented in lzma/container.h (src/liblzma/api/lzma/container.h in the source package or e.g. /usr/include/lzma/container.h depending on the install prefix).
		switch (ret) 
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
#pragma region Internal
/*
	enum class TypeOfCheck : uint_fast8_t
	{
		crc32 = 0x1,
		crc64= 0x4,
		sha256 = 0xa
	};
	
	struct Filter
	{
		uint64_t FilterId{0};
		std::vector<char> Properties;
	};
	struct BlockHeader
	{
		uint_fast16_t RealHeaderSize;
		uint64_t CompressedSize{0};
		uint64_t UncompressedSize{0};
		std::list<Filter> Filters;
	};
	*/
//	TypeOfCheck ParseHeader( uint8_t*& pBuffer )noexcept(false);
//	char* ParseFooter( char* pBuffer ) noexcept(false);
//	BlockHeader ParseBlockHeader( uint8_t* pBuffer )noexcept(false);

/*	TypeOfCheck ParseHeader( uint8_t*& pBuffer )
	{
		//uint8_t* pStart = pBuffer;
		constexpr const unsigned char HEADER_MAGIC[7] = { 0xFD, '7', 'z', 'X', 'Z', 0x00, 0x0 };
		
		for( size_t i=0; i<sizeof(HEADER_MAGIC); ++i )
		{
			if( *pBuffer++!=HEADER_MAGIC[i] )
				THROW( Exception( fmt::format("Header magic failed for {} - {} vs {}", i, (pBuffer-1)[i], HEADER_MAGIC[i])) );
		}
		auto pStreamStart = pBuffer-1;
		//assert( *pBuffer++==0x0 );
		TypeOfCheck typeOfCheck = static_cast<TypeOfCheck>( *pBuffer++ );
		//bool crc32 = typeOfCheck & 0x1;
		//bool crc64 = typeOfCheck & 0x4;
		//bool sha256 = typeOfCheck & 0xa;
		uint32_t crc32 = 0;
		memcpy( (char*)&crc32, pBuffer, 4 ); pBuffer+=4;
		//std::cout  << pBuffer[0] << ", " << std::itos(pBuffer[1]) << ", "<< std::itos(pBuffer[2]) << ", "<< std::itos(pBuffer[3]) << endl;

		boost::crc_32_type result;
		result.process_bytes( pStreamStart, 2 );
		auto expected = result.checksum();
		if( crc32!=expected )
			THROW( Exception( fmt::format("crc - {} vs {}", crc32, expected)) );
		return typeOfCheck;
	}
*/
/*
	char* ParseFooter( char* pBuffer )
	{
	#ifndef NDEBUG
		uint32_t crc32, backwardSize;
		memcpy( (char*)&crc32, pBuffer, 4 ); pBuffer+=4;
		memcpy( (char*)&backwardSize, pBuffer, 4 ); pBuffer+=4;
		/ *char streamFlags = ** / pBuffer++;// pBuffer;
		assert( *pBuffer++=='Y' );
		assert( *pBuffer++=='Z' );
	#endif	
	return pBuffer;
	}
	size_t decodeSize( uint8_t* buf, uint64_t *num, size_t size_max=9 )
	{
		if (size_max == 0)
			return 0;

		if (size_max > 9)
			size_max = 9;

		*num = buf[0] & 0x7F;
		size_t i = 0;

		while( buf[i++] & 0x80 ) 
		{
			if( i >= size_max || buf[i] == 0x00 )
				return 0;

			*num |= (uint64_t)(buf[i] & 0x7F) << (i * 7);
		}

		return i;
	}

	BlockHeader ParseBlockHeader( uint8_t* pBuffer )noexcept(false)
	{
		uint8_t* pStart = pBuffer;
		BlockHeader blockHeader;
		uint8_t blockHeaderSize = *pBuffer++;
		blockHeader.RealHeaderSize = (blockHeaderSize+1)*4;
		uint8_t blockFlags = *pBuffer++;
		bool compressedSizeField = blockFlags & 0x40;
		bool uncompressedSizeField = blockFlags & 0x80;
		uint8_t numberOfFilters = (blockFlags >> 6)+1;
		if( compressedSizeField )
			pBuffer+=decodeSize( pBuffer, &blockHeader.CompressedSize );
		if( uncompressedSizeField )
			pBuffer+=decodeSize( pBuffer, &blockHeader.UncompressedSize );
		for( size_t filterIndex = 0; filterIndex<numberOfFilters; ++filterIndex )
		{
			//auto pStartFlag = pBuffer;
			Filter filter;
			pBuffer+=decodeSize( pBuffer, &filter.FilterId );//0x21==LZMA2
			uint64_t propertySize=0;
			pBuffer+=decodeSize( pBuffer, &propertySize );
			filter.Properties.resize( propertySize );
			memcpy( filter.Properties.data(), pBuffer, propertySize ); pBuffer+=propertySize;
		}
		for( size_t iByte = pBuffer-pStart ;iByte<blockHeader.RealHeaderSize-4; ++iByte )
			assert( *pBuffer++=='\0' );
		uint32_t actualCrc32 = 0;
		memcpy( &actualCrc32, pStart+blockHeader.RealHeaderSize-4, 4 );
		boost::crc_32_type result;
		result.process_bytes( pStart, blockHeader.RealHeaderSize-4 );
		auto calculated = result.checksum();
		if( calculated!=actualCrc32 )
			THROW( Exception(fmt::format("crc - {} vs {}", actualCrc32, calculated)) );
		assert( calculated==actualCrc32 );
		return blockHeader;	
	}*/
#pragma endregion
}

