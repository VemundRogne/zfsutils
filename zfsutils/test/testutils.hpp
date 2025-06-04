#include <fcntl.h>
#include <fstream>

namespace testutils {

void write_string_to_file(std::string directory, std::string filename,
                          std::string content);
}
