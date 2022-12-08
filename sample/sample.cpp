#define _CRT_SECURE_NO_WARNINGS
#include "../mini_tiff.h"
#include <cassert>
#include <vector>

struct Test {
	int w, h, num_comps, bits_per_comp;
	const char* filename;
};

bool runTest(const Test& t) {

	std::vector< uint8_t > color_data;

	bool is_ok = MiniTiff::load(t.filename, [&](int w, int h, int num_comps, int bits_per_comp, MiniTiff::FileReader& f) {

		if (t.num_comps != num_comps)
			printf("%20s : num_comps = %d (expected %d)\n", t.filename, num_comps, t.num_comps);
		if (t.bits_per_comp != bits_per_comp)
			printf("%20s : num_bits  = %d (expected %d)\n", t.filename, bits_per_comp, t.bits_per_comp);
		if (t.w != w || t.h != h)
			printf("%20s : dimensions don't match\n", t.filename);

		if (w == t.w && h == t.h && t.num_comps == num_comps && bits_per_comp == t.bits_per_comp) {
			size_t total_bytes = w * h * num_comps * bits_per_comp / 8;
			color_data.resize(total_bytes);
			return f.readBytes(color_data.data(), total_bytes);
		}

		return false;
		});

	if (is_ok) {

		if (t.bits_per_comp == 32) {
			// Print some float values
			float* v = (float*)color_data.data();
			for (int i = 0; i < 12; ++i) {
				printf("%f ", v[i]);
			}
			printf("\n");
		}

		char ofilename[256];
		sprintf(ofilename, "saved_%s", t.filename);
		bool save_ok = MiniTiff::save(ofilename, t.w, t.h, t.num_comps, t.bits_per_comp, color_data.data());
		if (!save_ok) {
			printf("Saving %s failed\n", ofilename);
		}
		return save_ok;
	}

	return is_ok;
}

void dumpInfo( const char* ifilename ) {
	printf( "%s\n", ifilename);
	MiniTiff::info( ifilename, []( uint16_t id, uint32_t value, uint32_t value_type, uint32_t num_elems  ) {
		printf( "  %04x : %32s : %6d (%08x) x%d elems of type %d \n", id, MiniTiff::Tags::asStr( id ), value, value, num_elems, value_type);
	});
}

int main(int argc, char** argv) {

	const char* infile = "G_32x32_8b.tif";
	if( argc > 1 )
		infile = argv[1];
	dumpInfo(infile);

	Test tests[10] = {
		{ 32, 32, 1, 8, "G_32x32_8b.tif"},
		{ 32, 32, 3, 8, "RGB_32x32_8b.tif"},
		{ 32, 32, 4, 8, "RGBA_32x32_8b.tif"},
		
		{ 32, 32, 1, 16, "G_32x32_16b.tif"},
		{ 32, 32, 3, 16, "RGB_32x32_16b.tif"},
		{ 32, 32, 4, 16, "RGBA_32x32_16b.tif"},

		{ 32, 32, 1, 32, "G_32x32_32b.tif"},
		{ 32, 32, 3, 32, "RGB_32x32_32b.tif"},
		{ 32, 32, 4, 32, "RGBA_32x32_32b.tif"},
		{ 0 }
	};
	int n_ok = 0;
	int n_tests = 0;
	for (auto& t : tests) {
		if (!t.filename)
			break;
		++n_tests;
		if (runTest(t))
			n_ok++;
	}
	printf("%d/%d OK\n", n_ok, n_tests);
	return n_ok == n_tests ? 0 : -1;
}
