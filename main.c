
#include <swfdec/swfdec.h>
#include <stdio.h>
#include <stdlib.h>
#include <yaml.h>

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
  printf("Usage: %s {-w: _width_} {-h: _height_} {-s: _scale} {-f: _frames_} {-l: _loop_} {-d: _directory_prefix_} {-x: center_x} {-y: center_y} [input swf] [output filename]\n", appname );
}

int main (int argc, char **argv)
{
  SwfdecPlayer *player;
  SwfdecURL *url;
  yaml_emitter_t emitter;
  yaml_event_t event;
  FILE *output;

  int i;
  const char* directory_prefix = NULL;
  char output_filename_serial[256] = {0};
  unsigned long frame_interval = 0;
  unsigned long accum_interval = 0;
  int frame_count = 1;
  unsigned int override_width = 0;
  unsigned int override_height = 0;
  float override_scale = 1.0f;
  const char* input_filename = NULL;
  const char* output_filename = NULL;
  int base_index = 0;
  cairo_surface_t *surface[2] = {NULL, NULL};
  cairo_t *cr[2] = {NULL, NULL};
  int loop = 0;
  int center_x = -1;
  int center_y = -1;
  
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
      else if ( arg[1] == 's' )
      {
        override_scale = (float)atof(arg+3);
      }
      else if ( arg[1] == 'f' )
      {
        frame_count = atof(arg+3);
      }
      else if ( arg[1] == 'l' )
      {
        loop = atoi(arg+3);
      }
      else if ( arg[1] == 'd' )
      {
        directory_prefix = arg+3;
      }
      else if ( arg[1] == 'x' )
      {
        center_x = atoi(arg+3);
      }
      else if ( arg[1] == 'y' )
      {
        center_y = atoi(arg+3);
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

  /* Create the Emitter object. */
  yaml_emitter_initialize(&emitter);

  
  // init
  swfdec_init ();

  url = swfdec_url_new_from_input (input_filename);
  if (!url)
  {
     printf("Input file not found: %s.\n", input_filename);
     return 1;
  }

  /* Set a file output. */
  sprintf(output_filename_serial, "%s.yaml", output_filename);
  output = fopen(output_filename_serial, "wb");
  yaml_emitter_set_output_file(&emitter, output);
  //yaml_emitter_set_output(&emitter, write_handler, output);

  player = swfdec_player_new (NULL);
  swfdec_player_set_url (player, url);

  /* Create and emit the STREAM-START event. */
  yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING);
  yaml_emitter_emit(&emitter, &event);
  /* Create document */
  yaml_document_start_event_initialize(&event, NULL, NULL, NULL, 1);
  yaml_emitter_emit(&emitter, &event);
  /* Create mapping */
  yaml_mapping_start_event_initialize(&event, NULL, NULL, 1, YAML_ANY_SCALAR_STYLE);
  yaml_emitter_emit(&emitter, &event);

  yaml_scalar_event_initialize(&event, NULL, NULL, (yaml_char_t*)"type", strlen("type"), 1, 1, YAML_ANY_SCALAR_STYLE);
  yaml_emitter_emit(&emitter, &event);  
  yaml_scalar_event_initialize(&event, NULL, NULL, (yaml_char_t*)"keyframe", strlen("keyframe"), 1, 1, YAML_ANY_SCALAR_STYLE);
  yaml_emitter_emit(&emitter, &event);

  swfdec_player_advance (player, 0);

  if ( override_width == 0 || override_height == 0 )
  {
    swfdec_player_get_default_size(player, &override_width, &override_height);
  }

  /* need to set player size */
  swfdec_player_set_size(player, 
    (unsigned int)override_width * override_scale, 
    (unsigned int)override_height * override_scale);

  {
  char buf[256] = {0};

  yaml_scalar_event_initialize(&event, NULL, NULL, (yaml_char_t*)"defaultframerate", strlen("defaultframerate"), 1, 1, YAML_ANY_SCALAR_STYLE);
  yaml_emitter_emit(&emitter, &event);
  sprintf(buf, "%f", swfdec_player_get_rate(player));  
  yaml_scalar_event_initialize(&event, NULL, NULL, (yaml_char_t*)buf, strlen(buf), 1, 1, YAML_ANY_SCALAR_STYLE);
  yaml_emitter_emit(&emitter, &event);

  }

  yaml_scalar_event_initialize(&event, NULL, NULL, (yaml_char_t*)"loop", strlen("loop"), 1, 1, YAML_ANY_SCALAR_STYLE);
  yaml_emitter_emit(&emitter, &event);  
  if ( loop )
  {
    yaml_scalar_event_initialize(&event, NULL, NULL, (yaml_char_t*)"true", strlen("true"), 1, 1, YAML_ANY_SCALAR_STYLE);
    yaml_emitter_emit(&emitter, &event);
  }
  else
  {
    yaml_scalar_event_initialize(&event, NULL, NULL, (yaml_char_t*)"false", strlen("false"), 1, 1, YAML_ANY_SCALAR_STYLE);
    yaml_emitter_emit(&emitter, &event);
  }

  if (center_x >= 0 )
  {
    char buf[256] = {0};
    yaml_scalar_event_initialize(&event, NULL, NULL, (yaml_char_t*)"center_point_x", strlen("center_point_x"), 1, 1, YAML_ANY_SCALAR_STYLE); 
    yaml_emitter_emit(&emitter, &event);

    sprintf(buf, "%d", center_x);
    yaml_scalar_event_initialize(&event, NULL, NULL, (yaml_char_t*)buf, strlen(buf), 1, 1, YAML_ANY_SCALAR_STYLE);
    yaml_emitter_emit(&emitter, &event);
  }
  if (center_y >= 0 )
  {
    char buf[256] = {0};
    yaml_scalar_event_initialize(&event, NULL, NULL, (yaml_char_t*)"center_point_y", strlen("center_point_y"), 1, 1, YAML_ANY_SCALAR_STYLE); 
    yaml_emitter_emit(&emitter, &event);

    sprintf(buf, "%d", center_y);
    yaml_scalar_event_initialize(&event, NULL, NULL, (yaml_char_t*)buf, strlen(buf), 1, 1, YAML_ANY_SCALAR_STYLE);
    yaml_emitter_emit(&emitter, &event);
  }
  

  printf("Default frame rate: %f\n", swfdec_player_get_rate(player));
  frame_interval = (unsigned long)(1000.0 / swfdec_player_get_rate(player));
  printf("Default frame interval: %ld\n", frame_interval);

  
  yaml_scalar_event_initialize(&event, NULL, NULL, (yaml_char_t*)"content", strlen("content"), 1, 1, YAML_ANY_SCALAR_STYLE);
  yaml_emitter_emit(&emitter, &event);  

  /* Create mapping */
  yaml_mapping_start_event_initialize(&event, NULL, NULL, 1, YAML_ANY_SCALAR_STYLE);
  yaml_emitter_emit(&emitter, &event);

  for(i = 0; i < frame_count; ++i )
  {
    int new_index = (base_index + 1) % 2;
    surface[new_index] = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 
      (unsigned int)(override_width * override_scale), 
      (unsigned int)(override_height * override_scale));
    cr[new_index] = cairo_create (surface[new_index]);
    // render the image
    swfdec_player_render (player, cr[new_index]);

    frame_interval = swfdec_player_get_next_event(player);
    accum_interval += frame_interval;

    if ( compare_images(surface[base_index], surface[new_index]) <= 5 )
    {
      cairo_destroy (cr[new_index]);
      cairo_surface_destroy (surface[new_index]);
      cr[new_index] = NULL;
      surface[new_index] = NULL;

      if ( frame_interval > 0 )
        swfdec_player_advance (player, frame_interval);
    }
    else
    {

      if ( surface[base_index] && surface[new_index] )
      {
        char buf[256] = {0};
        sprintf(buf, "%f", (double)accum_interval / 1000.0);
        yaml_scalar_event_initialize(&event, NULL, NULL, (yaml_char_t*)output_filename_serial, strlen(output_filename_serial), 1, 1, YAML_ANY_SCALAR_STYLE);
        yaml_emitter_emit(&emitter, &event);
        yaml_scalar_event_initialize(&event, NULL, NULL, (yaml_char_t*)buf, strlen(buf), 1, 1, YAML_ANY_SCALAR_STYLE);
        yaml_emitter_emit(&emitter, &event);
        accum_interval = 0;
      }

      cairo_destroy (cr[base_index]);
      cairo_surface_destroy (surface[base_index]);
      cr[base_index] = NULL;
      surface[base_index] = NULL;

      if (directory_prefix)
        sprintf(output_filename_serial, "%s/%s%3.3d.png", directory_prefix, output_filename, i+1);
      else
        sprintf(output_filename_serial, "%s%3.3d.png", output_filename, i+1);

      cairo_surface_write_to_png (surface[new_index], output_filename_serial);
      if ( frame_interval > 0 )
        swfdec_player_advance (player, frame_interval);
      
      base_index = new_index;
    }
  }

  /* the last frame output */
  if ( surface[base_index] )
  {
    char buf[256] = {0};
    sprintf(buf, "%f", (double)accum_interval / 1000.0);
    yaml_scalar_event_initialize(&event, NULL, NULL, (yaml_char_t*)output_filename_serial, strlen(output_filename_serial), 1, 1, YAML_ANY_SCALAR_STYLE);
    yaml_emitter_emit(&emitter, &event);
    yaml_scalar_event_initialize(&event, NULL, NULL, (yaml_char_t*)buf, strlen(buf), 1, 1, YAML_ANY_SCALAR_STYLE);
    yaml_emitter_emit(&emitter, &event);
    accum_interval = 0;
  }

  cairo_destroy (cr[base_index]);
  cairo_surface_destroy (surface[base_index]);

  g_object_unref (player);
  swfdec_url_free(url);

  /* End mapping */
  yaml_mapping_end_event_initialize(&event);
  yaml_emitter_emit(&emitter, &event);

  /* End mapping */
  yaml_mapping_end_event_initialize(&event);
  yaml_emitter_emit(&emitter, &event);

  /* End of document */
  yaml_stream_end_event_initialize(&event);
  yaml_emitter_emit(&emitter, &event);

  /* Create and emit the STREAM-END event. */
  yaml_stream_end_event_initialize(&event);
  yaml_emitter_emit(&emitter, &event);

  yaml_emitter_flush(&emitter);
  /* Destroy the Emitter object. */
  yaml_emitter_delete(&emitter);

  fclose(output);

  return 0;
}


