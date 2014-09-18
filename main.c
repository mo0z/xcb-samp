#include <unistd.h>      /* pause() */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xcb/xcb.h>
#include <math.h>
#include <glib.h>

#define WIN_SIZE 300

#define PI 3.1415

GList *polygons;
xcb_point_t *current_polygon;
int current_no_points = 7;
xcb_point_t current_origin;
xcb_point_t current_vertex;
float current_tilt;
float current_radius;
int current_color_index = 0;
int current_color = 0xff0000;
int colors[] = { 0xff0000, 0xffff00, 0x00ff00, 0x00ffff, 0x0000ff, 0xff00ff, 0xffffff, 0x000000,
			     0xff8080, 0xffff80, 0x80ff80, 0x80ffff, 0x8080ff, 0xff80ff, 0x808080 };

int current_width = WIN_SIZE;
int current_height = WIN_SIZE;

typedef struct _polygon_list_element {
	int nvertex;
	xcb_point_t *polygon;
	int polygon_color;
} polygon_list_element;

polygon_list_element*
polygon_list_element_new(int nv, xcb_point_t *polygon, int color)
{
	polygon_list_element *l = calloc(1, sizeof *l);
	l->nvertex = nv;
	l->polygon = polygon;
	l->polygon_color = color;
	return l;
}


xcb_point_t *generate_polygon(int n, int radius, float tilt, int x, int y)
{
	  xcb_point_t *polygon = calloc(1, sizeof (xcb_point_t) * (n+1));

	  int i;
	  for(i = 0; i < n+1; i++)
	  {
		  polygon[i].x = x + radius * cos(tilt + 2*i*PI/n);
		  polygon[i].y = y + radius * sin(tilt + 2*i*PI/n);
	  }

	  return polygon;
}

void draw_polygon(xcb_connection_t *c, xcb_window_t win, uint32_t color, int nopts, xcb_point_t *points)
{
	  static xcb_gcontext_t _black, _color;
	  static uint32_t values[2];

	  if(!_black)
	  {
		  _black = xcb_generate_id(c);
		  values[0] = 0;
		  values[1] = 0;
		  xcb_create_gc(c, _black, win, XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES, values);
	  }

	  values[0] = color;
	  if(!_color)
	  {
		  _color = xcb_generate_id(c);
		  values[1] = 0;
		  xcb_create_gc(c, _color, win, XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES, values);
	  }
	  else
	  {
		  xcb_change_gc(c, _color, XCB_GC_FOREGROUND, values);
	  }

	  xcb_fill_poly(c, win, _color, XCB_POLY_SHAPE_NONCONVEX, XCB_COORD_MODE_ORIGIN, nopts, points);
	  xcb_poly_line(c, XCB_COORD_MODE_ORIGIN, win, _black, nopts+1, points);
}

void draw_text(xcb_connection_t *c, xcb_window_t win, char *text, int x, int y, int fg_color)
{
	static xcb_font_t font;
	static xcb_gcontext_t fcontext;

	if(0 == font)
	{
		font = xcb_generate_id(c);
		xcb_open_font(c, font, strlen("fixed"), "fixed");
	}

	if(0 == fcontext)
	{
		fcontext = xcb_generate_id(c);
		uint32_t values[3];
		values[0] = fg_color;
		values[1] = 0xffffff;
		values[2] = font;

		xcb_create_gc(c, fcontext, win, XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT, values);
	}
	else
	{
		xcb_change_gc(c, fcontext, XCB_GC_FOREGROUND, &fg_color);
	}


	xcb_image_text_8(c, strlen(text), win, fcontext, x, y, text);
}

void draw_demo_box(xcb_connection_t* c, xcb_window_t win, int current_no_points, int current_color) {

	xcb_point_t * demo_box = generate_polygon(4, 20, PI / 4, current_width - 20,
			current_height - 20);
	// draw a bounding square
	draw_polygon(c, win, 0xf0f0ff, 4, demo_box);
	free(demo_box);

	demo_box = generate_polygon(current_no_points, 15, PI / 4, current_width - 20,
			current_height - 20);
	draw_polygon(c, win, current_color, current_no_points, demo_box);
	free(demo_box);

	char *texts[3][2] =
	{
			{ "Left mouse button, drag", "draw_polygon" },
			{ "Wheel up/down", "raise / lower vertex number" },
			{ "Right mouse button", "change color" }
	};

	int i;
	for(i = 0; i < 3; i++)
	{
		draw_text(c, win, texts[i][0], 10, 20 + 15 * i, 0x404030);

		draw_text(c, win, " - ", 6*strlen(texts[i][0]) + 10, 20 + 15 * i, 0x000000);

		draw_text(c, win, texts[i][1], 18 + 6*strlen(texts[i][0]) + 10, 20 + 15 * i, 0x8040ff);

	}
}

