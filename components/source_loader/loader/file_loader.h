#ifndef LT_COMPONENT_ROW_FILE_READER_H
#define LT_COMPONENT_ROW_FILE_READER_H

#include <vector>
#include <string>
#include "components/source_loader/loader.h"

namespace component {
namespace sl {

class FileLoader : public Loader {
public:
	FileLoader(ReaderDelegate* watcher, Json reader_config);
	~FileLoader() override;

	int Initialize() override;
	int Load() override;
private:
	bool ParseAndCheckConfig();
	std::string parse_mode_;
	std::vector<std::string> files_;
};

}}
#endif
