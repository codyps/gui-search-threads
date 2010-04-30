/*
Cody Schafer and Brian Goodacre
Systems Programming
search-GUI
*/
#define _GNU_SOURCE
#include <stdbool.h>
#include <string.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>

#include "search_types.h"
#include "defaults.h"
#include "prefM.h"
#include "global.h"
#include "save.h"


int run_script(const char *program_name, const char *rel_script, const char *path,
	const char *other, FILE *fps[2])
{
	int new_in[2], new_out[2];
	int ret = pipe(new_in);
	if (ret != 0)
		return ret;
	ret = pipe(new_out);
	if (ret != 0)
		return ret;

	if (!fork()) {
		char *t_program_name = g_strdup_printf("%s",program_name);
		char *prog_path = dirname(t_program_name);

		char *script = g_strdup_printf("%s/%s",prog_path,rel_script);

		char *t_path = g_strdup_printf("%s",path);
		char *t_other = g_strdup_printf("%s",other);

		char *argv[]={script, t_path, t_other, NULL};

		dup2(new_out[1],STDOUT_FILENO);
		dup2(new_out[1],STDERR_FILENO);

		dup2(new_in[0],STDIN_FILENO);

		close(new_out[0]);
		close(new_in[0]);
		close(new_in[1]);
		close(new_out[1]);
		int ret = execv(argv[0],argv);
		_exit(ret);
	} else {
		close(new_out[1]);
		close(new_in[0]);

		// read from new_out[0]
		// write to new_in[1]

		fps[1] = fdopen(new_in[1],"w");
		fps[0] = fdopen(new_out[0],"r");
		return 0;
	}
}

void exec_refresh_index(const char *progname, const char *rel_script, const char *path) {
	FILE *fps[2];

	run_script(progname,rel_script,path,"2",fps);

	fprintf(fps[1],"q\n");

	char *line = 0; 
	size_t line_mem = 0;
	ssize_t line_len = getdelim(&line,&line_mem,'\0',fps[0]);

	if (line_len > 0) {
		gtk_text_buffer_set_text(globals.output_text,line,line_len);
	}

	printf("got(%d) \n",line_len);
	fclose(fps[1]);
	fclose(fps[0]);
	free(line);

}

void do_search(const char *search_str) {
	const char *progname = globals.progname;
	const char *rel_script = script_loc;
	const char *path = globals.search_path;

	char *other = (globals.auto_index)?"1":"0";
	char *mode_str = (globals.search_type == S_SA)?"sa":"so";

	char *t_program_name = g_strdup_printf("%s",progname);
	char *prog_path = dirname(t_program_name);

	char *t_path = g_strdup_printf("%s",path);
	char *t_other = g_strdup_printf("%s",other);

	char *script = g_strdup_printf("%s/%s \"%s\" %s > .tmp_stuff",prog_path,rel_script,t_path,t_other);


	FILE *in = popen(script,"w");

	fprintf(in,"%s %s\n",mode_str,search_str);
	fflush(in);
	
	pclose(in);

	gsize len = 0;
	gchar *contents = 0;
	bool junk =  g_file_get_contents(".tmp_stuff",
                             &contents,
                             &len,
                             0);
	if(junk) {
		gtk_text_buffer_set_text(globals.output_text,contents,len);
	}
}

/*
void do_search( const char *search_str) {
	const char *progname = globals.progname;
	const char *rel_script = script_loc;
	const char *path = globals.search_path;

	char *other = (globals.auto_index)?"1":"0";
	char *mode_str = (globals.search_type == S_SA)?"sa":"so";

	char *t_program_name = g_strdup_printf("%s",progname);
	char *prog_path = dirname(t_program_name);

	char *script = g_strdup_printf("%s/%s",prog_path,rel_script);

	char *t_path = g_strdup_printf("%s",path);
	char *t_other = g_strdup_printf("%s",other);

	char *argv[]={script, t_path, t_other, NULL};

	GPid pid; 
	gint in, out, err; 
	g_spawn_async_with_pipes( NULL, argv, 0, 0, 0, 0, &pid, &in, &out, &err,  NULL );

	FILE *fin = fdopen(in,"w");
	FILE *fout = fdopen(out,"r");	

	char *line = 0; 
	size_t line_mem = 0;
	ssize_t line_len;


	fprintf(fin,"%s %s\n",mode_str,search_str);
	fflush(fin);

	line_len = getdelim(&line,&line_mem,'\n',fout);

	printf("GOT %d %s",line_len, line);

	fprintf(fin,"q\n");
	fflush(fin);
	if (line_len > 0) {
		gtk_text_buffer_set_text(globals.output_text,line,line_len);
	}

}
*/

