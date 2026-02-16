#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <gtk/gtk.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <ctime>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <cmath>

// ---------------- GLOBAL VARIABLES ----------------
static pid_t ffmpeg_pid = -1;       // FFmpeg process ID
static GtkWidget *indicator;        // red dot indicator
static guint blink_id = 0;          // blink timer ID
static std::string last_file;       // last saved file
static bool running = true;         // main loop flag

// Region select variables
static bool selecting = false;
static gint start_x=0, start_y=0, end_x=0, end_y=0;
static GtkWidget *overlay_window = nullptr;
// ---------------------------------------------------

std::string timestamp() {
    time_t t = time(nullptr);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", localtime(&t));
    return buf;
}

gboolean blink(gpointer) {
    static bool on = true;
    on = !on;
    gtk_widget_set_visible(indicator, on);
    return TRUE;
}

// Stop recording
void stop_recording() {
    if (ffmpeg_pid > 0) {
        kill(ffmpeg_pid, SIGTERM);
        waitpid(ffmpeg_pid, NULL, 0);
        ffmpeg_pid = -1;
    }

    if (blink_id) {
        g_source_remove(blink_id);
        blink_id = 0;
    }

    gtk_widget_hide(indicator);

    if (!last_file.empty()) {
        GtkWidget *dialog = gtk_message_dialog_new(
            NULL,
            static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_NONE,
            "Recording saved to:\n%s",
            last_file.c_str()
        );

        gtk_window_set_default_size(GTK_WINDOW(dialog), 350, 130);

        GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        gtk_container_set_border_width(GTK_CONTAINER(content_area), 10);

        // Button box with spacing
        GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_container_set_border_width(GTK_CONTAINER(button_box), 5);
        gtk_container_add(GTK_CONTAINER(content_area), button_box);

        GtkWidget *btn1 = gtk_button_new_with_label("Open folder");
        GtkWidget *btn2 = gtk_button_new_with_label("Close");

        gtk_widget_set_can_focus(btn1, FALSE);
        gtk_widget_set_can_focus(btn2, FALSE);
        gtk_widget_set_focus_on_click(btn1, FALSE);
        gtk_widget_set_focus_on_click(btn2, FALSE);

        gtk_box_pack_start(GTK_BOX(button_box), btn1, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(button_box), btn2, TRUE, TRUE, 0);

        g_signal_connect(btn1, "clicked", G_CALLBACK(+[](GtkWidget*, gpointer){
            std::string cmd = "xdg-open \"" + std::string(getenv("HOME")) + "/Videos\"";
            system(cmd.c_str());
            gtk_main_quit();
        }), NULL);

        g_signal_connect(btn2, "clicked", G_CALLBACK(+[](GtkWidget*, gpointer){
            gtk_main_quit();
        }), NULL);

        gtk_window_set_decorated(GTK_WINDOW(dialog), TRUE);
        gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE);
        gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

        g_signal_connect(dialog, "delete-event", G_CALLBACK(+[](GtkWidget*, GdkEvent*, gpointer){
            kill(getpid(), SIGTERM);
            return TRUE;
        }), NULL);

        gtk_widget_show_all(dialog);
        gtk_main();
        gtk_widget_destroy(dialog);

        last_file.clear();
    }

    running = false;
}

// Start recording with region coordinates
void start_recording_region(int x, int y, int w, int h) {
    last_file = std::string(getenv("HOME")) + "/Videos/record_" + timestamp() + ".mkv";

    ffmpeg_pid = fork();
    if (!ffmpeg_pid) {
        std::string video_size = std::to_string(w) + "x" + std::to_string(h);
        std::string display = ":0.0+" + std::to_string(x) + "," + std::to_string(y);

        execlp("ffmpeg", "ffmpeg",
               "-video_size", video_size.c_str(),
               "-framerate", "30",
               "-f", "x11grab",
               "-i", display.c_str(),
               "-f", "alsa",
               "-i", "default",
               "-async", "1",
               "-c:v", "mpeg4",
               "-q:v", "5",
               "-c:a", "pcm_s16le",
               last_file.c_str(),
               NULL);
        _exit(1);
    }

    gtk_widget_show_all(indicator);
    blink_id = g_timeout_add(500, blink, NULL);
}

// Capture mode selection dialog
int show_capture_mode_dialog() {
    GtkWidget *dialog = gtk_message_dialog_new(
        NULL,
        static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_NONE,
        "Select how you want to capture \nNote: Stop Recording with Ctrl+Shift+End"
    );

    gtk_window_set_default_size(GTK_WINDOW(dialog), 350, 130);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content_area), 10);

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(button_box), 5);
    gtk_container_add(GTK_CONTAINER(content_area), button_box);

    GtkWidget *btn1 = gtk_button_new_with_label("Region Select");
    GtkWidget *btn2 = gtk_button_new_with_label("Full Screen");

    gtk_widget_set_can_focus(btn1, FALSE);
    gtk_widget_set_can_focus(btn2, FALSE);
    gtk_widget_set_focus_on_click(btn1, FALSE);
    gtk_widget_set_focus_on_click(btn2, FALSE);

    gtk_box_pack_start(GTK_BOX(button_box), btn1, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), btn2, TRUE, TRUE, 0);

    int result = 0;
    g_signal_connect(btn1, "clicked", G_CALLBACK(+[](GtkWidget*, gpointer data){
        int *res = (int*)data;
        *res = 1;
        gtk_main_quit();
    }), &result);

    g_signal_connect(btn2, "clicked", G_CALLBACK(+[](GtkWidget*, gpointer data){
        int *res = (int*)data;
        *res = 2;
        gtk_main_quit();
    }), &result);

    gtk_window_set_decorated(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE);

    g_signal_connect(dialog, "delete-event", G_CALLBACK(+[](GtkWidget*, GdkEvent*, gpointer){
        kill(getpid(), SIGTERM);
        return TRUE;
    }), NULL);

    // Show everything and run
    gtk_widget_show_all(dialog);
    gtk_main();
    gtk_widget_destroy(dialog);

    return result;
}

