#include "mikroxml.hpp"

#include <sstream>

#include <utki/unicode.hpp>

#include <utki/string.hpp>

using namespace mikroxml;

namespace{
const std::string commentTag_c = "!--";
const std::string doctypeTag_c = "!DOCTYPE";
const std::string doctypeElementTag_c = "!ELEMENT";
const std::string doctypeAttlistTag_c = "!ATTLIST";
const std::string doctypeEntityTag_c = "!ENTITY";
const std::string cdata_tag = "![CDATA[";
}

parser::parser() {
	this->buf.reserve(256);
	this->name.reserve(256);
	this->refCharBuf.reserve(10);
}

malformed_xml::malformed_xml(unsigned line_number, const std::string& message) :
		std::logic_error([line_number, &message](){
			std::stringstream ss;
			ss << message << " line: " << line_number;
			return ss.str();
		}())
{}

void parser::feed(utki::span<const char> data) {
	for(auto i = data.begin(), e = data.end(); i != e; ++i){
		switch(this->state){
			case State_e::IDLE:
				this->parseIdle(i, e);
				break;
			case State_e::TAG:
				this->parseTag(i, e);
				break;
			case State_e::TAG_EMPTY:
				this->parseTagEmpty(i, e);
				break;
			case State_e::TAG_SEEK_GT:
				this->parseTagSeekGt(i, e);
				break;
			case State_e::DECLARATION:
				this->parseDeclaration(i, e);
				break;
			case State_e::DECLARATION_END:
				this->parseDeclarationEnd(i, e);
				break;
			case State_e::COMMENT:
				this->parseComment(i, e);
				break;
			case State_e::COMMENT_END:
				this->parseCommentEnd(i, e);
				break;
			case State_e::ATTRIBUTES:
				this->parseAttributes(i, e);
				break;
			case State_e::ATTRIBUTE_NAME:
				this->parseAttributeName(i, e);
				break;
			case State_e::ATTRIBUTE_SEEK_TO_EQUALS:
				this->parseAttributeSeekToEquals(i, e);
				break;
			case State_e::ATTRIBUTE_SEEK_TO_VALUE:
				this->parseAttributeSeekToValue(i, e);
				break;
			case State_e::ATTRIBUTE_VALUE:
				this->parseAttributeValue(i, e);
				break;
			case State_e::CONTENT:
				this->parseContent(i, e);
				break;
			case State_e::REF_CHAR:
				this->parseRefChar(i, e);
				break;
			case State_e::DOCTYPE:
				this->parseDoctype(i, e);
				break;
			case State_e::DOCTYPE_BODY:
				this->parseDoctypeBody(i, e);
				break;
			case State_e::DOCTYPE_TAG:
				this->parseDoctypeTag(i, e);
				break;
			case State_e::DOCTYPE_SKIP_TAG:
				this->parseDoctypeSkipTag(i, e);
				break;
			case State_e::DOCTYPE_ENTITY_NAME:
				this->parseDoctypeEntityName(i, e);
				break;
			case State_e::DOCTYPE_ENTITY_SEEK_TO_VALUE:
				this->parseDoctypeEntitySeekToValue(i, e);
				break;
			case State_e::DOCTYPE_ENTITY_VALUE:
				this->parseDoctypeEntityValue(i, e);
				break;
			case State_e::SKIP_UNKNOWN_EXCLAMATION_MARK_CONSTRUCT:
				this->parseSkipUnknownExclamationMarkConstruct(i, e);
				break;
			case State_e::cdata:
				this->parse_cdata(i, e);
				break;
			case State_e::cdata_terminator:
				this->parse_cdata_terminator(i, e);
				break;
		}
		if(i == e){
			return;
		}
	}
}

