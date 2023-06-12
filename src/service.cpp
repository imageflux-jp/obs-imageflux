#include <obs-module.h>
#include <util/platform.h>
#include <util/dstr.h>
#include <string>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/json.hpp>

using namespace boost::json;

#define warn(format, ...) blog(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) blog(LOG_INFO, format, ##__VA_ARGS__)

extern "C" char *rtmp_services_file();
extern "C" char *rtmp_services_dir();
extern "C" char *rtmp_services_file2();

/*
rtmp-services/services.jsonの先頭にImageFluxサービスを追加する
*/
extern "C" bool insert_imageflux_service(void)
{
    value root;
    {
        char *file = rtmp_services_file();
        info("file=%s", file);
	if (!file) {
		return false;
	}
	char *file_data = os_quick_read_utf8_file(file);
	bfree(file);
	if (!file_data) {
		return false;
	}
	error_code ec;
	root = parse(file_data, ec);
	bfree(file_data);
	if (ec) {
		info("json parse error");
		return false;
	}
    }

    auto format_ver = root.at("format_version").as_int64();
    if(!format_ver){
        return false;
    }
    info("format_ver=%lld", format_ver);
    bool registered = false;
    auto services = root.at("services").as_array();
    for (size_t index = 0; index < services.size(); index++) {
	    auto service = services.at(index).as_object();
	    auto cur_name = service.at("name").as_string();
	    if (cur_name == "ImageFlux Live Streaming") {
		    registered = true;
		    break;
	    }
    }


    if(!registered){
	auto service = parse(
R"({
"name": "ImageFlux Live Streaming",
"common": true,
"more_info_link": "https://console.imageflux.jp/docs/",
"protocol": "imageflux_output",
"servers" : [{
    "name": "出力(詳細)ページで指定",
    "url": "imageflux_output://imageflux/signaling"
}],
"recommended": {
    "output": "imageflux_output"
},
"supported video codecs":["h264"]
})"
        );
	root.at("services").as_array().emplace(
		root.at("services").as_array().begin(),
		service.as_object());

	char *dir = rtmp_services_dir();
	char *file2 = rtmp_services_file2();
	info("file=%s", file2);
	os_mkdirs(dir);
	std::string s = serialize(root);
	info("json=%s", s.c_str());
	os_quick_write_utf8_file(file2, s.c_str(), s.size(), false);
        bfree(file2);
    }

    return true;
}