// Draw callback for selection rectangle
static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer) {
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    if (selecting) {
        cairo_set_source_rgba(cr, 0.98, 0.38, 0.008, 0.2);
        gint x = std::min(start_x, end_x);
        gint y = std::min(start_y, end_y);
        gint w = std::abs(end_x - start_x);
        gint h = std::abs(end_y - start_y);
        cairo_rectangle(cr, x, y, w, h);
        cairo_fill(cr);
    }
    return FALSE;
}

// Mouse press
static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer) {
    if (event->button == GDK_BUTTON_PRIMARY) {
        start_x = (gint)event->x;
        start_y = (gint)event->y;
        end_x = start_x;
        end_y = start_y;
        selecting = true;
        gtk_widget_queue_draw(widget);
    }
    return TRUE;
}

// Mouse motion
static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer) {
    if (selecting) {
        end_x = (gint)event->x;
        end_y = (gint)event->y;
        gtk_widget_queue_draw(widget);
    }
    return TRUE;
}

// Mouse release → start recording
static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer) {
    if (event->button == GDK_BUTTON_PRIMARY && selecting) {
        end_x = (gint)event->x;
        end_y = (gint)event->y;
        selecting = false;
        gtk_widget_queue_draw(widget);

        gtk_widget_hide(overlay_window);

        gint w = std::abs(end_x - start_x);
        gint h = std::abs(end_y - start_y);
        gint x = std::min(start_x, end_x);
        gint y = std::min(start_y, end_y);

        start_recording_region(x, y, w, h);
    }
    return TRUE;
}

// Start recording (full screen or region)
void start_recording(int screen_width, int screen_height) {
    int choice = show_capture_mode_dialog();

    if (choice == 2) {
        start_recording_region(0, 0, screen_width, screen_height);
        return;
    } else if (choice == 1) {
        // Region select overlay
        overlay_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_widget_set_app_paintable(overlay_window, TRUE);

        GdkScreen *screen = gtk_widget_get_screen(overlay_window);
        GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
        if (visual)
            gtk_widget_set_visual(overlay_window, visual);

        gtk_window_set_decorated(GTK_WINDOW(overlay_window), FALSE);
        gtk_window_set_keep_above(GTK_WINDOW(overlay_window), TRUE);
        gtk_window_set_accept_focus(GTK_WINDOW(overlay_window), FALSE);
        gtk_window_set_type_hint(GTK_WINDOW(overlay_window), GDK_WINDOW_TYPE_HINT_DOCK);
        gtk_window_fullscreen(GTK_WINDOW(overlay_window));

        GtkWidget *drawing_area = gtk_drawing_area_new();
        gtk_container_add(GTK_CONTAINER(overlay_window), drawing_area);

        g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw), NULL);
        g_signal_connect(drawing_area, "button-press-event", G_CALLBACK(on_button_press), NULL);
        g_signal_connect(drawing_area, "motion-notify-event", G_CALLBACK(on_motion_notify), NULL);
        g_signal_connect(drawing_area, "button-release-event", G_CALLBACK(on_button_release), NULL);

        gtk_widget_set_events(drawing_area,
                              GDK_BUTTON_PRESS_MASK |
                              GDK_POINTER_MOTION_MASK |
                              GDK_BUTTON_RELEASE_MASK);

        gtk_widget_show_all(overlay_window);
    }
}

// -------------------- MAIN --------------------
int main(int argc, char **argv) {
    gtk_init(&argc, &argv);

    // Apply CSS to remove focus indicator globally
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "* { outline: none; box-shadow: none; }",
        -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(provider);

    Display *d = XOpenDisplay(NULL);
    Window root = DefaultRootWindow(d);

    int screen = DefaultScreen(d);
    int width = DisplayWidth(d, screen);
    int height = DisplayHeight(d, screen);

    // Set up hotkey for stopping recording only (Ctrl+Shift+End)
    int keycode = XKeysymToKeycode(d, XK_End);
    XGrabKey(d, keycode, ControlMask|ShiftMask,
             root, True, GrabModeAsync, GrabModeAsync);

    // red indicator
    indicator = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_decorated(GTK_WINDOW(indicator), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(indicator), TRUE);
    gtk_window_move(GTK_WINDOW(indicator), width-40, 20);
    gtk_window_set_default_size(GTK_WINDOW(indicator), 20, 20);

    GtkWidget *area = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(indicator), area);

    g_signal_connect(area, "draw", G_CALLBACK(+[](GtkWidget*, cairo_t *cr){
        cairo_set_source_rgb(cr,1,0,0);
        cairo_arc(cr,10,10,8,0,2*3.14159);
        cairo_fill(cr);
        return FALSE;
    }), NULL);

    // Show the capture dialog immediately when program starts
    start_recording(width, height);

    XEvent ev;

    while (running) {
        while (XPending(d)) {
            XNextEvent(d, &ev);
            if (ev.type == KeyPress) {
                // Only handle stop recording hotkey (Ctrl+Shift+End)
                if (ffmpeg_pid > 0) {
                    stop_recording();
                }
            }
        }

        while (gtk_events_pending())
            gtk_main_iteration();

        usleep(10000);
    }

    return 0;
}