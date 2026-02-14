#ifndef MODAPI_DLL_EXPORT
#include <Spore\Pollinator\cAssetMetadata.h>
#include "FixGAPropcAssetMetadata.h"

namespace Pollinator
{
	//// cAssetMetadata ////

	auto_METHOD(cAssetMetadata, bool, ParseTagString, Args(const char16_t* tag), Args(tag));
}
#endif