/*
void do_search( const char *search_str) {
	FILE *fps[2];

	const char *progname = globals.progname;
	const char *rel_script = script_loc;
	const char *path = globals.search_path;

	char *thing = (globals.auto_index)?"1":"0";
	char *mode_str = (globals.search_type == S_SA)?"sa":"so";


	run_script(progname,rel_script,path,thing,fps);

	fprintf(fps[1],"%s %s\n",mode_str,search_str);
	fflush(fps[1]);

	char *line = 0; 
	size_t line_mem = 0;
	ssize_t line_len = getdelim(&line,&line_mem,'\n',fps[0]);

	fprintf(fps[1],"q\n");
	fflush(fps[1]);

	if (line_len > 0) {
		gtk_text_buffer_set_text(globals.output_text,line,line_len);
	}

	printf("got(%d) \n",line_len);
	fclose(fps[1]);
	fclose(fps[0]);
	free(line);


}
*/


static gboolean delete_event( GtkWidget *widget,
		GdkEvent *event, gpointer data)
{
	return FALSE;
}

static void destroy( GtkWidget *widget, gpointer data )
{
	// free globals
	g_free(globals.search_path);
	gtk_main_quit();
}

/* Handle Textbox entry event */
void eb_entry(GtkWidget *widget, gpointer data)
{
	g_print("Process inputed search string\n");
	//const gchar *text =  gtk_entry_get_text(GTK_ENTRY(widget));

	do_search(gtk_entry_get_text(GTK_ENTRY(widget)));	

}


/* Handle Quit button click */
void quit_click(GtkWidget *widget, gpointer data)
{
	g_free(globals.search_path);
	gtk_main_quit();
}

/* Handle Reindex button click */
void reindex_click(GtkWidget *widget, gpointer data)
{
	g_print("Do reindex\n");
	exec_refresh_index(globals.progname, script_loc,globals.search_path);
}

/* Handle Pref button click */
void pref_click(GtkWidget *widget, gpointer data)
{
	show_pref_dialog(data);
}

/* Handle Help "Content" click */
void help_click(GtkWidget *widget, gpointer data)
{
	GtkWidget *help_dialog = gtk_dialog_new_with_buttons("Help Content",
		NULL, 0, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL );

	GtkWidget *vbox = gtk_dialog_get_content_area(GTK_DIALOG(help_dialog));

	gchar *content_text = 
		"*Main Screen\n"
		"\tSearch Query Field Box for search query\n"
		"\tIndex output will go here\n"
		"*Click File->\n"
		"\t-Preferences to setup your search path\n"
		"\t-Preferences also for auto indexing\n"
		"\t-Reindex to update the index file\n"
		"\t-Quit to leave and go fish\n"
		"*Click Help->\n"
		"\t-Content is where you are\n"
		"\t-About for information about this gui-search\n"
		"\n";


	GtkWidget *label = gtk_label_new(content_text);

	gtk_box_pack_start(GTK_BOX(vbox),label,TRUE, TRUE, 10);

	gtk_widget_show(label);

	gtk_window_set_transient_for(GTK_WINDOW(help_dialog),GTK_WINDOW(data));
	

	gint ret = gtk_dialog_run(GTK_DIALOG(help_dialog));

	g_print("%s: got %d\n",__func__,ret);

	gtk_widget_destroy(help_dialog);
}

/* handle help "about" click */
void about_click(GtkWidget *widget, gpointer data) {
	GtkWidget *about_dialog = gtk_about_dialog_new();
	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about_dialog),
		"Search GUI");
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about_dialog),
		"v1.2");
	const gchar * authors[] = { "Cody Schafer", "Brian Goodacre", 0 };

	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about_dialog),
		authors);

	gtk_window_set_transient_for(GTK_WINDOW(about_dialog),GTK_WINDOW(data));

	gint ret = gtk_dialog_run(GTK_DIALOG(about_dialog));


	g_print("%s: got ret %d\n",__func__,ret);


	gtk_widget_destroy(about_dialog);
}


