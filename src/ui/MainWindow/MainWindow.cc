#include "MainWindow.h"
#include "TorrentClient/TorrentClient.h"
#include <gtk/gtk.h>
#include <iostream>
#include <sstream>
#include <thread>

#define G_APPLICATION_DEFAULT_FLAGS 0

MainWindow::MainWindow()
    : app_(nullptr), window_(nullptr), update_timer_id_(0),
      torrent_client_(nullptr) {}

MainWindow::~MainWindow() {
  if (update_timer_id_ != 0) {
    g_source_remove(update_timer_id_);
  }
  if (torrent_client_thread_.joinable()) {
    torrent_client_thread_.join(); // Ensure the thread is properly joined
  }
}

void MainWindow::activate(GtkApplication *app, gpointer user_data) {
  MainWindow *self = static_cast<MainWindow *>(user_data);
  self->setup_ui(app);
}

void MainWindow::setup_ui(GtkApplication *app) {
  window_ = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window_), "yatc");
  gtk_window_set_default_size(GTK_WINDOW(window_), 800, 400);

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_window_set_child(GTK_WINDOW(window_), vbox);

  // Setup menu bar
  setup_menu_bar(vbox);

  // Create and add the drawing area
  GtkDrawingArea *drawingArea = GTK_DRAWING_AREA(gtk_drawing_area_new());
  gtk_widget_set_size_request(GTK_WIDGET(drawingArea), 800, 400);
  gtk_drawing_area_set_draw_func(drawingArea, MainWindow::draw_callback, this,
                                 nullptr);
  gtk_box_append(GTK_BOX(vbox), GTK_WIDGET(drawingArea));

  drawing_areas_.push_back(GTK_WIDGET(drawingArea));

  apply_css();
  gtk_widget_set_visible(GTK_WIDGET(window_), TRUE);

  // Timer for callback
  update_timer_id_ = g_timeout_add_seconds(1, MainWindow::on_timeout, this);
}

gboolean MainWindow::on_timeout(gpointer user_data) {
  MainWindow *self = static_cast<MainWindow *>(user_data);
  for (GtkWidget *area : self->drawing_areas_) {
    gtk_widget_queue_draw(area);
  }
  return TRUE;
}

void MainWindow::draw_callback(GtkDrawingArea *area, cairo_t *cr, int width,
                               int height, gpointer user_data) {
  MainWindow *self = static_cast<MainWindow *>(user_data);
  if (self->torrent_client_) {
    TorrentInfo info = self->torrent_client_->download_info();

    // Calculate the progress in percentage
    float progress =
        1.0 - (static_cast<double>(info.pieces_needed) / info.total_pieces);

    std::stringstream ss;
    ss << "Torrent: " << info.name << "\n"
       << "Connections: " << info.connections << "\n"
       << "Progress: " << static_cast<int>(progress * 100) << "%";

    // Background
    cairo_set_source_rgb(cr, 0.95, 0.95, 0.95);
    cairo_paint(cr);

    // Margins
    double margin = 20.0;
    double adjusted_width = width - 2 * margin;
    double adjusted_height = height - 2 * margin;

    // Rounded corners
    double radius = 20.0;
    double degrees = M_PI / 180.0;
    cairo_new_sub_path(cr);
    cairo_arc(cr, margin + adjusted_width - radius, margin + radius, radius,
              -90 * degrees, 0 * degrees);
    cairo_arc(cr, margin + adjusted_width - radius,
              margin + adjusted_height - radius, radius, 0 * degrees,
              90 * degrees);
    cairo_arc(cr, margin + radius, margin + adjusted_height - radius, radius,
              90 * degrees, 180 * degrees);
    cairo_arc(cr, margin + radius, margin + radius, radius, 180 * degrees,
              270 * degrees);
    cairo_close_path(cr);
    cairo_set_source_rgb(cr, 0.2, 0.2, 0.2); // Darker gray
    cairo_fill(cr);

    // Torrent info
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 24);
    cairo_move_to(cr, margin + 20, margin + 50);
    cairo_show_text(cr, ss.str().c_str());

    // Bar
    double bar_width = adjusted_width - 40;
    double bar_height = 30;
    double bar_x = margin + 20;
    double bar_y = margin + 100;

    // Bar background
    cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
    cairo_rectangle(cr, bar_x, bar_y, bar_width, bar_height);
    cairo_fill(cr);

    // Progress bar fill
    cairo_pattern_t *progress_pattern = cairo_pattern_create_linear(
        bar_x, bar_y, bar_x + bar_width * progress, bar_y + bar_height);
    if (info.pieces_needed > 0) {
      cairo_pattern_add_color_stop_rgb(progress_pattern, 0.0, 0.2, 0.6,
                                       0.8); // Start blue
      cairo_pattern_add_color_stop_rgb(progress_pattern, 1.0, 0.1, 0.4,
                                       0.6); // End blue
    } else {
      cairo_pattern_add_color_stop_rgb(progress_pattern, 0.0, 0.2, 0.8,
                                       0.2); // Start green
      cairo_pattern_add_color_stop_rgb(progress_pattern, 1.0, 0.1, 0.6,
                                       0.1); // End green
    }
    cairo_set_source(cr, progress_pattern);
    cairo_rectangle(cr, bar_x, bar_y, bar_width * progress, bar_height);
    cairo_fill(cr);
    cairo_pattern_destroy(progress_pattern);

    // Progress bar border
    cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
    cairo_set_line_width(cr, 2);
    cairo_rectangle(cr, bar_x, bar_y, bar_width, bar_height);
    cairo_stroke(cr);
  }
}

