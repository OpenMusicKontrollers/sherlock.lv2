/*
 * Copyright (c) 2015 Hanspeter Portner (dev@open-music-kontrollers.ch)
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the Artistic License 2.0 as published by
 * The Perl Foundation.
 *
 * This source is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Artistic License 2.0 for more details.
 *
 * You should have received a copy of the Artistic License 2.0
 * along the source as a COPYING file. If not, obtain it from
 * http://www.perlfoundation.org/artistic_license_2_0.
 */

#include <sherlock.h>

#include <Elementary.h>

#include <lv2_eo_ui.h>

// Disable deprecation warnings for Blank and Resource
#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

typedef struct _UI UI;

struct _UI {
	eo_ui_t eoui;

	LV2UI_Write_Function write_function;
	LV2UI_Controller controller;
	
	LV2_URID_Map *map;
	LV2_URID_Unmap *unmap;
	struct {
		LV2_URID midi_MidiEvent;
		LV2_URID osc_OscEvent;
		LV2_URID atom_transfer;
	} uris;
	
	LV2_Atom_Forge forge;

	int w, h;
	Ecore_Evas *ee;
	Evas_Object *parent;
	Evas_Object *bg;
	Evas_Object *vbox;
	Evas_Object *list;
	Evas_Object *clear;

	Elm_Genlist_Item_Class *itc_sherlock;
	Elm_Genlist_Item_Class *itc_prop;
	Elm_Genlist_Item_Class *itc_vec;
	Elm_Genlist_Item_Class *itc_atom;
};

static inline int
_is_expandable(UI *ui, const uint32_t type)
{
	return (type == ui->forge.Object)
		|| (type == ui->forge.Blank)
		|| (type == ui->forge.Resource)
		|| (type == ui->forge.Tuple)
		|| (type == ui->forge.Vector);
}

static char * 
_atom_item_label_get(void *data, Evas_Object *obj, const char *part)
{
	UI *ui = evas_object_data_get(obj, "ui");
	const LV2_Atom *atom = data;

	if(!ui)
		return NULL;
	
	if(!strcmp(part, "elm.text"))
	{
		const char *uri = ui->unmap->unmap(ui->unmap->handle, atom->type);

		return uri ? strdup(uri) : NULL;
	}
	else if(!strcmp(part, "elm.text.sub"))
	{
		char buf [512];

		if(  (atom->type == ui->forge.Object)
			|| (atom->type == ui->forge.Blank)
			|| (atom->type == ui->forge.Resource) )
		{
			const LV2_Atom_Object *atom_object = data;
			const char *uri = ui->unmap->unmap(ui->unmap->handle,
				atom_object->body.otype);
			const char *id = ui->unmap->unmap(ui->unmap->handle,
				atom_object->body.id);
		
			//TODO print id
			sprintf(buf, "%s / %u", uri, atom_object->body.id);
		}
		else if(atom->type == ui->forge.Tuple)
		{
			const LV2_Atom_Tuple *atom_tuple = data;

			sprintf(buf, "");
		}
		else if(atom->type == ui->forge.Vector)
		{
			const LV2_Atom_Vector *atom_vector = data;
			const char *uri = ui->unmap->unmap(ui->unmap->handle,
				atom_vector->body.child_type);

			sprintf(buf, "%s", uri);
		}
		else if(atom->type == ui->forge.Int)
		{
			const LV2_Atom_Int *atom_int = data;

			sprintf(buf, "%d", atom_int->body);
		}
		else if(atom->type == ui->forge.Long)
		{
			const LV2_Atom_Long *atom_long = data;

			sprintf(buf, "%ld", atom_long->body);
		}
		else if(atom->type == ui->forge.Float)
		{
			const LV2_Atom_Float *atom_float = data;

			sprintf(buf, "%f", atom_float->body);
		}
		else if(atom->type == ui->forge.Double)
		{
			const LV2_Atom_Double *atom_double = data;

			sprintf(buf, "%lf", atom_double->body);
		}
		else if(atom->type == ui->forge.Bool)
		{
			const LV2_Atom_Int *atom_int = data;

			sprintf(buf, "%s", atom_int->body ? "true" : "false");
		}
		else if(atom->type == ui->forge.URID)
		{
			const LV2_Atom_URID *atom_urid = data;

			sprintf(buf, "%u", atom_urid->body);
		}
		else if(atom->type == ui->forge.String)
		{
			const char *str = LV2_ATOM_CONTENTS_CONST(LV2_Atom_String, atom);

			sprintf(buf, "%s", str);
		}
		else if(atom->type == ui->forge.Literal)
		{
			const char *str = LV2_ATOM_CONTENTS_CONST(LV2_Atom_Literal, atom);
			//FIXME report datatype and lang

			sprintf(buf, "%s", str);
		}
		else if(atom->type == ui->forge.URI)
		{
			const char *str = LV2_ATOM_BODY_CONST(atom);

			sprintf(buf, "%s", str);
		}
		else if(atom->type == ui->uris.midi_MidiEvent)
		{
			const uint8_t *midi = LV2_ATOM_BODY_CONST(atom);
			char *ptr = buf;
			char *end = buf + 512;

			for(int i=0; (i<atom->size) && (ptr<end); i++, ptr += 3)
				sprintf(ptr, "%02X ", midi[i]);
		}
		else if(atom->type == ui->uris.osc_OscEvent)
		{
			const char *osc = LV2_ATOM_BODY_CONST(atom);

			//TODO
			sprintf(buf, "%s", osc);
		}
		else
		{
			sprintf(buf, "");
		}

		return strdup(buf);
	}
	else
		return NULL;
}

