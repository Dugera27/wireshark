/* packet-mate.c
 * Routines for the mate Facility's Pseudo-Protocol dissection
 *
 * Copyright 2004, Luis E. Garcia Ontanon <gopo@webflies.org>
 *
 * $Id$
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


/**************************************************************************
 * This is the pseudo protocol dissector for the mate module.          ***
 * It is intended for this to be just the user interface to the module. ***
 **************************************************************************/

#include "mate.h"

static int mate_tap_data = 0;
static mate_config* mc = NULL;

static int proto_mate = -1;

static char* pref_mate_config_filename = "";
static char* current_mate_config_filename = NULL;

static proto_item *mate_i = NULL;

void attrs_tree(proto_tree* tree, tvbuff_t *tvb,mate_item* item) {
	AVPN* c;
	proto_item *avpl_i;
	proto_tree *avpl_t;
	int* hfi_p;
	
	avpl_i = proto_tree_add_text(tree,tvb,0,0,"%s Attributes",item->cfg->name);
	avpl_t = proto_item_add_subtree(avpl_i, item->cfg->ett_attr);

	for ( c = item->avpl->null.next; c->avp; c = c->next) {
		hfi_p = g_hash_table_lookup(item->cfg->my_hfids,(char*)c->avp->n);

		if (hfi_p) {
			proto_tree_add_string(avpl_t,*hfi_p,tvb,0,0,c->avp->v);
		} else {
			g_warning("MATE: error: undefined attribute: mate.%s.%s",item->cfg->name,c->avp->n);
			proto_tree_add_text(avpl_t,tvb,0,0,"Undefined attribute: %s=%s",c->avp->n, c->avp->v);
		}
	}
}

void mate_gop_tree(proto_tree* pdu_tree, tvbuff_t *tvb, mate_gop* gop);

void mate_gog_tree(proto_tree* tree, tvbuff_t *tvb, mate_gog* gog, mate_gop* gop) {
	proto_item *gog_item;
	proto_tree *gog_tree;
	proto_item *gog_time_item;
	proto_tree *gog_time_tree;
	proto_item *gog_gop_item;
	proto_tree *gog_gop_tree;
	mate_gop* gog_gops;
#ifdef _MATE_DEBUGGING
	proto_item* gog_key_item;
	proto_tree* gog_key_tree;
	guint i;
#endif
	
	gog_item = proto_tree_add_uint(tree,gog->cfg->hfid,tvb,0,0,gog->id);
	gog_tree = proto_item_add_subtree(gog_item,gog->cfg->ett);
			
	attrs_tree(gog_tree,tvb,gog);
	
	if (gog->cfg->show_times) {
		gog_time_item = proto_tree_add_text(gog_tree,tvb,0,0,"%s Times",gog->cfg->name);
		gog_time_tree = proto_item_add_subtree(gog_time_item, gog->cfg->ett_times);
		
		proto_tree_add_float(gog_time_tree, gog->cfg->hfid_start_time, tvb, 0, 0, gog->start_time);
		proto_tree_add_float(gog_time_tree, gog->cfg->hfid_last_time, tvb, 0, 0, gog->last_time - gog->start_time); 
	}
	
	gog_gop_item = proto_tree_add_uint(gog_tree, gog->cfg->hfid_gog_num_of_gops,
									   tvb, 0, 0, gog->num_of_gops);
	
	gog_gop_tree = proto_item_add_subtree(gog_gop_item, gog->cfg->ett_children);
	
	for (gog_gops = gog->gops; gog_gops; gog_gops = gog_gops->next) {
		
		if (gop != gog_gops) {
			if (gog->cfg->gop_as_subtree) {
				mate_gop_tree(gog_gop_tree, tvb, gog_gops);
			} else {
				proto_tree_add_uint(gog_gop_tree,gog_gops->cfg->hfid,tvb,0,0,gog_gops->id);
			}
		} else {
			 proto_tree_add_uint_format(gog_gop_tree,gop->cfg->hfid,tvb,0,0,gop->id,"%s of current frame: %d",gop->cfg->name,gop->id);
		}
	}
}

