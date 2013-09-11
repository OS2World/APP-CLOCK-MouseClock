/*
 * mouseclock.c -- A little program to display the current time using
 * the X root cursor
 * Author          : Jérôme Santini  <js@banane>
 * Created On      : Wed Mar 20 21:47:50 1996
 * Last Modified By: Jérôme Santini  <js@banane>
 * Last Modified On: Sun May  5 18:47:18 1996
 * File Path       : /home/js/misc/src/mouseclock/mouseclock.c
 * Update Count : 54
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

/* fonte par défaut */
/*char *default_font = "-*-lucida-bold-r-*-*-12-*-*-*-*-*-*-*"; */
char *default_font = "-*-helvetica-bold-r-*-*-10-*-*-*-*-*-*-*";

Font police;
XFontStruct *descr_police;

/* ---------------------------------------------------------------- */
Display *Dpy;
int Scr;
Window Root;
int shadow_size = 2;
int shadow_mode = 1;
int ampm = 0;

/* ---------------------------------------------------------------- */
void
perdu (const char *txt)
{
  fprintf (stderr, "mouseclock: ");
  perror (txt);
  exit (1);
}

/* ---------------------------------------------------------------- */
void
usage (const char *nom)
{
  printf ("Mouseclock v1.0\n");
  printf ("A program to display the current time using the X root cursor.\n");
  printf ("Jérôme Santini, Mars 1996\n\n");

  printf ("Usage: %s [opts]\n\n", nom);

  printf ("-bd <size>       shadow size\n");
  printf ("-bg <color>      background color\n");
  printf ("-display <dpy>   display name\n");
  printf ("-fg <color>      foreground color\n");
  printf ("-fn <font_name>  font to use\n");
  printf ("-12hr            use 12-hour format\n");

  exit (1);
}

/* ---------------------------------------------------------------- */
int
largeur_texte (const char *text)
{
  return (2 + XTextWidth (descr_police, text, strlen (text)));
}

/* ---------------------------------------------------------------- */
int
hauteur_texte (const char *text)
{
  return (descr_police->ascent + descr_police->descent);
}

/* ---------------------------------------------------------------- */
int
charger_font (Display * display, const char *nom)
{
  if ((police = XLoadFont (display, nom)))
    if ((descr_police = XQueryFont (display, police)))
      return (1);
  return (0);
}

/* ---------------------------------------------------------------- */
void
liberer_font (Display * display)
{
  XFreeFont (display, descr_police);
}

/* ---------------------------------------------------------------- */
void
restaurer_curseur ()
{
  XUndefineCursor (Dpy, Root);
  XCloseDisplay (Dpy);
  exit (0);
}

/* ---------------------------------------------------------------- */
Cursor
draw_time (int heure, int min, XColor fg, XColor bg)
{
  Pixmap cursor_bitmap, mask_bitmap;

  Cursor cursor;
  char buffer[6];
  int w;
  int h;

  sprintf (buffer, "%02d:%02d", heure, min);

  w = largeur_texte (buffer) + shadow_size;
  h = hauteur_texte (buffer) + shadow_size;

  cursor_bitmap = XCreatePixmap (Dpy,
				 DefaultRootWindow (Dpy),
				 w, h, 1);

  mask_bitmap = XCreatePixmap (Dpy,
			       DefaultRootWindow (Dpy),
			       w, h, 1);

  if (cursor_bitmap && mask_bitmap)
    {
      GC gc1 = XCreateGC (Dpy, cursor_bitmap, 0, NULL);
      GC gc2 = XCreateGC (Dpy, mask_bitmap, 0, NULL);

      if (gc1 && gc2)
	{
	  int len = strlen (buffer);
	  int x = 0;
	  int y = descr_police->ascent;
	  int offset = shadow_mode ? 0 : (shadow_size / 2 + shadow_size % 2);

	  XSetForeground (Dpy, gc1, 0);
	  XSetForeground (Dpy, gc2, shadow_mode ? 0 : 1);
	  XSetFont (Dpy, gc1, police);
	  XSetFont (Dpy, gc2, police);
	  XFillRectangle (Dpy, cursor_bitmap, gc1, 0, 0, w, h);
	  XFillRectangle (Dpy, mask_bitmap, gc2, 0, 0, w, h);

	  XSetForeground (Dpy, gc1, 1);
	  XSetForeground (Dpy, gc2, shadow_mode ? 1 : 0);

	  XDrawString (Dpy, cursor_bitmap, gc1,
		       x + offset, y + offset, buffer, len);

	  if (shadow_mode)
	    {
	      int i;

	      for (i = 0; i <= shadow_size; ++i)
		XDrawString (Dpy, mask_bitmap, gc2, x + i, y + i, buffer, len);
	    }

	  XFreeGC (Dpy, gc1);
	  XFreeGC (Dpy, gc2);
	}
      else
	{
	  perdu ("XCreateGC");
	}

    }
  else
    {
      perdu ("XCreatePixmap");
    }

  cursor = XCreatePixmapCursor (Dpy, cursor_bitmap, mask_bitmap, &fg, &bg,
				0, 0);

  XFreePixmap (Dpy, cursor_bitmap);
  XFreePixmap (Dpy, mask_bitmap);

  return (cursor);
}