static char * 
_prop_item_label_get(void *data, Evas_Object *obj, const char *part)
{
	UI *ui = evas_object_data_get(obj, "ui");
	const LV2_Atom_Property_Body *prop = data;

	if(!ui)
		return NULL;
	
	if(!strcmp(part, "elm.text"))
	{
		const char *uri = ui->unmap->unmap(ui->unmap->handle, prop->key);

		return uri ? strdup(uri) : NULL;
	}
	else if(!strcmp(part, "elm.text.sub"))
	{
		const char *uri = ui->unmap->unmap(ui->unmap->handle, prop->context);

		return uri ? strdup(uri) : NULL;
	}
	else
		return NULL;
}

static char * 
_sherlock_item_label_get(void *data, Evas_Object *obj, const char *part)
{
	UI *ui = evas_object_data_get(obj, "ui");
	const LV2_Atom_Event *ev = data;
	const LV2_Atom *atom = &ev->body;

	if(!ui)
		return NULL;

	/*
	char buf [512];
	sprintf(buf, "%04ld", frametime->body);
	sprintf(buf, "%u", event->size);
	*/

	if(!strcmp(part, "elm.text"))
	{
		const char *uri = ui->unmap->unmap(ui->unmap->handle, atom->type);

		return uri ? strdup(uri) : NULL;
	}
	else if(!strcmp(part, "elm.text.sub"))
	{
		return _atom_item_label_get((void *)atom, obj, part);
	}
	else
		return NULL;
}

static Evas_Object * 
_atom_item_content_get(void *data, Evas_Object *obj, const char *part)
{
	UI *ui = evas_object_data_get(obj, "ui");
	const LV2_Atom *atom = data;
	char buf [512];

	if(!ui)
		return NULL;

	if(!strcmp(part, "elm.swallow.icon"))
	{
		return NULL; //TODO
	}
	else if(!strcmp(part, "elm.swallow.end"))
	{
		sprintf(buf, "%u", atom->size);

		Evas_Object *label = elm_label_add(obj);
		elm_object_part_text_set(label, "default", buf);
		evas_object_color_set(label, 0x00, 0xbb, 0xbb, 0xff);
		evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(label, 1.f, EVAS_HINT_FILL);
		evas_object_show(label);

		return label;
	}
	else
		return NULL;
}

static Evas_Object * 
_prop_item_content_get(void *data, Evas_Object *obj, const char *part)
{
	UI *ui = evas_object_data_get(obj, "ui");
	const LV2_Atom_Property_Body *prop = data;
	char buf [512];

	if(!ui)
		return NULL;

	if(!strcmp(part, "elm.swallow.icon"))
	{
		return NULL; //TODO
	}
	else if(!strcmp(part, "elm.swallow.end"))
	{
		return NULL; //TODO
	}
	else
		return NULL;
}

