#include <sqlite3.h>

#include <nlohmann/json.hpp>

#include <string>
#include <ctime>
#include <iostream>
#include <optional>
#include <memory>

int main() {
	sqlite3* db = nullptr;
	if (sqlite3_open("data.sqlite3", &db) != SQLITE_OK) {
		// TODO: Couldn't open, handle!
		std::cerr << sqlite3_errmsg(db) << std::endl;
		return 1;
	}

	// Let's create the table if it doesn't exist
	{
		std::string sql =
			"CREATE TABLE IF NOT EXISTS bookmarks ("
				"id INTEGER PRIMARY KEY,"
				"time INTEGER NOT NULL,"
				"data JSON NOT NULL"
			");"
			// Can create indexes to any field to speed up queries
			"CREATE INDEX IF NOT EXISTS bookmarks_time ON bookmarks (time);"
			// Also create indexes to json fields
			"CREATE INDEX IF NOT EXISTS bookmarks_type ON bookmarks (json_extract(data, '$.type'));"
		;
		// sqlite3_exec is nice if you don't need to get data out and/or bind some data
		if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
			std::cerr << sqlite3_errmsg(db) << std::endl;
			return 1;
		}
	}

	// Let's add some data
	auto insertBookmark = [&](int64_t time, const nlohmann::json& json) -> std::optional<sqlite3_int64> {
		// First you prepare the query with some placeholders, the values could be added directly to the query
		// But the sqlite3_bind functions escape things correctly
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db, "INSERT INTO bookmarks (time, data) VALUES (@time, @data);", -1, &stmt, nullptr) != SQLITE_OK) {
			std::cerr << sqlite3_errmsg(db) << std::endl;
			return {};
		}
		// Inject the values
		if (sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, "@time"), time) != SQLITE_OK) {
			std::cerr << sqlite3_errmsg(db) << std::endl;
			return {};
		}

		auto data = json.dump();
		if (sqlite3_bind_text(stmt, sqlite3_bind_parameter_index(stmt, "@data"), data.c_str(), -1, nullptr) != SQLITE_OK) {
			std::cerr << sqlite3_errmsg(db) << std::endl;
			return {};
		}

		// Actually execute
		if (sqlite3_step(stmt) != SQLITE_DONE) {
			std::cerr << sqlite3_errmsg(db) << std::endl;
			return {};
		}

		auto id = sqlite3_last_insert_rowid(db);

		// Always cleanup stmt (this could be wrapped in some smart pointer)
		if (sqlite3_finalize(stmt) != SQLITE_OK) {
			std::cerr << sqlite3_errmsg(db) << std::endl;
			return {};
		}

		return id;
	};

	{
		auto time = std::time(nullptr);
		nlohmann::json json;
		json["description"] = "A bookmark";
		json["type"] = "KICK";
		json["time"] = time;
		if (!insertBookmark(time, json)) {
			return 1;
		}
	}

	{
		auto time = std::time(nullptr);
		nlohmann::json json;
		json["description"] = "A bookmark";
		json["type"] = "PENALTY";
		json["time"] = time;
		if (!insertBookmark(time, json)) {
			return 1;
		}
	}

	// Get all bookmarks
	{
		sqlite3_stmt *stmt;
		std::string sql = "SELECT id, time, data FROM bookmarks;";
		if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
			std::cerr << sqlite3_errmsg(db) << std::endl;
			return 1;
		}
		// Loop through all the rows
		int rc = 0;
		while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
			auto id = sqlite3_column_int64(stmt, 0);
			auto time = sqlite3_column_int64(stmt, 1);
			auto data = sqlite3_column_text(stmt, 2);

			auto bookmark = nlohmann::json::parse(data);
		}
		if (rc != SQLITE_DONE) {
			std::cerr << sqlite3_errmsg(db) << std::endl;
			return 1;
		}
		// Always cleanup stmt (this could be wrapped in some smart pointer)
		if (sqlite3_finalize(stmt) != SQLITE_OK) {
			std::cerr << sqlite3_errmsg(db) << std::endl;
			return {};
		}
	}

	{
		sqlite3_stmt *stmt;
		std::string sql = "SELECT id, time, data, json_extract(data, '$.type') as type FROM bookmarks WHERE type like 'KICK';";
		if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
			std::cerr << sqlite3_errmsg(db) << std::endl;
			return 1;
		}
		// Loop through all the rows
		int rc = 0;
		while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
			auto id = sqlite3_column_int64(stmt, 0);
			auto time = sqlite3_column_int64(stmt, 1);
			auto data = sqlite3_column_text(stmt, 2);

			auto bookmark = nlohmann::json::parse(data);
		}
		if (rc != SQLITE_DONE) {
			std::cerr << sqlite3_errmsg(db) << std::endl;
			return 1;
		}
		// Always cleanup stmt (this could be wrapped in some smart pointer)
		if (sqlite3_finalize(stmt) != SQLITE_OK) {
			std::cerr << sqlite3_errmsg(db) << std::endl;
			return {};
		}
	}

	if (db) sqlite3_close(db);

	return 0;
}