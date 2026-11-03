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


#include "GrokMTImage.h"
#include "Logger.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <cmath>
#ifdef GROK_DEBUG
#include "Timer.h"
#endif


using namespace std;


// Reference our logging object
extern Logger logfile;

unsigned int GrokMTImage::num_threads = 4; // overridden in setupLogging() from GROK_NUM_THREADS


// Handle info, warning and error messages from Grok. Unlike OpenJPEG's
// per-codec error callback, Grok's message handlers are installed once,
// globally, for the whole process (see GrokMTImage::setupLogging()).
static void grokmt_error_callback( const char* msg, void* ){
  if( IIPImage::logging ) logfile << "GrokMT error :: " << msg;
}

#ifdef GROK_DEBUG
static void grokmt_warning_callback( const char* msg, void* ){
  if( IIPImage::logging ) logfile << "GrokMT warning :: " << msg;
}
static void grokmt_info_callback( const char* msg, void* ){
  if( IIPImage::logging ) logfile << "GrokMT info :: " << msg;
}
#endif



void GrokMTImage::setupLogging()
{
  grk_msg_handlers handlers = {};
  handlers.error_callback = grokmt_error_callback;
#ifdef GROK_DEBUG
  handlers.warn_callback = grokmt_warning_callback;
  handlers.info_callback = grokmt_info_callback;
#endif
  grk_set_msg_handlers( handlers );

  // Read desired per-decompress thread count. Default of 4 was chosen from
  // benchmarking at the production 256x256 tile size (see
  // GROK_MT_COMPARISON.md): 1->~9.6ms, 2->~7.5ms, 4->~7.1ms (best),
  // 8->~8.3ms (oversubscription regression).
  const char* env = getenv( "GROK_NUM_THREADS" );
  if( env ){
    int n = atoi( env );
    if( n > 0 ) num_threads = (unsigned int) n;
  }

  if( IIPImage::logging ){
    logfile << "GrokMT :: Using " << num_threads << " threads per tile decode" << endl;
  }

  // Size Grok's shared worker pool to match our per-call thread budget.
  grk_initialize( NULL, num_threads, NULL );
}



void GrokMTImage::openImage()
{
  string filename = getFileName( currentX, currentY );

  // Update our timestamp
  updateTimestamp( filename );

  grk_stream_params stream_params = {};
  stream_params.is_read_stream = true;
  snprintf( stream_params.file, sizeof(stream_params.file), "%s", filename.c_str() );

  grk_decompress_parameters params = {};
  params.num_threads = num_threads;

#ifdef GROK_DEBUG
  Timer timer;
  timer.start();
#endif

  // Create the decompression codec and open the stream
  _codec = grk_decompress_init( &stream_params, &params );
  if( !_codec ){
    throw file_error( "GrokMT :: Unable to open '" + filename + "'" );
  }

#ifdef GROK_DEBUG
  logfile << "GrokMT :: openImage() :: " << "Stream created" << endl;
#endif

  // Read header
  memset( &_header_info, 0, sizeof(_header_info) );
  if( !grk_decompress_read_header( _codec, &_header_info ) ){
    throw file_error( "GrokMT :: process() :: grk_decompress_read_header() failed" );
  }

#ifdef GROK_DEBUG
  logfile << "GrokMT :: openImage() :: " << "Header read" << endl;
#endif

  // Load our metadata if not already loaded
  if( bpc == 0 ) loadImageInfo( currentX, currentY );

#ifdef GROK_DEBUG
  logfile << "GrokMT :: openImage() :: " << timer.getTime() << " microseconds" << endl;
#endif
}



void GrokMTImage::closeImage()
{
#ifdef GROK_DEBUG
  Timer timer;
  timer.start();
#endif

  if( _codec ){
    grk_object_unref( _codec );
    _codec = NULL;
  }

#ifdef GROK_DEBUG
  logfile << "GrokMT :: closeImage() :: " << timer.getTime() << " microseconds" << endl;
#endif
}



