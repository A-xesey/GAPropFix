#include "stdafx.h"
#include "UpdateDataBase.h"

using namespace Sporepedia::OTDB;

UpdateDataBase::UpdateDataBase()
{
}


UpdateDataBase::~UpdateDataBase()
{
}

void UpdateDataBase::ParseLine(const ArgScript::Line& line)
{
	ObjectTemplateDB.Write(true, true);
}

const char* UpdateDataBase::GetDescription(ArgScript::DescriptionMode mode) const
{
	return "UpdateDataBase: Force to update the game database content.\nWARNING: If there are many creations in your Pollination file, this may cause the game to crash due to a memory leak.\nI recommend you to download the 4GB patch and patch the game to avoid crashing.";
}