void mate_gop_tree(proto_tree* tree, tvbuff_t *tvb, mate_gop* gop) {
	proto_item *gop_item;
	proto_tree *gop_time_tree;
	proto_item *gop_time_item;
	proto_tree *gop_tree;
	proto_item *gop_pdu_item;
	proto_tree *gop_pdu_tree;
	mate_pdu* gop_pdus;
	float  rel_time;
	float  gop_time;
	float pdu_rel_time;
	gchar* pdu_str;
	gchar* type_str;
	guint32 pdu_item;
	
	gop_item = proto_tree_add_uint(tree,gop->cfg->hfid,tvb,0,0,gop->id);
	gop_tree = proto_item_add_subtree(gop_item, gop->cfg->ett);
	
	if (gop->gop_key) proto_tree_add_text(gop_tree,tvb,0,0,"GOP Key: %s",gop->gop_key);
	
	attrs_tree(gop_tree,tvb,gop);
	
	if (gop->cfg->show_times) {
		gop_time_item = proto_tree_add_text(gop_tree,tvb,0,0,"%s Times",gop->cfg->name);
		gop_time_tree = proto_item_add_subtree(gop_time_item, gop->cfg->ett_times);
		
		proto_tree_add_float(gop_time_tree, gop->cfg->hfid_start_time, tvb, 0, 0, gop->start_time);
		
		if (gop->released) { 
			proto_tree_add_float(gop_time_tree, gop->cfg->hfid_stop_time, tvb, 0, 0, gop->release_time - gop->start_time);
			proto_tree_add_float(gop_time_tree, gop->cfg->hfid_last_time, tvb, 0, 0, gop->last_time - gop->start_time); 
		} else {
			proto_tree_add_float(gop_time_tree, gop->cfg->hfid_last_time, tvb, 0, 0, gop->last_time - gop->start_time); 
		}
	}
	
	gop_pdu_item = proto_tree_add_uint(gop_tree, gop->cfg->hfid_gop_num_pdus, tvb, 0, 0,gop->num_of_pdus);

	if (gop->cfg->show_pdu_tree != mc->no_tree) {
		
		gop_pdu_tree = proto_item_add_subtree(gop_pdu_item, gop->cfg->ett_children);

		rel_time = gop_time = gop->start_time;

		type_str = (gop->cfg->show_pdu_tree == mc->frame_tree ) ? "in frame:" : "id:";
		
		for (gop_pdus = gop->pdus; gop_pdus; gop_pdus = gop_pdus->next) {

			pdu_item = (gop->cfg->show_pdu_tree == mc->frame_tree ) ? gop_pdus->frame : gop_pdus->id;

			if (gop_pdus->is_start) {
				pdu_str = "Start ";
			} else if (gop_pdus->is_stop) {
				pdu_str = "Stop ";
			} else if (gop_pdus->after_release) {
				pdu_str = "After stop ";
			} else {
				pdu_str = "";
			}
			
			pdu_rel_time = gop_pdus->time_in_gop != 0.0 ? gop_pdus->time_in_gop - rel_time : (float) 0.0;
			
			proto_tree_add_uint_format(gop_pdu_tree,gop->cfg->hfid_gop_pdu,tvb,0,0,pdu_item,
									   "%sPDU: %s %i (%f : %f)",pdu_str, type_str,
									   pdu_item, gop_pdus->time_in_gop,
									   pdu_rel_time);
			
			rel_time = gop_pdus->time_in_gop;
			
		}
	}
}


