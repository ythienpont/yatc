#include "TorrentClient/TorrentClient.h"
#include <gtk/gtk.h>
#include <iostream>
#include <memory>
#include <vector>

class MainWindow {
private:
  GtkApplication *app;
  GtkWidget *window;
  std::vector<GtkWidget *> drawingAreas; // Store pointers to drawing areas
  guint updateTimerId;
  std::unique_ptr<TorrentClient> torrentClient;

public:
  MainWindow();
  ~MainWindow();
  static void activate(GtkApplication *app, gpointer user_data);
  void setupUI(GtkApplication *app);
  static gboolean onTimeout(gpointer user_data);
  static void draw_callback(GtkDrawingArea *area, cairo_t *cr, int width,
                            int height, gpointer user_data);
  void setupMenuBar(GtkWidget *vbox);
  void applyCss();
  void run(int argc, char **argv);
  void on_file_open();
  static void file_open_response(GtkNativeDialog *dialog, int response_id,
                                 gpointer user_data);
};
