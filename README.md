# mini_tiff

This is a super simple header only library to read/write TIFF files supporting 16 bits per channel.
I use it to read and save files and see the files in Photoshop for example.

# Features

- Support mono and RGB files
- Support 8 and 16 bits per channel
- Only uncompressed TIFFs
- No metadata is saved/recovered.
- Data is assumed to be in a linear buffer when saving it
- No exceptions. API will return true if everything is ok, false if there is an error.

# Install

This is a header only libray, just copy the mini_tiff.h somewhere in your code. No need to link any library.

# Save a Tiff

Assuming you have your RGB image somewhere, with width x height pixels and 16 bits/channel

```c++
  bool is_ok = MiniTiff::save(out_filename, img.width, img.height, 3, 16, img.data());
```

# Load a Tiff

Load a tiff takes a bit longer, but you can read the file directly to your container without temporal allocations

```c++
  ImageRGB rgb;
  bool is_ok = MiniTiff::load(infilename, [&](int w, int h, int num_components, int bits_per_component, MiniTiff::FileReader& f) -> bool {
    if (bits_per_component != 16 || num_components != 3)
      return false;
    rgb.resize(w, h);
    return f.readBytes( rgb.data(), w * h * num_components * (bits_per_component / 8));
    });
```
