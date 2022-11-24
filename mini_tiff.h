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

	namespace internal {

		struct IFDEntry {
			uint16_t id = 0;
			uint16_t field_type = 4;	// int
			uint32_t num_items = 1;
			uint32_t value = 0;
			IFDEntry() = default;
			IFDEntry(uint16_t new_id, uint32_t new_value) : id(new_id), value(new_value) {}
		};

		struct Header {
			uint8_t  byte_order[2] = { 0x49, 0x49 };
			uint8_t  magic[2] = { 0x2a, 0x00 };
			uint32_t offset_first_ifd = 8;				// This is not strictly true
			bool isValid() const { return magic[0] == 0x2a && magic[1] == 0 && byte_order[0] == 0x49 && byte_order[1] == 0x49 && offset_first_ifd == 8; }
		};

		static constexpr uint16_t IFD_ImageType = 0x00FE;
		static constexpr uint16_t IFD_Width = 0x0100;
		static constexpr uint16_t IFD_Height = 0x0101;
		static constexpr uint16_t IFD_BitsPerSample = 0x0102;
		static constexpr uint16_t IFD_Compression = 0x0103;
		static constexpr uint16_t IFD_PhotometricInterpretation = 0x0106;
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
	}

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
			f.write((uint8_t)0xff);
		f.writeBytes(data, total_data_bytes);
		return true;
	}

	template< typename Fn >
	static bool load(const char* ifilename, Fn fn ) {

		using namespace internal;

		FileReader f;
		if (!f.open(ifilename))
			return false;

		Header header;
		f.read(header);
		if (!header.isValid())
			return false;

		uint16_t num_ifds = 0;
		f.read(num_ifds);

		int      w = 0;
		int      h = 0;
		int      num_components = 0;
		int      bits_per_component = 0;
		uint32_t offset_for_data = 0;
		uint32_t total_data_bytes = 0;
			
		for (int i = 0; i < num_ifds; ++i) {

			IFDEntry ifd;
			f.read(ifd);

			//printf("%04x:%04x:%04x:%04x", ifd.id, ifd.field_type, ifd.num_items, ifd.value);

			switch (ifd.id) {

			case IFD_ImageType:
				//printf(" ImageType");
				if (ifd.value != 0)
					return false;
				break;

			case IFD_Width:
				//printf(" Width");
				w = ifd.value;
				break;

			case IFD_Height:
				//printf(" Height");
				h = ifd.value;
				break;

			case IFD_BitsPerSample:
				//printf(" BitsPerSample");
				bits_per_component = ifd.value;
				break;

			case IFD_Compression:
				//printf(" Compression");
				if (ifd.value != 1)
					return false;
				break;

			case IFD_PhotometricInterpretation:
				//printf(" PhotometricInterpretation");
				if (ifd.value != 2 && ifd.value != 1)
					return false;
				break;

			case IFD_OffsetForData:
				//printf(" OffsetForData");
				offset_for_data = ifd.value;
				break;

			case IFD_NumComponents:
				//printf(" NumComponents");
				num_components = ifd.value;
				break;

			case IFD_RowsPerStrip:
				//printf(" RowsPerStrip");
				if (ifd.value != h)
					return false;
				break;

			case IFD_TotalBytesForData:
				//printf(" TotalBytesForData");
				total_data_bytes = ifd.value;
				break;

			case IFD_PlanarConfiguration:
				//printf(" Planar Configuration");
				if (ifd.value != 1)
					return false;
				break;

			case IFD_ExtraSamples:
				//printf(" ExtraSamples");
				break;

			case IFD_SampleFormat:
				//printf(" SampleFormat are %d", ifd.value);
				break;
					
			// Ignored
			case IFD_ICCProfile:
				//printf(" ICCProfile");
				break;

			case IFD_Exif:
				//printf(" Exif..");
				break;
			case IFD_XMLPacket:
				//printf(" XMLPacket..");
				break;
			case IFD_Photoshop:
				//printf(" Photshop..");
				break;
			case IFD_DateTime:
				//printf(" DateTime..");
				break;
			case IFD_Software:
				//printf(" Sofware..");
				break;
			case IFD_XResolution:			// Typically 72 inch
			case IFD_YResolution:
			case IFD_ResolutionUnits:		// Typically inch
				//printf(" Resolution..");
				break;
			case IFD_Orientation:
				//printf(" Orientation");
				break;
				// ignore value
				break;
			default:
				break;
			}

			//printf("\n");
		}

		if (w == 0 || h == 0 || total_data_bytes == 0 || offset_for_data == 0)
			return false;

		// This can be 8,16 ore 32, or an offset in the file to get the bits_per_each_component
		// Here I assume all the components have the same number of bits
		if (bits_per_component > 32) {
			// Interpret the value as offset in the file
			f.seek(bits_per_component);
			uint16_t us_bpc;
			f.read(us_bpc);
			bits_per_component = us_bpc;
		}

		f.seek(offset_for_data);

		return fn(w, h, num_components, bits_per_component, f);
	}

}
