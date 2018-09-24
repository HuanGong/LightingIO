//
// Created by gh on 18-9-23.
//

#ifndef LIGHTINGIO_MYSQL_LOADER_H
#define LIGHTINGIO_MYSQL_LOADER_H

#include "loader.h"
#include "../json_class_meta.h"
/*
 * "loader": {
 *    "type": "file",
 *    "mode": "line",
 *    "resources": [{
 *      "host":"127.0.0.1",
 *      "port":3306,
 *      "database": "uolo",
 *      "user": "root",
 *      "password": "gonghuan",
 *      "sql_query": "select name, age, info from users"
 *    }]
 *  }
 * */
namespace component {
namespace sl {

struct MysqlConfig : public JsonClassMeta {
	int32_t port;
	std::string host;
	std::string db;
	std::string user;
	std::string password;
	std::string query;
};

void to_json(const MysqlConfig& c, Json& out);
void from_json(const Json& j, MysqlConfig& c);

class MysqlLoader : public Loader {
public:
	MysqlLoader(LoaderDelegate* watcher, Json& reader_config);
	~MysqlLoader();

	int Initialize() override;
	int Load() override;
private:
	std::vector<MysqlConfig> mysql_sources;
};

}}
#endif //LIGHTINGIO_MYSQL_LOADER_H
