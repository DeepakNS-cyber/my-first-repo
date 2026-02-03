#include <gtk/gtk.h>
#include <json-glib/json-glib.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_ALGOS 1000
#define DATA_FILE "cube_algorithms.json"

typedef struct {
    char name[256];
    char type[64];
    char formula[1024];
    char top_layer[9];
    char side_front[3];
    char side_right[3];
    char side_back[3];
    char side_left[3];
    int id;
} Algorithm;

typedef struct {
    Algorithm algos[MAX_ALGOS];
    int count;
    
    GtkWidget *window;
    GtkWidget *list_view;
    GtkWidget *search_entry;
    GtkListStore *list_store;
    
    GtkWidget *form_window;
    GtkWidget *name_entry;
    GtkWidget *type_combo;
    GtkWidget *formula_text;
    
    char current_top[9];
    char current_sides[4][3];
    int editing_id;
    
    GtkWidget *cube_buttons[9];
    GtkWidget *side_buttons[4][3];
} AppData;

const char colors[] = {'Y', 'O', 'B', 'R', 'G', 'W', 'X'};

void set_button_color(GtkWidget *button, char color) {
    GdkRGBA rgba;
    switch(color) {
        case 'Y': gdk_rgba_parse(&rgba, "#FBBF24"); break;
        case 'O': gdk_rgba_parse(&rgba, "#F97316"); break;
        case 'B': gdk_rgba_parse(&rgba, "#3B82F6"); break;
        case 'R': gdk_rgba_parse(&rgba, "#EF4444"); break;
        case 'G': gdk_rgba_parse(&rgba, "#10B981"); break;
        case 'W': gdk_rgba_parse(&rgba, "#FFFFFF"); break;
        default: gdk_rgba_parse(&rgba, "#D1D5DB"); break;
    }
    
    GtkStyleContext *context = gtk_widget_get_style_context(button);
    GtkCssProvider *provider = gtk_css_provider_new();
    
    char css[512];
    snprintf(css, sizeof(css), 
        "button { background-image: none; background-color: rgb(%d,%d,%d); "
        "border: 2px solid #374151; min-width: 40px; min-height: 40px; "
        "border-radius: 4px; }",
        (int)(rgba.red * 255), (int)(rgba.green * 255), (int)(rgba.blue * 255));
    
    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(provider);
}

char cycle_color(char current) {
    for (int i = 0; i < 7; i++) {
        if (colors[i] == current) {
            return colors[(i + 1) % 7];
        }
    }
    return 'Y';
}

void on_cube_button_clicked(GtkWidget *widget, gpointer data) {
    AppData *app = (AppData *)data;
    int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "index"));
    
    app->current_top[index] = cycle_color(app->current_top[index]);
    set_button_color(widget, app->current_top[index]);
}

void on_side_button_clicked(GtkWidget *widget, gpointer data) {
    AppData *app = (AppData *)data;
    int side = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "side"));
    int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "index"));
    
    app->current_sides[side][index] = cycle_color(app->current_sides[side][index]);
    set_button_color(widget, app->current_sides[side][index]);
}

