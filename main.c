#include <unistd.h>      /* pause() */

#include <xcb/xcb.h>
#include <math.h>

#define WIN_SIZE 300

#define PI 3.1415

xcb_point_t *generate_polygon(int n, int radius, float tilt, int x, int y)
{
	  xcb_point_t *polygon = calloc(1, sizeof (xcb_point_t) * (n+1));

	  int i;
	  for(i = 0; i < n+1; i++)
	  {
		  polygon[i].x = x + radius * cos(PI/2 + 2*i*PI/n);
		  polygon[i].y = y + radius * sin(PI/2 + 2*i*PI/n);
	  }

	  return polygon;
}

void draw_polygon(xcb_connection_t *c, xcb_window_t win, xcb_gcontext_t ctx, int nopts, xcb_point_t *points)
{
	  static xcb_gcontext_t _black;

	  if(!_black)
	  {
		  _black = xcb_generate_id(c);
		  uint32_t values[2];
		  values[0] = 0;
		  values[1] = 0;
		  xcb_create_gc(c, _black, win, XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES, values);
	  }


	  xcb_fill_poly(c, win, ctx, XCB_POLY_SHAPE_NONCONVEX, XCB_COORD_MODE_ORIGIN, nopts, points);
	  xcb_poly_line(c, XCB_COORD_MODE_ORIGIN, win, _black, nopts+1, points);
}

int
main ()
{
  xcb_connection_t *c;
  xcb_screen_t     *screen;
  xcb_window_t      win;
  xcb_generic_event_t *e;

  /* Open the connection to the X server */
  c = xcb_connect (NULL, NULL);


  /* Get the first screen */
  screen = xcb_setup_roots_iterator (xcb_get_setup (c)).data;
  
  printf("screen->white_pixel = %x\n", screen->white_pixel);
  printf("screen->black_pixel = %x\n", screen->black_pixel);
  printf("screen->width_in_pixels = %d\n", screen->width_in_pixels);
  printf("screen->height_in_pixels = %d\n", screen->height_in_pixels);
  printf("screen->width_in_millimeters = %d\n", screen->width_in_millimeters);
  printf("screen->height_in_millimeters = %d\n", screen->height_in_millimeters);
  printf("screen->root_depth = %d\n", screen->root_depth);



  /* Ask for our window's Id */
  win = xcb_generate_id(c);
  
  uint32_t values[2];
  values[0] = screen->white_pixel;
  values[1] = XCB_EVENT_MASK_EXPOSURE;
  
  int xpos = (screen->width_in_pixels - WIN_SIZE)/2;
  int ypos = (screen->height_in_pixels - WIN_SIZE)/2;

  /* Create the window */
  xcb_create_window (c,                             /* Connection          */
                     XCB_COPY_FROM_PARENT,          /* depth (same as root)*/
                     win,                           /* window Id           */
                     screen->root,                  /* parent window       */
                     0, 0,                          /* x, y                */
                     WIN_SIZE, WIN_SIZE,            /* width, height       */
                     10,                            /* border_width        */
                     XCB_WINDOW_CLASS_INPUT_OUTPUT, /* class               */
                     screen->root_visual,           /* visual              */
                     XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, values);                      /* masks, not used yet */

  /* Map the window on the screen */
  xcb_map_window (c, win);
  
  /* create graphic context */
  xcb_gcontext_t    black, green, red, blue, yellow;
  black = xcb_generate_id(c);
  values[0] = screen->black_pixel;
  values[1] = 0;
  green = xcb_generate_id(c);
  xcb_create_gc(c, black, screen->root, XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES, values);
  values[0] = 0x00FF00;
  xcb_create_gc(c, green, screen->root, XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES, values);
  red = xcb_generate_id(c);
  values[0] = 0xFF0000;
  xcb_create_gc(c, red, screen->root, XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES, values);
  yellow = xcb_generate_id(c);
  values[0] = 0xFFFF00;
  xcb_create_gc(c, yellow, screen->root, XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES, values);
  blue = xcb_generate_id(c);
  values[0] = 0x0000FF;
  xcb_create_gc(c, blue, screen->root, XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES, values);

  
  xcb_point_t pts [] = 
  {
  	{ 30, 30 },
  	{ 60, 30 },
  	{ 60, 60 },
  	{ 30, 60 },
  	{ 30, 30 }
  };
  
  xcb_point_t *polygon = generate_polygon(5, 100, PI/2, 100, 100);


  /* Make sure commands are sent before we pause, so window is shown */
  xcb_flush (c);

  while((e = xcb_wait_for_event(c)))
  {
	  switch(e->response_type & ~0x80)
	  {
	  case XCB_EXPOSE:
		  printf("Expose!\n");

		  xcb_point_t *pts = generate_polygon(4, 60, PI/4, 80, 80);
		  draw_polygon(c, win, green, 4, pts);
		  free(pts);

		  pts = generate_polygon(5, 80, PI/4, 110, 80);
		  draw_polygon(c, win, red, 5, pts);
		  free(pts);

		  int i;
		  for(i = 0; i < 500; i++)
		  {
			  int x = WIN_SIZE / 10 * (rand()%10);
			  int y = WIN_SIZE / 10 * (rand()%10);
			  int n = 2 + log(rand() % 10000);
			  xcb_gcontext_t contexts[] = { red, green, blue, yellow, black };

			  int no_context = rand()%5;
			  float tilt = PI / 2 * (rand()%4);
			  int radius = 5 * (rand()%15);

			  printf("n=%d, radius=%d, tilt=%f, x=%d, y=%d\n", n, radius, tilt*180/PI, x, y);

			  pts = generate_polygon(n, radius, tilt, x, y);
			  draw_polygon(c, win, contexts[no_context], n, pts);
			  free(pts);
		  }

		  xcb_flush(c);
		  break;
	  default:
		  printf("unknown event %x\n", e->response_type);
		  break;
	  }
	  free(e);
  }

  free(polygon);

  pause ();    /* hold client until Ctrl-C */

  return 0;
}
