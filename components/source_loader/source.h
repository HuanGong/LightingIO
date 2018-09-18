//
// Created by gh on 18-9-16.
//

#ifndef LIGHTINGIO_COMPONENT_SOURCE_H
#define LIGHTINGIO_COMPONENT_SOURCE_H

#include "loader.h"
#include "parser.h"
#include <memory>

namespace component {
namespace sl {

class Source : public LoaderDelegate,
               public ParserDelegate {
public:
	Source(Json& source_config);
	virtual ~Source();

	bool Initialize();

	const std::string& Name() const {
		return source_name_;
	}

  //override from LoaderDelegate
  void OnFinish(int code) override;
  void OnReadData(const std::string&) override;
protected:
	Json source_config_;
	std::string source_name_;
  
  std::unique_ptr<Parser> parser_;
	std::unique_ptr<Loader> loader_;
};

}} //end component::sl
#endif //LIGHTINGIO_SOURCE_H
