#include <gtk/gtk.h>
#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include <sstream>
#include <iomanip>

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
static GtkWidget *prefs_dialog = nullptr;

// ---------------- FUNCTION DECLARATIONS ----------------
std::string get_config_path();
void ensure_config_dir();
void set_default_config();
void load_config();
void save_config();
static void on_close_clicked(GtkWidget *widget, gpointer data);
static void on_save_clicked(GtkWidget *widget, gpointer data);
static void on_format_changed(GtkComboBox *combo, gpointer data);
static void on_quality_changed(GtkComboBox *combo, gpointer data);
static void on_framerate_changed(GtkComboBox *combo, gpointer data);
static void on_audio_quality_changed(GtkComboBox *combo, gpointer data);
static void on_microphone_toggled(GtkToggleButton *btn, gpointer data);
static void on_system_audio_toggled(GtkToggleButton *btn, gpointer data);
static void on_directory_browse(GtkWidget *button, gpointer data);
static void on_filename_changed(GtkEditable *editable, gpointer data);
static void on_show_mouse_toggled(GtkToggleButton *btn, gpointer data);
static void on_show_dialog_toggled(GtkToggleButton *btn, gpointer data);
static void on_show_notifications_toggled(GtkToggleButton *btn, gpointer data);
static void on_color_set(GtkColorButton *button, gpointer data);
static void on_alpha_changed(GtkSpinButton *spin, gpointer data);
static void on_reset_default_color(GtkWidget *button, gpointer data);
static gboolean on_preview_draw(GtkWidget *widget, cairo_t *cr, gpointer data);
static gboolean on_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data);

// ---------------- CONFIG FILE FUNCTIONS ----------------
std::string get_config_path() {
    return std::string(getenv("HOME")) + "/.config/grec.conf";
}

void ensure_config_dir() {
    std::string config_dir = std::string(getenv("HOME")) + "/.config";
    mkdir(config_dir.c_str(), 0755);
}

void set_default_config() {
    config = GrecConfig();
}

void load_config() {
    std::string path = get_config_path();
    std::ifstream file(path);
    
    if (!file.is_open()) {
        ensure_config_dir();
        set_default_config();
        save_config();
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
    
    config.capture_audio = (config.record_microphone || config.record_system);
}

void save_config() {
    std::string path = get_config_path();
    std::ofstream file(path);
    
    if (!file.is_open()) return;
    
    file << "# Screen Recorder Configuration\n";
    file << "# Used by grec and grec_prefs\n";
    file << "save_format=" << config.save_format << "\n";
    file << "save_directory=" << config.save_directory << "\n";
    file << "filename_pattern=" << config.filename_pattern << "\n";
    file << "auto_increment=" << (config.auto_increment ? "true" : "false") << "\n";
    file << "framerate=" << config.framerate << "\n";
    file << "video_quality=" << config.video_quality << "\n";
    file << "show_mouse=" << (config.show_mouse ? "true" : "false") << "\n";
    file << "capture_audio=" << (config.capture_audio ? "true" : "false") << "\n";
    file << "record_microphone=" << (config.record_microphone ? "true" : "false") << "\n";
    file << "record_system=" << (config.record_system ? "true" : "false") << "\n";
    file << "audio_quality=" << config.audio_quality << "\n";
    file << "region_r=" << config.region_r << "\n";
    file << "region_g=" << config.region_g << "\n";
    file << "region_b=" << config.region_b << "\n";
    file << "region_alpha=" << config.region_alpha << "\n";
    file << "show_save_dialog=" << (config.show_save_dialog ? "true" : "false") << "\n";
    file << "show_notifications=" << (config.show_notifications ? "true" : "false") << "\n";
    file << "hotkey_start_stop=" << config.hotkey_start_stop << "\n";
    file << "hotkey_pause=" << config.hotkey_pause << "\n";
    file << "hotkey_abort=" << config.hotkey_abort << "\n";
    
    file.close();
}

// ---------------- UI CALLBACKS ----------------
static void on_close_clicked(GtkWidget *widget, gpointer data) {
    gtk_widget_destroy(prefs_dialog);
    prefs_dialog = nullptr;
    gtk_main_quit();
}

static void on_save_clicked(GtkWidget *widget, gpointer data) {
    save_config();
    gtk_widget_destroy(prefs_dialog);
    prefs_dialog = nullptr;
    gtk_main_quit();
}

static void on_format_changed(GtkComboBox *combo, gpointer data) {
    gint active = gtk_combo_box_get_active(combo);
    if (active == 0) config.save_format = "mp4";
    else if (active == 1) config.save_format = "mkv";
    else if (active == 2) config.save_format = "gif";
}

static void on_quality_changed(GtkComboBox *combo, gpointer data) {
    gint active = gtk_combo_box_get_active(combo);
    if (active == 0) config.video_quality = "high";
    else if (active == 1) config.video_quality = "balanced";
    else if (active == 2) config.video_quality = "small";
}

static void on_framerate_changed(GtkComboBox *combo, gpointer data) {
    gint active = gtk_combo_box_get_active(combo);
    if (active == 0) config.framerate = 15;
    else if (active == 1) config.framerate = 24;
    else if (active == 2) config.framerate = 30;
    else if (active == 3) config.framerate = 60;
}

static void on_audio_quality_changed(GtkComboBox *combo, gpointer data) {
    gint active = gtk_combo_box_get_active(combo);
    if (active == 0) config.audio_quality = "high";
    else if (active == 1) config.audio_quality = "medium";
    else if (active == 2) config.audio_quality = "low";
}

static void on_microphone_toggled(GtkToggleButton *btn, gpointer data) {
    config.record_microphone = gtk_toggle_button_get_active(btn);
    config.capture_audio = (config.record_microphone || config.record_system);
}

static void on_system_audio_toggled(GtkToggleButton *btn, gpointer data) {
    config.record_system = gtk_toggle_button_get_active(btn);
    config.capture_audio = (config.record_microphone || config.record_system);
}

static void on_directory_browse(GtkWidget *button, gpointer data) {
    GtkEntry *entry = GTK_ENTRY(data);
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Select Save Directory",
        GTK_WINDOW(prefs_dialog),
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Select", GTK_RESPONSE_ACCEPT,
        nullptr
    );
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        gtk_entry_set_text(entry, filename);
        config.save_directory = filename;
        g_free(filename);
    }
    
    gtk_widget_destroy(dialog);
}

