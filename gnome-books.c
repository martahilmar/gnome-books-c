#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
// #include <JavaScriptCore/JavaScript.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include "extensions.h"

GtkWidget *mainWindow;
WebKitWebView *webView;

static void load_book ();
static void render_book ();
static void 
previous_buttonCb ( GtkWidget *widget,
                    gpointer data );
static void 
next_buttonCb ( GtkWidget *widget,
                gpointer data );
static void generate_pagination ();

static gboolean
fill (gpointer user_data);

static void
web_view_javascript_finished (GObject      *object,
                              GAsyncResult *result,
                              gpointer      user_data);

static void 
destroy_windowCb( GtkWidget* widget, 
                 GtkWidget* window )
{
	gtk_main_quit();
}

static void 
close_WebViewCb( WebKitWebView* webView, 
                GtkWidget* window )
{
	printf("Bye bye\n");
	gtk_widget_destroy(window);
}

void
webkit_load_changed ( WebKitWebView  *web_view,
                      WebKitLoadEvent load_event,
                      gpointer        user_data )
{
    const gchar *uri = webkit_web_view_get_uri (web_view);

    switch (load_event) {
    case WEBKIT_LOAD_STARTED:
        /* New load, we have now a provisional URI */
        printf("webkit_load_started: '%s' \n", uri);
        /* Here we could start a spinner or update the
         * location bar with the provisional URI */
        break;
    case WEBKIT_LOAD_REDIRECTED:
        printf("webkit_load_redirected: '%s' \n", uri);
        break;
    case WEBKIT_LOAD_COMMITTED:
        /* The load is being performed. Current URI is
         * the final one and it won't change unless a new
         * load is requested or a navigation within the
         * same page is performed */
        printf("webkit_load_commited: '%s' \n", uri);
        break;
    case WEBKIT_LOAD_FINISHED:
        /* Load finished, we can now stop the spinner */
    	printf("webkit_load_finished: '%s' \n", uri);
        load_book();
        render_book();
        generate_pagination();
        break;
    }
}

gboolean
webkit_load_failed ( WebKitWebView  *web_view,
                     WebKitLoadEvent load_event,
                     gchar          *failing_uri,
                     gpointer        error,
                     gpointer        user_data )
{
	printf("Failed to load %s!\n", failing_uri);

	return TRUE;
}

gboolean
webkit_process_crashed ( WebKitWebView *web_view,
               			 gpointer       user_data )
{
	printf("WebKit Crashed!\n");

	return TRUE;
}

void
webkit_insecure_content_detected ( WebKitWebView             *web_view,
               					   WebKitInsecureContentEvent event,
               					   gpointer                   user_data )
{
	printf("webkit_insecure_content_detected\n");
}               

static void
request_cb ( WebKitURISchemeRequest *request, 
             gpointer data )
{
    const gchar *path = webkit_uri_scheme_request_get_path (request);
    GFile *file = NULL;
    GError *error = NULL;

    //printf("Path: %s \n", path);

    if (!path || path[0] == '\0') {
        file = g_file_new_for_path ("epub.js/examples/single.html");
        //file = g_file_new_for_uri ("resources:///gnome-books/epub/examples/single.html");
    } else {
        gchar *dir = g_get_current_dir ();
        gchar *fn = g_build_filename (dir, path, NULL);
        file = g_file_new_for_path (fn);
        g_free (dir);
        g_free (fn);
    }

    GFileInputStream *strm = g_file_read (file, NULL, &error);
    if (error) {
        webkit_uri_scheme_request_finish_error (request, error);
        return;
    }

    webkit_uri_scheme_request_finish (request, G_INPUT_STREAM (strm), -1, "text/html");
    g_object_unref (strm);
    g_object_unref (file);
}

static void
register_uri ()
{
    WebKitWebContext *context = webkit_web_context_get_default ();
    WebKitSecurityManager *security = webkit_web_context_get_security_manager (context);

    webkit_web_context_register_uri_scheme (context, "book", request_cb, NULL, NULL);
    webkit_security_manager_register_uri_scheme_as_cors_enabled (security, "book");
}

