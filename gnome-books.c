#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
// #include <JavaScriptCore/JavaScript.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

GtkWidget *mainWindow;
WebKitWebView *webView;

static void destroyWindowCb(GtkWidget* widget, GtkWidget* window)
{
	gtk_main_quit();
}

static void closeWebViewCb(WebKitWebView* webView, GtkWidget* window)
{
	printf("Bye bye\n");
	gtk_widget_destroy(window);
}

/*
gboolean
webkit_error_loading_uri (WebKitWebView  *web_view,
               WebKitWebFrame *web_frame,
               gchar          *uri,
               GError         *web_error,
               gpointer        user_data)
{
	printf("Could not load URI '%s': %s", uri, web_error->message);
	return TRUE;
}
*/
void
webkit_load_changed (WebKitWebView  *web_view,
                     WebKitLoadEvent load_event,
                     gpointer        user_data)
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
        break;
    }
}

gboolean
webkit_load_failed(WebKitWebView  *web_view,
                   WebKitLoadEvent load_event,
                   gchar          *failing_uri,
                   gpointer        error,
                   gpointer        user_data)
{
	printf("Failed to load %s!\n", failing_uri);

	return TRUE;
}

gboolean
webkit_process_crashed (WebKitWebView *web_view,
               			gpointer       user_data)
{
	printf("WebKit Crashed!\n");

	return TRUE;
}

void
webkit_insecure_content_detected (WebKitWebView             *web_view,
               					  WebKitInsecureContentEvent event,
               					  gpointer                   user_data)
{
	printf("webkit_insecure_content_detected\n");
}               

static void
request_cb (WebKitURISchemeRequest *request, gpointer data)
{
    const gchar *path = webkit_uri_scheme_request_get_path (request);
    GFile *file;

    printf("Path: %s \n", path);
    if (!path || path[0] == '\0') {
        file = g_file_new_for_path ("resource:///gnome-books/epub.js/examples/single.html");
        //file = g_file_new_for_uri ("resource:////org/gnome-books/epub/examples/single.HTMLl");
    } else {
        gchar *dir = g_get_current_dir ();
        gchar *fn = g_build_filename (dir, path, NULL);
        file = g_file_new_for_path (fn);
        g_free (dir);
        g_free (fn);
    }

    GError *error = NULL;
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

    webkit_web_context_register_uri_scheme (context, "sneaky", request_cb, NULL, NULL);
    webkit_security_manager_register_uri_scheme_as_cors_enabled (security, "sneaky");
}

static gboolean
load (gpointer pointer)
{   
    webkit_web_view_load_uri (WEBKIT_WEB_VIEW (webView), "sneaky:");

    return FALSE;
}

static void
create ()
{
	mainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(mainWindow), 1340, 768);
	gtk_window_set_position(GTK_WINDOW(mainWindow), GTK_WIN_POS_CENTER_ALWAYS);
    gtk_window_set_title (GTK_WINDOW (mainWindow), "GNOME Books");

	webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    gtk_container_add (GTK_CONTAINER (mainWindow), GTK_WIDGET(webView));

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

	g_signal_connect(mainWindow, "destroy", G_CALLBACK(destroyWindowCb), NULL);
    
	g_signal_connect(webView, "close", G_CALLBACK(closeWebViewCb), mainWindow);
	g_signal_connect(webView, "load-changed", G_CALLBACK(webkit_load_changed), NULL);
	g_signal_connect(webView, "load-failed", G_CALLBACK(webkit_load_failed), NULL);
	g_signal_connect(webView, "web-process-crashed", G_CALLBACK(webkit_process_crashed), NULL);
	g_signal_connect(webView, "insecure-content-detected", G_CALLBACK(webkit_insecure_content_detected), NULL);
    
	// Load the HTML content
	//webkit_web_view_load_uri(webView, "http://localhost:8080/examples/hypothesis.html");

	//gtk_widget_grab_focus(GTK_WIDGET(webView));
	gtk_widget_show_all(mainWindow);
}

int
main(int argc, char **argv)
{
    gtk_init(&argc, &argv);

    create();
    register_uri();

    g_idle_add(load, NULL);

    gtk_main();

    return 0;
}