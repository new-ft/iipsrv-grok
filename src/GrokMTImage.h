/*  IIP Server: Grok JPEG2000 handler (multi-threaded decode variant)

    Copyright (C) 2019-2026 Ruven Pillay.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef _GROKMTIMAGE_H
#define _GROKMTIMAGE_H

#include "IIPImage.h"
#include <grok.h>


#define TILESIZE 256


/// Image class for JPEG 2000 Images using Grok's internal multi-threaded
/// decompression for each tile. This is a separate, experimental decoder
/// variant alongside GrokImage (single-threaded): the per-request codec
/// open/close lifecycle is identical to GrokImage/OpenJPEGImage/KakaduImage
/// (grk_decompress_update()-based codec reuse across different decode
/// windows was tested and found to silently produce incorrect pixel data,
/// contrary to a hoped-for optimization - see GROK_MT_COMPARISON.md), but
/// each individual grk_decompress() call is allowed to use more than one
/// worker thread internally, which Grok's public API does officially
/// support and which was empirically verified to still produce correct,
/// bit-identical output.
class GrokMTImage : public IIPImage {

 private:

  grk_object* _codec;           /// decompression codec handle
  grk_header_info _header_info; /// header info populated on openImage()

  /// Number of worker threads Grok is allowed to use for a single
  /// grk_decompress() call. Configurable via the GROK_NUM_THREADS
  /// environment variable (default 4, chosen from benchmarking - see
  /// GROK_MT_COMPARISON.md: 4 threads gave the best result for a 256x256
  /// production tile size, with 8 threads regressing due to oversubscription
  /// overhead).
  static unsigned int num_threads;


  /// Main processing function
  /** @param r resolution
      @param l number of quality levels to decode
      @param x x coordinate
      @param y y coordinate
      @param w width of region
      @param h height of region
      @param d buffer to fill
   */
  void process( unsigned int r, int l, int x, int y, unsigned int w, unsigned int h, void* d );



 public:

  /// Constructor
  GrokMTImage() : IIPImage(){
    _codec = NULL;
    tile_widths.push_back(TILESIZE); tile_heights.push_back(TILESIZE);
  };


  /// Constructor
  /** @param path image path
   */
  GrokMTImage( const std::string& path)  : IIPImage(path){
    _codec = NULL;
    tile_widths.push_back(TILESIZE); tile_heights.push_back(TILESIZE);
  };


  /// Copy Constructor
  /** @param image GrokMT object
   */
  GrokMTImage( const GrokMTImage& image ): IIPImage( image ) {};


  /// Copy Constructor
  /** @param image IIPImage object
   */
  GrokMTImage( const IIPImage& image ) : IIPImage(image){
    _codec = NULL;
    tile_widths.push_back(TILESIZE); tile_heights.push_back(TILESIZE);
  };


  /// Destructor
  ~GrokMTImage(){ closeImage(); };


  /// Set up Grok library-wide logging handlers, read GROK_NUM_THREADS and
  /// initialize the library's shared worker thread pool accordingly.
  /// Must be called once at startup, before any GrokMTImage is used.
  static void setupLogging();


  /// Overloaded function for opening a JP2 image
  void openImage();


  /// Overloaded function for loading JP2 image information
  /** @param x horizontal sequence angle
      @param y vertical sequence angle
  */
  void loadImageInfo( int x, int y );


  /// Overloaded function for closing a JP2 image
  void closeImage();


  /// Return whether this image type directly handles region decoding
  bool regionDecoding(){ return true; };


  /// Overloaded function for getting a particular tile
  /** @param x horizontal sequence angle
      @param y vertical sequence angle
      @param r resolution
      @param l number of quality layers to decode
      @param t tile number
      @param e image encoding
   */
  RawTile getTile( int x, int y, unsigned int r, int l, unsigned int t, ImageEncoding e = ImageEncoding::RAW );


  /// Overloaded function for returning a region from image
  /**
    @param ha       horizontal angle
    @param va       vertical angle
    @param res      resolution
    @param layers   number of quality layers to decode
    @param x        x coordinate
    @param y        y coordinate
    @param w        width of region
    @param h        height of region
    @return         a RawTile object
  */
  RawTile getRegion( int ha, int va, unsigned int res, int layers, int x, int y, unsigned int w, unsigned int h );


  /// Get codec version
  /** @return codec version */
  static std::string getCodecVersion(){ return grk_version(); };


};

#endif