static Evas_Object * 
_sherlock_item_content_get(void *data, Evas_Object *obj, const char *part)
{
	UI *ui = evas_object_data_get(obj, "ui");
	const LV2_Atom_Event *ev = data;
	const LV2_Atom *atom = &ev->body;
	char buf [512];

	if(!ui)
		return NULL;

	if(!strcmp(part, "elm.swallow.icon"))
	{
		sprintf(buf, "%04ld", ev->time.frames);

		Evas_Object *label = elm_label_add(obj);
		elm_object_part_text_set(label, "default", buf);
		evas_object_color_set(label, 0xbb, 0xbb, 0x00, 0xff);
		evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(label, 1.f, EVAS_HINT_FILL);
		evas_object_show(label);

		return label;
	}
	else if(!strcmp(part, "elm.swallow.end"))
	{
		sprintf(buf, "%u", atom->size);

		Evas_Object *label = elm_label_add(obj);
		elm_object_part_text_set(label, "default", buf);
		evas_object_color_set(label, 0x00, 0xbb, 0xbb, 0xff);
		evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(label, 1.f, EVAS_HINT_FILL);
		evas_object_show(label);

		return label;
	}
	else
		return NULL;
}

static void
_item_del(void *data, Evas_Object *obj)
{
	free(data);
}

static void
_item_expand_request(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *itm = event_info;
	UI *ui = data;

	elm_genlist_item_expanded_set(itm, EINA_TRUE);
}

static void
_item_contract_request(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *itm = event_info;
	UI *ui = data;

	elm_genlist_item_expanded_set(itm, EINA_FALSE);
}

#undef LV2_ATOM_TUPLE_FOREACH

#define LV2_ATOM_TUPLE_FOREACH(tuple, iter) \
	for (LV2_Atom* (iter) = lv2_atom_tuple_begin(tuple); \
	     !lv2_atom_tuple_is_end(LV2_ATOM_BODY(tuple), (tuple)->atom.size, (iter)); \
	     (iter) = lv2_atom_tuple_next(iter))

static void
_atom_expand(UI *ui, const void *data, Evas_Object *obj, Elm_Object_Item *itm)
{
	const LV2_Atom *atom = data;

	if(  (atom->type == ui->forge.Object)
		|| (atom->type == ui->forge.Resource)
		|| (atom->type == ui->forge.Blank) )
	{
		const LV2_Atom_Object *atom_object = (const LV2_Atom_Object *)atom;
		//const LV2_Atom_Property_Body *prop;

		LV2_ATOM_OBJECT_FOREACH(atom_object, prop)
			elm_genlist_item_append(ui->list, ui->itc_prop, prop, itm,
				ELM_GENLIST_ITEM_TREE, NULL, NULL);
	}
	else if(atom->type == ui->forge.Tuple)
	{
		const LV2_Atom_Tuple *atom_tuple = (const LV2_Atom_Tuple *)atom;
		const LV2_Atom *elmnt;

		LV2_ATOM_TUPLE_FOREACH(atom_tuple, elmnt)
		//for(elmnt = lv2_atom_tuple_begin(atom_tuple);
		//	!lv2_atom_tuple_is_end(LV2_ATOM_BODY(atom_tuple), atom_tuple->atom.size, elmnt);
		//	elmnt = lv2_atom_tuple_next(elmnt))
		{
			Elm_Genlist_Item_Type type = _is_expandable(ui, elmnt->type)
				? ELM_GENLIST_ITEM_TREE
				: ELM_GENLIST_ITEM_NONE;

			elm_genlist_item_append(ui->list, ui->itc_atom, elmnt, itm,
				type, NULL, NULL);
		}
	}
	else if(atom->type == ui->forge.Vector)
	{
		const LV2_Atom_Vector *atom_vector = (const LV2_Atom_Vector *)atom;

		Elm_Genlist_Item_Type type = _is_expandable(ui, atom_vector->body.child_type)
			? ELM_GENLIST_ITEM_TREE
			: ELM_GENLIST_ITEM_NONE;

		int num = (atom_vector->atom.size - sizeof(LV2_Atom_Vector_Body))
			/ atom_vector->body.child_size;
		const uint8_t *body = LV2_ATOM_CONTENTS_CONST(LV2_Atom_Vector, atom_vector);
		for(int i=0; i<num; i++)
		{
			LV2_Atom *atom = malloc(sizeof(LV2_Atom) + atom_vector->body.child_size);
			atom->size = atom_vector->body.child_size;
			atom->type = atom_vector->body.child_type;
			memcpy(LV2_ATOM_BODY(atom), body + i*atom->size, atom->size);
			elm_genlist_item_append(ui->list, ui->itc_vec, atom, itm, type, NULL, NULL);
		}
	}
	else
		; // never reached
}

static void
_prop_expand(UI *ui, const void *data, Evas_Object *obj, Elm_Object_Item *itm)
{
	const LV2_Atom_Property_Body *prop = data;
	const LV2_Atom *atom = &prop->value;

	Elm_Genlist_Item_Type type = _is_expandable(ui, atom->type)
		? ELM_GENLIST_ITEM_TREE
		: ELM_GENLIST_ITEM_NONE;

	elm_genlist_item_append(ui->list, ui->itc_atom, atom, itm, type, NULL, NULL);
}