void parser::processParsedRefChar(){
	this->state = this->stateAfterRefChar;
	
	if(this->refCharBuf.size() == 0){
		return;
	}
	
	if(this->refCharBuf[0] == '#'){ // numeric character reference
		this->refCharBuf.push_back(0); // null-terminate

		char* endPtr;
		char* startPtr = &*(++this->refCharBuf.begin());
		int base;
		if(*startPtr == 'x'){ // hexadecimal format
			base = 16;
			++startPtr;
		}else{ // decimal format
			base = 10;
		}

		std::uint32_t unicode = std::strtoul(startPtr, &endPtr, base);
		if(endPtr != &*this->refCharBuf.rbegin()){
			std::stringstream ss;
			ss << "Unknown numeric character reference encountered: " << &*(++this->refCharBuf.begin());
			throw malformed_xml(this->lineNumber, ss.str());
		}
		auto utf8 = utki::to_utf8(char32_t(unicode));
		for(auto i = utf8.begin(), e = utf8.end(); *i != '\0' && i != e; ++i){
			this->buf.push_back(*i);
		}
	}else{ // character name reference
		std::string refCharString(this->refCharBuf.data(), this->refCharBuf.size());
		
		auto i = this->doctypeEntities.find(refCharString);
		if(i != this->doctypeEntities.end()){
			this->buf.insert(std::end(this->buf), std::begin(i->second), std::end(i->second));
		}else if(std::string("amp") == refCharString){
			this->buf.push_back('&');
		}else if(std::string("lt") == refCharString){
			this->buf.push_back('<');
		}else if(std::string("gt") == refCharString){
			this->buf.push_back('>');
		}else if(std::string("quot") == refCharString){
			this->buf.push_back('"');
		}else if(std::string("apos") == refCharString){
			this->buf.push_back('\'');
		}else{
			std::stringstream ss;
			ss << "Unknown name character reference encountered: " << this->refCharBuf.data();
			throw malformed_xml(this->lineNumber, ss.str());
		}
	}
	
	this->refCharBuf.clear();
}

void parser::parseRefChar(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e){
	for(; i != e; ++i){
		switch(*i){
			case ';':
				this->processParsedRefChar();
				return;
			case '\n':
				++this->lineNumber;
				// fall-through
			default:
				this->refCharBuf.push_back(*i);
				break;
		}
	}
}

void parser::parseTagEmpty(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e) {
	ASSERT(this->buf.size() == 0)
	ASSERT(this->name.size() == 0)
	for(; i != e; ++i){
		switch(*i){
			case '>':
				this->on_attributes_end(true);
				this->on_element_end(utki::make_span<char>(nullptr, 0));
				this->state = State_e::IDLE;
				return;
			default:
				throw malformed_xml(this->lineNumber, "Unexpected '/' character in attribute list encountered.");
		}
	}
}

void parser::parseContent(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '<':
				this->on_content_parsed(utki::make_span(this->buf));
				this->buf.clear();
				this->state = State_e::TAG;
				return;
			case '&':
				ASSERT(this->refCharBuf.size() == 0)
				this->stateAfterRefChar = this->state;
				this->state = State_e::REF_CHAR;
				return;
			case '\r':
				// ignore
				break;
			case '\n':
				++this->lineNumber;
				// fall-through
			default:
				this->buf.push_back(*i);
				break;
		}
	}
}

void parser::handleAttributeParsed() {
	this->on_attribute_parsed(utki::make_span(this->name), utki::make_span(this->buf));
	this->name.clear();
	this->buf.clear();
	this->state = State_e::ATTRIBUTES;
}

void parser::parseAttributeValue(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e) {
	ASSERT(this->name.size() != 0)
	for(; i != e; ++i){
		switch(*i){
			case '\'':
				if(this->attrValueQuoteChar == '\''){
					this->handleAttributeParsed();
					return;
				}
				this->buf.push_back(*i);
				break;
			case '"':
				if(this->attrValueQuoteChar == '"'){
					this->handleAttributeParsed();
					return;
				}
				this->buf.push_back(*i);
				break;
			case '&':
				ASSERT(this->refCharBuf.size() == 0)
				this->stateAfterRefChar = this->state;
				this->state = State_e::REF_CHAR;
				return;
			case '\r':
				// ignore
				break;
			case '\n':
				++this->lineNumber;
				// fall-through
			default:
				this->buf.push_back(*i);
				break;
		}
	}
}

