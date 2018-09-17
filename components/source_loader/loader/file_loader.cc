
#include <fstream>
#include <stdio.h>
#include "file_loader.h"
#include "glog/logging.h"
#include "base/memory/scoped_guard.h"

namespace component {
namespace sl {

FileLoader::FileLoader(ReaderDelegate* watcher, Json reader_config)
	: Loader(watcher, reader_config) {
}

FileLoader::~FileLoader() {
}

int FileLoader::Initialize() {
	if (!ParseAndCheckConfig()) {
		return -1;
	}

	return Load();
}

int FileLoader::Load() {
	LOG(INFO) << "file loader start load resources; file count:" << files_.size();
	int result = 0;

	std::ifstream ifs;

	for (auto& file : files_) {
		ifs.open(file);
		if (!ifs.is_open()) {
			LOG(ERROR) << "file [" << file << "] can't be open correctly";
			continue;
		}
		{
			base::ScopedGuard guard([&]() {
				ifs.close();
			});

			if (parse_mode_ == "line") {
				std::string line;

				while(!ifs.eof()) {

					std::getline(ifs, line);
					if (watcher_) {
						watcher_->OnReadData(line);
					}
				};

			} else if (parse_mode_ == "content") {
				std::string content;

				ifs.seekg(0, std::ios::end);
				content.reserve(ifs.tellg());
				ifs.seekg(0, std::ios::beg);
				content.assign((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

				if (watcher_) {
					watcher_->OnReadData(content);
				}
			} else {
				LOG(ERROR) << "not supported mode to read file:[" << file << "]";
				result = -1;
				break;
			}
		}
	}

	if (watcher_) {
		watcher_->OnFinish(result);
	}
	LOG(INFO) << "file loader load resources finished; all file loaded";
	return result;
}

bool FileLoader::ParseAndCheckConfig() {
	if (config_.at("type").get<std::string>() != "file") {
		return false;
	}
	parse_mode_ = config_.at("mode").get<std::string>();
	if (parse_mode_.empty()) {
		return false;
	}
	Json resources = config_["resources"];
	if (!resources.is_array()) {
		return false;
	}
	for (Json::iterator iter = resources.begin(); iter != resources.end(); iter++) {
		CHECK((*iter).is_object());
		std::string file = (*iter)["path"].get<std::string>();
		if (file.empty()) {
			continue;
		}
		files_.push_back(file);
	}
	return true;
}

}}
