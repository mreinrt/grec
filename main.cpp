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
#include <fstream>
#include <iostream>
#include <vector>
#include <memory>
#include <cstdio>

// ---------------- CONFIG STRUCTURE ----------------
struct GrecConfig {
    // Output settings
    std::string save_format = "mp4";
    std::string save_directory = "~/Videos";
    std::string filename_pattern = "recording_%Y%m%d_%H%M%S";
    bool auto_increment = false;
    
    // Video settings
    int framerate = 30;
    std::string video_quality = "balanced";
    bool show_mouse = true;
    
    // Audio settings
    bool capture_audio = true;
    bool record_microphone = false;
    bool record_system = true;
    std::string audio_quality = "medium";
    
    // Region Selector settings
    double region_r = 0.98;
    double region_g = 0.38;
    double region_b = 0.008;
    double region_alpha = 0.2;
    
    // Post-recording
    bool show_save_dialog = true;
    bool show_notifications = true;
    
    // Hotkeys
    std::string hotkey_start_stop = "Ctrl+Shift+R";
    std::string hotkey_pause = "Ctrl+Shift+P";
    std::string hotkey_abort = "Ctrl+Shift+Esc";
};

static GrecConfig config;

// ---------------- PULSEAUDIO DETECTION ----------------
struct AudioSources {
    std::string microphone;
    std::string monitor;
    bool has_pulse = false;
};

AudioSources detect_audio_sources() {
    AudioSources sources;
    
    // Check if pactl is available
    FILE* pipe = popen("which pactl 2>/dev/null", "r");
    if (pipe) {
        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            sources.has_pulse = true;
        }
        pclose(pipe);
    }
    
    if (!sources.has_pulse) {
        return sources;
    }
    
    // Get monitor source (system audio)
    pipe = popen("pactl list sources 2>/dev/null | grep -E 'Name:|device.class' | grep -B1 monitor | grep Name | head -1 | awk '{print $2}'", "r");
    if (pipe) {
        char buffer[256];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            std::string name = buffer;
            name.erase(name.find_last_not_of(" \n\r\t") + 1);
            if (!name.empty()) {
                sources.monitor = name;
            }
        }
        pclose(pipe);
    }
    
    // Get microphone (first non-monitor source)
    pipe = popen("pactl list sources 2>/dev/null | grep -E 'Name:|device.class' | grep -B1 monitor | grep -v monitor | grep Name | head -1 | awk '{print $2}'", "r");
    if (pipe) {
        char buffer[256];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            std::string name = buffer;
            name.erase(name.find_last_not_of(" \n\r\t") + 1);
            if (!name.empty()) {
                sources.microphone = name;
            }
        }
        pclose(pipe);
    }
    
    // Fallback: try to get any sources
    if (sources.microphone.empty()) {
        pipe = popen("pactl list sources short 2>/dev/null | grep -v '.monitor' | head -1 | awk '{print $2}'", "r");
        if (pipe) {
            char buffer[256];
            if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                std::string name = buffer;
                name.erase(name.find_last_not_of(" \n\r\t") + 1);
                sources.microphone = name;
            }
            pclose(pipe);
        }
    }
    
    if (sources.monitor.empty()) {
        pipe = popen("pactl list sources short 2>/dev/null | grep '.monitor' | head -1 | awk '{print $2}'", "r");
        if (pipe) {
            char buffer[256];
            if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                std::string name = buffer;
                name.erase(name.find_last_not_of(" \n\r\t") + 1);
                sources.monitor = name;
            }
            pclose(pipe);
        }
    }
    
    return sources;
}

// ---------------- CONFIG LOADING ----------------
std::string expand_path(const std::string& path) {
    if (path.empty() || path[0] != '~') return path;
    const char* home = getenv("HOME");
    if (!home) return path;
    return std::string(home) + path.substr(1);
}

