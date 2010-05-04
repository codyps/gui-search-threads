#include <gtk/gtk.h>
#include <stdlib.h>
#include "global.h"
#include "save.h"


/*Making the Preferences Menu*/
void show_pref_dialog(GtkWidget *parent)
{
	
	GtkWidget *pref_dia = 
		gtk_dialog_new_with_buttons("Preferences", 
				    GTK_WINDOW(parent), 
				    0, 
				    GTK_STOCK_OK,
				    GTK_RESPONSE_ACCEPT,
				    GTK_STOCK_CANCEL,
				    GTK_RESPONSE_REJECT,
				    NULL);
	    
	GtkWidget *pref_dia_vbox =
		gtk_dialog_get_content_area(GTK_DIALOG(pref_dia));

	//Thread entry text box
	GtkWidget *thread_box = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(thread_box),"%s",globals.thread_count);
	gtk_box_pack_start(
		GTK_BOX(pref_dia_vbox), thread_box, FALSE, FALSE, 2);
	gtk_widget_show(thread_box);
	
			    
	//text entry box for the preferences Dialog
	GtkWidget *path_box = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(path_box),globals.search_path);
	gtk_box_pack_start(
		GTK_BOX(pref_dia_vbox), path_box, FALSE, FALSE, 2);
	gtk_widget_show(path_box);
	
	/* Check button inside vbox*/
	GtkWidget *auto_index_button =
		gtk_check_button_new_with_label("Auto Detect Changes");
	gtk_box_pack_start(
		GTK_BOX(pref_dia_vbox),auto_index_button, FALSE, FALSE, 10);

	gtk_widget_show(auto_index_button);

	gint ret;

PREF_AGAIN:	
	ret = gtk_dialog_run(GTK_DIALOG(pref_dia));
	
	g_print("%s: ret %d\n",__func__,ret);
	if( ret == GTK_RESPONSE_ACCEPT){
		/*first check it if is valid*/
		/*If invalid, ERROR dialog*/
	
		const gchar *new_path = gtk_entry_get_text(GTK_ENTRY(path_box));
		int thread_num = atoi(gtk_entry_get_text(GTK_ENTRY(thread_box)));
		printf("%d\n", thread_num);
		
		if ( !g_file_test(new_path,G_FILE_TEST_IS_DIR) ) {
			GtkWidget *error_dia=gtk_message_dialog_new(
					GTK_WINDOW(pref_dia),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK,
					"Invalid Directory Selected"
					);
			gtk_dialog_run(GTK_DIALOG(error_dia));
			gtk_widget_destroy(error_dia);
			goto PREF_AGAIN;
		}
		else if(thread_num < 1)
		{
			GtkWidget *error_dia=gtk_message_dialog_new(
					GTK_WINDOW(pref_dia),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK,
					"Invalid Thread Number"
					);
			gtk_dialog_run(GTK_DIALOG(error_dia));
			gtk_widget_destroy(error_dia);
			goto PREF_AGAIN;
		}
		
		/*valid path*/
		g_free(globals.search_path);
		globals.search_path = g_strdup_printf("%s", new_path);
		globals.thread_count = thread_num;
		
		g_print("New: %s\n",globals.search_path);

		globals.auto_index = gtk_toggle_button_get_active(
				GTK_TOGGLE_BUTTON(auto_index_button));
		
		save_prefs();
	}
	gtk_widget_destroy(pref_dia);	 
}
