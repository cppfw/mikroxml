#include "mikroxml.hpp"

#include <sstream>

using namespace mikroxml;

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
		}
		if(i == e){
			return;
		}
	}
}

void Parser::parseAttributes(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->lineNumber;
				//fall-through
			case ' ':
			case '\t':
				break;
			case '>':
				ASSERT(this->buf.size() == 0)
				this->state = State_e::IDLE;
				return;
			default:
				break;
		}
	}
}


void Parser::parseComment(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e) {
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->lineNumber;
				break;
			case '-':
				this->state = State_e::COMMENT_END;
				return;
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
			if(this->buf.size() == 4){
				if(this->buf[1] == 'x' && this->buf[2] == 'm' && this->buf[3] == 'l'){
					this->buf.clear();
					this->state = State_e::DECLARATION;
					return;
				}
			}
			throw MalformedDocumentExc(this->lineNumber, "Unknown declaration opening tag, should be <?xml.");
		case '!':
			//comment or whatever
			if(this->buf.size() == 3){
				if(this->buf[1] == '-' && this->buf[2] == '-'){
					this->buf.clear();
					this->state = State_e::COMMENT;
					return;
				}
			}
			throw MalformedDocumentExc(this->lineNumber, "Unknown <! construct encountered, only comments <!-- --> are supported.");
		default:
			this->onElementStart(utki::wrapBuf(this->buf));
			this->elementNameStack.push_back(std::string(&*this->buf.begin(), this->buf.size()));
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
				this->processParsedTagName();
				return;
			case '>':
				this->processParsedTagName();
				if(this->state == State_e::ATTRIBUTES){
					this->state = State_e::IDLE;
				}
				return;
			default:
				this->buf.push_back(*i);
				break;
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
			case ' ':
			case '\t':
				break;
			case '\n':
				++this->lineNumber;
				break;
			case '<':
				this->state = State_e::TAG;
				return;
			default:
				{
					std::stringstream ss;
					ss << "Unexpected character encountered: " << char(*i) << ", expected '<'.";
					throw MalformedDocumentExc(this->lineNumber, ss.str());
				}
		}
	}
}
