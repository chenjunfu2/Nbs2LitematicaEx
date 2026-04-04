#include "MyAlgorithm.hpp"
#include <stdio.h>

int main(void)
{
	std::vector<uint8_t> strInput;
	strInput.reserve(1000010);
	int c;
	while ((c = getchar()) != EOF && c != '\n')
	{
		strInput.push_back(c);
	}

	auto ret = DoublingCountingRadixSortSuffixArray(INT8_MAX, strInput);

	for (auto &it : ret)
	{
		printf("%zu ", it);
	}

	putchar('\n');
	return 0;
}
