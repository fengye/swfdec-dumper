
#include <swfdec/swfdec.h>
#include <stdio.h>
#include <stdlib.h>

static int compare_images(cairo_surface_t* image1, cairo_surface_t* image2)
{
  
  unsigned char* data1, *data2;
  int i, j, width, height, row_stride;
  int diff = 0;

  if (!image1 || !image2 )
  {
    return INT_MAX;
  }
  if (cairo_image_surface_get_width(image1) != cairo_image_surface_get_width(image2) ||
      cairo_image_surface_get_height(image1) != cairo_image_surface_get_height(image2))
  {
    return INT_MAX;
  }

  width = cairo_image_surface_get_width(image1);
  height = cairo_image_surface_get_height(image2);

  data1 = cairo_image_surface_get_data(image1);
  data2 = cairo_image_surface_get_data(image2);

  row_stride = cairo_image_surface_get_stride(image1);

  for(i = 0; i < height; ++i )
  {
    for(j = 0; j < width*4; ++j )
    {
      if ( data1[i*row_stride+j] != data2[i*row_stride+j] )
      {
        diff++;
      }
    }
  }
  return diff;
}

static void print_usage( const char* appname)
{
  printf("Usage: %s {-w: _width_} {-h: _height_} {-f: _frames_} [input swf] [output filename]\n", appname );
}

int main (int argc, char **argv)
{
  SwfdecPlayer *player;
  SwfdecURL *url;
  
  int i;
  char output_filename_serial[256] = {0};
  unsigned long frame_interval = 0;
  int frame_count = 1;
  unsigned int override_width = 0;
  unsigned int override_height = 0;
  const char* input_filename = NULL;
  const char* output_filename = NULL;
  int base_index = 0;
  cairo_surface_t *surface[2] = {NULL, NULL};
  cairo_t *cr[2] = {NULL, NULL};

  /* Parse the command line arguments */
  for(i = 1; i < argc; ++i )
  {
    const char* arg = argv[i];
    if ( arg[0] == '-' )
    {
      if ( arg[1] == 'w' )
      {
        override_width = atoi(arg+3);
      }
      else if ( arg[1] == 'h' )
      {
        override_height = atoi(arg+3);
      }
      else if ( arg[1] == 'f' )
      {
        frame_count = atof(arg+3);
      }
      else
      {
        printf("Unknown option: %s\n", arg);
      }
    }
    else
    {
      if (!input_filename )
      {
        input_filename = arg;
      }
      else
      {
        output_filename = arg;
      }
    }
  }

  if (!input_filename || !output_filename)
  {
    print_usage(argv[0]);
    return 1;
  }

  // init
  swfdec_init ();

  url = swfdec_url_new_from_input (input_filename);
  if (!url)
  {
     printf("Input file not found: %s.\n", input_filename);
     return 1;
  }

  player = swfdec_player_new (NULL);
  swfdec_player_set_url (player, url);

  swfdec_player_advance (player, 0);

  if ( override_width == 0 || override_height == 0 )
  {
    swfdec_player_get_default_size(player, &override_width, &override_height);
  }
  else
  {
    /* need to set player size */
    swfdec_player_set_size(player, override_width, override_height);
  }

  printf("Default frame rate: %f\n", swfdec_player_get_rate(player));
  frame_interval = (unsigned long)(1000.0 / swfdec_player_get_rate(player));
  printf("Default frame interval: %ld\n", frame_interval);

  //swfdec_player_advance (player, frame_interval/2);

  for(i = 0; i < frame_count; ++i )
  {
    int new_index = (base_index + 1) % 2;
    surface[new_index] = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, override_width, override_height);
    cr[new_index] = cairo_create (surface[new_index]);
    // render the image
    swfdec_player_render (player, cr[new_index]);

    

#if 1
    if ( compare_images(surface[base_index], surface[new_index]) <= 5 )
    {
      cairo_destroy (cr[new_index]);
      cairo_surface_destroy (surface[new_index]);
      cr[new_index] = NULL;
      surface[new_index] = NULL;

      frame_interval = swfdec_player_get_next_event(player);
      if ( frame_interval > 0 )
        swfdec_player_advance (player, frame_interval);
    }
    else
#endif
    {

      cairo_destroy (cr[base_index]);
      cairo_surface_destroy (surface[base_index]);
      cr[base_index] = NULL;
      surface[base_index] = NULL;

      sprintf(output_filename_serial, "%s%3.3d.png", output_filename, i+1);
      cairo_surface_write_to_png (surface[new_index], output_filename_serial);
      frame_interval = swfdec_player_get_next_event(player);
      if ( frame_interval > 0 )
        swfdec_player_advance (player, frame_interval);
      

      base_index = new_index;
    }
  }

  cairo_destroy (cr[base_index]);
  cairo_surface_destroy (surface[base_index]);

  g_object_unref (player);
  swfdec_url_free(url);

  return 0;
}


