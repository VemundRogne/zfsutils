#include <fcntl.h>
#include <fstream>
#include <iostream>

namespace testutils {

/* This func should _not_ handle any potential exceptions; the test itself
 * should be responsible for that
 */
void write_string_to_file(std::string directory, std::string filename,
                          std::string content) {
    std::ofstream outFile;
    outFile.exceptions(std::ofstream::badbit | std::ofstream::failbit);
    outFile.open(directory + "/" + filename);
    outFile << content << std::endl;
    outFile.close();
}

} // namespace testutils