void parser::parseAttributeSeekToValue(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->lineNumber;
				// fall-through
			case ' ':
			case '\t':
			case '\r':
				break;
			case '\'':
				this->attrValueQuoteChar = '\'';
				this->state = State_e::ATTRIBUTE_VALUE;
				return;
			case '"':
				this->attrValueQuoteChar = '"';
				this->state = State_e::ATTRIBUTE_VALUE;
				return;
			default:
				throw malformed_xml(this->lineNumber, "Unexpected character encountered, expected \"'\" or '\"'.");
		}
	}
}

void parser::parseAttributeSeekToEquals(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->lineNumber;
				// fall-through
			case ' ':
			case '\t':
			case '\r':
				break;
			case '=':
				ASSERT(this->name.size() != 0)
				ASSERT(this->buf.size() == 0)
				this->state = State_e::ATTRIBUTE_SEEK_TO_VALUE;
				return;
			default:
				{
					std::stringstream ss;
					ss << "Unexpected character encountered (0x" << std::hex << unsigned(*i) << "), expected '='";
					throw malformed_xml(this->lineNumber, ss.str());
				}
		}
	}
}

void parser::parseAttributeName(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->lineNumber;
				// fall-through
			case ' ':
			case '\t':
			case '\r':
				ASSERT(this->name.size() != 0)
				this->state = State_e::ATTRIBUTE_SEEK_TO_EQUALS;
				return;
			case '=':
				ASSERT(this->buf.size() == 0)
				this->state = State_e::ATTRIBUTE_SEEK_TO_VALUE;
				return;
			default:
				this->name.push_back(*i);
				break;
		}
	}
}

void parser::parseAttributes(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e) {
	ASSERT(this->buf.size() == 0)
	ASSERT(this->name.size() == 0)
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->lineNumber;
				// fall-through
			case ' ':
			case '\t':
			case '\r':
				break;
			case '/':
				this->state = State_e::TAG_EMPTY;
				return;
			case '>':
				this->on_attributes_end(false);
				this->state = State_e::IDLE;
				return;
			case '=':
				throw malformed_xml(this->lineNumber, "unexpected '=' encountered");
			default:
				this->name.push_back(*i);
				this->state = State_e::ATTRIBUTE_NAME;
				return;
		}
	}
}

void parser::parseComment(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '-':
				this->state = State_e::COMMENT_END;
				return;
			case '\n':
				++this->lineNumber;
				// fall-through
			default:
				break;
		}
	}
}

void parser::parseCommentEnd(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->lineNumber;
				// fall-through
			default:
				this->buf.clear();
				this->state = State_e::COMMENT;
				return;
			case '-':
				if(this->buf.size() == 1){
					this->buf.clear();
					this->state = State_e::COMMENT;
					return;
				}
				ASSERT(this->buf.size() == 0)
				this->buf.push_back('-');
				break;
			case '>':
				if(this->buf.size() == 1){
					this->buf.clear();
					this->state = State_e::IDLE;
					return;
				}
				ASSERT(this->buf.size() == 0)
				this->buf.clear();
				this->state = State_e::COMMENT;
				return;
		}
	}
}

void parser::end(){
	if(this->state != State_e::IDLE){
		std::array<char, 1> newLine = {{'\n'}};
		this->feed(utki::make_span(newLine));
	}
}

namespace{
bool startsWith(const std::vector<char>& vec, const std::string& str){
	if(vec.size() < str.size()){
		return false;
	}
	
	for(unsigned i = 0; i != str.size(); ++i){
		if(str[i] != vec[i]){
			return false;
		}
	}
	return true;
}
}

