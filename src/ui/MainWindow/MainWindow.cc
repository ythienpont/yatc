#include "MainWindow.h"

MainWindow::MainWindow()
    : app(nullptr), window(nullptr), updateTimerId(0), torrentClient(nullptr) {}

MainWindow::~MainWindow() {
  if (updateTimerId != 0) {
    g_source_remove(updateTimerId);
  }
}

void MainWindow::activate(GtkApplication *app, gpointer user_data) {
  MainWindow *self = static_cast<MainWindow *>(user_data);
  self->setupUI(app);
}

void MainWindow::setupUI(GtkApplication *app) {
  window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "MainWindow");
  gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_window_set_child(GTK_WINDOW(window), vbox);

  setupMenuBar(vbox);

  gtk_widget_set_visible(window, TRUE);
}

gboolean MainWindow::onTimeout(gpointer user_data) {
  // Update logic here
  return TRUE; // Continue calling the timeout function
}

void MainWindow::draw_callback(GtkDrawingArea *area, cairo_t *cr, int width,
                               int height, gpointer user_data) {
  MainWindow *self = static_cast<MainWindow *>(user_data);
  if (self->torrentClient) {
    auto info = self->torrentClient->download_info();
    // Assuming download_info() returns a string or any drawable information
    cairo_move_to(cr, 10, 10);
    cairo_show_text(cr, info.c_str());
  }
}

void MainWindow::setupMenuBar(GtkWidget *vbox) {
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

  g_action_map_add_action_entries(G_ACTION_MAP(app), actions,
                                  G_N_ELEMENTS(actions), this);
}

void MainWindow::applyCss() {
  // CSS application logic here
}

void MainWindow::run(int argc, char **argv) {
  app = gtk_application_new("com.example.GtkApplication",
                            G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(MainWindow::activate), this);
  g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
}

void MainWindow::on_file_open() {
  GtkNativeDialog *dialog = GTK_NATIVE_DIALOG(gtk_file_chooser_native_new(
      "Open File", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN, "_Open",
      "_Cancel"));

  g_signal_connect(dialog, "response",
                   G_CALLBACK(MainWindow::file_open_response), this);
  gtk_native_dialog_show(dialog);
}

void MainWindow::file_open_response(GtkNativeDialog *dialog, int response_id,
                                    gpointer user_data) {
  if (response_id == GTK_RESPONSE_ACCEPT) {
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
    g_autoptr(GFile) file = gtk_file_chooser_get_file(chooser);
    char *filepath = g_file_get_path(file);
    std::cout << "Selected file: " << filepath << std::endl;

    // Initialize the TorrentClient with the selected file
    MainWindow *self = static_cast<MainWindow *>(user_data);
    self->torrentClient = std::make_unique<TorrentClient>(filepath);
    self->torrentClient->start();

    g_free(filepath);

    // Force a redraw to update with new download info
    for (GtkWidget *area : self->drawingAreas) {
      gtk_widget_queue_draw(area);
    }
  }
  g_object_unref(dialog);
}
