#include "mikroxml.hpp"

#include <sstream>

#include <unikod/utf8.hpp>

using namespace mikroxml;

Parser::Parser() {
	this->buf.reserve(256);
	this->attributeName.reserve(256);
	this->refCharBuf.reserve(10);
}


Parser::MalformedDocumentExc::MalformedDocumentExc(unsigned lineNumber, const std::string& message) :
		Exc([lineNumber, &message](){
			std::stringstream ss;
			ss << message << " Line: " << lineNumber;
			return ss.str();
		}())
{}


void Parser::feed(const utki::Buf<char> data) {
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
			case State_e::SKIP_EXCLAMATION_MARK_DECLARATION:
				this->parseSkipExclamationMarkDeclaration(i, e);
				break;
		}
		if(i == e){
			return;
		}
	}
}

void Parser::processParsedRefChar() {
	this->state = this->stateAfterRefChar;
	
	if(this->refCharBuf.size() == 0){
		return;
	}
	
	if(this->refCharBuf[0] == '#'){
		//numeric character reference
		char* endPtr;
		char* startPtr = &*(++this->refCharBuf.begin());
		std::uint32_t unicode = std::strtoul(startPtr, &endPtr, 16);
		if(endPtr != startPtr + this->refCharBuf.size() - 1){
			std::stringstream ss;
			ss << "Unknown numeric character reference encountered: " << &*(++this->refCharBuf.begin());
			throw MalformedDocumentExc(this->lineNumber, ss.str());
		}
		auto utf8 = unikod::toUtf8(char32_t(unicode));
		for(auto i = utf8.begin(), e = utf8.end(); *i != '\0' && i != e; ++i){
			this->buf.push_back(*i);
		}
	}else{
		//character name reference
		this->refCharBuf.push_back('\0'); //zero-terminate
		if(std::string("amp") == &*this->refCharBuf.begin()){
			this->buf.push_back('&');
		}else if(std::string("lt") == &*this->refCharBuf.begin()){
			this->buf.push_back('<');
		}else if(std::string("gt") == &*this->refCharBuf.begin()){
			this->buf.push_back('>');
		}else if(std::string("quot") == &*this->refCharBuf.begin()){
			this->buf.push_back('"');
		}else if(std::string("apos") == &*this->refCharBuf.begin()){
			this->buf.push_back('\'');
		}else{
			std::stringstream ss;
			ss << "Unknown name character reference encountered: " << &*this->refCharBuf.begin();
			throw MalformedDocumentExc(this->lineNumber, ss.str());
		}
	}
	
	this->refCharBuf.clear();
}


void Parser::parseRefChar(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case ';':
				this->processParsedRefChar();
				return;
			case '\n':
				++this->lineNumber;
				//fall-through
			default:
				this->refCharBuf.push_back(*i);
				break;
		}
	}
}


void Parser::parseTagEmpty(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e) {
	ASSERT(this->buf.size() == 0)
	ASSERT(this->attributeName.size() == 0)
	for(; i != e; ++i){
		switch(*i){
			case '>':
				this->onAttributesEnd(true);
				this->onElementEnd(utki::wrapBuf<char>(nullptr, 0));
				this->state = State_e::IDLE;
				return;
			default:
				throw MalformedDocumentExc(this->lineNumber, "Unexpected '/' character in attribute list encountered.");
		}
	}
}


void Parser::parseContent(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '<':
				this->onContentParsed(utki::wrapBuf(this->buf));
				this->buf.clear();
				this->state = State_e::TAG;
				return;
			case '&':
				ASSERT(this->refCharBuf.size() == 0)
				this->stateAfterRefChar = this->state;
				this->state = State_e::REF_CHAR;
				return;
			case '\r':
				//ignore
				break;
			case '\n':
				++this->lineNumber;
				//fall-through
			default:
				this->buf.push_back(*i);
				break;
		}
	}
}


void Parser::handleAttributeParsed() {
	this->onAttributeParsed(utki::wrapBuf(this->attributeName), utki::wrapBuf(this->buf));
	this->attributeName.clear();
	this->buf.clear();
	this->state = State_e::ATTRIBUTES;
}


void Parser::parseAttributeValue(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e) {
	ASSERT(this->attributeName.size() != 0)
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
				//ignore
				break;
			case '\n':
				++this->lineNumber;
				//fall-through
			default:
				this->buf.push_back(*i);
				break;
		}
	}
}

void Parser::parseAttributeSeekToValue(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->lineNumber;
				//fall-through
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
				throw MalformedDocumentExc(this->lineNumber, "Unexpected character encountered, expected \"'\" or '\"'.");
		}
	}
}


void Parser::parseAttributeSeekToEquals(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->lineNumber;
				//fall-through
			case ' ':
			case '\t':
			case '\r':
				break;
			case '=':
				ASSERT(this->attributeName.size() != 0)
				ASSERT(this->buf.size() == 0)
				this->state = State_e::ATTRIBUTE_SEEK_TO_VALUE;
				return;
			default:
				{
					std::stringstream ss;
					ss << "Unexpected character encountered (0x" << std::hex << unsigned(*i) << "), expected '='";
					throw MalformedDocumentExc(this->lineNumber, ss.str());
				}
		}
	}
}