void GrokMTImage::loadImageInfo( int seq, int ang )
{

#ifdef GROK_DEBUG
  Timer timer;
  timer.start();
#endif

  numResolutions = _header_info.numresolutions;
  quality_layers = _header_info.num_layers;

  channels = _header_info.header_image.numcomps;
  bpc = _header_info.header_image.comps[0].prec;

  // Empty any existing list of available resolution sizes
  image_widths.clear();
  image_heights.clear();

  // Save first resolution level
  unsigned int w = _header_info.header_image.x1 - _header_info.header_image.x0;
  unsigned int h = _header_info.header_image.y1 - _header_info.header_image.y0;
  image_widths.push_back(w);
  image_heights.push_back(h);

#ifdef GROK_DEBUG
  logfile << "GrokMT :: DWT Levels: " << numResolutions << endl;
  logfile << "GrokMT :: Resolution : " << w << "x" << h << endl;
#endif

  // Loop through each resolution and calculate the image dimensions -
  // for JPEG2000, these are defined as ceil(x/2)
  for( unsigned int c=1; c<numResolutions; c++ ){
    w = ceil( w / 2.0 );
    h = ceil( h / 2.0 );
    image_widths.push_back(w);
    image_heights.push_back(h);
#ifdef GROK_DEBUG
    logfile << "GrokMT :: Resolution : " << w << "x" << h << endl;
#endif
  }

  // If we don't have enough resolutions to fit a whole image into a single tile
  // we need to generate them ourselves virtually.
  unsigned int n = 1;
  w = image_widths[0];
  h = image_heights[0];
  while( (w>tile_widths[0]) || (h>tile_heights[0]) ){
    n++;
    w = ceil( w / 2.0 );
    h = ceil( h / 2.0 );
    if( n > numResolutions ){
      image_widths.push_back(w);
      image_heights.push_back(h);
    }
  }

  if( n > numResolutions ){
#ifdef GROK_DEBUG
    logfile << "GrokMT :: Warning! Insufficient resolution levels in JPEG2000 stream. Will generate "
	    << n-numResolutions << " extra levels dynamically -" << endl
	    << "GrokMT :: However, you are advised to regenerate the file with at least " << n << " levels" << endl;
#endif
    virtual_levels = n-numResolutions;
  }
  numResolutions = n;

  // Need to assign basic colorspace information
  if( channels == 1 ){
    colorspace = (bpc==1)? ColorSpace::BINARY : ColorSpace::GREYSCALE;
  }
  else if( channels == 3 ) colorspace = ColorSpace::sRGB;

#ifdef GROK_DEBUG
  logfile << "GrokMT :: " << bpc << " bit data" << endl
	  << "GrokMT :: " << channels << " channels" << endl
	  << "GrokMT :: " << quality_layers << " quality layers detected" << endl;
#endif

  // For bilevel images, force channels to 1 as we sometimes come across such images which claim 3 channels
  if( bpc == 1 ) channels = 1;

  // Get the max and min values for our data type
  for( unsigned int i=0; i<channels; i++ ){
    min.push_back( 0.0 );
    if( bpc > 8 && bpc < 16 ) max.push_back( 1<<bpc );
    if( bpc == 16 ) max.push_back( 65535.0 );
    else max.push_back( 255.0 );
  }

  // Indicate that our metadata has been read
  isSet = true;

#ifdef GROK_DEBUG
  logfile << "GrokMT :: loadImageInfo() :: " << timer.getTime() << " microseconds" << endl;
#endif
}



// Get an individual tile
RawTile GrokMTImage::getTile( int seq, int ang, unsigned int res, int layers, unsigned int tile, ImageEncoding e )
{

  // Scale up our output bit depth to the nearest factor of 8
  unsigned obpc = bpc;
  if( bpc <= 16 && bpc > 8 ) obpc = 16;
  else if( bpc <= 8 ) obpc = 8;

#ifdef GROK_DEBUG
  Timer timer;
  timer.start();
#endif

  if( res > numResolutions ){
    ostringstream tile_no;
    tile_no << "GrokMT :: Asked for non-existent resolution: " << res;
    throw file_error( tile_no.str() );
  }

  int vipsres = getNativeResolution( res );

  unsigned int tw = tile_widths[0];
  unsigned int th = tile_heights[0];

  // Get the width and height for last row and column tiles
  unsigned int rem_x = image_widths[vipsres] % tile_widths[0];
  unsigned int rem_y = image_heights[vipsres] % tile_heights[0];

  // Calculate the number of tiles in each direction
  unsigned int ntlx = (image_widths[vipsres] / tile_widths[0]) + (rem_x == 0 ? 0 : 1);
  unsigned int ntly = (image_heights[vipsres] / tile_heights[0]) + (rem_y == 0 ? 0 : 1);

  // Check whether requested tile exists
  if( tile >= ntlx*ntly ){
    ostringstream tile_no;
    tile_no << "GrokMT :: Asked for non-existent tile: " << tile;
    throw file_error( tile_no.str() );
  }

  // Alter the tile size if it's in the last column
  if( ( tile % ntlx == ntlx - 1 ) && ( rem_x != 0 ) ) {
    tw = rem_x;
  }

  // Alter the tile size if it's in the bottom row
  if( ( tile / ntlx == ntly - 1 ) && rem_y != 0 ) {
    th = rem_y;
  }

  // Calculate the pixel offsets for this tile
  int xoffset = (tile % ntlx) * tile_widths[0];
  int yoffset = (unsigned int) floor((double)(tile/ntlx)) * tile_heights[0];

#ifdef GROK_DEBUG
  logfile << "GrokMT :: Tile size: " << tw << "x" << th << " @" << channels << endl;
#endif

  // Grok (like OpenJPEG) only supports 8 or 16 bit output
  if( !( (obpc == 8) || (obpc == 16) ) ) throw file_error( "GrokMT :: Unsupported number of bits" );

  // Create our Rawtile object and initialize with data
  RawTile rawtile( tile, res, seq, ang, tw, th, channels, obpc );
  rawtile.filename = getImagePath();
  rawtile.timestamp = timestamp;
  rawtile.allocate();

  // Process the tile
  process( res, layers, xoffset, yoffset, tw, th, rawtile.data );

#ifdef GROK_DEBUG
  logfile << "GrokMT :: getTile() :: " << timer.getTime() << " microseconds" << endl;
#endif

  return rawtile;
}



