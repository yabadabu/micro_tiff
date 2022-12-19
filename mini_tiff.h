#pragma once

#include <cstdio>
#include <cstdint>

/*
	Usage:

	#include "mini_tiff.h"

	// Save a tiff. wxh RGB 16 bits/channel
	bool is_ok = MiniTiff::save(out_filename, img.w, img.h, 3, 16, img.data());

	// Load a tiff takes longer, but you can read the file directly to your container without tmp allocations
	ImageRGB rgb;
	bool is_ok = MiniTiff::load(infilename, [&](int w, int h, int num_components, int bits_per_component, MiniTiff::FileReader& f) {
		if (bits_per_component != 16 || num_components != 3)
			return false;
		rgb.resize(w, h);
		return f.readBytes( rgb.data(), w * h * num_components * (bits_per_component / 8));
		});

*/

namespace MiniTiff {

	static constexpr uint16_t IFD_ImageType = 0x00FE;
	static constexpr uint16_t IFD_Width = 0x0100;
	static constexpr uint16_t IFD_Height = 0x0101;
	static constexpr uint16_t IFD_BitsPerSample = 0x0102;
	static constexpr uint16_t IFD_Compression = 0x0103;
	static constexpr uint16_t IFD_PhotometricInterpretation = 0x0106;
	static constexpr uint16_t IFD_FillOrder = 0x010A;
	static constexpr uint16_t IFD_OffsetForData = 0x0111;
	static constexpr uint16_t IFD_Orientation = 0x0112;
	static constexpr uint16_t IFD_NumComponents = 0x0115;
	static constexpr uint16_t IFD_RowsPerStrip = 0x0116;
	static constexpr uint16_t IFD_TotalBytesForData = 0x0117;
	static constexpr uint16_t IFD_ExtraSamples = 0x0152;			// Meaning of the alpha channel (for RGBA images)
	static constexpr uint16_t IFD_SampleFormat = 0x0153;			// Data are uints(1), ints(2) or floats(3)?

	static constexpr uint16_t IFD_XResolution = 0x011A;
	static constexpr uint16_t IFD_YResolution = 0x011B;
	static constexpr uint16_t IFD_ResolutionUnits = 0x0128;			// Inches
	static constexpr uint16_t IFD_Software = 0x0131;
	static constexpr uint16_t IFD_DateTime = 0x0132;
	static constexpr uint16_t IFD_XMLPacket = 0x02BC;
	static constexpr uint16_t IFD_Photoshop = 0x8649;
	static constexpr uint16_t IFD_PlanarConfiguration = 0x011c;		// Interleaved?
	static constexpr uint16_t IFD_Exif = 0x8769;
	static constexpr uint16_t IFD_ICCProfile = 0x8773;

	struct Tags {
		static const char* asStr( uint16_t tag_id ) {
			#define DECL_TAG_NAME(x) if( tag_id == IFD_##x ) return #x
			DECL_TAG_NAME(ImageType);
			DECL_TAG_NAME(Width);
			DECL_TAG_NAME(Height);
			DECL_TAG_NAME(BitsPerSample);
			DECL_TAG_NAME(Compression);
			DECL_TAG_NAME(PhotometricInterpretation);
			DECL_TAG_NAME(FillOrder);
			DECL_TAG_NAME(TotalBytesForData);
			DECL_TAG_NAME(OffsetForData);
			DECL_TAG_NAME(Orientation);
			DECL_TAG_NAME(NumComponents);
			DECL_TAG_NAME(RowsPerStrip);
			DECL_TAG_NAME(ExtraSamples);
			DECL_TAG_NAME(SampleFormat);

			DECL_TAG_NAME(XResolution);
			DECL_TAG_NAME(YResolution);
			DECL_TAG_NAME(ResolutionUnits);
			DECL_TAG_NAME(Software);
			DECL_TAG_NAME(DateTime);
			DECL_TAG_NAME(XMLPacket);
			DECL_TAG_NAME(Photoshop);
			DECL_TAG_NAME(PlanarConfiguration);
			DECL_TAG_NAME(Exif);
			DECL_TAG_NAME(ICCProfile);
			#undef DECL_TAG_NAME
			return "Unknown";
		}
	};