GtkWidget *mk_main_window(void)
{
	/* { window */
	GtkWidget *main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(main_window),250,200);
	/*    { vbox */
	GtkWidget *vbox = gtk_vbox_new( FALSE, 0 );

	/*       { menu bar */
	GtkWidget *menu_bar = gtk_menu_bar_new();
	
	/*          { file menu */
	GtkWidget *mi_file  = gtk_menu_item_new_with_label("File");
	GtkWidget *sm_file = gtk_menu_new();

	GtkWidget *mi_s_pref = gtk_menu_item_new_with_label("Preferences");

	g_signal_connect(mi_s_pref, "activate",
		G_CALLBACK(pref_click), main_window);
	gtk_menu_append(sm_file,mi_s_pref);
	gtk_widget_show(mi_s_pref);

	GtkWidget *mi_s_reindex = gtk_menu_item_new_with_label("Reindex");
	g_signal_connect(mi_s_reindex, "activate",
		G_CALLBACK(reindex_click), NULL);
	gtk_menu_append(sm_file,mi_s_reindex);
	gtk_widget_show(mi_s_reindex);

	GtkWidget *mi_s_quit = gtk_menu_item_new_with_label("Quit");
	g_signal_connect(mi_s_quit, "activate",
		G_CALLBACK(quit_click), NULL);
	gtk_menu_append(sm_file,mi_s_quit);
	gtk_widget_show(mi_s_quit);

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi_file),sm_file);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar),mi_file);
	gtk_widget_show(mi_file);

	/*          } */
	/*          { help menu */
	GtkWidget *mi_help = gtk_menu_item_new_with_label("Help");
	GtkWidget *sm_help = gtk_menu_new();

	/*             { content item */	
	GtkWidget *mi_s_content = gtk_menu_item_new_with_label("Content");
	g_signal_connect(mi_s_content, "activate",
		G_CALLBACK(help_click), main_window);
	gtk_menu_append(sm_help,mi_s_content);
	gtk_widget_show(mi_s_content);
	/*             } (content item) */
	/*             { about item */

	GtkWidget *mi_s_about   = gtk_menu_item_new_with_label("About");

	g_signal_connect(mi_s_about, "activate",
		G_CALLBACK(about_click), main_window);

	gtk_menu_append(sm_help,mi_s_about);
	gtk_widget_show(mi_s_about);

	/*             } (about item) */

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi_help),sm_help);
	
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar),mi_help);
	gtk_widget_show(mi_help);

	/*          } (help menu) */
	gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 2);
	gtk_widget_show(menu_bar);

	/*       } (menu bar) */
	/*       { text entry box */
	const gchar *eb_init_str = "enter search query here";

	GtkWidget *entry_box = gtk_entry_new();

	gtk_entry_set_text(GTK_ENTRY(entry_box),eb_init_str);

	g_signal_connect(entry_box, "activate",
		G_CALLBACK(eb_entry), main_window);

	gtk_box_pack_start(GTK_BOX(vbox), entry_box, FALSE, FALSE, 2);
	gtk_widget_show(entry_box);

	


	
	
	
	/*       } */
	/*       { output box */
	GtkWidget *scrollywin;
	scrollywin = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(
		GTK_SCROLLED_WINDOW(scrollywin),
		GTK_POLICY_AUTOMATIC,
		GTK_POLICY_AUTOMATIC
		);
	gtk_box_pack_start(GTK_BOX(vbox),scrollywin, TRUE, TRUE, 0);
	
	GtkWidget *text_output;
	GtkTextBuffer *buffer_output;
	text_output = gtk_text_view_new();
	buffer_output = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_output));
	gtk_text_buffer_set_text (buffer_output, "output will go here", -1);
	
	globals.output_text = buffer_output;
	gtk_container_add(GTK_CONTAINER(scrollywin),text_output);
	
	gtk_widget_show(scrollywin);
	gtk_widget_show(text_output);
	
	/*       } */
	
	gtk_container_add(GTK_CONTAINER(main_window),vbox);
	gtk_widget_show(vbox);

	/*    } (vbox) */
	g_signal_connect(main_window, "destroy",
		G_CALLBACK(destroy), NULL);
	g_signal_connect(main_window, "delete-event",
		G_CALLBACK(delete_event), NULL);


	/* } (window) */

	return main_window;	
}



int main(int argc, char **argv)
{
	gtk_init(&argc,&argv);
	
	/*initialized*/
	globals.search_path=NULL;  //empty.
	globals.auto_index=0;  //no auto index
	globals.search_type = S_SO;//or
	globals.progname = argv[0];
	
	read_prefs();	
	
	GtkWidget *main_window = mk_main_window();
	gtk_widget_show(main_window);

	save_prefs();

	gtk_main();

	return 0;
}