void mate_pdu_tree(mate_pdu *pdu, tvbuff_t *tvb, proto_tree* tree) {
	proto_item *pdu_item;
	proto_tree *pdu_tree;
	
	if ( ! pdu ) return;
	
	if (pdu->gop && pdu->gop->gog) {
		proto_item_append_text(mate_i," %s:%d->%s:%d->%s:%d",
							   pdu->cfg->name,pdu->id,
							   pdu->gop->cfg->name,pdu->gop->id,
							   pdu->gop->gog->cfg->name,pdu->gop->gog->id);
	} else if (pdu->gop) {
		proto_item_append_text(mate_i," %s:%d->%s:%d",
							   pdu->cfg->name,pdu->id,
							   pdu->gop->cfg->name,pdu->gop->id);
	} else {
		proto_item_append_text(mate_i," %s:%d",pdu->cfg->name,pdu->id);
	}
	
	pdu_item = proto_tree_add_uint(tree,pdu->cfg->hfid,tvb,0,0,pdu->id);
	pdu_tree = proto_item_add_subtree(pdu_item, pdu->cfg->ett);
	proto_tree_add_float(pdu_tree,pdu->cfg->hfid_pdu_rel_time, tvb, 0, 0, pdu->rel_time);		

	if (pdu->gop) {
		proto_tree_add_float(pdu_tree,pdu->cfg->hfid_pdu_time_in_gop, tvb, 0, 0, pdu->time_in_gop);		
		mate_gop_tree(pdu_tree,tvb,pdu->gop);

		if (pdu->gop->gog)
			mate_gog_tree(pdu_tree,tvb,pdu->gop->gog,pdu->gop);
	}
	
	if (pdu->avpl) {
		attrs_tree(pdu_tree,tvb,pdu);
	}
}

extern void mate_tree(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree) {
	mate_pdu* pdus;
	proto_tree *mate_t;
	
	if ( ! mc || ! tree ) return;

	mate_analyze_frame(pinfo,tree);

	if (( pdus = mate_get_pdus(pinfo->fd->num) )) {
		for ( ; pdus; pdus = pdus->next_in_frame) {
			mate_i = proto_tree_add_protocol_format(tree,mc->hfid_mate,tvb,0,0,"MATE");
			mate_t = proto_item_add_subtree(mate_i, mc->ett_root);			
			mate_pdu_tree(pdus,tvb,mate_t);
		}
	}
}

static int mate_packet(void *prs _U_,  packet_info* tree _U_, epan_dissect_t *edt _U_, const void *dummy _U_) {
	/* nothing to do yet */
	return 0;
}

static void init_mate(void) {
	GString* tap_error = NULL;

	tap_error = register_tap_listener("frame", &mate_tap_data,
									  (char*) mc->tap_filter,
									  (tap_reset_cb) NULL,
									  mate_packet,
									  (tap_draw_cb) NULL);
	
	if ( tap_error ) {
		g_warning("mate: couldn't (re)register tap: %s",tap_error->str);
		g_string_free(tap_error, TRUE);
		mate_tap_data = 0;
		return;
	} else {
		mate_tap_data = 1;
	}
	
	initialize_mate_runtime();
}

extern
void
proto_reg_handoff_mate(void)
{
	if ( *pref_mate_config_filename != '\0' ) {
		
		if (current_mate_config_filename) {
			report_failure("Mate cannot reconfigure itself.\n"
						   "for changes to be applied you have to save the preferences and restart ethereal\n");
			return;
		} 
		
		if (!mc) { 
			mc = mate_make_config((char*)pref_mate_config_filename,proto_mate);
			
			if (mc) {
				/* XXX: alignment warnings, what do they mean? */
				proto_register_field_array(proto_mate, (hf_register_info*) mc->hfrs->data, mc->hfrs->len );
				proto_register_subtree_array((gint**) mc->ett->data, mc->ett->len);
				register_init_routine(init_mate);
				if (current_mate_config_filename == NULL) initialize_mate_runtime();
			}
			
			current_mate_config_filename = pref_mate_config_filename;

		}
	}
}

extern
void
proto_register_mate(void)
{
	module_t *mate_module;

	proto_mate = proto_register_protocol("Meta Analysis Tracing Engine", "MATE", "mate");
	register_dissector("mate",mate_tree,proto_mate);
	mate_module = prefs_register_protocol(proto_mate, proto_reg_handoff_mate);
	prefs_register_string_preference(mate_module, "config_filename",
									 "Configuration Filename",
									 "The name of the file containing the mate module's configuration",
									 &pref_mate_config_filename);
}