static void on_filename_changed(GtkEditable *editable, gpointer data) {
    const char *text = gtk_entry_get_text(GTK_ENTRY(editable));
    config.filename_pattern = text;
}

static void on_show_mouse_toggled(GtkToggleButton *btn, gpointer data) {
    config.show_mouse = gtk_toggle_button_get_active(btn);
}

static void on_show_dialog_toggled(GtkToggleButton *btn, gpointer data) {
    config.show_save_dialog = gtk_toggle_button_get_active(btn);
}

static void on_show_notifications_toggled(GtkToggleButton *btn, gpointer data) {
    config.show_notifications = gtk_toggle_button_get_active(btn);
}

static void on_color_set(GtkColorButton *button, gpointer data) {
    GdkRGBA color;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(button), &color);
    
    config.region_r = color.red;
    config.region_g = color.green;
    config.region_b = color.blue;
    
    gtk_widget_queue_draw(GTK_WIDGET(g_object_get_data(G_OBJECT(prefs_dialog), "color_preview")));
}

static void on_alpha_changed(GtkSpinButton *spin, gpointer data) {
    config.region_alpha = gtk_spin_button_get_value(spin);
    gtk_widget_queue_draw(GTK_WIDGET(g_object_get_data(G_OBJECT(prefs_dialog), "color_preview")));
}

static void on_reset_default_color(GtkWidget *button, gpointer data) {
    config.region_r = 0.98;
    config.region_g = 0.38;
    config.region_b = 0.008;
    config.region_alpha = 0.2;
    
    // Update UI
    GdkRGBA color;
    color.red = config.region_r;
    color.green = config.region_g;
    color.blue = config.region_b;
    color.alpha = 1.0;
    
    GtkColorButton *color_btn = GTK_COLOR_BUTTON(data);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(color_btn), &color);
    
    GtkWidget *alpha_spin = GTK_WIDGET(g_object_get_data(G_OBJECT(prefs_dialog), "alpha_spin"));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(alpha_spin), config.region_alpha);
    
    gtk_widget_queue_draw(GTK_WIDGET(g_object_get_data(G_OBJECT(prefs_dialog), "color_preview")));
}

static gboolean on_preview_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    
    int width = allocation.width;
    int height = allocation.height;
    
    // Draw checkerboard background
    int check_size = 10;
    for (int x = 0; x < width; x += check_size) {
        for (int y = 0; y < height; y += check_size) {
            if ((x / check_size + y / check_size) % 2 == 0) {
                cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
            } else {
                cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
            }
            cairo_rectangle(cr, x, y, check_size, check_size);
            cairo_fill(cr);
        }
    }
    
    // Draw color with transparency
    cairo_set_source_rgba(cr, config.region_r, config.region_g, config.region_b, config.region_alpha);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
    
    return FALSE;
}

