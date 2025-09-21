#include <cookie.h>

std::random_device rd;
std::mt19937_64 gen(rd());
std::mutex randomMutex;

uint64_t randomNumber() {
	randomMutex.lock();
	std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);
	uint64_t randomNumber = dist(gen);
	randomMutex.unlock();
	return randomNumber;
}

Cookie::Cookie() :m_gname(""), m_name(""), m_value(""), m_expires(""), m_maxage(24 * 60 * 60), m_secure(false), m_httpOnly(false), m_path(""), m_domain("") {

}

Cookie::Cookie(const std::string& gname, const std::string& name, const std::string& value, std::string expires, uint64_t maxage , bool secure, bool httpOnly , std::string path , std::string domain)
	: m_gname(name), m_name(name), m_value(value), m_expires(expires), m_maxage(maxage), m_secure(secure), m_httpOnly(httpOnly), m_path(path), m_domain(domain)
{
	
}

void Cookie::generateSessionId() {
	if (m_value == "") {
		Cookie::generateRandomSessionId(m_value);
		m_gname = m_value;
		m_name = "SessionID";
	}
	httpdlog("DEBUG", "Cookie created: " + toString());
}


void Cookie::convertToString(uint8_t* value, std::string& randomSessionId) {
	int i = 0;
	randomSessionId = "";
	char buffer[2] = { 0, 0 };
	while (i < 31 * 8) {
		if (value[i] == 0) {
			randomSessionId += "0";
		}
		else if (value[i] >= '0' && value[i] <= '9') {
			buffer[0] = (char)value[i];
			buffer[1] = 0;
			randomSessionId += buffer;
		}
		else if (value[i] >= 'A' && value[i] <= 'Z') {
			buffer[0] = (char)value[i];
			buffer[1] = 0;
			randomSessionId += buffer;
		}
		else if (value[i] >= 'a' && value[i] <= 'z') {
			buffer[0] = (char)value[i];
			randomSessionId += buffer;
		}
		else {
			randomSessionId += std::to_string(value[i] % 100);
		}
		i++;
	}
}

void Cookie::generateRandomSessionId(std::string& randomSessionId) {
	uint64_t randNumber[32];
	randNumber[31] = 0; // Null-terminate the array
	int i = 0;
	while (i < 31) {
		randNumber[i] = randomNumber();
		i++;
	}
	//httpdlog("DEBUG", "Generated random numbers for session id");
	try {
		uint8_t* bytePtr = reinterpret_cast<uint8_t*>(randNumber);
		Cookie::convertToString(bytePtr, randomSessionId);
	}
	catch (...) {
		httpdlog("ERROR", "Exception in generating random session id ");
	}

	//httpdlog("DEBUG", "Generated random numbers for session id = "+ randomSessionId );
}

std::string Cookie::toString()  {
	std::string cookieStr = m_name + "=" + m_value + "; ";
	if (!m_expires.empty()) {
		cookieStr += "Expires=" + m_expires + "; ";
	}
	if (m_maxage >= 0) {
		cookieStr += "Max-Age=" + std::to_string(m_maxage) + "; ";
	}
	if (m_secure) {
		cookieStr += "Secure; ";
	}
	if (m_httpOnly) {
		cookieStr += "HttpOnly; ";
	}
	if (!m_sameSite.empty()) {
		cookieStr += "SameSite=" + m_sameSite + "; ";
	}
	if (!m_path.empty()) {
		cookieStr += "Path=" + m_path + "; ";
	}
	if (!m_domain.empty()) {
		cookieStr += "Domain=" + m_domain + "; ";
	}
	// Remove the trailing "; "
	if (cookieStr.size() > 2) {
		cookieStr = cookieStr.substr(0, cookieStr.size() - 2);
	}
	return cookieStr;
}

std::string Cookie::toGroupString()  {
	std::string cookieStr = m_name + "=" + m_value + "; ";
	if (!m_expires.empty()) {
		cookieStr += "Expires=" + m_expires + "; ";
	}
	if (m_maxage > 0) {
		cookieStr += "Max-Age=" + std::to_string(m_maxage) + "; ";
	}
	if (m_secure) {
		cookieStr += "Secure; ";
	}
	if (m_httpOnly) {
		cookieStr += "HttpOnly; ";
	}
	if (!m_sameSite.empty()) {
		cookieStr += "SameSite=" + m_sameSite + "; ";
	}
	if (!m_path.empty()) {
		cookieStr += "Path=" + m_path + "; ";
	}
	if (!m_domain.empty()) {
		cookieStr += "Domain=" + m_domain + "; ";
	}
	// Remove the trailing "; "
	if (cookieStr.size() > 2) {
		cookieStr = cookieStr.substr(0, cookieStr.size() - 2);
	}
	cookieStr = m_gname + ":" + cookieStr + "\r\n";
	return cookieStr;
}

void Cookie::parseCookieStringWithGroup(const std::string& cookieStr) {
	size_t colonPos = cookieStr.find(":");
	if (colonPos != std::string::npos) {
		m_gname = cookieStr.substr(0, colonPos);
		httpdlog("DEBUG", "Parsing cookie string with group name: " + m_gname);
		std::string rest = cookieStr.substr(colonPos + 1);
		parseCookieString(rest);
	}
	else {
		httpdlog("DEBUG", "Parsing cookie string with no group name");
	}
}