/* taken from xancur ---------------------------------------------- */
/* Resolve name to actual X color. */
XColor
NameToXColor (char *name, unsigned long pixel)
{
  XColor color;

  if (name && *name)
    {
      if (!XParseColor (Dpy, DefaultColormap (Dpy, Scr), name, &color))
	{
	  fprintf (stderr,
		   "mouseclock: unknown color or bad color format: %s\n",
		   name);
	  usage ("mouseclock");
	}
    }
  else
    {
      color.pixel = pixel;
      XQueryColor (Dpy, DefaultColormap (Dpy, Scr), &color);
    }

  return (color);
}

/* ---------------------------------------------------------------- */
int
main (int argc, char **argv)
{
  char *display_name = NULL;
  char *Foreground = NULL;
  char *Background = NULL;
  int i;
  int delay = 5;
  char *Fontname = default_font;

  for (i = 1; i < argc; i++)
    {
      if (!strcmp ("-display", argv[i]) || !strcmp ("-d", argv[i]))
	{
	  if (++i >= argc)
	    usage (argv[0]);
	  display_name = argv[i];
	  continue;
	}
      if (!strcmp ("-fn", argv[i]))
	{
	  if (++i >= argc)
	    usage (argv[0]);
	  Fontname = argv[i];
	  continue;
	}
      if (!strcmp ("-fg", argv[i]))
	{
	  if (++i >= argc)
	    usage (argv[0]);
	  Foreground = argv[i];
	  continue;
	}
      if (!strcmp ("-bg", argv[i]))
	{
	  if (++i >= argc)
	    usage (argv[0]);
	  Background = argv[i];
	  continue;
	}
      if (!strcmp ("-bd", argv[i]))
	{
	  if (++i >= argc)
	    usage (argv[0]);
	  shadow_size = atoi (argv[i]);
	  continue;
	}
      if (!strcmp ("-delay", argv[i]))
	{
	  if (++i >= argc)
	    usage (argv[0]);
	  delay = atoi (argv[i]);
	  continue;
	}
      if (!strcmp ("-box", argv[i]))
	{
	  shadow_mode = 0;
	  continue;
	}
      if (!strcmp ("-12hr", argv[i]))
	{
	  ampm = 1;
	  continue;
	}
      usage (argv[0]);
    }

  if (!(Dpy = XOpenDisplay (display_name)))
    {
      fprintf (stderr,
	       "mouseclock:  unable to open display '%s'\n",
	       XDisplayName (display_name));
      exit (-1);
    }
  Root = RootWindow (Dpy, Scr = DefaultScreen (Dpy));

  signal (SIGTERM, restaurer_curseur);
  signal (SIGHUP, restaurer_curseur);
  signal (SIGINT, restaurer_curseur);

  if (charger_font (Dpy, Fontname))
    {
      XColor fg = NameToXColor (Foreground, WhitePixel (Dpy, Scr));
      XColor bg = NameToXColor (Background, BlackPixel (Dpy, Scr));
      int h, m;
      int last_h = -1, last_m = -1;

      for (;;)
	{
	  time_t now = time (NULL);
	  struct tm *now_tm = localtime (&now);
	  Cursor cur;

	  h = ampm && (now_tm->tm_hour - 12 > 0) ? now_tm->tm_hour - 12 : now_tm->tm_hour;
	  m = now_tm->tm_min;

	  if (h != last_h || m != last_m)
	    {
	      cur = draw_time (h, m, fg, bg);
	      XDefineCursor (Dpy, Root, cur);
	      XFlush (Dpy);
	      XFreeCursor (Dpy, cur);
	      last_h = h;
	      last_m = m;
	    }

	  sleep (delay);
	}
    }
  else
    {
      perdu (Fontname);
    }

  restaurer_curseur ();
  return (0);
}

/* EOF ------------------------------------------------------------ */
