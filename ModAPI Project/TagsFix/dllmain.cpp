// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "UpdateDataBase.h"
//#include "PollinatorClassPlaceholder.h"
//#include <Spore/GalaxyGameEntry/GlobalGGEUI.h>

using namespace Sporepedia::OTDB;

void Initialize()
{
	CheatManager.AddCheat("UpdateDataBase", new UpdateDataBase());
}

void Dispose()
{
	// This method is called when the game is closing
}

/// deleting space symbs between word
/// used to fix tags of creations that were downloaded through the game
void strip(eastl::string16& inputSrtring)
{
	auto start_it = inputSrtring.begin();
	auto end_it = inputSrtring.rbegin();
	while (*start_it == *u" ") ++start_it;
	if (start_it != inputSrtring.end()) {
		while (*end_it == *u" ") ++end_it;
	}
	inputSrtring = eastl::string16(start_it, end_it.base());
};

/// checks if creation has gaprop tag in pollen_metada file
bool HasGAPropMetadata(const ResourceKey& key)
{
	ResourceObjectPtr keyObject;
	ResourceManager.GetResource({ key.instanceID, TypeIDs::pollen_metadata, key.groupID }, &keyObject);
	if (keyObject != nullptr)	
	{
		cAssetMetadataPtr metaData = object_cast<Pollinator::cAssetMetadata>(keyObject);
		/*App::ConsolePrintF("-----------------------------------", metaData->mName);
		App::ConsolePrintF("Metadata: has gaprop. Name: %ls", metaData->mName);*/
		if (metaData != nullptr && (metaData->mTags.size() != 0))
		{
			for (const eastl::string16& tag : metaData->mTags)
			{
				auto start_it = tag.begin();
				auto end_it = tag.rbegin();
				while (*start_it == *u" ") ++start_it;
				if (start_it != tag.end()) {
					while (*end_it == *u" ") ++end_it;
				}
				if (eastl::string16(start_it, end_it.base()) == u"gaprop")
				{
					keyObject = nullptr;
					metaData = nullptr;
					return true;
				}
			}
		}
	}
	return false;
};

/// checks if creation has gaprop tag in summary file
bool HasGAPropSummary(const ResourceKey& key)
{
	ResourceObjectPtr keyObject;
	ResourceManager.GetResource({ key.instanceID, 0x02D5C9AF, key.groupID }, &keyObject);
	if (keyObject != nullptr)
	{
		ParameterResource* params = object_cast<ParameterResource>(keyObject);
		if (params != nullptr && params->mParameters.size() != 0)
		{
			for (const Parameter& param : params->mParameters)
			{
				//0xcd6e902c is gaprop param, 1 means that creation has gaprop tag
				if (param.paramID == 0xcd6e902c && param.valueInt == 1)
				{
					//App::ConsolePrintF("Summary: has gaprop");
					params = nullptr;
					return true;
				}
			}
		}
		params = nullptr;
	}
	return false;
};

/// checks if creation has gaprop in summary and/or pollen_metadata
/// since the donwloaded through the game creation often has a space symb on the tag, some specific check may not be performed
/// and parameter don't set in summary file. So, we need to check it in both files
bool HasGAProp(const ResourceKey& key)
{
	if (HasGAPropMetadata(key) || HasGAPropSummary(key)) return true;
	else return false;
};

