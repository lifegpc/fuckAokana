# fuckAokana
## Usage
```bash
fuckaokana  # Print help information.
```
### Extract an archive
```bash
fuckaokana e evcg.dat  # Extract evcg.dat to evcg/
fuckaokana e evcg.dat custom  # Extract evcg.dat to custom/
fuckaokana e -y evcg.dat  # Extract evcg.dat to evcg/ and overwrite existed file
fuckaokana e video_op  # Eetract video_op to video_op_extract/
```
### Extract all archives in a directory
```bash
fuckaokana a  # Extract all archives in current directory and output files will be writed in <Archive Name>/
fuckaokana a PATH  # Extract all archives in PATH and output files will be writed in PATH/<Archive Name>/
fuckaokana a PATH PATH2  # Extract all archives in PATH and output files will be writed in PATH2/<Archive Name>/
```
### Extract and merge all cgs
```bash
fuckaokana c  # Extract and merge all cgs from archives in current directory and output files will be writed in cg/
fuckaokana c PATH  # Extract and merge all cgs from archives in PATH and output files will be writed in PATH/cg/
fuckaokana c PATH PATH2  # Extract and merge all cgs from archives in PATH and output files will be writed in PATH2
```
## Build
### CMake Options
|Options|Description|Default|
|:-:|:-:|:-:|
|`ENABLE_FFMPEG`|Use ffmpeg's library to merge CG.|`ON`|
|`ENABLE_LZ4`|Use liblz4 to decompress data in UnityFS archive.|`ON`|
### FFMPEG Requirements
- `webp` decoder
- `image_webp_pipe` or other webp demuxer
- `libwebp` encoder (Need linked to `libwebp`)
- `webp` muxer
- `overlay` filter
- Follow library needed:
    - `libavutil` (Part of ffmpeg)
    - `libavcodec` (Part of ffmpeg)
    - `libavformat` (Part of ffmpeg)
    - `libavfilter` (Part of ffmpeg)
    - `libswscale` (Part of ffmpeg)
    - `libwebp` (External library)
## Other
The decrypt method is from [here](https://github.com/Adebasquer/ExtractAokana/).
