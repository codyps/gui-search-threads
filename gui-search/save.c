
#include <gtk/gtk.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"

#include "save.h"
#include "defaults.h"

bool read_prefs(void)
{
	GKeyFile *gkf = g_key_file_new();
	//GError *e;

	if (!g_key_file_load_from_file(gkf,config_path,0,NULL)) {
		g_print("%s: error loading file\n",__func__);

		globals.auto_index = 0;
		globals.search_path = 
			g_strdup_printf("%s",default_path);
		g_print("using default path %s\n",globals.search_path);

		return false;
	}

	gchar *group_s = g_key_file_get_start_group(gkf);
	if (!group_s) {
		return false;
	}

	gchar *k_index = g_key_file_get_value(gkf,group_s,config_key_auto_index, NULL);
	gchar *k_path  = g_key_file_get_value(gkf,group_s,config_key_path,NULL);

	if (k_index) {
		int d;
		int ret = sscanf(k_index,"%d",&d);
		if (ret == 1) {
			globals.auto_index = d;
		} else {
			globals.auto_index = 0;
		}
		g_free(k_index);
	} else {

		globals.auto_index = 0;
	}
	
	if(k_path) {
		globals.search_path = k_path;
		g_print("got path from config %s\n",globals.search_path);
	} else {
		globals.search_path = 
			g_strdup_printf("%s",default_path);
		g_print("using default path %s\n",globals.search_path);
	}
	return true;
}

bool save_prefs(void)
{
	GKeyFile *gkf = g_key_file_new();

	g_key_file_set_value(
		gkf,
		config_group,
		config_key_path,
		globals.search_path);

	char *auto_index_val =
		g_strdup_printf("%d",globals.auto_index);
	g_key_file_set_value(
		gkf,
		config_group, 
		config_key_auto_index, 
		auto_index_val);
	g_free(auto_index_val);	


	gsize len;
	gchar *filedata = g_key_file_to_data(gkf,&len,NULL);

	if (!filedata) {
		g_print("error saving preferences\n");
		return false;
	}

	FILE *fp = fopen(config_path,"w");
	size_t wrote_len = fwrite(filedata,1,len,fp);
	if (wrote_len != len) {
		g_print("blast. writing failed\n");
		return false;
	}

	fclose(fp);
	return true;
}