/// func that returns the creation to new game
/// i noticed that there are no gaprop tag check when u choose random creation to the new game,
/// so i decided to add it
static_detour(anonymous_namespace_sGetRandomKey, ResourceKey* (ResourceKey*,const vector<ResourceKey>&, ResourceKey*))
{
	ResourceKey* detoured(ResourceKey * dst, const vector<ResourceKey>&AssetList, ResourceKey * GGEKey)
	{
		//recon code from the game
		if (AssetList.size() == 0)
		{
			dst->instanceID = 0;
			dst->typeID = 0;
			dst->groupID = 0;
			return dst;
		}
		/*else if (AssetList.size() == 1)
		{
			dst->instanceID = AssetList.mpBegin->instanceID;
			dst->typeID = AssetList.mpBegin->typeID;
			dst->groupID = AssetList.mpBegin->groupID;
			return dst;
		}*/
		else
		{
			ResourceKey key;
		goOutGAProp:
			do
			{
				int rando = rand(AssetList.size() - 1);
				key = AssetList[rando];
				if (key.instanceID != GGEKey->instanceID || key.typeID != GGEKey->typeID) break;
			} while (key.groupID == GGEKey->groupID);

			dst->instanceID = key.instanceID;
			dst->typeID = key.typeID;
			dst->groupID = key.groupID;

			//if creation has gaprop, then we go back to while-loop
			if (HasGAProp(*dst))
				goto goOutGAProp;
			/*else
				App::ConsolePrintF("creation 0x%x without gaprop tag", dst->instanceID);*/

			return dst;
		}
	}
};

/// the main func that used to make summary file to new/imported/donwload creation
/// detoured it to fix the tag space symb bug, also it's used in the ReIndexIfNecessary (Write) function
/// that need to update database content (basically Pollination file i think)
member_detour(cMetadataSummarizer_ExctractParameters, ISummarizer, bool(const ResourceKey&, eastl::vector<Parameter>&))
{
	bool detoured(const ResourceKey & key, eastl::vector<Parameter>&dst)
	{
		// here we're rewrite tags...
		ResourceObjectPtr object;
		ResourceManager.GetResource({ key.instanceID, TypeIDs::pollen_metadata, key.groupID }, &object);
		if (object != nullptr)
		{
			cAssetMetadataPtr asset = object_cast<Pollinator::cAssetMetadata>(object);
			if (asset != nullptr && asset->mTags.size() != 0)
			{
				eastl::vector<eastl::string16> newTags;
				for (eastl::string16& tag : asset->mTags)
				{
					// deleting spaces between tag
					//App::ConsolePrintF("old tag: \"%ls\"", tag);
					strip(tag);
					//App::ConsolePrintF("new tag: \"%ls\"", tag);
					newTags.push_back(tag);
				}
				// clear tags, assing new tags and then rewrite pollen_metadata file (idk why i add cache func i'm silly)
				asset->mTags.clear();
				asset->mTags = newTags;
				ResourceManager.WriteResource(asset.get(), nullptr, ResourceManager.FindDatabase(asset->mAssetKey));
				ResourceManager.CacheResource(asset.get(), true);
				//App::ConsolePrintF("cMetadataSummarizer_ExctractParameters: done rewriting");
			}
		}
		// ...and then we make/remake creation summary file
		return original_function(this, key, dst);
	}
};

