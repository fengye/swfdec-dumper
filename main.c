
#include <swfdec/swfdec.h>

int main (int argc, char **argv)
{
  SwfdecPlayer *player;
  SwfdecURL *url;
  cairo_surface_t *surface;
  cairo_t *cr;

  // init
  swfdec_init ();

  url = swfdec_url_new_from_input (argv[1]);

  player = swfdec_player_new (NULL);
  swfdec_player_set_url (player, url);

  swfdec_player_advance (player, 100);

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1024, 768);
  cr = cairo_create (surface);

  // render the image
  swfdec_player_render (player, cr);

  cairo_destroy (cr);
  g_object_unref (player);
  swfdec_url_free(url);

  cairo_surface_write_to_png (surface, argv[2]);
  cairo_surface_destroy (surface);

  return 0;
}