void parser::processParsedTagName() {
	if(this->buf.size() == 0){
		throw malformed_xml(this->lineNumber, "tag name cannot be empty");
	}
	
	switch(this->buf[0]){
		case '?':
			// some declaration, we just skip it.
			this->buf.clear();
			this->state = State_e::DECLARATION;
			return;
		case '!':
//			TRACE(<< "this->buf = " << std::string(&*this->buf.begin(), this->buf.size()) << std::endl)
			if(startsWith(this->buf, doctypeTag_c)){
				this->state = State_e::DOCTYPE;
			}else{
				this->state = State_e::SKIP_UNKNOWN_EXCLAMATION_MARK_CONSTRUCT;
			}
			this->buf.clear();
			return;
		case '/':
			if(this->buf.size() <= 1){
				throw malformed_xml(this->lineNumber, "end tag cannot be empty");
			}
			this->on_element_end(utki::make_span(&*(++this->buf.begin()), this->buf.size() - 1));
			this->buf.clear();
			this->state = State_e::TAG_SEEK_GT;
			return;
		default:
			this->on_element_start(utki::make_span(this->buf));
			this->buf.clear();
			this->state = State_e::ATTRIBUTES;
			return;
	}
}

void parser::parseTag(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->lineNumber;
				// fall-through
			case ' ':
			case '\t':
			case '\r':
				this->processParsedTagName();
				return;
			case '>':
				this->processParsedTagName();
				switch(this->state){
					case State_e::ATTRIBUTES:
						this->on_attributes_end(false);
						// fall-through
					default:
						this->state = State_e::IDLE;
						break;
				}
				return;
			case '[':
				this->buf.push_back(*i);
				if(this->buf.size() == cdata_tag.size() && startsWith(this->buf, cdata_tag)){
					this->buf.clear();
					this->state = State_e::cdata;
					return;
				}
				break;
			case '-':
				this->buf.push_back(*i);
				if(this->buf.size() == commentTag_c.size() && startsWith(this->buf, commentTag_c)){
					this->state = State_e::COMMENT;
					this->buf.clear();
					return;
				}
				break;
			case '/':
				if(this->buf.size() != 0){
					this->processParsedTagName();

					// After parsing usual tag we expect attributes, but since we got '/' the tag has no any attributes, so it is empty.
					// In other cases, like '!DOCTYPE' tag the state should remain.
					if(this->state == State_e::ATTRIBUTES){
						this->state = State_e::TAG_EMPTY;
					}
					return;
				}
				// fall-through
			default:
				this->buf.push_back(*i);
				break;
		}
	}
}

void parser::parseDoctype(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '>':
				ASSERT(this->buf.size() == 0)
				this->state = State_e::IDLE;
				return;
			case '[':
				this->state = State_e::DOCTYPE_BODY;
				return;
			case '\n':
				++this->lineNumber;
				// fall-through
			default:
				break;
		}
	}
}

void parser::parseDoctypeBody(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case ']':
				ASSERT(this->buf.size() == 0)
				this->state = State_e::DOCTYPE;
				return;
			case '<':
				this->state = State_e::DOCTYPE_TAG;
				return;
			case '\n':
				++this->lineNumber;
				// fall-through
			default:
				break;
		}
	}
}

void parser::parseDoctypeTag(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->lineNumber;
				// fall-through
			case ' ':
			case '\t':
			case '\r':
				if(this->buf.size() == 0){
					throw malformed_xml(this->lineNumber, "Empty DOCTYPE tag name encountered");
				}
				
				if(
						startsWith(this->buf, doctypeElementTag_c) ||
						startsWith(this->buf, doctypeAttlistTag_c)
				){
					this->state = State_e::DOCTYPE_SKIP_TAG;
				}else if(startsWith(this->buf, doctypeEntityTag_c)){
					this->state = State_e::DOCTYPE_ENTITY_NAME;
				}else{
					throw malformed_xml(this->lineNumber, "Unknown DOCTYPE tag encountered");
				}
				this->buf.clear();
				return;
			case '>':
				throw malformed_xml(this->lineNumber, "Unexpected > character while parsing DOCTYPE tag");
			default:
				this->buf.push_back(*i);
				break;
		}
	}
}

void parser::parseDoctypeSkipTag(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '>':
				ASSERT(this->buf.size() == 0)
				this->state = State_e::DOCTYPE_BODY;
				return;
			case '\n':
				++this->lineNumber;
				// fall-through
			default:
				break;
		}
	}
}

