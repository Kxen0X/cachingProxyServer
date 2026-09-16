#pragma once

#include <net_common.h>



// Last recently used cache 
class LRUCache {
public:

	LRUCache(size_t mxS = 50 * 1024 * 1024);

	std::optional<std::string> get(const std::string&);

	void add(const std::string&, std::string);
	void clear();
private:
	void longUsedDataRemove();

private:

	std::list<std::pair<std::string, std::string>> data;
	std::unordered_map<std::string, std::list<std::pair<std::string, std::string>>::iterator> iters;

	size_t currentSize;
	size_t maxSize;

};