	namespace internal {

		struct IFDEntry {
			uint16_t id = 0;
			uint16_t field_type = 4;	// 1:Byte, 2:Char, 3:Short, 4:Int, 5:Rational (num/den), 11:Float, 12:Double
			uint32_t num_items = 1;
			uint32_t value = 0;
			IFDEntry() = default;
			IFDEntry(uint16_t new_id, uint32_t new_value) : id(new_id), value(new_value) {}
			void swap() {
				id = swap16(id);
				field_type = swap16(field_type);
				num_items = swap32(num_items);

				if( id == IFD_BitsPerSample ) {
					if( field_type == 3 && num_items == 1 )
						value = swap16( value );
					else 
					  value = swap32( value );
				} else {
					if( field_type == 3 )
						value = swap16(value);
					else
					  value = swap32( value );
				}
			}

			static uint32_t swap32( uint32_t x ) {
				return ((x>>24)&0xff) | 
	             ((x<<8)&0xff0000) | 
	             ((x>>8)&0xff00) |
	             ((x<<24)&0xff000000);
			}
			static uint16_t swap16( uint16_t x ) {
				return (x>>8) | (x<<8);
			}
		};

		struct Header {
			uint8_t  byte_order[2] = { 0x49, 0x49 };
			uint8_t  magic[2] = { 0x2a, 0x00 };
			uint32_t offset_first_ifd = 8;				// This is not strictly true
			bool isValid() const {
				if( byte_order[0] != byte_order[1] )
					return false;
				if( byte_order[0] == 0x49 && magic[0] == 0x2a && magic[1] == 0x00 )
					return true;
				if( byte_order[0] == 0x4D && magic[1] == 0x2a && magic[0] == 0x00 )
					return true;
				return false;
			}
			bool mustSwapBytes() const {
				return magic[1] == 0x2a;
			}
		};

	};

	struct FileWriter {
		FILE* f = nullptr;
		size_t bytes_written = 0;
		~FileWriter() {
			if (f)
				fclose(f);
		}
		bool create(const char* ofilename) {
			f = fopen(ofilename, "wb");
			return f != nullptr;
		}
		void writeBytes(const void* data, size_t num_bytes) {
			fwrite(data, 1, num_bytes, f);
			bytes_written += num_bytes;
		}
		template< typename T >
		void write(T t) {
			writeBytes(&t, sizeof(T));
		}
	};

	struct FileReader {
		FILE* f = nullptr;
		size_t bytes_read = 0;
		bool   swap_16b_data = false;
		bool   swap_32b_data = false;
		~FileReader() {
			if (f)
				fclose(f);
		}
		bool open(const char* ofilename) {
			f = fopen(ofilename, "rb");
			return f != nullptr;
		}
		bool readBytes(void* data, size_t num_bytes) {
			size_t n = fread(data, 1, num_bytes, f);
			bytes_read += num_bytes;

			// Swap component data inside the lib
			if( swap_16b_data ) {
				uint16_t* p = (uint16_t*) data;
				for( int i=0; i<num_bytes/2; ++i, ++p ) 
					*p = internal::IFDEntry::swap16(*p);
			}
			else if( swap_32b_data ) {
				uint32_t* p = (uint32_t*) data;
				for( int i=0; i<num_bytes/4; ++i, ++p ) 
					*p = internal::IFDEntry::swap32(*p);
			}

			return n == num_bytes;
		}
		template< typename T >
		bool read(T& t) {
			return readBytes(&t, sizeof(T));
		}
		void seek(uint32_t offset) {
			fseek(f, offset, SEEK_SET);
		}
	};

