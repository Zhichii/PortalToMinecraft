
#ifndef RJSON_H
#define RJSON_H
#include <string>

namespace rjson {
	struct value {
		enum Type {
			TYPE_NULL,
			TYPE_BOOL,
			TYPE_REAL,
			TYPE_SIGN,
			TYPE_UNSI,
			TYPE_LIST,
			TYPE_MAPP,
			TYPE_STRI
		};
		Type type = TYPE_SIGN;
		double realVal = 0;
		long long inteVal = 0;
		std::string striVal;
		std::string tag;
		value* head = nullptr;
		value* tail = nullptr;
		value* next = nullptr;
		value(std::string __tag = "") {
			this->head = nullptr;
			this->tail = nullptr;
			this->next = nullptr;
			this->tag = __tag;
			this->type = TYPE_NULL;
		}
		value(value& value) {

		}
		void releasePayload() {
			value* cur = this->head;
			value* next;
			while (cur != nullptr) {
				next = cur->next;
				delete cur;
				cur = next;
			}
			this->head = nullptr;
			this->tail = nullptr;
		}
		~value() { this->releasePayload(); }
		value& append() {
			if (this->head == nullptr) {
				this->head = new value;
				this->tail = this->head;
			}
			else {
				this->tail->next = new value;
				this->tail = this->tail->next;
			}
			return *this->tail;
		}
		value& operator[](size_t index) {
			if (this->type != TYPE_LIST) throw std::exception("Not list!!! ");
			size_t ind = 0;
			value* cur = this->head;
			while (cur != nullptr) {
				if (ind == index) {
					return *cur;
				}
				cur = cur->next;
				ind++;
			}
			throw std::exception("Not found!!! ");
		}
		value& operator[](std::string key) {
			if (this->type != TYPE_MAPP) throw std::exception("Not mapp!!! ");
			value* cur = this->head;
			while (cur != nullptr) {
				if (cur->tag == key) return *cur;
				cur = cur->next;
			}
			value& n = this->append();
			n.tag = key;
			return n;
		}
		inline value* operator=(bool val) { this->releasePayload(); this->type = TYPE_BOOL; this->inteVal = val; return this; }
		inline value* operator=(short val) { this->releasePayload(); this->type = TYPE_SIGN; this->inteVal = val; return this; }
		inline value* operator=(int val) { this->releasePayload(); this->type = TYPE_SIGN; this->inteVal = val; return this; }
		inline value* operator=(long long val) { this->releasePayload(); this->type = TYPE_SIGN; this->inteVal = val; return this; }
		inline value* operator=(double val) { this->releasePayload(); this->type = TYPE_REAL; this->realVal = val; return this; }
		inline value* operator=(unsigned short val) { this->releasePayload(); this->type = TYPE_UNSI; this->inteVal = (long long)val; return this; }
		inline value* operator=(unsigned int val) { this->releasePayload(); this->type = TYPE_UNSI; this->inteVal = (long long)val; return this; }
		inline value* operator=(unsigned long long val) { this->releasePayload(); this->type = TYPE_UNSI; this->inteVal = (long long)val; return this; }
		inline value* operator=(std::string val) { this->releasePayload(); this->type = TYPE_STRI; this->striVal = val; return this; }
		inline value* operator=(char* val) { this->releasePayload(); this->type = TYPE_STRI; this->striVal = val; return this; }
		inline value* operator=(const char* val) { this->releasePayload(); this->type = TYPE_STRI; this->striVal = val; return this; }
		inline value* setNull() { this->releasePayload(); this->type = TYPE_NULL; return this; }
		inline bool asBool() {
			if (this->type == TYPE_BOOL) return this->inteVal;
			else return 0;
		}
		inline long long asSigned() {
			if (this->type == TYPE_SIGN) return this->inteVal;
			else return 0;
		}
		inline unsigned long long asUnsigned() {
			if (this->type == TYPE_UNSI) return (unsigned long long)this->inteVal;
			else return 0;
		}
		inline double asFloat() {
			if (this->type == TYPE_REAL) return this->realVal;
			else return 0;
		}
		inline std::string asString() {
			if (this->type == TYPE_STRI) return this->striVal;
			else return std::string();
		}
		inline bool isNull() {
			return this->type == TYPE_NULL;
		}
		void write(std::string* output, size_t indent = 4, size_t __indent=0, bool __showtag=0) {
			size_t doFormat = (indent!=0);
			for (size_t i = 0; i < __indent*doFormat; i++) (*output) += " ";
			if (__showtag) {
				(*output) += "\"";
				for (const char& i : this->tag) {
					switch (i) {
					case '\\': (*output) += "\\\\"; break;
					case '\'': (*output) += "\\\'"; break;
					case '\"': (*output) += "\\\""; break;
					case '\a': (*output) += "\\a"; break;
					case '\b': (*output) += "\\b"; break;
					case '\f': (*output) += "\\f"; break;
					case '\n': (*output) += "\\n"; break;
					case '\r': (*output) += "\\r"; break;
					case '\t': (*output) += "\\t"; break;
					case '\v': (*output) += "\\v"; break;
					default:   (*output) += i;     break;
					}
				}
				(*output) += "\":";
				if (doFormat) (*output) += " ";
			}
			switch (this->type) {
			case TYPE_STRI: {
				(*output) += "\"";
				for (const char& i : this->striVal) {
					switch (i) {
					case '\\': (*output) += "\\\\"; break;
					case '\'': (*output) += "\\\'"; break;
					case '\"': (*output) += "\\\""; break;
					case '\a': (*output) += "\\a"; break;
					case '\b': (*output) += "\\b"; break;
					case '\f': (*output) += "\\f"; break;
					case '\n': (*output) += "\\n"; break;
					case '\r': (*output) += "\\r"; break;
					case '\t': (*output) += "\\t"; break;
					case '\v': (*output) += "\\v"; break;
					default:   (*output) += i;     break;
					}
				}
				(*output) += "\"";
			} break;
			case TYPE_LIST:
			case TYPE_MAPP: {
				if (this->type == TYPE_LIST) (*output) += "[";
				if (this->type == TYPE_MAPP) (*output) += "{";
				if (this->head != nullptr && doFormat) (*output) += "\n";
				value* cur = this->head;
				while (cur!=nullptr && cur!=this->tail) {
					cur->write(output, (__indent+indent)*doFormat, this->type==TYPE_MAPP);
					(*output) += ",";
					if (doFormat) (*output) += " \n";
					cur = cur->next;
				}
				if (this->tail != nullptr) {
					cur->write(output, (__indent+indent)*doFormat, this->type==TYPE_MAPP);
					if (doFormat) (*output) += "\n";
				}
				if (this->head != nullptr) for (size_t i = 0; i < (__indent)*doFormat; i++) (*output) += " ";
				if (this->type == TYPE_LIST) (*output) += "]";
				if (this->type == TYPE_MAPP) (*output) += "}";
			} break;
			case TYPE_BOOL: {
				(*output) += (this->inteVal)?("true"):("false");
			} break;
			case TYPE_NULL: {
				(*output) += "null";
			} break;
			default: {
				(*output) += std::to_string(this->inteVal);
			} break;
			}
		}
	};
}

#endif // RJSON_H