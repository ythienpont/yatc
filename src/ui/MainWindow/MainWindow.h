#include "TorrentClient/TorrentClient.h"
#include <gtk/gtk.h>
#include <memory>
#include <thread>
#include <vector>

class MainWindow {
public:
  /**
   * @brief Constructor for MainWindow.
   */
  MainWindow();

  /**
   * @brief Destructor for MainWindow.
   */
  ~MainWindow();

  /**
   * @brief Runs the main application loop.
   *
   * @param argc Argument count.
   * @param argv Argument vector.
   */
  void run(int argc, char **argv);

private:
  GtkApplication *app_;   /**< Pointer to the GtkApplication instance. */
  GtkWidget *window_;     /**< Pointer to the main application window widget. */
  guint update_timer_id_; /**< Timer ID for periodic updates. */
  std::unique_ptr<TorrentClient>
      torrent_client_; /**< Unique pointer to the TorrentClient instance. */
  std::thread
      torrent_client_thread_; /**< Thread for running the TorrentClient. */
  std::vector<GtkWidget *>
      drawing_areas_; /**< Vector of pointers to drawing area widgets. */

  /**
   * @brief Callback for GtkApplication activation.
   *
   * @param app The GtkApplication.
   * @param user_data User data passed to the callback.
   */
  static void activate(GtkApplication *app, gpointer user_data);

  /**
   * @brief Set up the user interface.
   *
   * @param app The GtkApplication.
   */
  void setup_ui(GtkApplication *app);

  /**
   * @brief Timeout callback for periodic updates.
   *
   * @param user_data User data passed to the callback.
   * @return gboolean TRUE to continue calling the timeout function.
   */
  static gboolean on_timeout(gpointer user_data);

  /**
   * @brief Drawing callback for the drawing area.
   *
   * @param area The drawing area.
   * @param cr The Cairo context.
   * @param width The width of the drawing area.
   * @param height The height of the drawing area.
   * @param user_data User data passed to the callback.
   */
  static void draw_callback(GtkDrawingArea *area, cairo_t *cr, int width,
                            int height, gpointer user_data);

  /**
   * @brief Set up the menu bar.
   *
   * @param vbox The vbox container to which the menu bar will be added.
   */
  void setup_menu_bar(GtkWidget *vbox);

  /**
   * @brief Apply CSS styles to the widgets.
   */
  void apply_css();

  /**
   * @brief Handler for the "File Open" action.
   */
  void on_file_open();

  /**
   * @brief Response callback for the file open dialog.
   *
   * @param dialog The native file dialog.
   * @param response_id The response ID.
   * @param user_data User data passed to the callback.
   */
  static void file_open_response(GtkNativeDialog *dialog, int response_id,
                                 gpointer user_data);
};