void save_to_file(AppData *app) {
    JsonBuilder *builder = json_builder_new();
    json_builder_begin_array(builder);
    
    for (int i = 0; i < app->count; i++) {
        json_builder_begin_object(builder);
        
        json_builder_set_member_name(builder, "id");
        json_builder_add_int_value(builder, app->algos[i].id);
        
        json_builder_set_member_name(builder, "name");
        json_builder_add_string_value(builder, app->algos[i].name);
        
        json_builder_set_member_name(builder, "type");
        json_builder_add_string_value(builder, app->algos[i].type);
        
        json_builder_set_member_name(builder, "formula");
        json_builder_add_string_value(builder, app->algos[i].formula);
        
        json_builder_set_member_name(builder, "top_layer");
        char top_str[10];
        memcpy(top_str, app->algos[i].top_layer, 9);
        top_str[9] = '\0';
        json_builder_add_string_value(builder, top_str);
        
        json_builder_set_member_name(builder, "side_front");
        char front_str[4];
        memcpy(front_str, app->algos[i].side_front, 3);
        front_str[3] = '\0';
        json_builder_add_string_value(builder, front_str);
        
        json_builder_set_member_name(builder, "side_right");
        char right_str[4];
        memcpy(right_str, app->algos[i].side_right, 3);
        right_str[3] = '\0';
        json_builder_add_string_value(builder, right_str);
        
        json_builder_set_member_name(builder, "side_back");
        char back_str[4];
        memcpy(back_str, app->algos[i].side_back, 3);
        back_str[3] = '\0';
        json_builder_add_string_value(builder, back_str);
        
        json_builder_set_member_name(builder, "side_left");
        char left_str[4];
        memcpy(left_str, app->algos[i].side_left, 3);
        left_str[3] = '\0';
        json_builder_add_string_value(builder, left_str);
        
        json_builder_end_object(builder);
    }
    
    json_builder_end_array(builder);
    
    JsonGenerator *gen = json_generator_new();
    JsonNode *root = json_builder_get_root(builder);
    json_generator_set_root(gen, root);
    json_generator_set_pretty(gen, TRUE);
    
    json_generator_to_file(gen, DATA_FILE, NULL);
    
    json_node_free(root);
    g_object_unref(gen);
    g_object_unref(builder);
}

void load_from_file(AppData *app) {
    if (!g_file_test(DATA_FILE, G_FILE_TEST_EXISTS)) {
        return;
    }
    
    GError *error = NULL;
    JsonParser *parser = json_parser_new();
    
    if (!json_parser_load_from_file(parser, DATA_FILE, &error)) {
        if (error) {
            g_error_free(error);
        }
        g_object_unref(parser);
        return;
    }
    
    JsonNode *root = json_parser_get_root(parser);
    if (!root || !JSON_NODE_HOLDS_ARRAY(root)) {
        g_object_unref(parser);
        return;
    }
    
    JsonArray *array = json_node_get_array(root);
    guint len = json_array_get_length(array);
    
    app->count = 0;
    for (guint i = 0; i < len && app->count < MAX_ALGOS; i++) {
        JsonObject *obj = json_array_get_object_element(array, i);
        if (!obj) continue;
        
        app->algos[app->count].id = json_object_get_int_member(obj, "id");
        
        const char *name = json_object_get_string_member(obj, "name");
        if (name) strncpy(app->algos[app->count].name, name, 255);
        
        const char *type = json_object_get_string_member(obj, "type");
        if (type) strncpy(app->algos[app->count].type, type, 63);
        
        const char *formula = json_object_get_string_member(obj, "formula");
        if (formula) strncpy(app->algos[app->count].formula, formula, 1023);
        
        const char *top = json_object_get_string_member(obj, "top_layer");
        if (top && strlen(top) >= 9) {
            memcpy(app->algos[app->count].top_layer, top, 9);
        }
        
        const char *front = json_object_get_string_member(obj, "side_front");
        if (front && strlen(front) >= 3) {
            memcpy(app->algos[app->count].side_front, front, 3);
        }
        
        const char *right = json_object_get_string_member(obj, "side_right");
        if (right && strlen(right) >= 3) {
            memcpy(app->algos[app->count].side_right, right, 3);
        }
        
        const char *back = json_object_get_string_member(obj, "side_back");
        if (back && strlen(back) >= 3) {
            memcpy(app->algos[app->count].side_back, back, 3);
        }
        
        const char *left = json_object_get_string_member(obj, "side_left");
        if (left && strlen(left) >= 3) {
            memcpy(app->algos[app->count].side_left, left, 3);
        }
        
        app->count++;
    }
    
    g_object_unref(parser);
}