void load_config() {
    std::string path = std::string(getenv("HOME")) + "/.config/grec.conf";
    std::ifstream file(path);
    
    if (!file.is_open()) {
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        size_t pos = line.find('=');
        if (pos == std::string::npos) continue;
        
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        
        if (key == "save_format") config.save_format = value;
        else if (key == "save_directory") config.save_directory = value;
        else if (key == "filename_pattern") config.filename_pattern = value;
        else if (key == "auto_increment") config.auto_increment = (value == "true");
        else if (key == "framerate") config.framerate = std::stoi(value);
        else if (key == "video_quality") config.video_quality = value;
        else if (key == "show_mouse") config.show_mouse = (value == "true");
        else if (key == "capture_audio") config.capture_audio = (value == "true");
        else if (key == "record_microphone") config.record_microphone = (value == "true");
        else if (key == "record_system") config.record_system = (value == "true");
        else if (key == "audio_quality") config.audio_quality = value;
        else if (key == "region_r") config.region_r = std::stod(value);
        else if (key == "region_g") config.region_g = std::stod(value);
        else if (key == "region_b") config.region_b = std::stod(value);
        else if (key == "region_alpha") config.region_alpha = std::stod(value);
        else if (key == "show_save_dialog") config.show_save_dialog = (value == "true");
        else if (key == "show_notifications") config.show_notifications = (value == "true");
        else if (key == "hotkey_start_stop") config.hotkey_start_stop = value;
        else if (key == "hotkey_pause") config.hotkey_pause = value;
        else if (key == "hotkey_abort") config.hotkey_abort = value;
    }
    
    file.close();
}

// ---------------- GLOBAL VARIABLES ----------------
static pid_t ffmpeg_pid = -1;
static GtkWidget *indicator;
static guint blink_id = 0;
static std::string last_file;
static bool running = true;

// Region select variables
static bool selecting = false;
static gint start_x=0, start_y=0, end_x=0, end_y=0;
static GtkWidget *overlay_window = nullptr;

// ---------------- UTILITY FUNCTIONS ----------------
std::string timestamp() {
    time_t t = time(nullptr);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", localtime(&t));
    return buf;
}

std::string generate_filename() {
    std::string dir = expand_path(config.save_directory);
    
    std::string mkdir_cmd = "mkdir -p " + dir;
    system(mkdir_cmd.c_str());
    
    std::string pattern = config.filename_pattern;
    time_t t = time(nullptr);
    struct tm *tm_info = localtime(&t);
    
    char time_buf[128];
    strftime(time_buf, sizeof(time_buf), pattern.c_str(), tm_info);
    std::string result = time_buf;
    
    if (config.save_format == "mp4") {
        result += ".mp4";
    } else if (config.save_format == "mkv") {
        result += ".mkv";
    } else if (config.save_format == "gif") {
        result += ".gif";
    } else {
        result += ".mp4";
    }
    
    return dir + "/" + result;
}

gboolean blink(gpointer) {
    static bool on = true;
    on = !on;
    gtk_widget_set_visible(indicator, on);
    return TRUE;
}

// ---------------- RECORDING FUNCTIONS ----------------
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

    if (!last_file.empty() && config.show_save_dialog) {
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
            std::string cmd = "xdg-open \"" + expand_path(config.save_directory) + "\"";
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
    }

    last_file.clear();
    running = false;
}