int repaint(xcb_connection_t *c, xcb_window_t win)
{
	  GList *p = polygons;

	  static xcb_gcontext_t _clean;
	  static uint32_t values[2];

	  if(!_clean)
	  {
		  _clean = xcb_generate_id(c);
		  values[0] = 0xffffff;
		  values[1] = 0;
		  xcb_create_gc(c, _clean, win, XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES, values);
	  }

	  xcb_rectangle_t r;
	  r.x = 0;
	  r.y = 0;
	  r.width = WIN_SIZE;
	  r.height = WIN_SIZE;
	  xcb_poly_fill_rectangle(c, win, _clean, 1, &r);

	  for( ; p; p = p->next)
	  {
		  polygon_list_element *lelem = (polygon_list_element*)p->data;
		  draw_polygon(c, win, lelem->polygon_color, lelem->nvertex, lelem->polygon);

	  }

	  if(current_polygon)
	  {
		  draw_polygon(c, win, current_color, current_no_points, current_polygon);
	  }

	  draw_demo_box(c, win, current_no_points, current_color);
	  xcb_flush(c);
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
  values[1] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS
		     | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_BUTTON_MOTION;
  
  int xpos = (screen->width_in_pixels - WIN_SIZE)/2;
  int ypos = (screen->height_in_pixels - WIN_SIZE)/2;

  printf("xpos = %d ypos = %d\n", xpos, ypos);

  /* Create the window */
  xcb_create_window (c,                             /* Connection          */
                     XCB_COPY_FROM_PARENT,          /* depth (same as root)*/
                     win,                           /* window Id           */
                     screen->root,                  /* parent window       */
                     50, 50,                          /* x, y                */
                     WIN_SIZE, WIN_SIZE,            /* width, height       */
                     10,                            /* border_width        */
                     XCB_WINDOW_CLASS_INPUT_OUTPUT, /* class               */
                     screen->root_visual,           /* visual              */
                     XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, values);                      /* masks, not used yet */

  /* Map the window on the screen */
  xcb_map_window (c, win);


  /* Make sure commands are sent before we pause, so window is shown */
  xcb_flush (c);

  while((e = xcb_wait_for_event(c)))
  {
	  switch(e->response_type & ~0x80)
	  {
	  case XCB_EXPOSE:
	  {
		  xcb_expose_event_t *ev = (xcb_expose_event_t*)e;


		  printf("Expose! (%d, %d)\n", ev->width, ev->height);
		  current_width = ev->width;
		  current_height = ev->height;
		  repaint(c, win);
	  }
	  break;
	  case XCB_BUTTON_PRESS:
	  {
		  xcb_button_press_event_t *ev = (xcb_button_press_event_t*)e;
		  printf("pressed [%d, %d], detail = %d\n", ev->event_x, ev->event_y, ev->detail);
		  if(XCB_BUTTON_INDEX_1 == ev->detail)
		  {
			  current_origin.x = ev->event_x;
		  	  current_origin.y = ev->event_y;
		  	 // current_no_points = 5;
		  } else if(XCB_BUTTON_INDEX_3 == ev->detail)
	      {
			  current_color_index++;
			  current_color_index %= (sizeof colors / sizeof *colors);
			  current_color = colors[current_color_index];

			  draw_demo_box(c, win, current_no_points, current_color);
			  xcb_flush(c);
	      }
		  else if(XCB_BUTTON_INDEX_4 == ev->detail) /* scroll mouse wheel up */
		  {
			  current_no_points ++;
			  printf("Current points: %d\n" , current_no_points);
			  repaint(c, win);
		  } else if(XCB_BUTTON_INDEX_5 == ev->detail) /* scroll mouse wheel down */
		  {
			  -- current_no_points;
			  printf("Current points: %d\n", current_no_points);
			  repaint(c, win);
		  }
	  }
		  break;
	  case XCB_BUTTON_RELEASE:
	  {
		  xcb_button_release_event_t *ev = (xcb_button_release_event_t*)e;
		  printf("released [%d, %d], detail = %d\n", ev->event_x, ev->event_y, ev->detail);

		  if(XCB_BUTTON_INDEX_1 == ev->detail)
		  {
			  polygons = g_list_append(polygons, polygon_list_element_new(current_no_points, current_polygon, current_color));
		  	  current_polygon = NULL; // free prevention
		  }
		  break;
	  }
	  case XCB_MOTION_NOTIFY:
	  {
		  xcb_motion_notify_event_t *ev = (xcb_motion_notify_event_t*)e;

		  printf("ev->state = %d\n", ev->state);
		  if((ev->state & XCB_EVENT_MASK_BUTTON_1_MOTION) == XCB_EVENT_MASK_BUTTON_1_MOTION)
		  {
			  printf("motion [%d, %d], state = %d\n", ev->event_x, ev->event_y, ev->state);
			  current_vertex.x = ev->event_x;
			  current_vertex.y = ev->event_y;
			  float dx =  (current_vertex.x - current_origin.x);
			  float dy =  (current_vertex.y - current_origin.y);
			  current_radius = sqrt(dx*dx + dy*dy);
			  current_tilt = atan(dy/ dx);

			  printf("Current radius = %f\n", current_radius);
			  printf("Current tilt = %f deg\n", current_tilt * 180.0 / PI);

			  xcb_point_t *cp = current_polygon;
			  current_polygon = generate_polygon(current_no_points, current_radius, current_tilt, current_origin.x, current_origin.y);

			 // draw_polygon(c, win, 0xffcc00, current_no_points, current_polygon);
			  repaint(c, win);
		  }
		  break;
	  }
	  default:
		  printf("unknown event %x\n", e->response_type);
		  break;
	  }
	  free(e);
  }

  //free(polygon);

  pause ();    /* hold client until Ctrl-C */

  return 0;
}