void refresh_list(AppData *app) {
    gtk_list_store_clear(app->list_store);
    
    const char *search_text = gtk_entry_get_text(GTK_ENTRY(app->search_entry));
    
    for (int i = 0; i < app->count; i++) {
        if (strlen(search_text) > 0) {
            char search_lower[256];
            strncpy(search_lower, search_text, 255);
            for (int j = 0; search_lower[j]; j++) {
                search_lower[j] = tolower(search_lower[j]);
            }
            
            char name_lower[256];
            strncpy(name_lower, app->algos[i].name, 255);
            for (int j = 0; name_lower[j]; j++) {
                name_lower[j] = tolower(name_lower[j]);
            }
            
            if (!strstr(name_lower, search_lower) &&
                !strstr(app->algos[i].type, search_text) &&
                !strstr(app->algos[i].formula, search_text)) {
                continue;
            }
        }
        
        GtkTreeIter iter;
        gtk_list_store_append(app->list_store, &iter);
        gtk_list_store_set(app->list_store, &iter,
                          0, app->algos[i].name,
                          1, app->algos[i].type,
                          2, app->algos[i].formula,
                          3, app->algos[i].id,
                          -1);
    }
}

void on_search_changed(GtkEntry *entry, gpointer data) {
    AppData *app = (AppData *)data;
    refresh_list(app);
}

void reset_form(AppData *app) {
    gtk_entry_set_text(GTK_ENTRY(app->name_entry), "");
    gtk_combo_box_set_active(GTK_COMBO_BOX(app->type_combo), 0);
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->formula_text));
    gtk_text_buffer_set_text(buffer, "", -1);
    
    for (int i = 0; i < 9; i++) {
        app->current_top[i] = 'Y';
        set_button_color(app->cube_buttons[i], 'Y');
    }
    
    for (int s = 0; s < 4; s++) {
        for (int i = 0; i < 3; i++) {
            app->current_sides[s][i] = 'X';
            set_button_color(app->side_buttons[s][i], 'X');
        }
    }
    
    app->editing_id = -1;
}

void on_save_clicked(GtkWidget *widget, gpointer data) {
    AppData *app = (AppData *)data;
    
    const char *name = gtk_entry_get_text(GTK_ENTRY(app->name_entry));
    if (strlen(name) == 0) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->form_window),
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_WARNING,
                                                   GTK_BUTTONS_OK,
                                                   "Please enter an algorithm name");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->formula_text));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    char *formula = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    
    if (strlen(formula) == 0) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->form_window),
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_WARNING,
                                                   GTK_BUTTONS_OK,
                                                   "Please enter a formula");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_free(formula);
        return;
    }
    
    Algorithm *algo = NULL;
    if (app->editing_id >= 0) {
        for (int i = 0; i < app->count; i++) {
            if (app->algos[i].id == app->editing_id) {
                algo = &app->algos[i];
                break;
            }
        }
    } else {
        if (app->count < MAX_ALGOS) {
            algo = &app->algos[app->count];
            algo->id = (int)time(NULL) + rand();
            app->count++;
        }
    }
    
    if (algo) {
        strncpy(algo->name, name, 255);
        algo->name[255] = '\0';
        
        const char *type = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(app->type_combo));
        if (type) {
            strncpy(algo->type, type, 63);
            algo->type[63] = '\0';
        }
        
        strncpy(algo->formula, formula, 1023);
        algo->formula[1023] = '\0';
        
        memcpy(algo->top_layer, app->current_top, 9);
        memcpy(algo->side_front, app->current_sides[0], 3);
        memcpy(algo->side_right, app->current_sides[1], 3);
        memcpy(algo->side_back, app->current_sides[2], 3);
        memcpy(algo->side_left, app->current_sides[3], 3);
        
        save_to_file(app);
        refresh_list(app);
    }
    
    g_free(formula);
    gtk_widget_hide(app->form_window);
}

void on_add_clicked(GtkWidget *widget, gpointer data) {
    AppData *app = (AppData *)data;
    reset_form(app);
    gtk_window_set_title(GTK_WINDOW(app->form_window), "Add New Algorithm");
    gtk_widget_show_all(app->form_window);
}

