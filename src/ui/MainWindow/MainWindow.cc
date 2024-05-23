#include "MainWindow.h"

MainWindow::MainWindow() {
  app = gtk_application_new("org.example.bittorrentclient",
                            G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(MainWindow::activate), this);
  torrents.emplace_back("ExampleFile1.zip", "5 mins", 100);
  torrents.emplace_back("ExampleFile2.zip", "10 mins", 100);
}

MainWindow::~MainWindow() {
  g_object_unref(app);
  if (updateTimerId != 0) {
    g_source_remove(updateTimerId);
  }
}

void MainWindow::activate(GtkApplication *app, gpointer user_data) {
  MainWindow *self = static_cast<MainWindow *>(user_data);
  self->setupUI(app);
  self->updateTimerId = g_timeout_add_seconds(
      2, reinterpret_cast<GSourceFunc>(MainWindow::onTimeout), self);
}

void MainWindow::setupMenuBar(GtkWidget *vbox) {
  auto menu = g_menu_new();
  auto file_menu = g_menu_new();

  g_menu_append(file_menu, "Add Torrent", "app.add_torrent");
  g_menu_append_section(menu, "File", G_MENU_MODEL(file_menu));

  auto menu_button = gtk_menu_button_new();
  gtk_widget_add_css_class(menu_button, "menu-button");
  auto popover_menu = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
  gtk_menu_button_set_popover(GTK_MENU_BUTTON(menu_button), popover_menu);
  gtk_menu_button_set_label(GTK_MENU_BUTTON(menu_button), "Actions");

  gtk_box_append(GTK_BOX(vbox),
                 menu_button); // Append directly to the passed vbox
}

void MainWindow::setupUI(GtkApplication *app) {
  window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "BitTorrent Client Prototype");
  gtk_window_set_default_size(GTK_WINDOW(window), 600, 300);

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_window_set_child(GTK_WINDOW(window), vbox);

  // Setup menu bar
  // setupMenuBar(vbox);

  // Continue setting up the rest of the UI
  for (auto &torrent : torrents) {
    GtkWidget *info_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(vbox), info_box);

    GtkWidget *label_name = gtk_label_new(torrent.name.c_str());
    gtk_box_append(GTK_BOX(info_box), label_name);

    GtkWidget *label_eta = gtk_label_new(torrent.eta.c_str());
    gtk_box_append(GTK_BOX(info_box), label_eta);

    GtkDrawingArea *drawingArea = GTK_DRAWING_AREA(gtk_drawing_area_new());
    gtk_widget_set_size_request(GTK_WIDGET(drawingArea), 600, 50);
    gtk_drawing_area_set_draw_func(drawingArea, MainWindow::draw_callback,
                                   &torrent, nullptr);
    gtk_box_append(GTK_BOX(vbox), GTK_WIDGET(drawingArea));

    drawingAreas.push_back(GTK_WIDGET(drawingArea));
  }

  applyCss();
  gtk_widget_set_visible(GTK_WIDGET(window), TRUE);
}

gboolean MainWindow::onTimeout(gpointer user_data) {
  MainWindow *self = static_cast<MainWindow *>(user_data);
  for (auto &torrent : self->torrents) {
    torrent.updatePieces();
  }
  for (auto *area : self->drawingAreas) {
    gtk_widget_queue_draw(area); // Redraw each drawing area
  }
  return G_SOURCE_CONTINUE;
}

void MainWindow::draw_callback(GtkDrawingArea *area, cairo_t *cr, int width,
                               int height, gpointer user_data) {
  TorrentInfo *torrent = static_cast<TorrentInfo *>(user_data);
  int totalPieces = torrent->pieces.size();
  double pieceWidth = width / static_cast<double>(totalPieces);

  for (size_t i = 0; i < torrent->pieces.size(); ++i) {
    switch (torrent->pieces[i]) {
    case PieceState::Left:
      cairo_set_source_rgb(cr, 1, 1, 1); // White
      break;
    case PieceState::Started:
      cairo_set_source_rgb(cr, 0.26, 0.47,
                           0.88); // Blue
      break;
    case PieceState::Downloaded:
      cairo_set_source_rgb(cr, 0.33, 0.84,
                           0.42); // Green
      break;
    }
    cairo_rectangle(cr, i * pieceWidth, 0, pieceWidth, height);
    cairo_fill(cr);
  }
}
void MainWindow::applyCss() {
  GtkCssProvider *provider = gtk_css_provider_new();
  gtk_css_provider_load_from_string(provider, "window, .menu-button {"
                                              "  background-color: #2E3440;"
                                              "  color: #ECEFF4;"
                                              "}"
                                              "label {"
                                              "  color: #ECEFF4;"
                                              "  font-size: 18pt;"
                                              "  margin: 0 5px;"
                                              "}"
                                              "button {"
                                              "  background-color: #4C566A;"
                                              "  color: #ECEFF4;"
                                              "  padding: 6px 12px;"
                                              "  border: none;"
                                              "  border-radius: 4px;"
                                              "  box-shadow: none;"
                                              "}"
                                              "button:hover {"
                                              "  background-color: #81A1C1;"
                                              "}"
                                              ".popover {"
                                              "  background-color: #434C5E;"
                                              "  color: #ECEFF4;"
                                              "}"
                                              ".popover menuitem:hover {"
                                              "  background-color: #81A1C1;"
                                              "}");
  gtk_style_context_add_provider_for_display(
      gdk_display_get_default(), GTK_STYLE_PROVIDER(provider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

void MainWindow::run(int argc, char **argv) {
  g_application_run(G_APPLICATION(app), argc, argv);
}
