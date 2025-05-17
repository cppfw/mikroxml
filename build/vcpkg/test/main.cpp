#include <mikroxml/mikroxml.hpp>

int main(int argc, const char** argv){
    try{
        throw mikroxml::malformed_xml(10, "file.xml");
    }catch(std::exception& e){
        std::cout << "exception caught: " << e.what() << std::endl;
    }

    return 0;
}
