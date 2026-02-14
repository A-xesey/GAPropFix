#pragma once
#include <Spore/Pollinator/cAssetMetadata.h>

namespace Pollinator
{
	class cAssetMetadata
	{	
	public:
		bool ParseTagString(cAssetMetadata, const char16_t* tag);
	};
	namespace Addresses(cAssetMetadata)
	{
		DeclareAddress(ParseTagString);
	}
}
