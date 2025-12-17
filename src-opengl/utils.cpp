#include "utils.h"
#include "common.h"
#include <fstream>

namespace ntt {

std::string readFile(const std::string& filepath)
{
	std::ifstream fileStream(filepath, std::ios::in);
	ASSERT(fileStream.is_open());

	std::string content;
	std::string line;
	while (std::getline(fileStream, line))
	{
		content += line + "\n";
	}

	fileStream.close();
	return content;
}

} // namespace ntt