void on_edit_clicked(GtkWidget *widget, gpointer data) {
    AppData *app = (AppData *)data;
    
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app->list_view));
    GtkTreeIter iter;
    GtkTreeModel *model;
    
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        int id;
        gtk_tree_model_get(model, &iter, 3, &id, -1);
        
        Algorithm *algo = NULL;
        for (int i = 0; i < app->count; i++) {
            if (app->algos[i].id == id) {
                algo = &app->algos[i];
                break;
            }
        }
        
        if (algo) {
            app->editing_id = id;
            gtk_entry_set_text(GTK_ENTRY(app->name_entry), algo->name);
            
            const char *types[] = {"OLL", "PLL", "F2L", "ZBLL", "COLL", "Other"};
            for (int i = 0; i < 6; i++) {
                if (strcmp(algo->type, types[i]) == 0) {
                    gtk_combo_box_set_active(GTK_COMBO_BOX(app->type_combo), i);
                    break;
                }
            }
            
            GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->formula_text));
            gtk_text_buffer_set_text(buffer, algo->formula, -1);
            
            memcpy(app->current_top, algo->top_layer, 9);
            memcpy(app->current_sides[0], algo->side_front, 3);
            memcpy(app->current_sides[1], algo->side_right, 3);
            memcpy(app->current_sides[2], algo->side_back, 3);
            memcpy(app->current_sides[3], algo->side_left, 3);
            
            for (int i = 0; i < 9; i++) {
                set_button_color(app->cube_buttons[i], app->current_top[i]);
            }
            
            for (int s = 0; s < 4; s++) {
                for (int i = 0; i < 3; i++) {
                    set_button_color(app->side_buttons[s][i], app->current_sides[s][i]);
                }
            }
            
            gtk_window_set_title(GTK_WINDOW(app->form_window), "Edit Algorithm");
            gtk_widget_show_all(app->form_window);
        }
    }
}

void on_delete_clicked(GtkWidget *widget, gpointer data) {
    AppData *app = (AppData *)data;
    
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app->list_view));
    GtkTreeIter iter;
    GtkTreeModel *model;
    
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        int id;
        gtk_tree_model_get(model, &iter, 3, &id, -1);
        
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->window),
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_QUESTION,
                                                   GTK_BUTTONS_YES_NO,
                                                   "Delete this algorithm?");
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        
        if (response == GTK_RESPONSE_YES) {
            for (int i = 0; i < app->count; i++) {
                if (app->algos[i].id == id) {
                    for (int j = i; j < app->count - 1; j++) {
                        app->algos[j] = app->algos[j + 1];
                    }
                    app->count--;
                    break;
                }
            }
            save_to_file(app);
            refresh_list(app);
        }
    }
}

void on_reset_colors_clicked(GtkWidget *widget, gpointer data) {
    AppData *app = (AppData *)data;
    
    for (int i = 0; i < 9; i++) {
        app->current_top[i] = 'Y';
        set_button_color(app->cube_buttons[i], 'Y');
    }
    
    for (int s = 0; s < 4; s++) {
        for (int i = 0; i < 3; i++) {
            app->current_sides[s][i] = 'X';
            set_button_color(app->side_buttons[s][i], 'X');
        }
    }
}

