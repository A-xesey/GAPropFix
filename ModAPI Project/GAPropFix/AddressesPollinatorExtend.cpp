#ifdef MODAPI_DLL_EXPORT
#include "stdafx.h"
#include <Spore\Pollinator\cAssetMetadata.h>
#include "FixGAPropcAssetMetadata.h"

namespace Pollinator
{
	namespace Addresses(cAssetMetadata)
	{
		DefineAddress(ParseTagString, SelectAddress(0x550bd0, 0x550bd0));
	}
}
#endif