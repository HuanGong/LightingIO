//
// Created by gh on 18-9-23.
//

#include <sstream>
#include "mysql_loader.h"
#include "glog/logging.h"
#include <driver.h>
#include <mysql_connection.h>
#include <cppconn/exception.h>
#include <cppconn/connection.h>


namespace component {
namespace sl {

void to_json(const MysqlConfig& c, Json& out) {
	out["port"] = c.port;
	out["host"] = c.host;
	out["database"] = c.db;
	out["user"] = c.user;
	out["password"] = c.password;
	out["sql_query"] =c.query;
}
void from_json(const Json& j, MysqlConfig& c) {
	if (!j.is_object()) {
		c.SetNull(true);
	}
	try {
		c.host = j.at("host");
		c.port = j.at("port");
		c.db = j.at("database");
		c.user = j.at("user");
		c.password = j.at("password");
		c.query = j.at("sql_query");
		c.SetNull(false);
	} catch (...) {
		c.SetNull(true);
	}
}

MysqlLoader::MysqlLoader(LoaderDelegate* watcher, const Json& reader_config)
	: Loader(watcher, reader_config) {

}
MysqlLoader::~MysqlLoader() {

}

int MysqlLoader::Initialize() {
	if (!config_.is_object()) {
		LOG(WARNING) << __FUNCTION__ << " mysql loader config error:" << config_;
		return -1;
	}
	int result = -1;
	do {
		Json resources = config_.value("resources", "");
		if (!resources.is_array()) {
			break;
		}
		for (Json::iterator it = resources.begin(); it != resources.end(); it++) {
			MysqlConfig mysql_config = (*it);
			if (mysql_config.IsNull()) {
				continue;
			}
			mysql_sources.push_back(mysql_config);
		}
		if (0 == mysql_sources.size()) {
			break;
		}

		result = 0;
	} while(0);
	return result;
}

int MysqlLoader::Load() {

	sql::Driver *driver driver = get_driver_instance();

	for (const auto& config : mysql_sources) {
		std::ostringstream oss;
		oss << "tcp://" << config.host << ":" << config.port;

		std::shared_ptr<sql::Connection> con
		try {
			con.reset(driver->connect(oss.str(), config.user, config.password));
			if (!con) {
				LOG(WARNING) << " Sql Connection Failed";
				continue;
			}

			con->setSchema(config.db);

			std::shared_ptr<sql::Statement> stmt(con->createStatement());
			std::shared_ptr<sql::ResultSet> res = stmt->executeQuery(config.query);

			while(res->next()) {
				cout << "id: " << res->getInt(1) << endl;
				cout << "name: " << res->getString(2) << endl;
				cout << "price: " << res->getDouble(3) << endl;
				cout << "created: " << res->getString(4) << endl;
			}
			delete res;
			delete stmt;

		} catch (sql::SQLException &e) {
			LOG(WARNING) << __FUNCTION__ << " connect mysql: " << oss.str() << " err:" <<
			e.what() << " error code:" << e.getErrorCode << " status:" << e.getSQLState();
		} catch (...) {
			LOG(WARNING) << __FUNCTION__ << " other error";
		}
	}
	return 0;
};

}}