void create_form_window(AppData *app) {
    app->form_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->form_window), "Add Algorithm");
    gtk_window_set_default_size(GTK_WINDOW(app->form_window), 700, 600);
    gtk_window_set_resizable(GTK_WINDOW(app->form_window), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(app->form_window), GTK_WINDOW(app->window));
    gtk_window_set_modal(GTK_WINDOW(app->form_window), TRUE);
    g_signal_connect(app->form_window, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);
    
 GtkWidget *scroll_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_window),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(app->form_window), scroll_window);
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);
    gtk_container_add(GTK_CONTAINER(scroll_window), vbox);
    
    GtkWidget *hbox_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *form_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(form_title), "<span size='large' weight='bold'>Algorithm Details</span>");
    gtk_widget_set_halign(form_title, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(hbox_header), form_title, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_header, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 5);
    
    GtkWidget *name_label = gtk_label_new("Algorithm Name:");
    gtk_widget_set_halign(name_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), name_label, FALSE, FALSE, 0);
    
    app->name_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app->name_entry), "e.g., T-Perm, Antisune");
    gtk_box_pack_start(GTK_BOX(vbox), app->name_entry, FALSE, FALSE, 0);
    
    GtkWidget *type_label = gtk_label_new("Type:");
    gtk_widget_set_halign(type_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), type_label, FALSE, FALSE, 0);
    
    app->type_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->type_combo), "OLL");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->type_combo), "PLL");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->type_combo), "F2L");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->type_combo), "ZBLL");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->type_combo), "COLL");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->type_combo), "Other");
    gtk_combo_box_set_active(GTK_COMBO_BOX(app->type_combo), 0);
    gtk_box_pack_start(GTK_BOX(vbox), app->type_combo, FALSE, FALSE, 0);
    
    GtkWidget *hbox_cube_label = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *cube_label = gtk_label_new("Mark Cube State (Click to cycle colors):");
    gtk_widget_set_halign(cube_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(hbox_cube_label), cube_label, TRUE, TRUE, 0);
    
    GtkWidget *reset_btn = gtk_button_new_with_label("Reset Colors");
    g_signal_connect(reset_btn, "clicked", G_CALLBACK(on_reset_colors_clicked), app);
    gtk_box_pack_start(GTK_BOX(hbox_cube_label), reset_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_cube_label, FALSE, FALSE, 0);
    
    GtkWidget *cube_frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(cube_frame), GTK_SHADOW_IN);
    
    GtkWidget *cube_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(cube_container), 20);
    
    GtkWidget *back_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_widget_set_halign(back_box, GTK_ALIGN_CENTER);
    for (int i = 0; i < 3; i++) {
        app->side_buttons[2][i] = gtk_button_new();
        gtk_widget_set_size_request(app->side_buttons[2][i], 30, 30);
        g_object_set_data(G_OBJECT(app->side_buttons[2][i]), "side", GINT_TO_POINTER(2));
        g_object_set_data(G_OBJECT(app->side_buttons[2][i]), "index", GINT_TO_POINTER(i));
        g_signal_connect(app->side_buttons[2][i], "clicked", G_CALLBACK(on_side_button_clicked), app);
        gtk_box_pack_start(GTK_BOX(back_box), app->side_buttons[2][i], FALSE, FALSE, 0);
        set_button_color(app->side_buttons[2][i], 'X');
    }
    gtk_box_pack_start(GTK_BOX(cube_container), back_box, FALSE, FALSE, 0);
    
    GtkWidget *middle_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_halign(middle_box, GTK_ALIGN_CENTER);
    
    GtkWidget *left_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    for (int i = 0; i < 3; i++) {
        app->side_buttons[3][i] = gtk_button_new();
        gtk_widget_set_size_request(app->side_buttons[3][i], 30, 30);
        g_object_set_data(G_OBJECT(app->side_buttons[3][i]), "side", GINT_TO_POINTER(3));
        g_object_set_data(G_OBJECT(app->side_buttons[3][i]), "index", GINT_TO_POINTER(i));
        g_signal_connect(app->side_buttons[3][i], "clicked", G_CALLBACK(on_side_button_clicked), app);
        gtk_box_pack_start(GTK_BOX(left_box), app->side_buttons[3][i], FALSE, FALSE, 0);
        set_button_color(app->side_buttons[3][i], 'X');
    }
    gtk_box_pack_start(GTK_BOX(middle_box), left_box, FALSE, FALSE, 0);
    
    GtkWidget *cube_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(cube_grid), 2);
    gtk_grid_set_column_spacing(GTK_GRID(cube_grid), 2);
    
    for (int i = 0; i < 9; i++) {
        app->cube_buttons[i] = gtk_button_new();
        gtk_widget_set_size_request(app->cube_buttons[i], 40, 40);
        g_object_set_data(G_OBJECT(app->cube_buttons[i]), "index", GINT_TO_POINTER(i));
        g_signal_connect(app->cube_buttons[i], "clicked", G_CALLBACK(on_cube_button_clicked), app);
        gtk_grid_attach(GTK_GRID(cube_grid), app->cube_buttons[i], i % 3, i / 3, 1, 1);
        set_button_color(app->cube_buttons[i], 'Y');
    }
    gtk_box_pack_start(GTK_BOX(middle_box), cube_grid, FALSE, FALSE, 0);
    
    GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    for (int i = 0; i < 3; i++) {
        app->side_buttons[1][i] = gtk_button_new();
        gtk_widget_set_size_request(app->side_buttons[1][i],30, 30);
g_object_set_data(G_OBJECT(app->side_buttons[1][i]), "side", GINT_TO_POINTER(1));
g_object_set_data(G_OBJECT(app->side_buttons[1][i]), "index", GINT_TO_POINTER(i));
g_signal_connect(app->side_buttons[1][i], "clicked", G_CALLBACK(on_side_button_clicked), app);
gtk_box_pack_start(GTK_BOX(right_box), app->side_buttons[1][i], FALSE, FALSE, 0);
set_button_color(app->side_buttons[1][i], 'X');
}
gtk_box_pack_start(GTK_BOX(middle_box), right_box, FALSE, FALSE, 0);
gtk_box_pack_start(GTK_BOX(cube_container), middle_box, FALSE, FALSE, 0);