	static bool save(const char* ofilename, int w, int h, int num_components, int bits_per_component, const void* data ) {

		using namespace internal;

		// Validate input parameters
		if( ! ((w > 0)
			&& (h > 0)
			&& (bits_per_component == 8 || bits_per_component == 16 || bits_per_component == 32)
			&& (num_components == 1 || num_components == 3 || num_components == 4)
			&& (data)
			))
			return false;

		FileWriter f;
		if (!f.create(ofilename))
			return false;

		f.write(Header{});

		uint16_t num_ifds = 10;

		// When saving floats, we store an additional IFDEntry entry
		if( bits_per_component == 32 )
			++num_ifds;

		f.write(num_ifds);

		uint32_t bytes_per_component = bits_per_component / 8;
		uint32_t offset_for_data = 256;
		uint32_t total_data_bytes = w * h * num_components * bytes_per_component;
		uint32_t photometric_interpretation = (num_components == 1) ? 1 : 2;

		// https://www.awaresystems.be/imaging/tiff/tifftags/baseline.html
		f.write(IFDEntry(IFD_ImageType, 0));		// Image Type
		f.write(IFDEntry(IFD_Width, w));			// width
		f.write(IFDEntry(IFD_Height, h));			// Height
		f.write(IFDEntry(IFD_BitsPerSample, bits_per_component));		// 8, 16 or 32
		f.write(IFDEntry(IFD_Compression, 1));		// Compression : 1 = none
		f.write(IFDEntry(IFD_PhotometricInterpretation, photometric_interpretation));		// PhotometricInterpretation : 2: RGB, 1:Grey
		f.write(IFDEntry(IFD_OffsetForData, offset_for_data));	// StripOffsets : offset to start of actual data
		f.write(IFDEntry(IFD_NumComponents, num_components));		// SamplesPerPixel : 3

		if( bits_per_component == 32 )
			f.write(IFDEntry(IFD_SampleFormat, 3));		// data are floats (3)

		f.write(IFDEntry(IFD_RowsPerStrip, h));	// Height
		f.write(IFDEntry(IFD_TotalBytesForData, total_data_bytes));
		size_t num_padding_bytes = offset_for_data - f.bytes_written;
		for (int i = 0; i < num_padding_bytes; ++i)
			f.write((uint8_t)0x00);
		f.writeBytes(data, total_data_bytes);
		return true;
	}

	template< typename Fn >
	static bool info(const char* ifilename, Fn fn ) {

		using namespace internal;

		FileReader f;
		if (!f.open(ifilename))
			return false;

		Header header;
		f.read(header);
		if (!header.isValid())
			return false;

		bool swap_bytes = header.mustSwapBytes();
		if( swap_bytes ) header.offset_first_ifd = IFDEntry::swap32( header.offset_first_ifd );
		f.seek(header.offset_first_ifd);

		uint16_t num_ifds = 0;
		f.read(num_ifds);
		if( swap_bytes ) num_ifds = IFDEntry::swap16(num_ifds);

		for (int i = 0; i < num_ifds; ++i) {
			IFDEntry ifd;
			f.read(ifd);
			if( swap_bytes ) ifd.swap();
			
			fn( ifd.id, ifd.value, ifd.field_type, ifd.num_items );
		}

		return true;
	}

	#define tiff_printf	 if( 0 ) printf