static gboolean on_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
    gtk_main_quit();
    return FALSE;
}

// ---------------- MAIN DIALOG ----------------
void show_preferences_dialog() {
    // Load current config
    load_config();
    
    // Create dialog
    prefs_dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(prefs_dialog), "Grec Preferences");
    gtk_window_set_default_size(GTK_WINDOW(prefs_dialog), 600, 550);
    gtk_window_set_position(GTK_WINDOW(prefs_dialog), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(prefs_dialog), 10);
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(prefs_dialog));
    
    // Create notebook for tabs
    GtkWidget *notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(content), notebook, TRUE, TRUE, 0);
    
    // ===== OUTPUT TAB =====
    GtkWidget *output_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(output_tab), 15);
    
    // Format selection
    GtkWidget *format_frame = gtk_frame_new("Save Format");
    gtk_box_pack_start(GTK_BOX(output_tab), format_frame, FALSE, FALSE, 0);
    
    GtkWidget *format_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(format_box), 10);
    gtk_container_add(GTK_CONTAINER(format_frame), format_box);
    
    GtkWidget *format_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(format_combo), "MP4 (Most compatible)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(format_combo), "MKV (Feature rich)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(format_combo), "GIF (No audio)");
    
    if (config.save_format == "mp4") gtk_combo_box_set_active(GTK_COMBO_BOX(format_combo), 0);
    else if (config.save_format == "mkv") gtk_combo_box_set_active(GTK_COMBO_BOX(format_combo), 1);
    else if (config.save_format == "gif") gtk_combo_box_set_active(GTK_COMBO_BOX(format_combo), 2);
    
    g_signal_connect(format_combo, "changed", G_CALLBACK(on_format_changed), nullptr);
    gtk_box_pack_start(GTK_BOX(format_box), format_combo, TRUE, TRUE, 0);
    
    // Save directory
    GtkWidget *dir_frame = gtk_frame_new("Save Directory");
    gtk_box_pack_start(GTK_BOX(output_tab), dir_frame, FALSE, FALSE, 0);
    
    GtkWidget *dir_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(dir_box), 10);
    gtk_container_add(GTK_CONTAINER(dir_frame), dir_box);
    
    GtkWidget *dir_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(dir_entry), config.save_directory.c_str());
    gtk_box_pack_start(GTK_BOX(dir_box), dir_entry, TRUE, TRUE, 0);
    
    GtkWidget *dir_button = gtk_button_new_with_label("Browse");
    g_signal_connect(dir_button, "clicked", G_CALLBACK(on_directory_browse), dir_entry);
    gtk_box_pack_start(GTK_BOX(dir_box), dir_button, FALSE, FALSE, 0);
    
    // Filename pattern
    GtkWidget *name_frame = gtk_frame_new("Filename Pattern");
    gtk_box_pack_start(GTK_BOX(output_tab), name_frame, FALSE, FALSE, 0);
    
    GtkWidget *name_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(name_box), 10);
    gtk_container_add(GTK_CONTAINER(name_frame), name_box);
    
    GtkWidget *name_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(name_entry), config.filename_pattern.c_str());
    g_signal_connect(name_entry, "changed", G_CALLBACK(on_filename_changed), nullptr);
    gtk_box_pack_start(GTK_BOX(name_box), name_entry, FALSE, FALSE, 0);
    
    GtkWidget *name_hint = gtk_label_new("Use %Y%m%d_%H%M%S for timestamp\nExample: recording_20231225_143022.mp4");
    gtk_box_pack_start(GTK_BOX(name_box), name_hint, FALSE, FALSE, 0);
    
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), output_tab, gtk_label_new("Output"));
    
    // ===== VIDEO TAB =====
    GtkWidget *video_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(video_tab), 15);
    
    // Quality
    GtkWidget *quality_frame = gtk_frame_new("Video Quality");
    gtk_box_pack_start(GTK_BOX(video_tab), quality_frame, FALSE, FALSE, 0);
    
    GtkWidget *quality_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(quality_box), 10);
    gtk_container_add(GTK_CONTAINER(quality_frame), quality_box);
    
    GtkWidget *quality_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(quality_combo), "High (largest files)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(quality_combo), "Balanced");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(quality_combo), "Small (smallest files)");
    
    if (config.video_quality == "high") gtk_combo_box_set_active(GTK_COMBO_BOX(quality_combo), 0);
    else if (config.video_quality == "balanced") gtk_combo_box_set_active(GTK_COMBO_BOX(quality_combo), 1);
    else if (config.video_quality == "small") gtk_combo_box_set_active(GTK_COMBO_BOX(quality_combo), 2);
    
    g_signal_connect(quality_combo, "changed", G_CALLBACK(on_quality_changed), nullptr);
    gtk_box_pack_start(GTK_BOX(quality_box), quality_combo, TRUE, TRUE, 0);
    
    // Framerate
    GtkWidget *fps_frame = gtk_frame_new("Framerate");
    gtk_box_pack_start(GTK_BOX(video_tab), fps_frame, FALSE, FALSE, 0);
    
    GtkWidget *fps_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(fps_box), 10);
    gtk_container_add(GTK_CONTAINER(fps_frame), fps_box);
    
    GtkWidget *fps_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(fps_combo), "15 fps (smallest files)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(fps_combo), "24 fps (film)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(fps_combo), "30 fps (standard)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(fps_combo), "60 fps (smooth)");
    
    int fps_index = 2;
    if (config.framerate == 15) fps_index = 0;
    else if (config.framerate == 24) fps_index = 1;
    else if (config.framerate == 30) fps_index = 2;
    else if (config.framerate == 60) fps_index = 3;
    
    gtk_combo_box_set_active(GTK_COMBO_BOX(fps_combo), fps_index);
    g_signal_connect(fps_combo, "changed", G_CALLBACK(on_framerate_changed), nullptr);
    gtk_box_pack_start(GTK_BOX(fps_box), fps_combo, TRUE, TRUE, 0);
    
    // Mouse
    GtkWidget *mouse_check = gtk_check_button_new_with_label("Show mouse cursor in recording");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mouse_check), config.show_mouse);
    g_signal_connect(mouse_check, "toggled", G_CALLBACK(on_show_mouse_toggled), nullptr);
    gtk_box_pack_start(GTK_BOX(video_tab), mouse_check, FALSE, FALSE, 0);
    
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), video_tab, gtk_label_new("Video"));
    
    // ===== AUDIO TAB =====
    GtkWidget *audio_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(audio_tab), 15);
    
    // Audio source selection
    GtkWidget *source_frame = gtk_frame_new("Audio Sources (select both for mixed audio)");
    gtk_box_pack_start(GTK_BOX(audio_tab), source_frame, FALSE, FALSE, 0);
    
    GtkWidget *source_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(source_box), 10);
    gtk_container_add(GTK_CONTAINER(source_frame), source_box);
    
    GtkWidget *mic_check = gtk_check_button_new_with_label("Record Microphone");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mic_check), config.record_microphone);
    g_signal_connect(mic_check, "toggled", G_CALLBACK(on_microphone_toggled), nullptr);
    gtk_box_pack_start(GTK_BOX(source_box), mic_check, FALSE, FALSE, 0);
    
    GtkWidget *system_check = gtk_check_button_new_with_label("Record System Audio");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(system_check), config.record_system);
    g_signal_connect(system_check, "toggled", G_CALLBACK(on_system_audio_toggled), nullptr);
    gtk_box_pack_start(GTK_BOX(source_box), system_check, FALSE, FALSE, 0);
    
    // Audio quality
    GtkWidget *audio_quality_frame = gtk_frame_new("Audio Quality");
    gtk_box_pack_start(GTK_BOX(audio_tab), audio_quality_frame, FALSE, FALSE, 0);
    
    GtkWidget *audio_quality_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(audio_quality_box), 10);
    gtk_container_add(GTK_CONTAINER(audio_quality_frame), audio_quality_box);
    
    GtkWidget *audio_quality_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(audio_quality_combo), "High (192kbps)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(audio_quality_combo), "Medium (128kbps)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(audio_quality_combo), "Low (64kbps)");
    
    int aq_index = 1;
    if (config.audio_quality == "high") aq_index = 0;
    else if (config.audio_quality == "medium") aq_index = 1;
    else if (config.audio_quality == "low") aq_index = 2;
    
    gtk_combo_box_set_active(GTK_COMBO_BOX(audio_quality_combo), aq_index);
    g_signal_connect(audio_quality_combo, "changed", G_CALLBACK(on_audio_quality_changed), nullptr);
    gtk_box_pack_start(GTK_BOX(audio_quality_box), audio_quality_combo, TRUE, TRUE, 0);
    
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), audio_tab, gtk_label_new("Audio"));
    
    // ===== REGION SELECTOR TAB =====
    GtkWidget *region_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(region_tab), 15);
    
    GtkWidget *color_frame = gtk_frame_new("Selection Overlay Color");
    gtk_box_pack_start(GTK_BOX(region_tab), color_frame, FALSE, FALSE, 0);
    
    GtkWidget *color_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(color_box), 10);
    gtk_container_add(GTK_CONTAINER(color_frame), color_box);
    
    GtkWidget *color_btn = gtk_color_button_new();
    GdkRGBA color;
    color.red = config.region_r;
    color.green = config.region_g;
    color.blue = config.region_b;
    color.alpha = 1.0;
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(color_btn), &color);
    gtk_box_pack_start(GTK_BOX(color_box), color_btn, FALSE, FALSE, 0);
    g_signal_connect(color_btn, "color-set", G_CALLBACK(on_color_set), nullptr);
    
    GtkWidget *alpha_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(color_box), alpha_box, FALSE, FALSE, 0);
    
    GtkWidget *alpha_label = gtk_label_new("Transparency:");
    gtk_box_pack_start(GTK_BOX(alpha_box), alpha_label, FALSE, FALSE, 0);
    
    GtkWidget *alpha_spin = gtk_spin_button_new_with_range(0.0, 1.0, 0.05);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(alpha_spin), config.region_alpha);
    gtk_box_pack_start(GTK_BOX(alpha_box), alpha_spin, TRUE, TRUE, 0);
    g_signal_connect(alpha_spin, "value-changed", G_CALLBACK(on_alpha_changed), nullptr);
    
    GtkWidget *reset_btn = gtk_button_new_with_label("Reset to Default Color");
    g_signal_connect(reset_btn, "clicked", G_CALLBACK(on_reset_default_color), color_btn);
    gtk_box_pack_start(GTK_BOX(color_box), reset_btn, FALSE, FALSE, 0);
    
    g_object_set_data(G_OBJECT(prefs_dialog), "alpha_spin", alpha_spin);
    
    // Preview area
    GtkWidget *preview_frame = gtk_frame_new("Preview");
    gtk_box_pack_start(GTK_BOX(region_tab), preview_frame, TRUE, TRUE, 0);
    
    GtkWidget *color_preview = gtk_drawing_area_new();
    gtk_widget_set_size_request(color_preview, -1, 100);
    g_signal_connect(color_preview, "draw", G_CALLBACK(on_preview_draw), nullptr);
    gtk_container_add(GTK_CONTAINER(preview_frame), color_preview);
    g_object_set_data(G_OBJECT(prefs_dialog), "color_preview", color_preview);
    
    GtkWidget *info_label = gtk_label_new(
        "Choose the overlay color and transparency for the region selector."
    );
    gtk_box_pack_start(GTK_BOX(region_tab), info_label, FALSE, FALSE, 0);
    
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), region_tab, gtk_label_new("Region Selector"));
    
    // ===== AFTER RECORDING TAB =====
    GtkWidget *after_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(after_tab), 15);
    
    GtkWidget *show_dialog_check = gtk_check_button_new_with_label("Show save location dialog after recording");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_dialog_check), config.show_save_dialog);
    g_signal_connect(show_dialog_check, "toggled", G_CALLBACK(on_show_dialog_toggled), nullptr);
    gtk_box_pack_start(GTK_BOX(after_tab), show_dialog_check, FALSE, FALSE, 0);
    
    GtkWidget *notify_check = gtk_check_button_new_with_label("Show desktop notifications");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(notify_check), config.show_notifications);
    g_signal_connect(notify_check, "toggled", G_CALLBACK(on_show_notifications_toggled), nullptr);
    gtk_box_pack_start(GTK_BOX(after_tab), notify_check, FALSE, FALSE, 0);
    
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), after_tab, gtk_label_new("After Recording"));
    
    // Buttons
    GtkWidget *button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
    gtk_box_pack_start(GTK_BOX(content), button_box, FALSE, FALSE, 5);
    
    GtkWidget *cancel_btn = gtk_button_new_with_label("Cancel");
    GtkWidget *save_btn = gtk_button_new_with_label("Save");
    
    gtk_container_add(GTK_CONTAINER(button_box), cancel_btn);
    gtk_container_add(GTK_CONTAINER(button_box), save_btn);
    
    g_signal_connect(cancel_btn, "clicked", G_CALLBACK(on_close_clicked), nullptr);
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_save_clicked), nullptr);
    g_signal_connect(prefs_dialog, "delete-event", G_CALLBACK(on_delete_event), nullptr);
    
    gtk_widget_show_all(prefs_dialog);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    show_preferences_dialog();
    
    gtk_main();
    
    return 0;
}