GtkWidget *front_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
gtk_widget_set_halign(front_box, GTK_ALIGN_CENTER);
for (int i = 0; i < 3; i++) {
    app->side_buttons[0][i] = gtk_button_new();
    gtk_widget_set_size_request(app->side_buttons[0][i], 30, 30);
    g_object_set_data(G_OBJECT(app->side_buttons[0][i]), "side", GINT_TO_POINTER(0));
    g_object_set_data(G_OBJECT(app->side_buttons[0][i]), "index", GINT_TO_POINTER(i));
    g_signal_connect(app->side_buttons[0][i], "clicked", G_CALLBACK(on_side_button_clicked), app);
    gtk_box_pack_start(GTK_BOX(front_box), app->side_buttons[0][i], FALSE, FALSE, 0);
    set_button_color(app->side_buttons[0][i], 'X');
}
gtk_box_pack_start(GTK_BOX(cube_container), front_box, FALSE, FALSE, 0);

GtkWidget *inst_label = gtk_label_new(NULL);
gtk_label_set_markup(GTK_LABEL(inst_label), 
    "<small>• Top layer shows orientation (OLL)\n"
    "• Side stickers show edge/corner colors (PLL)\n"
    "• Center piece is reference point</small>");
gtk_label_set_justify(GTK_LABEL(inst_label), GTK_JUSTIFY_CENTER);
gtk_box_pack_start(GTK_BOX(cube_container), inst_label, FALSE, FALSE, 5);

gtk_container_add(GTK_CONTAINER(cube_frame), cube_container);
gtk_box_pack_start(GTK_BOX(vbox), cube_frame, FALSE, FALSE, 10);

GtkWidget *formula_label = gtk_label_new("Algorithm Formula:");
gtk_widget_set_halign(formula_label, GTK_ALIGN_START);
gtk_box_pack_start(GTK_BOX(vbox), formula_label, FALSE, FALSE, 0);

GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
gtk_widget_set_size_request(scroll, -1, 100);
gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                               GTK_POLICY_AUTOMATIC,
                               GTK_POLICY_AUTOMATIC);
app->formula_text = gtk_text_view_new();
gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(app->formula_text), GTK_WRAP_WORD);
gtk_container_add(GTK_CONTAINER(scroll), app->formula_text);
gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);

GtkWidget *note_label = gtk_label_new(NULL);
gtk_label_set_markup(GTK_LABEL(note_label), 
    "<small>Use standard notation: R, L, U, D, F, B (add ' for counter-clockwise, 2 for double turn)</small>");
gtk_widget_set_halign(note_label, GTK_ALIGN_START);
gtk_box_pack_start(GTK_BOX(vbox), note_label, FALSE, FALSE, 0);

GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