/// detoured it before to fix gaprop tag issue, but then i released that's doesn't work
//eastl::vector<ResourceKey> CreateValidList(eastl::vector<ResourceKey>& oldList, const eastl::string16& text)
//{
//	if (oldList.size() != 0 && GameModeManager.GetActiveModeID() != GameModeIDs::kScenarioMode)
//	{
//		eastl::vector<ResourceKey> ValidAssetList;
//		for (const ResourceKey& key : oldList)
//		{
//			if (HasGaProp(key)) ValidAssetList.push_back(key);
//		}
//		if (ValidAssetList.size() != 0)
//		{
//			if (text.begin() != text.end())
//				App::ConsolePrintF("%ls ValidAssetList exists and refine the assetList to valid (no gaprop)", text);
//			return ValidAssetList;
//		}
//	}
//	return oldList;
//}
//
//member_detour(GetAssetList, cObjectTemplateDB, bool(eastl::vector<ResourceKey>&, uint32_t, eastl::vector<QueryParameter>&))
//{
//	bool detoured(eastl::vector<ResourceKey>& dst, uint32_t maybeModelType, eastl::vector<QueryParameter>& creationSummary)
//	{
//		bool res = original_function(this, dst, maybeModelType, creationSummary);
//		/*if (res)
//		{
//			auto help = CreateValidList(dst, u"GetAssetListGeneral");
//			dst = help;
//		}*/
//		return res;
//	}
//};
//
//member_detour(GetAssetListUnknown, cObjectTemplateDB, bool(eastl::vector<ResourceKey>&, eastl::vector<QueryParameter>&))
//{
//	bool detoured(eastl::vector<ResourceKey>&dst, eastl::vector<QueryParameter>&creationSummary)
//	{
//		bool res = original_function(this, dst, creationSummary);
//		if (res)
//		{
//			auto help = CreateValidList(dst, u"GetAssetListUnknown");
//			dst = help;
//		}
//		return res;
//	}
//};
//
//member_detour(GetMatchedAssetList, cObjectTemplateDB, bool(eastl::vector<ResourceKey>&, uint32_t, ResourceKey&, eastl::vector<QueryParameter>&))
//{
//	bool detoured(eastl::vector<ResourceKey>&dst, uint32_t maybeModelType, ResourceKey & creation, eastl::vector<QueryParameter>&creationSummary)
//	{
//		bool res = original_function(this, dst, maybeModelType, creation, creationSummary);
//		if (res)
//		{
//			auto help = CreateValidList(dst, u"GetMatchedAssetList");
//			dst = help;
//		}
//		return res;
//	}
//};
//member_detour(cAssetMetadata_GetTag, Pollinator::cAssetMetadata, const char16_t* (int))
//{
//	const char16_t* detoured(int index)
//	{
//		if (this->mTags.size() != 0)
//		{
//			/*eastl::string16 newTags = u"";
//			for (const eastl::string16& tag : this->mTags)
//			{
//				bool start = std::isspace(*tag.begin());
//				bool end = std::isspace(*tag.rbegin());
//				if (start || end) newTags += strip(tag);
//				else newTags += tag;
//
//				if (tag != *this->mTags.mpEnd)
//					newTags += + u",";
//			}
//			newTags += *u",gettagdone";
//			App::ConsolePrintF("newTags is \"%ls\"", newTags);*/
//			/*for (const eastl::string16 tag : this->mTags)
//			{
//				newTags += tag;
//				if (tag != *this->mTags.mpEnd)
//					newTags += +u",";
//			}*/
//			//App::ConsolePrintF("tag was updated");
//			/*vector<eastl::string16> newTags;
//			for (const eastl::string16& tag : this->mTags)
//			{
//				eastl_size_t start = tag.find_first_not_of(u" ");
//				eastl_size_t end = tag.find_last_not_of(u" ");
//				eastl::string16 newString = tag.substr(start, end - start + 1);
//				newTags.push_back(newString);
//			}
//			this->mTags = newTags;*/
//		}
//		return original_function(this, index);
//	}
//};
//member_detour(cAssetMetadataResourceFactory_WriteResource, Pollinator::PollinatorClassPlaceholder, bool(Resource::ResourceObject*, Resource::IRecord*, int*, uint32_t))
//{
//	bool detoured(Resource::ResourceObject * param_1, Resource::IRecord * stream, int* param_3, uint32_t type)
//	{
//		App::ConsolePrintF("cAssetMetadataResourceFactory_WriteResource");
//		return original_function(this, param_1, stream, param_3, type);
//	}
//};

void AttachDetours()
{
	anonymous_namespace_sGetRandomKey::attach(Address(ModAPI::ChooseAddress(0xdb8430,0xde3cf0)));
	cMetadataSummarizer_ExctractParameters::attach(Address(ModAPI::ChooseAddress(0x5531e0, 0x55a580)));
	//cAssetMetadataResourceFactory_WriteResource::attach(Address(0x54b7b0));
	
	//GetAssetList::attach(Address(0x562e60));
	/*GetAssetListUnknown::attach(Address(0x562db0));
	GetMatchedAssetList::attach(Address(0x562f20));*/
	//cAssetMetadata_GetTag::attach(Address(0x5509e0));
}

// Generally, you don't need to touch any code here
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		ModAPI::AddPostInitFunction(Initialize);
		ModAPI::AddDisposeFunction(Dispose);

		PrepareDetours(hModule);
		AttachDetours();
		CommitDetours();
		break;

	case DLL_PROCESS_DETACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}