	template< typename Fn, typename FnMeta = void>
	static bool load(const char* ifilename, Fn fn) {

		using namespace internal;

		FileReader f;
		if (!f.open(ifilename))
			return false;

		Header header;
		f.read(header);
		if (!header.isValid())
			return false;

		bool swap_bytes = header.mustSwapBytes();
		if( swap_bytes ) header.offset_first_ifd = IFDEntry::swap32( header.offset_first_ifd );
		f.seek(header.offset_first_ifd);

		tiff_printf("OffsetFirstIFD: %08x (%d). Swap:%d\n", header.offset_first_ifd , header.offset_first_ifd, swap_bytes );

		uint16_t num_ifds = 0;
		f.read(num_ifds);
		if( swap_bytes ) num_ifds = IFDEntry::swap16(num_ifds);

		int      w = 0;
		int      h = 0;
		int      num_components = 0;
		int      bits_per_component = 0;
		uint32_t offset_for_data = -1;
		uint32_t total_data_bytes = 0;
		bool     swap_component_data = false;
			
		for (int i = 0; i < num_ifds; ++i) {

			IFDEntry ifd;
			f.read(ifd);

			if( swap_bytes ) ifd.swap();

			tiff_printf("%04x:%04x:%04x:%08x %s: ", ifd.id, ifd.field_type, ifd.num_items, ifd.value, Tags::asStr( ifd.id ));

			switch (ifd.id) {

			case IFD_ImageType:
				if (ifd.value != 0)
					return false;
				break;

			case IFD_Width:
				tiff_printf("%d", ifd.value);
				w = ifd.value;
				break;

			case IFD_Height:
				tiff_printf("%d", ifd.value);
				h = ifd.value;
				break;

			case IFD_BitsPerSample:
				tiff_printf("(At @0x%08x)", ifd.value);
				bits_per_component = ifd.value;
				break;

			case IFD_Compression:
				tiff_printf("%d", ifd.value);
				if (ifd.value != 1)
					return false;
				break;

			case IFD_PhotometricInterpretation:
				if (ifd.value != 2 && ifd.value != 1)
					return false;
				break;

			case IFD_OffsetForData:
				tiff_printf("(At @0x%08x)", ifd.value);
				offset_for_data = ifd.value;
				break;

			case IFD_NumComponents:
				tiff_printf("%d", ifd.value);
				num_components = ifd.value;
				break;

			case IFD_RowsPerStrip:
				tiff_printf("%d (should be %d)", ifd.value, h);
				if (ifd.value != h)
					return false;
				break;

			case IFD_TotalBytesForData:
				tiff_printf("%d", ifd.value);
				total_data_bytes = ifd.value;
				break;

			case IFD_PlanarConfiguration:
				if (ifd.value != 1)
					return false;
				break;

			case IFD_SampleFormat:
				tiff_printf("%d", ifd.value);
				break;

			case IFD_FillOrder:
				tiff_printf("%d", ifd.value);
				swap_component_data = (ifd.value == 1);
				break;
					
			// Ignored
			case IFD_ICCProfile:
				break;

			case IFD_ExtraSamples:
				break;
			case IFD_Exif:
				break;
			case IFD_XMLPacket:
				break;
			case IFD_Photoshop:
				break;
			case IFD_DateTime:
				break;
			case IFD_Software:
				break;
			case IFD_XResolution:			// Typically 72 inch
			case IFD_YResolution:
			case IFD_ResolutionUnits:		// Typically inch
				break;
			case IFD_Orientation:
				break;
			default:
				break;
			}

			tiff_printf("\n");
		}

		if (w == 0 || h == 0 || total_data_bytes == 0 || offset_for_data == -1) {
			tiff_printf( "Didn't read needed data: w:%d h:%d total_data_bytes:%d offset_for_data:%d\n", w, h, total_data_bytes, offset_for_data);
			return false;
		}

		// This can be 8,16 ore 32, or an offset in the file to get the bits_per_each_component
		// Here I assume all the components have the same number of bits
		if (bits_per_component != 8 && bits_per_component != 16 && bits_per_component != 32) {
			// Interpret the value as offset in the file
			f.seek(bits_per_component);
			uint16_t us_bpc;
			f.read(us_bpc);
			bits_per_component = us_bpc;
			if( swap_bytes ) bits_per_component = IFDEntry::swap16(bits_per_component);
			if (bits_per_component != 8 && bits_per_component != 16 && bits_per_component != 32) {
				tiff_printf( "Invalid bits per component: %d (%08x)\n", bits_per_component, bits_per_component);
				return false;
			}
		}

		f.seek(offset_for_data);
		tiff_printf( "Read needed data: w:%d h:%d bits_per_component:%d total_data_bytes:%d offset_for_data:%d\n", w, h, bits_per_component, total_data_bytes, offset_for_data);

		// Configure the reader to swap the bytes of each component so the user does not have to deal with it
		if( swap_component_data && bits_per_component == 16 )
			f.swap_16b_data = true;
		if( swap_component_data && bits_per_component == 32 )
			f.swap_32b_data = true;

		return fn(w, h, num_components, bits_per_component, f);
	}


}
