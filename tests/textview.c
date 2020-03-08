/* GtkTextView test
 */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

void
focus_out_cb(GtkWidget* widget) {
  g_assert(GTK_IS_TEXT_VIEW(widget));
  gtk_im_context_reset(GTK_TEXT_VIEW(widget)->im_context);
}

int
key_pressed(GtkWidget* widget, GdkEventKey* event) {
  if( event->keyval == GDK_Escape ) {
    gtk_main_quit();
  }

  return FALSE;
}

int main(int argc, char** argv) {
  GtkWidget* window;
  GtkWidget* textview;
  GtkTextBuffer* buffer;
  GtkWidget* scrwin;

  gtk_set_locale();
  gtk_init(&argc, &argv);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  textview = gtk_text_view_new();
  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
  scrwin = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_set_border_width(GTK_CONTAINER(scrwin), 10);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrwin), textview);
  gtk_container_add(GTK_CONTAINER(window), scrwin);

  gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
  g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(key_pressed), NULL);
  g_signal_connect(G_OBJECT(textview), "focus_out_event", G_CALLBACK(focus_out_cb), NULL);

  gtk_widget_set_usize(window, 600, 400);
  gtk_widget_show_all(window);

  gtk_main();

  return 0;
}