static gboolean
load (gpointer pointer)
{   
    webkit_web_view_load_uri (WEBKIT_WEB_VIEW (webView), "book:");

    return FALSE;
}

static void
load_book ()
{
    gchar *load_command = g_strdup_printf ("var Book = ePub('/epub.js/reader/moby-dick/', { width: 1076, height: 588 });");
    webkit_web_view_run_javascript (webView, load_command, NULL, NULL, NULL);
    g_free(load_command);
}

static void
render_book ()
{
    gchar *render_command = g_strdup_printf ("var rendered = Book.renderTo('area').then(function(){});");
    webkit_web_view_run_javascript (webView, render_command, NULL, NULL, NULL);
    g_free(render_command);
}

static void 
previous_buttonCb ( GtkWidget *widget,
                    gpointer   data )
{ 
    gchar *previous_command = g_strdup_printf ("Book.prevPage();");
    webkit_web_view_run_javascript (webView, previous_command, NULL, NULL, NULL);
    g_free(previous_command);
}

static void 
next_buttonCb ( GtkWidget *widget,
                gpointer  data )
{ 
    gchar *next_command = g_strdup_printf ("Book.nextPage();");
    webkit_web_view_run_javascript (webView, next_command, NULL, NULL, NULL);
    g_free(next_command);
}


static void
generate_pagination ()
{
    gchar               *pagination_command;
    GError              *error   = NULL;
    GFile               *file    = NULL;
    GFileInputStream    *strm;
    GFileInfo           *info;
    gchar               *buffer;
    int                 total_size = -1;
    size_t              length;

    file = g_file_new_for_path ("pagination.js");
    strm = g_file_read (file, NULL, &error);

    info = g_file_input_stream_query_info (G_FILE_INPUT_STREAM (strm), G_FILE_ATTRIBUTE_STANDARD_SIZE, NULL, &error);
    if (info)
    {
        if (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_STANDARD_SIZE))
            total_size = g_file_info_get_size (info);
        g_object_unref (info);
    }

    if(total_size > 0) 
    {
        buffer = (char *) malloc(sizeof(char) * total_size);
        memset(buffer, 0, total_size);
        g_input_stream_read (G_INPUT_STREAM(strm), buffer, total_size-1, NULL, &error);
    }
    
    if (error) {
        return;
    }

    pagination_command = buffer;
    webkit_web_view_run_javascript (webView, pagination_command, NULL, NULL, NULL);
    //webkit_web_view_run_javascript (webView, pagination_command, NULL, web_view_javascript_finished, NULL);

    g_input_stream_close(G_INPUT_STREAM(strm), NULL, &error);
    g_object_unref (strm);    
    g_object_unref (file);
    g_free (pagination_command);
}

static void
web_view_javascript_finished (GObject      *object,
                              GAsyncResult *result,
                              gpointer      user_data)
{
    WebKitJavascriptResult *js_result;
    JSValueRef              value;
    JSGlobalContextRef      context;
    GError                 *error = NULL;

    js_result = webkit_web_view_run_javascript_finish (WEBKIT_WEB_VIEW (object), result, &error);
    if (!js_result) {
        g_warning ("Error running javascript: %s", error->message);
        g_error_free (error);
        return;
    }

    context = webkit_javascript_result_get_global_context (js_result);
    value = webkit_javascript_result_get_value (js_result);
    if (JSValueIsString (context, value)) {
        JSStringRef js_str_value;
        gchar      *str_value;
        gsize       str_length;

        js_str_value = JSValueToStringCopy (context, value, NULL);
        str_length = JSStringGetMaximumUTF8CStringSize (js_str_value);
        str_value = (gchar *)g_malloc (str_length);
        JSStringGetUTF8CString (js_str_value, str_value, str_length);
        JSStringRelease (js_str_value);
        g_print ("------ Script result: %s\n", str_value);
        g_free (str_value);
    } else {
        g_warning ("Error running javascript: unexpected return value");
    }
    webkit_javascript_result_unref (js_result);
}

