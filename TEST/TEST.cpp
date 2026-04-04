#include "MyAlgorithm.hpp"
#include <stdio.h>

int main(void)
{
	std::vector<size_t> strInput;
	strInput.reserve(1000010);
	int c;
	while ((c = getchar()) != EOF && c != '\n')
	{
		strInput.push_back((size_t)c);
	}

	auto ret = DoublingCountingRadixSortSuffixArray(INT8_MAX, strInput);

	for (auto &it : ret)
	{
		printf("%zu ", it);
	}

	putchar('\n');
	return 0;
}
