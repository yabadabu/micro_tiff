#define _CRT_SECURE_NO_WARNINGS
#include "../mini_tiff.h"
#include <cassert>
#include <vector>

struct Test {
	int w, h, num_comps, bits_per_comp;
	const char* filename;
	bool show_contents;
};

uint16_t swap16( uint16_t x ) {
	return (x>>8) | (x<<8);
}

bool runTest(const Test& t) {

	printf( "Loading %s\n", t.filename );

	std::vector< uint8_t > color_data;
	int ah = 0, aw = 0, anum_comps;

	bool is_ok = MiniTiff::load(t.filename, [&](int w, int h, int num_comps, int bits_per_comp, MiniTiff::FileReader& f) {

		if (t.num_comps != num_comps)
			printf("%20s : num_comps = %d (expected %d)\n", t.filename, num_comps, t.num_comps);
		if (t.bits_per_comp != bits_per_comp)
			printf("%20s : num_bits  = %d (expected %d)\n", t.filename, bits_per_comp, t.bits_per_comp);
		if (t.w != w || t.h != h)
			printf("%20s : dimensions don't match\n", t.filename);

		if (w == t.w && h == t.h && t.num_comps == num_comps && bits_per_comp == t.bits_per_comp) {
			aw = w;
			ah = h;
			anum_comps = num_comps;
			size_t total_bytes = w * h * num_comps * bits_per_comp / 8;
			color_data.resize(total_bytes);
			return f.readBytes(color_data.data(), total_bytes);
		}

		return false;
		});

	if (is_ok) {

		if (t.show_contents) {


			if (t.bits_per_comp == 16) {

				uint16_t* v0 = (uint16_t*)color_data.data();
				uint16_t* v = v0;
				for( int i=0; i<color_data.size() / 2; ++i, ++v ) 
					*v = swap16( *v );

				// Print some float values
				v = v0;
				for (int i = 0; i < aw; ++i) {
					if( i % anum_comps == 0  && i > 0)
						printf( "\n");
					printf("%04x ", v[i]);
				}
				printf("\n");
			}
		}

		char ofilename[256];
		sprintf(ofilename, "saved_%s", t.filename);
		bool save_ok = MiniTiff::save(ofilename, t.w, t.h, t.num_comps, t.bits_per_comp, color_data.data());
		if (!save_ok) {
			printf("Saving %s failed\n", ofilename);
		}

		bool reload_ok = MiniTiff::load(ofilename, [&](int w, int h, int num_comps, int bits_per_comp, MiniTiff::FileReader& f) {
			if( w != t.w ) printf( "Dimensions don't match\n");
			return true;
		});
		if (!reload_ok) {
			printf("Reloading saved filed %s failed\n", ofilename);
		}

		return save_ok;
	}

	return is_ok;
}

int main(int argc, char** argv) {

	//Test tests[2] = {
	Test tests[21] = {

		{ 720, 486, 3, 8, "brain_604.tif"},

		{ 32, 32, 1, 8, "G_32x32_8b.tif"},
		{ 32, 32, 3, 8, "RGB_32x32_8b.tif"},
		{ 32, 32, 4, 8, "RGBA_32x32_8b.tif"},
		
		{ 32, 32, 1, 16, "G_32x32_16b.tif"},
		{ 32, 32, 3, 16, "RGB_32x32_16b.tif"},
		{ 32, 32, 4, 16, "RGBA_32x32_16b.tif"},

		{ 32, 32, 1, 32, "G_32x32_32b.tif"},
		{ 32, 32, 3, 32, "RGB_32x32_32b.tif"},
		{ 32, 32, 4, 32, "RGBA_32x32_32b.tif"},

		{ 32, 32, 1, 8, "G_32x32_8b_BE.tif"},
		{ 32, 32, 3, 8, "RGB_32x32_8b_BE.tif"},
		{ 32, 32, 4, 8, "RGBA_32x32_8b_BE.tif"},
		
		{ 32, 32, 1, 16, "G_32x32_16b_BE.tif"},
		{ 32, 32, 4, 16, "RGB_32x32_16b_BE.tif"},
		{ 32, 32, 4, 16, "RGBA_32x32_16b_BE.tif"},

		{ 32, 32, 4, 32, "RGB_32x32_32b_BE.tif"},
		{ 32, 32, 4, 32, "RGBA_32x32_32b_BE.tif"},
		{ 0 }
	};
	int n_ok = 0;
	int n_tests = 0;
	for (auto& t : tests) {
		if (!t.filename)
			break;
		++n_tests;
		if (runTest(t)) {
			n_ok++;
		} else {
			printf( "%s failed\n", t.filename );
			//break;
		}
	}
	printf("%d/%d OK\n", n_ok, n_tests);
	return n_ok == n_tests ? 0 : -1;
}