static void
_sherlock_expand(UI *ui, const void *data, Evas_Object *obj, Elm_Object_Item *itm)
{
	const LV2_Atom_Event *ev = data;
	const LV2_Atom *atom = &ev->body;

	if(atom)
		_atom_expand(ui, atom, obj, itm);
}

static void
_item_expanded(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *itm = event_info;
	UI *ui = data;

	const Elm_Genlist_Item_Class *class = elm_genlist_item_item_class_get(itm);
	const void *udata = elm_object_item_data_get(itm);

	if(udata)
	{
		if(class  == ui->itc_sherlock)
			_sherlock_expand(ui, udata, obj, itm);
		else if(class == ui->itc_prop)
			_prop_expand(ui, udata, obj, itm);
		else if(class == ui->itc_vec)
			_atom_expand(ui, udata, obj, itm);
		else if(class == ui->itc_atom)
			_atom_expand(ui, udata, obj, itm);
	}
}

static void
_item_contracted(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *itm = event_info;
	UI *ui = data;

	elm_genlist_item_subitems_clear(itm);
}

static void
_clear_clicked(void *data, Evas_Object *obj, void *event_info)
{
	UI *ui = data;

	elm_genlist_clear(ui->list);
}

static Evas_Object *
_content_get(eo_ui_t *eoui)
{
	UI *ui = (void *)eoui - offsetof(UI, eoui);

	ui->vbox = elm_box_add(eoui->win);
	elm_box_horizontal_set(ui->vbox, EINA_FALSE);
	elm_box_homogeneous_set(ui->vbox, EINA_FALSE);
	elm_box_padding_set(ui->vbox, 0, 10);

	ui->list = elm_genlist_add(ui->vbox);
	elm_genlist_select_mode_set(ui->list, ELM_OBJECT_SELECT_MODE_NONE);
	elm_genlist_homogeneous_set(ui->list, EINA_TRUE); // for lazy-loading
	evas_object_data_set(ui->list, "ui", ui);
	//evas_object_smart_callback_add(ui->list, "selected", _item_selected, ui);
	evas_object_smart_callback_add(ui->list, "expand,request",
		_item_expand_request, ui);
	evas_object_smart_callback_add(ui->list, "contract,request",
		_item_contract_request, ui);
	evas_object_smart_callback_add(ui->list, "expanded", _item_expanded, ui);
	evas_object_smart_callback_add(ui->list, "contracted", _item_contracted, ui);
	evas_object_size_hint_weight_set(ui->list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ui->list, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(ui->list);
	elm_box_pack_end(ui->vbox, ui->list);

	ui->clear = elm_button_add(ui->vbox);
	elm_object_part_text_set(ui->clear, "default", "Clear");
	evas_object_smart_callback_add(ui->clear, "clicked", _clear_clicked, ui);
	//evas_object_size_hint_weight_set(ui->clear, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ui->clear, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(ui->clear);
	elm_box_pack_end(ui->vbox, ui->clear);

	return ui->vbox;
}

static LV2UI_Handle
instantiate(const LV2UI_Descriptor *descriptor, const char *plugin_uri,
	const char *bundle_path, LV2UI_Write_Function write_function,
	LV2UI_Controller controller, LV2UI_Widget *widget,
	const LV2_Feature *const *features)
{
	if(strcmp(plugin_uri, SHERLOCK_ATOM_INSPECTOR_URI))
		return NULL;

	eo_ui_driver_t driver;
	if(descriptor == &atom_inspector_eo)
		driver = EO_UI_DRIVER_EO;
	else if(descriptor == &atom_inspector_ui)
		driver = EO_UI_DRIVER_UI;
	else if(descriptor == &atom_inspector_x11)
		driver = EO_UI_DRIVER_X11;
	else if(descriptor == &atom_inspector_kx)
		driver = EO_UI_DRIVER_KX;
	else
		return NULL;

	UI *ui = calloc(1, sizeof(UI));
	if(!ui)
		return NULL;

	eo_ui_t *eoui = &ui->eoui;
	eoui->driver = driver;
	eoui->content_get = _content_get;
	eoui->w = 400,
	eoui->h = 400;

	ui->write_function = write_function;
	ui->controller = controller;

	void *parent = NULL;
	LV2UI_Resize *resize = NULL;
	
	int i, j;
	for(i=0; features[i]; i++)
	{
		if(!strcmp(features[i]->URI, LV2_URID__map))
			ui->map = (LV2_URID_Map *)features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_URID__unmap))
			ui->unmap = (LV2_URID_Unmap *)features[i]->data;
  }
	
	ui->uris.midi_MidiEvent = ui->map->map(ui->map->handle, LV2_MIDI__MidiEvent);
	ui->uris.osc_OscEvent = ui->map->map(ui->map->handle, LV2_OSC__OscEvent);
	ui->uris.atom_transfer = ui->map->map(ui->map->handle, LV2_ATOM__atomTransfer);
	
	lv2_atom_forge_init(&ui->forge, ui->map);

	ui->itc_sherlock = elm_genlist_item_class_new();
	ui->itc_sherlock->item_style = "double_label";
	ui->itc_sherlock->func.text_get = _sherlock_item_label_get;
	ui->itc_sherlock->func.content_get = _sherlock_item_content_get;
	ui->itc_sherlock->func.state_get = NULL;
	ui->itc_sherlock->func.del = _item_del;
	
	ui->itc_prop = elm_genlist_item_class_new();
	ui->itc_prop->item_style = "double_label";
	ui->itc_prop->func.text_get = _prop_item_label_get;
	ui->itc_prop->func.content_get = _prop_item_content_get;
	ui->itc_prop->func.state_get = NULL;
	ui->itc_prop->func.del = NULL;
	
	ui->itc_vec = elm_genlist_item_class_new();
	ui->itc_vec->item_style = "double_label";
	ui->itc_vec->func.text_get = _atom_item_label_get;
	ui->itc_vec->func.content_get = _atom_item_content_get;
	ui->itc_vec->func.state_get = NULL;
	ui->itc_vec->func.del = _item_del;
	
	ui->itc_atom = elm_genlist_item_class_new();
	ui->itc_atom->item_style = "double_label";
	ui->itc_atom->func.text_get = _atom_item_label_get;
	ui->itc_atom->func.content_get = _atom_item_content_get;
	ui->itc_atom->func.state_get = NULL;
	ui->itc_atom->func.del = NULL;

	if(eoui_instantiate(eoui, descriptor, plugin_uri, bundle_path, write_function,
		controller, widget, features))
	{
		free(ui);
		return NULL;
	}

	return ui;
}

