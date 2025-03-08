module texture_panel:item;

c_texture_panel_item::c_texture_panel_item()
{
    is_active = 0;
}

c_texture_panel_item::~c_texture_panel_item()
{
}

void c_texture_panel_item::Activate(bool load)
{
    OnActivate(load);
}

void c_texture_panel_item::Deactivate()
{
    OnDeactivate();
}

void c_texture_panel_item::Load()
{
    OnLoad();
}
