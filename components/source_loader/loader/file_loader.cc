
#include <fstream>
#include <stdio.h>
#include "file_loader.h"
#include "glog/logging.h"
#include "base/base_constants.h"
#include "base/memory/scoped_guard.h"

namespace component {
namespace sl {

FileLoader::FileLoader(LoaderDelegate* watcher, Json reader_config)
	: Loader(watcher, reader_config) {
}

FileLoader::~FileLoader() {
}

int FileLoader::Initialize() {
	if (!ParseAndCheckConfig()) {
		LOG(WARNING) << __FUNCTION__ << " Initialize failed, config:" << config_;
		return -1;
	}
	return 0;
}

int FileLoader::Load() {
	VLOG(GLOG_VTRACE) << __FUNCTION__ << " Enter, source count:" << files_.size();

	int result = 0;

	std::ifstream ifs;

	for (auto& file : files_) {
		ifs.open(file);
		if (!ifs.is_open()) {
			LOG(ERROR) << __FUNCTION__ << " file [" << file << "] can't be open correctly";
			continue;
		}

		{
			base::ScopedGuard guard([&]() {
				ifs.close();
			});

			if (parser_content_mode_ == "line") {
				std::string line;

				while(!ifs.eof()) {
					std::getline(ifs, line);
					watcher_->OnReadData(line);
				};
			} else if (parser_content_mode_ == "content") {
				std::string content;

				ifs.seekg(0, std::ios::end);
				content.reserve(ifs.tellg());
				ifs.seekg(0, std::ios::beg);
				content.assign((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

				watcher_->OnReadData(content);
			} else {
				LOG(ERROR) << "not supported mode to read file:[" << file << "]";
				result = -1;
				break;
			}
		}
	}

	watcher_->OnFinish(result);
	VLOG(GLOG_VTRACE) << __FUNCTION__ << " Leave with result :" << result;
	return result;
}

bool FileLoader::ParseAndCheckConfig() {
	if (!config_.is_object()) return false;

	const static Json nil_json;

	if (config_.value("type", "") != "file") {
		return false;
	}
	const Json& resources = config_.value("resources", nil_json);
	if (!resources.is_array()) {
		return false;
	}
	for (Json::const_iterator iter = resources.begin(); iter != resources.end(); iter++) {
		CHECK((*iter).is_object());
		std::string file = iter->value("path", "");
		if (file.empty()) {
			LOG(INFO) << __FUNCTION__ << " resource:" << *iter << " be omited for empty path";
			continue;
		}
		files_.push_back(file);
	}
	return true;
}

}}