void MainWindow::setup_menu_bar(GtkWidget *vbox) {
  GMenu *menuModel = g_menu_new();
  GMenu *fileMenu = g_menu_new();

  g_menu_append_submenu(G_MENU(menuModel), "File", G_MENU_MODEL(fileMenu));
  g_menu_append(G_MENU(fileMenu), "Open", "app.open");

  GtkWidget *menubar = gtk_menu_button_new();
  g_object_set(menubar, "menu-model", G_MENU_MODEL(menuModel), NULL);

  gtk_box_append(GTK_BOX(vbox), menubar);

  GActionEntry actions[] = {
      {"open",
       [](GSimpleAction *, GVariant *, gpointer user_data) {
         MainWindow *self = static_cast<MainWindow *>(user_data);
         self->on_file_open();
       },
       nullptr,
       nullptr,
       nullptr,
       {0}}};

  g_action_map_add_action_entries(G_ACTION_MAP(app_), actions,
                                  G_N_ELEMENTS(actions), this);
}

void MainWindow::apply_css() {}

void MainWindow::run(int argc, char **argv) {
  app_ = gtk_application_new("com.example.GtkApplication",
                             G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app_, "activate", G_CALLBACK(MainWindow::activate), this);
  g_application_run(G_APPLICATION(app_), argc, argv);
  g_object_unref(app_);
}

void MainWindow::on_file_open() {
  GtkNativeDialog *dialog = GTK_NATIVE_DIALOG(gtk_file_chooser_native_new(
      "Open File", GTK_WINDOW(window_), GTK_FILE_CHOOSER_ACTION_OPEN, "_Open",
      "_Cancel"));

  g_signal_connect(dialog, "response",
                   G_CALLBACK(MainWindow::file_open_response), this);
  gtk_native_dialog_show(dialog);
}

void MainWindow::file_open_response(GtkNativeDialog *dialog, int response_id,
                                    gpointer user_data) {
  if (response_id == GTK_RESPONSE_ACCEPT) {
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
    g_autoptr(GFile) file =
        gtk_file_chooser_get_file(chooser); // TODO: Remove deprecated method
    char *filepath = g_file_get_path(file);

    // Initialize TorrentClient
    MainWindow *self = static_cast<MainWindow *>(user_data);
    self->torrent_client_ = std::make_unique<TorrentClient>(filepath);

    // Create thread
    self->torrent_client_thread_ =
        std::thread([self]() { self->torrent_client_->start(); });

    g_free(filepath);

    // Force a redraw to update with new download info
    for (GtkWidget *area : self->drawing_areas_) {
      gtk_widget_queue_draw(area);
    }
  }
  g_object_unref(dialog);
}