GtkWidget *save_btn = gtk_button_new_with_label("💾 Save Algorithm");
g_signal_connect(save_btn, "clicked", G_CALLBACK(on_save_clicked), app);
gtk_box_pack_start(GTK_BOX(button_box), save_btn, TRUE, TRUE, 0);

GtkWidget *cancel_btn = gtk_button_new_with_label("Cancel");
g_signal_connect_swapped(cancel_btn, "clicked", G_CALLBACK(gtk_widget_hide), app->form_window);
gtk_box_pack_start(GTK_BOX(button_box), cancel_btn, TRUE, TRUE, 0);

gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, FALSE, 0);
}
int main(int argc, char *argv[]) {
gtk_init(&argc, &argv);
srand(time(NULL));

AppData app;
memset(&app, 0, sizeof(AppData));
app.editing_id = -1;

load_from_file(&app);

app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
gtk_window_set_title(GTK_WINDOW(app.window), "Cube Algorithm Manager");
gtk_window_set_default_size(GTK_WINDOW(app.window), 1000, 600);
g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);
gtk_container_add(GTK_CONTAINER(app.window), vbox);

GtkWidget *header = gtk_label_new(NULL);
gtk_label_set_markup(GTK_LABEL(header), 
    "<span size='xx-large' weight='bold'>Cube Algorithm Manager</span>\n"
    "<span size='medium'>Store and organize your advanced OLL and PLL algorithms</span>");
gtk_label_set_justify(GTK_LABEL(header), GTK_JUSTIFY_CENTER);
gtk_box_pack_start(GTK_BOX(vbox), header, FALSE, FALSE, 5);

gtk_box_pack_start(GTK_BOX(vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 5);

GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

app.search_entry = gtk_entry_new();
gtk_entry_set_placeholder_text(GTK_ENTRY(app.search_entry), "🔍 Search algorithms...");
g_signal_connect(app.search_entry, "changed", G_CALLBACK(on_search_changed), &app);
gtk_box_pack_start(GTK_BOX(hbox), app.search_entry, TRUE, TRUE, 0);

GtkWidget *add_btn = gtk_button_new_with_label("+ Add Algorithm");
g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_clicked), &app);
gtk_box_pack_start(GTK_BOX(hbox), add_btn, FALSE, FALSE, 0);

GtkWidget *edit_btn = gtk_button_new_with_label("✏ Edit");
g_signal_connect(edit_btn, "clicked", G_CALLBACK(on_edit_clicked), &app);
gtk_box_pack_start(GTK_BOX(hbox), edit_btn, FALSE, FALSE, 0);

GtkWidget *del_btn = gtk_button_new_with_label("🗑 Delete");
g_signal_connect(del_btn, "clicked", G_CALLBACK(on_delete_clicked), &app);
gtk_box_pack_start(GTK_BOX(hbox), del_btn, FALSE, FALSE, 0);

gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                               GTK_POLICY_AUTOMATIC,
                               GTK_POLICY_AUTOMATIC);

app.list_store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, 
                                    G_TYPE_STRING, G_TYPE_INT);
app.list_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(app.list_store));
gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(app.list_view), TRUE);

GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
GtkTreeViewColumn *col;

col = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", 0, NULL);
gtk_tree_view_column_set_expand(col, FALSE);
gtk_tree_view_column_set_min_width(col, 150);
gtk_tree_view_append_column(GTK_TREE_VIEW(app.list_view), col);

col = gtk_tree_view_column_new_with_attributes("Type", renderer, "text", 1, NULL);
gtk_tree_view_column_set_min_width(col, 80);
gtk_tree_view_append_column(GTK_TREE_VIEW(app.list_view), col);

col = gtk_tree_view_column_new_with_attributes("Formula", renderer, "text", 2, NULL);
gtk_tree_view_column_set_expand(col, TRUE);
gtk_tree_view_append_column(GTK_TREE_VIEW(app.list_view), col);

gtk_container_add(GTK_CONTAINER(scroll), app.list_view);
gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);

create_form_window(&app);
refresh_list(&app);

gtk_widget_show_all(app.window);
gtk_main();

return 0;
}
