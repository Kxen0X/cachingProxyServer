#pragma once

#include <net_common.h>



// Last recently used cache 
// 50 Mb by default
class LRUCache {
public:

	LRUCache(size_t mxS = 50 * 1024 * 1024) : currentSize{ 0 }, maxSize{ mxS } {

	}

	//LRUCache(LRUCache& other) = delete;

	std::optional<std::string> get(const std::string &key) {
		auto it = iters.find(key);
		if (it == iters.end()) return std::nullopt;
		data.splice(data.begin(), data, it->second);
		return data.front().second;
	}

	void add(const std::string &key, std::string Newdata) {
		if (key.size() + Newdata.size() > maxSize) {
			std::cout << "THE DATA SIZE IS TOO LARGE TO BE WRITTEN TO THE CACHE";
			return;
		}
		auto it = iters.find(key);

		if (it != iters.end()) {
			currentSize -= key.size() + (it->second)->second.size();
			this->data.erase(it->second);
			iters.erase(key);

		}

		if (currentSize + key.size() + Newdata.size() > maxSize) {
			while (!this->data.empty() && (currentSize + key.size() + Newdata.size() > maxSize)) {
				longUsedDataRemove();
			}
			std::cout << "INSUFFICIENT CACHE SPACE, DATA THAT HAD NOT BEEN USED FOR A LONG TIME WAS REMOVED" << std::endl;
		}

		currentSize += key.size() + Newdata.size();
		this->data.push_front({ key, std::move(Newdata) });
		iters[key] = this->data.begin();
			
	}
	void clear() {
		data.clear();
		iters.clear();
		currentSize = 0;
	}
private:
	void longUsedDataRemove() {
		currentSize -= this->data.back().first.size() + this->data.back().second.size();
		iters.erase(this->data.back().first);
		this->data.pop_back();
	}

private:

	std::list<std::pair<std::string, std::string>> data;
	std::unordered_map<std::string, std::list<std::pair<std::string, std::string>>::iterator> iters;

	size_t currentSize;
	size_t maxSize;

};
