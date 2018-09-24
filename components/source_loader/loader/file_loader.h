#ifndef LT_COMPONENT_ROW_FILE_READER_H
#define LT_COMPONENT_ROW_FILE_READER_H

#include <vector>
#include <string>
#include "loader.h"

namespace component {
namespace sl {

class FileLoader : public Loader {
public:
	FileLoader(LoaderDelegate* watcher, Json reader_config);
	~FileLoader();

	int Initialize() override;
	int Load() override;
private:
	bool ParseAndCheckConfig();
	std::string parse_mode_;
	std::vector<std::string> files_;
};

}}
#endif
