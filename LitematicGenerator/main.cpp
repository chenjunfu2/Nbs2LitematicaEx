#include "LitematicGenerator.hpp"

int main(void)
{
	auto ab = AirBlock{}.ToCompound();
	auto nb = NoteBlock{}.ToCompound();
	auto rb = RepeaterBlock{}.ToCompound();

	NBT_Print{}("AirBlock:\n");
	NBT_Helper::Print(ab);
	NBT_Print{}("\nNoteBlock:\n");
	NBT_Helper::Print(nb);
	NBT_Print{}("\nRepeaterBlock:\n");
	NBT_Helper::Print(rb);
	NBT_Print{}("\n");

	return 0;
}