// Get an entire region and not just a tile
RawTile GrokMTImage::getRegion( int ha, int va, unsigned int res, int layers, int x, int y, unsigned int w, unsigned int h ){

  // Scale up our output bit depth to the nearest factor of 8
  unsigned int obpc = bpc;
  if( bpc <= 16 && bpc > 8 ) obpc = 16;
  else if( bpc <= 8 ) obpc = 8;

#ifdef GROK_DEBUG
  Timer timer;
  timer.start();
#endif

  // Grok (like OpenJPEG) only supports 8 or 16 bit output
  if( !( (obpc == 8) || (obpc == 16) ) ) throw file_error( "GrokMT :: Unsupported number of bits" );

  RawTile rawtile( 0, res, ha, va, w, h, channels, obpc );
  rawtile.filename = getImagePath();
  rawtile.timestamp = timestamp;
  rawtile.allocate();

  process( res, layers, x, y, w, h, rawtile.data );

#ifdef GROK_DEBUG
  logfile << "GrokMT :: getRegion() :: " << timer.getTime() << " microseconds" << endl;
#endif

  return rawtile;
}



// Main processing function
void GrokMTImage::process( unsigned int res, int layers, int xoffset, int yoffset, unsigned int tw, unsigned int th, void *d )
{
  // Same per-request codec open/close lifecycle as GrokImage/OpenJPEGImage/
  // KakaduImage - grk_decompress_update()-based codec reuse across distinct
  // windows was tested and found to produce incorrect pixel data (silently
  // returns true but stale/wrong output), so it is NOT used here. The only
  // difference from GrokImage is params.num_threads > 1 below, which Grok's
  // public API does support correctly for a single decompress() call.
  closeImage();

  // Scale up our output bit depth to the nearest factor of 8
  unsigned int obpc = bpc;
  if( bpc <= 16 && bpc > 8 ) obpc = 16;
  else if( bpc <= 8 ) obpc = 8;

  unsigned int factor = 1;                  // Downsampling factor - set it to default value
  int vipsres = getNativeResolution( res ); // Reverse resolution number

  // Calculate number of extra resolutions needed that have not been encoded in the image
  if( res < virtual_levels ){
    factor = 2 * (virtual_levels - res);
    xoffset *= factor;
    yoffset *= factor;
    tw *= factor;
    th *= factor;
    // Set our resolution level back to the smallest original resolution
    vipsres = numResolutions - 1 - virtual_levels;
#ifdef GROK_DEBUG
    logfile << "GrokMT :: using smallest existing resolution " << virtual_levels << endl;
#endif
  }

  // Set the number of layers to half of the number of detected layers if we have not set the
  // layers parameter manually. If layers is set to less than 0, use all layers.
  if( layers < 0 ) layers = quality_layers;
  else if( layers == 0 ) layers = ceil( quality_layers/2.0 );

  // Also make sure we have at least 1 layer
  if( layers < 1 ) layers = 1;

  // Requested image location and size projected onto full resolution image.
  unsigned int x0 = xoffset << vipsres;
  unsigned int y0 = yoffset << vipsres;
  unsigned int w0 = (xoffset + tw) << vipsres;
  unsigned int h0 = (yoffset + th) << vipsres;

  // Make sure we don't overrun the dimensions of the full-size image
  if( x0 > image_widths[0]  ) x0 = image_widths[0]  - 1;
  if( y0 > image_heights[0] ) y0 = image_heights[0] - 1;
  if( w0 > image_widths[0]  ) w0 = image_widths[0];
  if( h0 > image_heights[0] ) h0 = image_heights[0];

#ifdef GROK_DEBUG
  logfile << "GrokMT :: decoding " << layers << " quality layers using " << num_threads << " threads" << endl;
  logfile << "GrokMT :: requested region at requested resolution: position: "
	  << xoffset << "x" << yoffset << ". size: " << tw << "x" << th << endl;
  logfile << "GrokMT :: region size mapped to full resolution: " << (tw<<vipsres) << "x" << (th<<vipsres) << endl;
#endif

  string filename = getFileName( currentX, currentY );

  grk_stream_params stream_params = {};
  stream_params.is_read_stream = true;
  snprintf( stream_params.file, sizeof(stream_params.file), "%s", filename.c_str() );

  grk_decompress_parameters params = {};
  params.num_threads = num_threads;
  params.core.reduce = (uint8_t) vipsres;
  params.core.layers_to_decompress = (uint16_t) layers;
  params.dw_x0 = (double) x0;
  params.dw_y0 = (double) y0;
  params.dw_x1 = (double) w0;
  params.dw_y1 = (double) h0;
  params.dw_reduced = false;

  _codec = grk_decompress_init( &stream_params, &params );
  if( !_codec ){
    throw file_error( "GrokMT :: process() :: grk_decompress_init() failed for '" + filename + "'" );
  }

  if( !grk_decompress_read_header( _codec, NULL ) ){
    throw file_error( "GrokMT :: process() :: grk_decompress_read_header() failed" );
  }

  if( !grk_decompress( _codec, NULL ) ){
    throw file_error( "GrokMT :: process() :: grk_decompress() failed" );
  }

  grk_image* image = grk_decompress_get_image( _codec );
  if( !image ){
    throw file_error( "GrokMT :: process() :: grk_decompress_get_image() returned NULL" );
  }

  // Extract any ICC profile - can only get it after decoding
  if( image->meta && image->meta->color.icc_profile_buf && image->meta->color.icc_profile_len > 0 ){
    metadata.insert( {"icc", string((const char*)image->meta->color.icc_profile_buf, image->meta->color.icc_profile_len)} );
#ifdef GROK_DEBUG
    logfile << "GrokMT :: ICC profile detected with size " << image->meta->color.icc_profile_len << endl;
#endif
  }

  // Copy our decoded data by looping over all pixels (same access pattern as GrokImage).
  size_t n = 0;

  for( unsigned int j=0; j<th; j+=factor ){
    for( unsigned int i=0; i<tw; i+=factor ){
      for( unsigned int k=0; k<channels && k<image->numcomps; k++ ){
        grk_image_comp& c = image->comps[k];
        size_t index = (size_t)j * c.stride + i;

        int32_t raw;
        if( c.data_type == GRK_INT_16 ) raw = ((const int16_t*)c.data)[index];
        else raw = ((const int32_t*)c.data)[index];

        // Handle 16 and 8 bit output data. We also need to make sure we
        // scale intermediate bit depths to 8 or 16 bit range.
        if( obpc == 16 ){
          unsigned short       v = (unsigned short)(raw & 0x0000ffff);
          if( bpc == 10 )      v = (v << 6) | (v >> 4);
          else if( bpc == 12 ) v = (v << 4) | (v >> 8);
          else if( bpc == 14 ) v = (v << 2) | (v >> 12);
          ((unsigned short*)d)[n++] = v;
        }
        else{
          // 1 bit binary (bi-level), 2 bit, 4 bit and 6 bit images need to be scaled up to 8 bit range
          unsigned char        v = (unsigned char)(raw & 0x000000ff);
          if( bpc == 1 )      v *= 255;
          else if( bpc == 2 ) v *= 85;
          else if( bpc == 4 ) v *= 17;
          else if( bpc == 6 ) v = (v << 2) | (v >> 4);
          ((unsigned char*)d)[n++] = v;
        }
      }
    }
  }

  // We need to close the codec here in case we try to use it multiple times
  // in the same request pipeline
  closeImage();
}