void Cookie::parseCookieString(const std::string& cookieStr) {
	// Simple parser for demonstration purposes
	size_t pos = 0;
	size_t end = cookieStr.find(";");
	while (end != std::string::npos) {
		std::string token = cookieStr.substr(pos, end - pos);
		size_t eqPos = token.find("=");
		if (eqPos != std::string::npos) {
			std::string key = token.substr(0, eqPos);
			std::string value = token.substr(eqPos + 1);
			if (key == "Expires") {
				httpdlog("DEBUG", "Found Expires attribute: " + value);
				m_expires = value;
			}
			else if (key == "Max-Age") {
				httpdlog("DEBUG", "Found Max-Age attribute: " + value);
				m_maxage = std::stoull(value);
			}
			else if (key == "Secure") {
				httpdlog("DEBUG", "Found Secure attribute: " + value);
				m_secure = true;
			}
			else if (key == "HttpOnly") {
				httpdlog("DEBUG", "Found HttpOnly attribute: " + value);
				m_httpOnly = true;
			}
			else if (key == "SameSite") {
				httpdlog("DEBUG", "Found SameSite attribute: " + value);
				m_sameSite = value;
			}
			else if (key == "Path") {
				httpdlog("DEBUG", "Found Path attribute: " + value);
				m_path = value;
			}
			else if (key == "Domain") {
				httpdlog("DEBUG", "Found Domain attribute: " + value);
				m_domain = value;
			}
			else if (m_name.empty()) { // First key-value pair is the cookie name and value
				httpdlog("DEBUG", "Found Key Value: "+ key + " -> " + value);
				m_name = key;
				m_value = value;
			}
		}
		pos = end + 2; // Move past "; "
		end = cookieStr.find(";", pos);
	}
	// Handle the last token
	std::string token = cookieStr.substr(pos);
	size_t eqPos = token.find("=");
	if (eqPos != std::string::npos && m_name.empty()) {
		
		m_name = token.substr(0, eqPos);
		m_value = token.substr(eqPos + 1);
		httpdlog("DEBUG", "Found Key Value: " + m_name + " -> " + m_value);
	}
}

void Cookie::getCookieCStr(char* cookieCstr) {
	cookieCstr[0] = '\0'; // Ensure the std::string is empty before concatenation
	std::string cookieStr = toString();
	strcpy_s(cookieCstr, cookieStr.length(), cookieStr.c_str());
}


CookieMap  CookieManager::m_cookies;
std::mutex CookieManager::m_mutex;

CookieManager::CookieManager() {

}

CookieManager::~CookieManager() {
	closeFile();
	openFile();
	saveAll();
	for (auto& pair : m_cookies) {
		if (pair.second != nullptr) {
			delete pair.second;
		}
	}
	m_cookies.clear();
}

void CookieManager::openFile() {
	std::lock_guard<std::mutex> lock(m_mutex);
	std::string tempFileName = "C:\\HttpdWin\\Storage\\cookies.dat";
	if (m_file.is_open()) {
		m_file.close();
	}
	m_file.open(tempFileName, ios::in | ios::trunc | ios::out);
	if (m_file.is_open()) {
		httpdlog("DEBUG", "Opened file for writing: " + tempFileName);
	}
	else {
		httpdlog("ERROR", "Error opening file for writing: " + tempFileName);
	}
}

void CookieManager::openFileForReading() {
	std::lock_guard<std::mutex> lock(m_mutex);
	std::string tempFileName = "C:\\HttpdWin\\Storage\\cookies.dat";
	if (m_file.is_open()) {
		m_file.close();
	}
	m_file.open(tempFileName, ios::in | ios::out);
	if (m_file.is_open()) {
		httpdlog("INFO", "Opened file for reading: " + tempFileName);
	}
	else {
		httpdlog("ERROR", "Error opening file for reading: " + tempFileName);
	}
}

void CookieManager::closeFile() {
	std::lock_guard<std::mutex> lock(m_mutex);
	if (m_file.is_open()) {
		m_file.close();
	}
	else {
		httpdlog("ERROR", "Error file is not open ");
	}
}

void CookieManager::saveAll() {
	std::lock_guard<std::mutex> lock(m_mutex);
	if (m_file.is_open()) {
		CookieMap::iterator it = m_cookies.begin();
		while (it != m_cookies.end()) {
			CookieList* list = it->second;
			if (list == 0) {
				++it; 
				continue;
			}
			CookieList::iterator it2 = list->begin();
			while( it2 != list->end()) {
				m_file << it2->toGroupString();
				++it2;
			}
			++it;
		}
		m_file.flush();
		//m_file.close();
	}
	else {
		//httpdlog("INFO", "Error file is not open ");
	}
}