static gboolean
fill (gpointer  user_data)
{
    GtkWidget *progress_bar = user_data;

    // Get the current progress
    gdouble fraction;
    fraction = gtk_progress_bar_get_fraction (GTK_PROGRESS_BAR (progress_bar));

    // Increase the bar by 10% each time this function is called
    // fraction += 0.1;

    // Fill in the bar with the new fraction
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress_bar), fraction);

    // Ensures that the fraction stays below 1.0
    if (fraction < 1.0) 
        return TRUE;

    return FALSE;
}

static void
create_GUI ()
{
    mainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(mainWindow), 1340, 768);
    gtk_window_set_position(GTK_WINDOW(mainWindow), GTK_WIN_POS_CENTER_ALWAYS);
    gtk_window_set_title (GTK_WINDOW (mainWindow), "GNOME Books");

    GtkWidget   *grid;
    GtkWidget   *vbox, *hbox, *box;
    GtkWidget   *prev_button;
    GtkWidget   *next_button;
    GtkWidget   *progress_bar;
    gdouble     fraction = 0.0;

    grid = gtk_grid_new ();

    prev_button = gtk_button_new_with_label ("<");
    g_signal_connect (prev_button, "clicked", G_CALLBACK (previous_buttonCb), NULL);

    next_button = gtk_button_new_with_label (">");
    g_signal_connect (next_button, "clicked", G_CALLBACK (next_buttonCb), NULL);
    
    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);

    webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    progress_bar = gtk_progress_bar_new ();

    gtk_box_pack_start (GTK_BOX (hbox), prev_button, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET(webView), TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), next_button, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), progress_bar, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (box), hbox, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (box), vbox, FALSE, FALSE, 0);
    
    
    gtk_container_add (GTK_CONTAINER (mainWindow), GTK_WIDGET (box));
    /*
    gtk_grid_attach (GTK_GRID (grid), GTK_WIDGET (webView), 1, 1, 1, 1);
    gtk_grid_attach_next_to (GTK_GRID (grid),
                           GTK_WIDGET (webView),
                           prev_button,
                           GTK_POS_BOTTOM, 1, 1);
    */
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress_bar), fraction);
    //g_timeout_add (500, fill, GTK_PROGRESS_BAR (progress_bar));

    WebKitSettings *s = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(webView));
    
    webkit_settings_set_enable_write_console_messages_to_stdout(s, TRUE);
    webkit_settings_set_enable_fullscreen(s, TRUE);

    g_object_set(G_OBJECT(s),"enable-fullscreen", TRUE,
                             "enable-javascript", TRUE, 
                             "enable-developer-extras", TRUE,
                             "enable-xss-auditor", FALSE,
                             "enable-plugins", FALSE,
                             "enable-write-console-messages-to-stdout", TRUE, NULL);
    // g_object_set(G_OBJECT(s),"auto-load-images", FALSE, NULL); // We only load the current pages +-1

    webkit_web_view_set_settings(WEBKIT_WEB_VIEW(webView), s);

    g_signal_connect(mainWindow, "destroy", G_CALLBACK(destroy_windowCb), NULL);
    
    g_signal_connect(webView, "close", G_CALLBACK(close_WebViewCb), mainWindow);
    g_signal_connect(webView, "load-changed", G_CALLBACK(webkit_load_changed), NULL);
    g_signal_connect(webView, "load-failed", G_CALLBACK(webkit_load_failed), NULL);
    g_signal_connect(webView, "web-process-crashed", G_CALLBACK(webkit_process_crashed), NULL);
    g_signal_connect(webView, "insecure-content-detected", G_CALLBACK(webkit_insecure_content_detected), NULL);

    // Make sure that when the browser area becomes visible, it will get mouse
    // and keyboard events
    gtk_widget_grab_focus(GTK_WIDGET(webView));
    gtk_widget_show_all(mainWindow);
}

int
main(int argc, char **argv)
{
    gtk_init(&argc, &argv);

    create_GUI();
    register_uri();

    g_idle_add(load, NULL);

    gtk_main();

    return 0;
}