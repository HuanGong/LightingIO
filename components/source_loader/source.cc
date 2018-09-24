//
// Created by gh on 18-9-16.
//

#include "source.h"
#include "glog/logging.h"
#include "loader/loader_factory.h"
#include "parser/column_parser.h"

namespace component {
namespace sl {

static std::map<std::string, std::function<Parser* ()> > g_parser_creator = {
		{"list", []() ->Parser* { return new ColumnParser(); }},
		{"column", []() ->Parser* { return new ColumnParser(); }},
};

void RegisterParserCreator(const std::string& name, std::function<Parser* ()> creator) {
	g_parser_creator[name] = creator;
};

Source::Source(Json& source_config)
	: source_config_(source_config) {
}

Source::~Source() {
}

bool Source::Initialize() {
	const static Json nil_json;
	CHECK(source_config_.is_object());

	const Json& loader_config = source_config_.value("loader", nil_json);
	if (!loader_config.is_object()) {
		LOG(ERROR) << __FUNCTION__ << " bad config:" << source_config_;
		return false;
	}

	Json parser_config;
	parser_config = source_config_.value("parser", nil_json);
	if (!parser_config.is_object()) {
		LOG(ERROR) << " Source config with out parser config:" << source_config_;
		return false;
	}

	std::string parser_name = parser_config.value("type", "");
	auto creator = g_parser_creator.find(parser_name);
	if (creator == g_parser_creator.end()) {
		LOG(ERROR) << __FUNCTION__ << " no parser for type:" << parser_name;
		return false;
	}
	parser_.reset(creator->second());
	if (!parser_->Initialize(parser_config)) {
		LOG(ERROR) << __FUNCTION__ << " Failed Initialize Parser:" << parser_config;
		return false;
	}
	parser_->SetDelegate(this);

	loader_.reset(LoaderFactory::Instance().CreateLoader(this, loader_config));
	CHECK(loader_);
	loader_->SetParserContentMode(parser_->ContentMode());

	if (0 != loader_->Initialize()) {
		LOG(ERROR) << " Failed Initialize Loader:" << loader_config;
		return false;
	}

	return true;
}

int Source::StartLoad() {
	return loader_->Load();
}

void Source::OnFinish(int code) {
  parser_->OnSourceLoadFinished(code);
}

void Source::OnReadData(const std::string& content) {
	LOG(INFO) << __FUNCTION__ << " enter";
  parser_->ParseContent(content);
	LOG(INFO) << __FUNCTION__ << " leave";
}


}} //component::sl