void Parser::parseAttributeName(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->lineNumber;
				//fall-through
			case ' ':
			case '\t':
			case '\r':
				ASSERT(this->attributeName.size() != 0)
				this->state = State_e::ATTRIBUTE_SEEK_TO_EQUALS;
				return;
			case '=':
				ASSERT(this->buf.size() == 0)
				this->state = State_e::ATTRIBUTE_SEEK_TO_VALUE;
				return;
			default:
				this->attributeName.push_back(*i);
				break;
		}
	}
}


void Parser::parseAttributes(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e) {
	ASSERT(this->buf.size() == 0)
	ASSERT(this->attributeName.size() == 0)
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->lineNumber;
				//fall-through
			case ' ':
			case '\t':
			case '\r':
				break;
			case '/':
				this->state = State_e::TAG_EMPTY;
				return;
			case '>':
				this->onAttributesEnd(false);
				this->state = State_e::IDLE;
				return;
			case '=':
				throw MalformedDocumentExc(this->lineNumber, "unexpected '=' encountered");
			default:
				this->attributeName.push_back(*i);
				this->state = State_e::ATTRIBUTE_NAME;
				return;
		}
	}
}


void Parser::parseComment(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '-':
				this->state = State_e::COMMENT_END;
				return;
			case '\n':
				++this->lineNumber;
				//fall-through
			default:
				break;
		}
	}
}

void Parser::parseCommentEnd(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->lineNumber;
				//fall-through
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



void Parser::end() {
	if(this->state != State_e::IDLE){
		std::array<char, 1> newLine = {{'\n'}};
		this->feed(utki::wrapBuf(newLine));
	}
}

void Parser::processParsedTagName() {
	if(this->buf.size() == 0){
		throw MalformedDocumentExc(this->lineNumber, "tag name cannot be empty");
	}
	
	switch(this->buf[0]){
		case '?':
			//some declaration, we just skip it.
			this->buf.clear();
			this->state = State_e::DECLARATION;
			return;
		case '!':
			//comment or whatever
			if(this->buf.size() >= 3){
				if(this->buf[1] == '-' && this->buf[2] == '-'){
					this->buf.clear();
					this->state = State_e::COMMENT;
					return;
				}
			}
			//Unknown ! construct, just skip it
			this->buf.clear();
			this->state = State_e::SKIP_EXCLAMATION_MARK_DECLARATION;
			return;
		case '/':
			if(this->buf.size() <= 1){
				throw MalformedDocumentExc(this->lineNumber, "end tag cannot be empty");
			}
			this->onElementEnd(utki::wrapBuf(&*(++this->buf.begin()), this->buf.size() - 1));
			this->buf.clear();
			this->state = State_e::TAG_SEEK_GT;
			return;
		default:
			this->onElementStart(utki::wrapBuf(this->buf));
			this->buf.clear();
			this->state = State_e::ATTRIBUTES;
			return;
	}
}


void Parser::parseTag(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->lineNumber;
				//fall-through
			case ' ':
			case '\t':
			case '\r':
				this->processParsedTagName();
				return;
			case '>':
				this->processParsedTagName();
				switch(this->state){
					case State_e::ATTRIBUTES:
						this->onAttributesEnd(false);
						//fall-through
					case State_e::SKIP_EXCLAMATION_MARK_DECLARATION:
					case State_e::TAG_SEEK_GT:
						this->state = State_e::IDLE;
						break;
					default:
						break;
				}
				return;
			case '/':
				if(this->buf.size() != 0){
					this->processParsedTagName();
					this->state = State_e::TAG_EMPTY;
					return;
				}
				//fall-through
			default:
				this->buf.push_back(*i);
				break;
		}
	}
}

void Parser::parseSkipExclamationMarkDeclaration(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '>':
				ASSERT(this->buf.size() == 0)
				this->state = State_e::IDLE;
				return;
			case '\n':
				++this->lineNumber;
				//fall-through
			default:
				break;
		}
	}
}


void Parser::parseTagSeekGt(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->lineNumber;
				//fall-through
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
					throw MalformedDocumentExc(this->lineNumber, ss.str());
				}
		}
	}
}


void Parser::parseDeclaration(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '?':
				this->state = State_e::DECLARATION_END;
				return;
			case '\n':
				++this->lineNumber;
				//fall-through
			default:
				break;
		}
	}
}

void Parser::parseDeclarationEnd(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '>':
				this->state = State_e::IDLE;
				return;
			case '\n':
				++this->lineNumber;
				//fall-through
			default:
				this->state = State_e::DECLARATION;
				return;
		}
	}
}


void Parser::parseIdle(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e) {
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
				//ignore
				break;
			case '\n':
				++this->lineNumber;
				//fall-through
			default:
				ASSERT(this->buf.size() == 0)
				this->buf.push_back(*i);
				this->state = State_e::CONTENT;
				return;
		}
	}
}



void Parser::feed(const std::string& str){
	this->feed(utki::wrapBuf(str.c_str(), str.length()));
}
