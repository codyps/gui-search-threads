#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <gtk/gtk.h>
#include <sys/types.h>
#include <stdbool.h>
#include <dirent.h>
#include <errno.h>
#include "search_types.h"



struct global_data {
	enum search_method search_type;
	gchar *search_path;
	bool auto_index;
	char *progname;
	GtkTextBuffer *output_text;
	int thread_count;
};

extern struct global_data globals;


/*
extern enum search_method s_method;
extern gchar *searchPath;
extern int autoIndexer;
*/


#endif
