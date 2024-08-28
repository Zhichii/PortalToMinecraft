#include <string>

namespace Json {
	struct Value {
		enum Type {
			TYPE_NULL,
			TYPE_BOOL,
			TYPE_REAL,
			TYPE_INTE,
			TYPE_LIST,
			TYPE_MAPP,
			TYPE_STRI
		};
		Type type = TYPE_INTE;
		double realVal = 0;
		long long inteVal = 0;
		std::string striVal;
		std::string tag;
		Value* head = nullptr;
		Value* tail = nullptr;
		Value* next = nullptr;
		Value(std::string tag = "") {
			this->head = nullptr;
			this->tail = nullptr;
			this->next = nullptr;
			this->tag = std::string();
		}
		Value(Value& value) {

		}
		void ReleasePayload() {
			Value* cur = this->head;
			Value* next;
			while (cur != nullptr) {
				next = cur->next;
				delete cur;
				cur = next;
			}
		}
		~Value() { this->ReleasePayload(); }
		Value* addElem() {
			if (this->head == nullptr) {
				this->head = new Value;
				this->tail = this->head;
			}
			else {
				this->tail->next = new Value;
				this->tail = this->tail->next;
			}
			return this->tail;
		}
		Value* operator[](size_t index) {
			if (this->type != TYPE_LIST) return nullptr;
			size_t ind = 0;
			Value* cur = this->head;
			while (cur != nullptr) {
				if (ind == index) {
					return cur;
				}
				cur = cur->next;
				ind++;
			}
			return nullptr;
		}
		Value* operator[](std::string key) {
			if (this->type != TYPE_LIST) return nullptr;
			Value* cur = this->head;
			while (cur != nullptr) {
				if (cur->tag == key) return cur;
				cur = cur->next;
			}
			cur = this->addElem();
			cur->tag = key;
			return cur;
		}
		inline Value* operator=(bool val) { this->ReleasePayload(); this->type = TYPE_BOOL; this->inteVal = val; return this; }
		inline Value* operator=(long long val) { this->ReleasePayload(); this->type = TYPE_INTE; this->inteVal = val; return this; }
		inline Value* operator=(double val) { this->ReleasePayload(); this->type = TYPE_REAL; this->realVal = val; return this; }
		inline Value* operator=(std::string val) { this->ReleasePayload(); this->type = TYPE_STRI; this->striVal = val; return this; }
		inline Value* setNull() { this->ReleasePayload(); this->type = TYPE_NULL; return this; }
		inline bool asBool() {
			if (this->type == TYPE_BOOL) return this->inteVal;
			else return 0;
		}
		inline long long asInt() {
			if (this->type == TYPE_INTE) return this->inteVal;
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
		inline bool IsNull() {
			return this->type == TYPE_NULL;
		}
		void WriteString(std::string* output, size_t curInd = 0) {
			if (this->tag != std::string()) (*output) += "\"" + this->tag + "\":";
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
					}
				}
				(*output) += "\"";
			} break;
			case TYPE_LIST:
			case TYPE_MAPP: {
				if (this->type == TYPE_LIST) (*output) += "[";
				if (this->type == TYPE_MAPP) (*output) += "{";
				Value* cur = this->head;
				while (cur != nullptr) {
					cur->WriteString(output);
					(*output) += ", ";
					cur = cur->next;
				}
				if (this->type == TYPE_LIST) (*output) += "]";
				if (this->type == TYPE_MAPP) (*output) += "}";
			} break;
			default: {
				(*output) += std::to_string(this->inteVal);
			} break;
			}
		}
	};
}