#include "../Nbs2LitematicEx/MyAlgorithm.hpp"
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

	const auto ret = DoublingCountingRadixSortSuffixArray(INT8_MAX, strInput);
	const auto ret2 = HeightArray(strInput, ret);

	for (auto &it : ret.vSuffixArray)
	{
		printf("%zu ", it);
	}

	putchar('\n');

	for (auto &it : ret2)
	{
		printf("%zu ", it);
	}

	putchar('\n');
	return 0;
}
