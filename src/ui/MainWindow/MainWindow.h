#include "../TorrentInfo/TorrentInfo.h"
#include <gtk/gtk.h>

class MainWindow {
private:
  GtkApplication *app;
  GtkWidget *window;
  std::vector<TorrentInfo> torrents;
  std::vector<GtkWidget *> drawingAreas; // Store pointers to drawing areas
  guint updateTimerId;

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

  static void on_add_torrent_activate(GMenu *menuitem, gpointer user_data) {
    // Function to handle the add torrent action
    g_print("Add Torrent clicked\n");
  }
};
