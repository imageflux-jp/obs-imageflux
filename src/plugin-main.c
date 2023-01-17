#include <obs-defs.h>
#include <obs-internal.h>
#include <obs-module.h>
#include <util/platform.h>
#include <util/dstr.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "ImageFlux Live Streaming";
}

#define warn(format, ...) blog(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) blog(LOG_INFO, format, ##__VA_ARGS__)

extern struct obs_output_info imageflux_output_info;
extern struct obs_encoder_info imageflux_encoder_info;

char *rtmp_services_file();
char *rtmp_services_file2();
char *rtmp_services_dir();

char *rtmp_services_file()
{
	{
		char *config_path = obs_module_config_path("services.json");
		if (config_path) {
			struct dstr output = {0};
			dstr_copy(&output, config_path);
			dstr_replace(&output, "/obs-imageflux/",
				     "/rtmp-services/");
			info("config_path=%s", output.array);
			if (os_file_exists(output.array)) {
				return output.array;
			}
			bfree(output.array);
		}
	}
	{
		obs_module_t *module = obs_current_module();
		struct dstr output = {0};
		dstr_copy(&output, module->data_path);
		if (!dstr_is_empty(&output) && dstr_end(&output) != '/')
			dstr_cat_ch(&output, '/');
		dstr_replace(&output, "/obs-imageflux/", "/rtmp-services/");
		dstr_cat(&output, "services.json");
		info("config_path=%s", output.array);
		if (os_file_exists(output.array)) {
			return output.array;
		}
		bfree(output.array);
	}
	return NULL;
}

char *rtmp_services_file2()
{
	{
		char *config_path = obs_module_config_path("services.json");
		info("config_path=%s", config_path);
		if (config_path) {
			struct dstr output = {0};
			dstr_copy(&output, config_path);
			dstr_replace(&output, "/obs-imageflux/",
				     "/rtmp-services/");
			return output.array;
		}
	}
	return NULL;
}

char *rtmp_services_dir()
{
	{
		char *config_path = obs_module_config_path("");
		info("config_path=%s", config_path);
		if (config_path) {
			struct dstr output = {0};
			dstr_copy(&output, config_path);
			dstr_replace(&output, "/obs-imageflux/",
				     "/rtmp-services/");
			return output.array;
		}
	}
	return NULL;
}

extern bool insert_imageflux_service(void);

bool obs_module_load(void)
{
	obs_register_output(&imageflux_output_info);
	obs_register_encoder(&imageflux_encoder_info);

	insert_imageflux_service();

//	pthread_t thread;
//	pthread_create(&thread, NULL, insert_thread, NULL);

	// DWORD   dwThreadId = 0;
	// HANDLE  hThread = 0;
	// hThread = CreateThread( 
	// 	NULL,                   // default security attributes
	// 	0,                      // use default stack size  
	// 	MyThreadFunction,       // thread function name
	// 	NULL,          // argument to thread function 
	// 	0,                      // use default creation flags 
	// 	&dwThreadId);   // returns the thread identifier 

	return true;
}
