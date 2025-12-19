#include <gst/gst.h>
#include <iostream>

// Callback to print state changes (for debugging/demonstration)
static void on_state_changed(GstBus *bus, GstMessage *msg, gpointer data) {
  GstState old_state, new_state, pending_state;
  gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);

  // Convert states to string for readable output
  std::cout << "State changed from " << gst_element_state_get_name(old_state)
            << " to " << gst_element_state_get_name(new_state) << std::endl;
}

// Callback for error handling
static void on_error(GstBus *bus, GstMessage *msg, gpointer data) {
  GError *err = nullptr;
  gchar *debug_info = nullptr;

  gst_message_parse_error(msg, &err, &debug_info);

  std::cerr << "Error received from element " << GST_OBJECT_NAME(msg->src)
            << ": " << err->message << std::endl;
  std::cerr << "Debugging information: " << (debug_info ? debug_info : "none")
            << std::endl;

  g_clear_error(&err);
  g_free(debug_info);

  GMainLoop *loop = (GMainLoop *)data;
  g_main_loop_quit(loop);
}

// Callback for End of Stream
static void on_eos(GstBus *bus, GstMessage *msg, gpointer data) {
  std::cout << "End of stream reached." << std::endl;
  GMainLoop *loop = (GMainLoop *)data;
  g_main_loop_quit(loop);
}

// Callback for Buffering messages (optional but good practice for network
// streams)
static void on_buffering(GstBus *bus, GstMessage *msg, gpointer data) {
  gint percent;
  gst_message_parse_buffering(msg, &percent);
  std::cout << "Buffering: " << percent << "%"
            << "\r" << std::flush;
  // Note: detailed buffering logic (pausing pipeline) would go here
}

// Main application logic (moved from main)
int app_main(int argc, char *argv[], void *user_data) {
  // 1. Initialize GStreamer (already called by wrapper or we call it here if
  // not) On macOS gst_macos_main might init it, but safe to call again or check
  // docs. Standard practice: call gst_init inside the thread function if not
  // done.
  GError *error = NULL;
  if (!gst_init_check(&argc, &argv, &error)) {
    std::cerr << "Failed to initialize GStreamer: "
              << (error ? error->message : "unknown error") << std::endl;
    return -1;
  }

  std::cout << "Initializing GStreamer Player..." << std::endl;

  // 2. Create the pipeline using 'playbin'
  GstElement *pipeline = gst_element_factory_make("playbin", "player");

  if (!pipeline) {
    std::cerr << "Not all elements could be created. 'playbin' missing?"
              << std::endl;
    return -1;
  }

  // 3. Set the URI to play
  // Using a public HLS test stream (Big Buck Bunny)
  const char *default_uri = "https://test-streams.mux.dev/x36xhzz/x36xhzz.m3u8";

  // Allow user to provide a URI via command line
  if (argc > 1) {
    default_uri = argv[1];
  }

  g_object_set(pipeline, "uri", default_uri, NULL);
  std::cout << "Setting URI to: " << default_uri << std::endl;

  // 4. Create a GLib Main Loop to handle messages
  GMainLoop *loop = g_main_loop_new(NULL, FALSE);

  // 5. Watch the bus for messages
  GstBus *bus = gst_element_get_bus(pipeline);
  gst_bus_add_signal_watch(bus);

  // Connect signals to callbacks
  g_signal_connect(bus, "message::state-changed", G_CALLBACK(on_state_changed),
                   NULL);
  g_signal_connect(bus, "message::error", G_CALLBACK(on_error), loop);
  g_signal_connect(bus, "message::eos", G_CALLBACK(on_eos), loop);
  g_signal_connect(bus, "message::buffering", G_CALLBACK(on_buffering), NULL);

  gst_object_unref(bus);

  // 6. Start playback
  std::cout << "Starting playback..." << std::endl;
  GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);

  if (ret == GST_STATE_CHANGE_FAILURE) {
    std::cerr << "Unable to set the pipeline to the playing state."
              << std::endl;
    gst_object_unref(pipeline);
    return -1;
  }

  // 7. Run the main loop (blocking)
  g_main_loop_run(loop);

  // 8. Cleanup
  std::cout << "\nStopping playback and cleaning up..." << std::endl;
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);
  g_main_loop_unref(loop);

  return 0;
}

int main(int argc, char *argv[]) {
#if defined(__APPLE__)
  // On macOS, the Cocoa event loop must run on the main thread for video sinks.
  // gst_macos_main handles this by running the provided function in a secondary
  // thread while the main thread runs the Cocoa loop.
  return gst_macos_main(app_main, argc, argv, NULL);
#else
  return app_main(argc, argv, NULL);
#endif
}