static void
cleanup(LV2UI_Handle handle)
{
	UI *ui = handle;

	eoui_cleanup(&ui->eoui);

	elm_genlist_item_class_free(ui->itc_atom);
	elm_genlist_item_class_free(ui->itc_vec);
	elm_genlist_item_class_free(ui->itc_prop);
	elm_genlist_item_class_free(ui->itc_sherlock);

	free(ui);
}

static void
port_event(LV2UI_Handle handle, uint32_t i, uint32_t size, uint32_t urid,
	const void *buf)
{
	UI *ui = handle;

	if( (i == 0) && (urid == ui->uris.atom_transfer) )
	{
		Elm_Object_Item *itm;

		const LV2_Atom_Sequence *seq = buf;

		LV2_ATOM_SEQUENCE_FOREACH(seq, elmnt)
		{
			size_t len = sizeof(LV2_Atom_Event) + elmnt->body.size;
			LV2_Atom_Event *ev = malloc(len);
			memcpy(ev, elmnt, len);
			
			/* TODO would be correct
			const LV2_Atom *atom = &elmnt->body;
			Elm_Genlist_Item_Type type = _is_expandable(ui, atom->type)
				? ELM_GENLIST_ITEM_TREE
				: ELM_GENLIST_ITEM_NONE;
			*/
			Elm_Genlist_Item_Type type = ELM_GENLIST_ITEM_TREE; // TODO looks nicer

			itm = elm_genlist_item_append(ui->list, ui->itc_sherlock, ev, NULL,
				type, NULL, NULL);
		}
	}
}

const LV2UI_Descriptor atom_inspector_eo = {
	.URI						= SHERLOCK_ATOM_INSPECTOR_EO_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_eo_extension_data
};

const LV2UI_Descriptor atom_inspector_ui = {
	.URI						= SHERLOCK_ATOM_INSPECTOR_UI_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_ui_extension_data
};

const LV2UI_Descriptor atom_inspector_x11 = {
	.URI						= SHERLOCK_ATOM_INSPECTOR_X11_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_x11_extension_data
};

const LV2UI_Descriptor atom_inspector_kx = {
	.URI						= SHERLOCK_ATOM_INSPECTOR_KX_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_kx_extension_data
};