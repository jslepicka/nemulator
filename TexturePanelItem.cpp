#include "TexturePanelItem.h"

TexturePanelItem::TexturePanelItem()
{
	is_active = 0;
}

TexturePanelItem::~TexturePanelItem()
{
}

void TexturePanelItem::Activate(bool load)
{
	OnActivate(load);
}

void TexturePanelItem::Deactivate()
{
	OnDeactivate();
}

void TexturePanelItem::Load()
{
	OnLoad();
}
