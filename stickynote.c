#include <errno.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define NOTES_DIR ".stickynotes"
#define DEFAULT_WIDTH 300
#define DEFAULT_HEIGHT 300

typedef struct {
  GtkWidget *window;
  GtkWidget *text_view;
  GtkTextBuffer *buffer;
  char *filepath;
  int note_id;
} StickyNote;

void ensure_notes_dir(void) {
  struct stat st = {0};
  const char *home = getenv("HOME");
  char path[512];

  snprintf(path, sizeof(path), "%s/%s", home, NOTES_DIR);

  if (stat(path, &st) == -1) {
    mkdir(path, 0700);
  }
}

char *get_note_filepath(int note_id) {
  const char *home = getenv("HOME");
  char *filepath = malloc(512);
  snprintf(filepath, 512, "%s/%s/note_%d.txt", home, NOTES_DIR, note_id);
  return filepath;
}

char *load_note_content(const char *filepath) {
  FILE *f = fopen(filepath, "r");
  if (!f)
    return NULL;

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *content = malloc(size + 1);
  fread(content, 1, size, f);
  content[size] = '\0';

  fclose(f);
  return content;
}

void save_note_content(const char *filepath, const char *text) {
  FILE *f = fopen(filepath, "w");
  if (!f) {
    fprintf(stderr, "Failed to save note: %s\n", strerror(errno));
    return;
  }

  fprintf(f, "%s", text);
  fclose(f);
}

void on_text_changed(GtkTextBuffer *buffer, gpointer user_data) {
  StickyNote *note = (StickyNote *)user_data;

  GtkTextIter start, end;
  gtk_text_buffer_get_bounds(buffer, &start, &end);
  char *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

  save_note_content(note->filepath, text);

  g_free(text);
}

gboolean on_delete_event(GtkWidget *widget, GdkEvent *event,
                         gpointer user_data) {
  StickyNote *note = (StickyNote *)user_data;

  GtkTextIter start, end;
  gtk_text_buffer_get_bounds(note->buffer, &start, &end);
  char *text = gtk_text_buffer_get_text(note->buffer, &start, &end, FALSE);
  save_note_content(note->filepath, text);
  g_free(text);

  free(note->filepath);
  free(note);

  gtk_main_quit();
  return FALSE;
}

StickyNote *create_sticky_note(int note_id) {
  StickyNote *note = malloc(sizeof(StickyNote));
  note->note_id = note_id;
  note->filepath = get_note_filepath(note_id);

  note->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(note->window), "Sticky Note");
  gtk_window_set_default_size(GTK_WINDOW(note->window), DEFAULT_WIDTH,
                              DEFAULT_HEIGHT);
  gtk_window_set_keep_above(GTK_WINDOW(note->window), TRUE);

  GdkRGBA color;
  gdk_rgba_parse(&color, "#000000");
  gtk_widget_override_background_color(note->window, GTK_STATE_FLAG_NORMAL,
                                       &color);

  GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  note->text_view = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(note->text_view), GTK_WRAP_WORD);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(note->text_view), 15);
  gtk_text_view_set_right_margin(GTK_TEXT_VIEW(note->text_view), 15);
  gtk_text_view_set_top_margin(GTK_TEXT_VIEW(note->text_view), 15);
  gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(note->text_view), 15);

  gtk_widget_override_background_color(note->text_view, GTK_STATE_FLAG_NORMAL,
                                       &color);

  // Set cursor color to light gray using CSS
  GtkCssProvider *css_provider = gtk_css_provider_new();
  const char *css = "textview { caret-color: #d3d3d3; }";
  gtk_css_provider_load_from_data(css_provider, css, -1, NULL);
  GtkStyleContext *context = gtk_widget_get_style_context(note->text_view);
  gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(css_provider),
                                 GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref(css_provider);

  GdkRGBA text_color;
  gdk_rgba_parse(&text_color, "#ffffff");
  gtk_widget_override_color(note->text_view, GTK_STATE_FLAG_NORMAL,
                            &text_color);

  note->buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(note->text_view));
  char *content = load_note_content(note->filepath);
  if (content) {
    gtk_text_buffer_set_text(note->buffer, content, -1);
    free(content);
  }

  g_signal_connect(note->buffer, "changed", G_CALLBACK(on_text_changed), note);
  g_signal_connect(note->window, "delete-event", G_CALLBACK(on_delete_event),
                   note);

  gtk_container_add(GTK_CONTAINER(scrolled), note->text_view);
  gtk_container_add(GTK_CONTAINER(note->window), scrolled);

  gtk_widget_show_all(note->window);

  return note;
}

int main(int argc, char *argv[]) {
  gtk_init(&argc, &argv);

  ensure_notes_dir();

  int note_id = 1;
  if (argc > 1) {
    note_id = atoi(argv[1]);
  }

  create_sticky_note(note_id);

  gtk_main();

  return 0;
}