void parser::parseDoctypeEntityName(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->lineNumber;
				// fall-through
			case ' ':
			case '\t':
			case '\r':
				if(this->buf.size() == 0){
					break;
				}
				
				this->name = std::move(this->buf);
				ASSERT(this->buf.size() == 0)
				
				this->state = State_e::DOCTYPE_ENTITY_SEEK_TO_VALUE;
				return;
			default:
				this->buf.push_back(*i);
				break;
		}
	}
}

void parser::parseDoctypeEntitySeekToValue(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->lineNumber;
				// fall-through
			case ' ':
			case '\t':
			case '\r':
				break;
			case '"':
				this->state = State_e::DOCTYPE_ENTITY_VALUE;
				return;
			default:
				throw malformed_xml(this->lineNumber, "Unexpected character encountered while seeking to DOCTYPE entity value, expected '\"'.");
		}
	}
}

void parser::parseDoctypeEntityValue(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '"':
				this->doctypeEntities.insert(std::make_pair(utki::make_string(this->name), std::move(this->buf)));
				
				this->name.clear();
				
				ASSERT(this->buf.size() == 0)
				
				this->state = State_e::DOCTYPE_SKIP_TAG;
				return;
			case '\n':
				++this->lineNumber;
				// fall-through
			default:
				this->buf.push_back(*i);
				break;
		}
	}
}

void parser::parseSkipUnknownExclamationMarkConstruct(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '>':
				ASSERT(this->buf.size() == 0)
				this->state = State_e::IDLE;
				return;
			case '\n':
				++this->lineNumber;
				// fall-through
			default:
				break;
		}
	}
}

void parser::parseTagSeekGt(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->lineNumber;
				// fall-through
			case ' ':
			case '\t':
			case '\r':
				break;
			case '>':
				this->state = State_e::IDLE;
				return;
			default:
				{
					std::stringstream ss;
					ss << "Unexpected character encountered (" << *i << "), expected '>'.";
					throw malformed_xml(this->lineNumber, ss.str());
				}
		}
	}
}

void parser::parseDeclaration(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '?':
				this->state = State_e::DECLARATION_END;
				return;
			case '\n':
				++this->lineNumber;
				// fall-through
			default:
				break;
		}
	}
}

void parser::parseDeclarationEnd(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '>':
				this->state = State_e::IDLE;
				return;
			case '\n':
				++this->lineNumber;
				// fall-through
			default:
				this->state = State_e::DECLARATION;
				return;
		}
	}
}

void parser::parseIdle(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '<':
				this->state = State_e::TAG;
				return;
			case '&':
				this->stateAfterRefChar = State_e::CONTENT;
				this->state = State_e::REF_CHAR;
				return;
			case '\r':
				// ignore
				break;
			case '\n':
				++this->lineNumber;
				// fall-through
			default:
				ASSERT(this->buf.size() == 0)
				this->buf.push_back(*i);
				this->state = State_e::CONTENT;
				return;
		}
	}
}

void parser::feed(const std::string& str){
	this->feed(utki::make_span(str.c_str(), str.length()));
}

void parser::parse_cdata(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e){
	for(; i != e; ++i){
		switch(*i){
			case ']':
				this->buf.push_back(*i);
				this->state = State_e::cdata_terminator;
				return;
			default:
				this->buf.push_back(*i);
				break;
		}
	}
}

void parser::parse_cdata_terminator(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e){
	for(; i != e; ++i){
		switch(*i){
			case ']':
				ASSERT(!this->buf.empty())
				ASSERT(this->buf.back() == ']')
				this->buf.push_back(']');
				break;
			case '>':
				ASSERT(!this->buf.empty())
				ASSERT(this->buf.back() == ']')
				if(this->buf.size() < 2 || this->buf[this->buf.size() - 2] != ']'){
					this->buf.push_back('>');
					this->state = State_e::cdata;
				}else{ // CDATA block ended
					this->on_content_parsed(utki::make_span(this->buf.data(), this->buf.size() - 2));
					this->buf.clear();
					this->state = State_e::IDLE;
				}
				return;
			default:
				this->buf.push_back(*i);
				this->state = State_e::cdata;
				return;
		}
	}
}
