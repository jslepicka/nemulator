#include "TexturePanelItem.h"

TexturePanelItem::TexturePanelItem(void)
{
	is_active = 0;
}

TexturePanelItem::~TexturePanelItem(void)
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
