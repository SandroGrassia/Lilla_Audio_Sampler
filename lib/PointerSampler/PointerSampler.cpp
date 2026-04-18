/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "PointerSampler.h"

void PointerSampler::Set_pointer_to_first_menu_element(void)
{
	pointer.field_name = field_DS_Menu;
	pointer.menu_element = static_cast<DS_menu_element_name>(element_Menu_DS[0]);
	pointer.value_element = value_DS_Recording;
	Display_Sampler.DS_show_pointer_frame(pointer, true);
}

FLASHMEM
void PointerSampler::Move_pointer(const int value)
{
	bool change = false;
	pointer_old = pointer;

	switch (pointer.field_name)
	{
	case field_DS_Menu:
	{
		int menu_position = position_Menu_DS[pointer.menu_element];

		if (value == 1)
		{
			if (menu_position == DS_menu_max)
			{
				pointer.field_name = field_DS_Value;
				pointer.value_element = value_DS_Recording;
				change = true;
			}
			else
			{
				pointer.menu_element = static_cast<DS_menu_element_name>(element_Menu_DS[++menu_position]);
				change = true;
			}
		}
		else if (value == -1 && menu_position > 0)
		{
			pointer.menu_element = static_cast<DS_menu_element_name>(element_Menu_DS[--menu_position]);
			change = true;
		}
	}
	break;

	case field_DS_Value:
	{
		if (value == -1)
		{
			pointer.field_name = field_DS_Menu;
			pointer.menu_element = static_cast<DS_menu_element_name>(element_Menu_DS[DS_menu_max]);
			change = true;
		}
	}
	break;
	}

	if (change)
	{
		Display_Sampler.DS_show_pointer_frame(pointer_old, false);
		Display_Sampler.DS_show_pointer_frame(pointer, true);
	}
}

void PointerSampler::Move_pointer_within_menu(const int value)
{
	bool change = false;
	pointer_old = pointer;

	if (pointer.field_name == field_DS_Menu)
	{
		int menu_position = position_Menu_DS[pointer.menu_element];

		if (value == 1)
		{
			if (menu_position < DS_menu_max)
			{
				pointer.menu_element = static_cast<DS_menu_element_name>(element_Menu_DS[++menu_position]);
				change = true;
			}
		}
		else if (value == -1)
		{
			if (menu_position > 0)
				{
					pointer.menu_element = static_cast<DS_menu_element_name>(element_Menu_DS[--menu_position]);
					change = true;
				}
		}
	}

	if (change)
	{
		Display_Sampler.DS_show_pointer_frame(pointer_old, false);
		Display_Sampler.DS_show_pointer_frame(pointer, true);
	}
}

FLASHMEM
void PointerSampler::Restore_pointer(void)
{
	if (pointer.field_name == field_DS_Menu)
	{
		pointer.menu_element = static_cast<DS_menu_element_name>(element_Menu_DS[0]);
	}
}

void PointerSampler::Show_pointer(const bool show)
{
	Display_Sampler.DS_show_pointer_frame(pointer, show);
}

DS_pointer_struct PointerSampler::Get_pointer(void)
{
	return pointer;
}