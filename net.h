
#ifndef RIVERLAUNCHER3_NET_H
#define RIVERLAUNCHER3_NET_H
#include <curl/curl.h>

struct Data {
	std::vector<size_t> size;
	size_t totalSz;
	std::vector<char*> data;
};

size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
	Data* dat = (Data*)userdata;
	size_t sz = size*nmemb;
	auto* data = new char[sz];
	memcpy_s(data, sz, ptr, sz);
	dat->size.push_back(sz);
	dat->totalSz += sz;
	dat->data.push_back(data);
	return sz;
}

// Use the following format:
//     scheme://host:port/path
int URLGet(const std::string& url, char*&data, size_t&sz) {
	CURL* curlp = curl_easy_init();
	CURLcode cc;
	Data dat;
	dat.totalSz = 0;
	dat.data = {};
	dat.size = {};
	cc = curl_easy_setopt(curlp, CURLOPT_URL, url.c_str());
	if (cc != CURLE_OK) {
		writeLog("Getting [%s] failed 1: %d", url.c_str(), cc);
		curl_easy_cleanup(curlp);
		return 1;
	}
	cc = curl_easy_setopt(curlp, CURLOPT_WRITEDATA, &dat);
	if (cc != CURLE_OK) {
		writeLog("Getting [%s] failed 2: %d", url.c_str(), cc);
		curl_easy_cleanup(curlp);
		return 2;
	}
	cc = curl_easy_setopt(curlp, CURLOPT_WRITEFUNCTION, write_callback);
	if (cc != CURLE_OK) {
		writeLog("Getting [%s] failed 3: %d", url.c_str(), cc);
		curl_easy_cleanup(curlp);
		return 3;
	}
	cc = curl_easy_perform(curlp);
	if (cc != CURLE_OK) {
		writeLog("Getting [%s] failed 4: %d", url.c_str(), cc);
		curl_easy_cleanup(curlp);
		for (const auto& i : dat.data) {
			delete i;
		}
		return 4;
	}
	curl_easy_cleanup(curlp);
	sz = dat.totalSz;
	data = new char[sz];
	char* stream = data;
	for (size_t i = 0; i < dat.data.size(); i++) {
		memcpy(stream, dat.data[i], dat.size[i]);
		stream += dat.size[i];
		delete dat.data[i];
	}
	return 0;
}

#endif // RIVERLAUNCHER3_NET_H