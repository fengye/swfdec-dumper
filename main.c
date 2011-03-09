
#include <swfdec/swfdec.h>
#include <stdio.h>
#include <stdlib.h>

static void print_usage( const char* appname)
{
  printf("Usage: %s {-w: _width_} {-h: _height_} {-f: _frames_} [input swf] [output filename]\n", appname );
}

int main (int argc, char **argv)
{
  SwfdecPlayer *player;
  SwfdecURL *url;
  cairo_surface_t *surface;
  cairo_t *cr;
  int i;
  char output_filename_serial[256] = {0};
  unsigned long frame_interval = 0;
  int frame_count = 1;
  unsigned int override_width = 0;
  unsigned int override_height = 0;
  const char* input_filename = NULL;
  const char* output_filename = NULL;

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

  frame_interval = (unsigned long)(1000.0 / swfdec_player_get_rate(player));

  for(i = 0; i < frame_count; ++i )
  {
    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, override_width, override_height);
    cr = cairo_create (surface);
    // render the image
    swfdec_player_render (player, cr);
    sprintf(output_filename_serial, "%s%3.3d.png", output_filename, i+1);
    cairo_surface_write_to_png (surface, output_filename_serial);
    swfdec_player_advance (player, frame_interval);
    cairo_destroy (cr);
    cairo_surface_destroy (surface);
  }

  g_object_unref (player);
  swfdec_url_free(url);

  return 0;
}