void start_recording_region(int x, int y, int w, int h) {
    last_file = generate_filename();

    ffmpeg_pid = fork();
    if (!ffmpeg_pid) {
        // Detect audio sources
        static AudioSources audio_sources = detect_audio_sources();
        
        std::string video_size = std::to_string(w) + "x" + std::to_string(h);
        const char* display_env = getenv("DISPLAY");
        std::string display = display_env ? display_env : ":0";
        std::string grab_input = display + "+" + std::to_string(x) + "," + std::to_string(y);
        
        // Handle GIF output
        if (config.save_format == "gif") {
            execlp("ffmpeg", "ffmpeg",
                   "-video_size", video_size.c_str(),
                   "-framerate", std::to_string(config.framerate).c_str(),
                   "-f", "x11grab",
                   "-i", grab_input.c_str(),
                   "-vf", ("fps=" + std::to_string(config.framerate) + ",scale=640:-1:flags=lanczos").c_str(),
                   "-c:v", "gif",
                   "-y",
                   last_file.c_str(),
                   NULL);
            _exit(1);
        }
        
        // Build FFmpeg command
        std::vector<const char*> args;
        args.push_back("ffmpeg");
        args.push_back("-video_size");
        args.push_back(video_size.c_str());
        args.push_back("-framerate");
        args.push_back(std::to_string(config.framerate).c_str());
        args.push_back("-f");
        args.push_back("x11grab");
        args.push_back("-i");
        args.push_back(grab_input.c_str());
        
        // Add audio inputs based on config and detected sources
        if (config.capture_audio) {
            if (audio_sources.has_pulse) {
                if (config.record_microphone && config.record_system) {
                    // Both microphone and system audio
                    if (!audio_sources.microphone.empty()) {
                        args.push_back("-f");
                        args.push_back("pulse");
                        args.push_back("-i");
                        args.push_back(audio_sources.microphone.c_str());
                    }
                    
                    if (!audio_sources.monitor.empty()) {
                        args.push_back("-f");
                        args.push_back("pulse");
                        args.push_back("-i");
                        args.push_back(audio_sources.monitor.c_str());
                    }
                    
                    // Mix if we have both
                    if (!audio_sources.microphone.empty() && !audio_sources.monitor.empty()) {
                        args.push_back("-filter_complex");
                        args.push_back("amix=inputs=2:duration=longest");
                    }
                }
                else if (config.record_microphone && !audio_sources.microphone.empty()) {
                    // Microphone only
                    args.push_back("-f");
                    args.push_back("pulse");
                    args.push_back("-i");
                    args.push_back(audio_sources.microphone.c_str());
                }
                else if (config.record_system && !audio_sources.monitor.empty()) {
                    // System audio only
                    args.push_back("-f");
                    args.push_back("pulse");
                    args.push_back("-i");
                    args.push_back(audio_sources.monitor.c_str());
                }
            } else {
                // Fallback to ALSA default
                args.push_back("-f");
                args.push_back("alsa");
                args.push_back("-i");
                args.push_back("default");
            }
        }
        
        // Video codec settings
        args.push_back("-c:v");
        args.push_back("mpeg4");
        args.push_back("-q:v");
        args.push_back("5");
        args.push_back("-pix_fmt");
        args.push_back("yuv420p");
        
        // Audio codec settings
        if (config.capture_audio) {
            args.push_back("-c:a");
            args.push_back("aac");
            args.push_back("-b:a");
            args.push_back(config.audio_quality == "high" ? "192k" : 
                          (config.audio_quality == "medium" ? "128k" : "64k"));
            args.push_back("-ac");
            args.push_back("2");
        } else {
            args.push_back("-an");
        }
        
        args.push_back("-y");
        args.push_back(last_file.c_str());
        args.push_back(nullptr);
        
        // Execute ffmpeg
        execvp("ffmpeg", (char* const*)args.data());
        _exit(1);
    }

    gtk_widget_show_all(indicator);
    blink_id = g_timeout_add(500, blink, NULL);
}

// ---------------- REGION SELECTION UI ----------------
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

    gtk_widget_show_all(dialog);
    gtk_main();
    gtk_widget_destroy(dialog);

    return result;
}

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer) {
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    if (selecting) {
        cairo_set_source_rgba(cr, config.region_r, config.region_g, config.region_b, config.region_alpha);
        gint x = std::min(start_x, end_x);
        gint y = std::min(start_y, end_y);
        gint w = std::abs(end_x - start_x);
        gint h = std::abs(end_y - start_y);
        cairo_rectangle(cr, x, y, w, h);
        cairo_fill(cr);
    }
    return FALSE;
}

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

static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer) {
    if (selecting) {
        end_x = (gint)event->x;
        end_y = (gint)event->y;
        gtk_widget_queue_draw(widget);
    }
    return TRUE;
}

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

void start_recording(int screen_width, int screen_height) {
    int choice = show_capture_mode_dialog();

    if (choice == 2) {
        start_recording_region(0, 0, screen_width, screen_height);
        return;
    } else if (choice == 1) {
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

// ---------------- MAIN ----------------
int main(int argc, char **argv) {
    load_config();
    
    gtk_init(&argc, &argv);

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

    int keycode = XKeysymToKeycode(d, XK_End);
    XGrabKey(d, keycode, ControlMask|ShiftMask,
             root, True, GrabModeAsync, GrabModeAsync);

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

    start_recording(width, height);

    XEvent ev;

    while (running) {
        while (XPending(d)) {
            XNextEvent(d, &ev);
            if (ev.type == KeyPress) {
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