void CookieManager::loadAll(){
	std::lock_guard<std::mutex> lock(m_mutex);
	std::string tempFileName = "C:\\HttpdWin\\Storage\\cookies.dat";
	if (m_file.is_open()) {
		std::string line;
		CookieList* list = 0;
		bool flag = false;
		while (std::getline(m_file, line)) {
			httpdlog("DEBUG", "Read line: " + line);
			if ( line.length() <= 10 ) //omitting small lines and empty lines
				continue;
			Cookie cookie("", "", "");
			cookie.parseCookieStringWithGroup(line);
			if (list == 0 )
			{
				list = new CookieList();
				list->push_back(cookie);
				m_cookies.insert({ list->front().m_gname, list });
				httpdlog("DEBUG", "Adding to map during null list: " + list->front().m_gname);
			}
			else if ( list->size() == 0) {
				list->push_back(cookie);
				m_cookies.insert({ list->front().m_gname, list });
				httpdlog("DEBUG", "Adding to map during empty list : " + list->front().m_gname);
			}
			else if (list->front().m_gname == cookie.m_gname) {
				list->push_back(cookie);
			}
			else if (list->front().m_gname != cookie.m_gname) {
				list = new CookieList();
				list->push_back(cookie);
				httpdlog("DEBUG", "Adding to map different list : " + list->front().m_gname);
				m_cookies.insert({ list->front().m_gname, list });	
			}
		}
	}
	else {
		httpdlog("INFO", "Error opening file for reading: " + tempFileName);
	}
}	

bool CookieManager::add(std::string gname, CookieList *cl) {
	std::lock_guard<std::mutex> lock(m_mutex);
	auto it = m_cookies.find(gname);
	if (it == m_cookies.end()) {

		//m_cookies[gname] = cl;
		m_cookies.insert({ gname, cl });
		httpdlog("INFO", "Cookie group name added to map");
		return true;
	}
	else {
		httpdlog("INFO", "Cookie group name already exists: " + gname);
		return false;
	}
}

void CookieManager::add(Cookie* cookie) {
	std::lock_guard<std::mutex> lock(m_mutex);
	auto it = m_cookies.find(cookie->m_gname);
	if (it != m_cookies.end()) {
		CookieList* cookieList = it->second;
		if (cookieList == 0)
		{
			cookieList = new CookieList();
			//m_cookies[cookie->m_gname] = cookieList;
			cookieList->push_back(*cookie);
		}
		else {
			cookieList->push_back(*cookie);
		}
	}
	else {
		CookieList* cookieList = new CookieList();
		//m_cookies[cookie->m_gname] = cookieList;
		cookieList->push_back(*cookie);
	}
}

void CookieManager::print() {
#if 0
	std::lock_guard<std::mutex> lock(m_mutex);
	auto it = m_cookies.begin();
	while (it != m_cookies.end()) {
		httpdlog("DEBUG", "Cookie Group: " + it->first);
		it++;
	}
#endif
}

void CookieManager::remove(Cookie* cookie) {
	std::lock_guard<std::mutex> lock(m_mutex);
	auto it = m_cookies.find(cookie->m_gname);
	if (it != m_cookies.end()) {
		CookieList* cookieList = it->second;
		if (cookieList != 0) {
			cookieList->remove(*cookie);
		}
	}
}

CookieList * CookieManager::get(std::string name) {
	CookieList *list = 0;
	std::lock_guard<std::mutex> lock(m_mutex);
	auto it = m_cookies.find(name);
	if (it != m_cookies.end()) {
		
		list = it->second;
		if( list )
			httpdlog("DEBUG", "Cookie found in map and is valid ");
		else
			httpdlog("DEBUG", "Cookie found in map but is null ");
	}
	return list;
}

void CookieManager::clear(std::string name) {
	CookieList* list = 0;
	std::lock_guard<std::mutex> lock(m_mutex);
	auto it = m_cookies.find(name);
	if (it != m_cookies.end()) {

		list = it->second;
		if (list) {
			list->clear();
			delete list;
			m_cookies.erase(it);
		} 
		else {
			m_cookies.erase(it);
			httpdlog("DEBUG", "Cookie found in map but is null ");
		}
	}
}


void CookieManager::get(std::string name, Cookie* cookie) {
	cookie = 0;
	std::lock_guard<std::mutex> lock(m_mutex);
	auto it = m_cookies.find(name);
	if (it != m_cookies.end()) {
		CookieList* cookieList = it->second;
		if (cookieList == 0) return;

		CookieList::iterator it2 = cookieList->begin();
		while (it2 != cookieList->end()) {
			if (it2->m_value == name && it2->m_gname == name) {
				cookie = &(*it2);
				return;
			}
			++it2;
		}
	}
}

void CookieManager::set(const Cookie& cookie) {
	std::lock_guard<std::mutex> lock(m_mutex);
	auto it = m_cookies.find(cookie.m_gname);
	if (it != m_cookies.end()) {
		CookieList* list = it->second;
		if (list == 0) {
			list = new CookieList();
			m_cookies[cookie.m_gname] = list;
		}
		list->push_back(cookie);
	}
	else {
		CookieList* list = new CookieList();
		m_cookies[cookie.m_gname] = list;
		list->push_back(cookie);
	}

}
