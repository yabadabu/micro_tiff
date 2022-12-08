# mini_tiff

This is a super simple header only library to read/write TIFF files supporting 8, 16 bits per channel (unsigned shorts), and 32 bits per channel (floats).
I'm using it to read and save files in 16bits/float and check the results inside Photoshop for example.

# Features

- Support mono, RGB and RGB+Alpha files
- Support 8,16 bits or 32 bits per channel (uint8/uint16/float)
- Only uncompressed TIFFs
- Minimal metadata is saved
- Data is assumed to be in a linear buffer when saving it
- No exceptions. API will return true if everything is ok, false if there is an error.
- For 4 channels images, pixel layout is Red, Green, Blue, Alpha
- No memory allocations
- You can also recover basic metadata using the info

# Install

This is a header only libray, just copy the mini_tiff.h somewhere in your code. No need to link any library.

# Save a Tiff

Assuming you have your RGB image somewhere. The provided pointer must contain ```width``` x ```height``` x ```number_of_channels``` x ```bits/channel``` ( 2 in this example, because we want a 16 bits per component tiff )

```c++
  bool is_ok = MiniTiff::save(out_filename, img.width, img.height, 3, 16, img.data());
```

# Load a Tiff

Load a tiff takes a bit longer. You need to provide the input filename, and a lambda which will receive the parsed basic parameters from the tiff, and a FileReader object, which has a method ```readBytes```  to read bytes directly into your container. No need to close the file.

If file can't be parsed the lambda will not get called and a false is returned.

```c++
  ImageRGB rgb;
  bool is_ok = MiniTiff::load(infilename, [&](int w, int h, int num_components, int bits_per_component, MiniTiff::FileReader& f) -> bool {
    // Confirm the tiff contains the format we expect in this concrete example
    if (bits_per_component != 16 || num_components != 3)
      return false;
    // Redimension my image buffer
    rgb.resize(w, h);
    // Read the data from f.
    return f.readBytes( rgb.data(), w * h * num_components * (bits_per_component / 8));
    });
```

# List TAGs

Basic metadata can be recovered by providing a lambda that will be called for each IFDTag. The helper function ```Tags::asStr``` will return a const char* for the basic tags.

```c++
  MiniTiff::info( ifilename, []( uint16_t id, uint32_t value, uint32_t value_type, uint32_t num_elems  ) {
    printf( "  %04x : %32s : %6d (%08x) x%d elems of type %d \n", id, MiniTiff::Tags::asStr( id ), value, value, num_elems, value_type);
  });
```
