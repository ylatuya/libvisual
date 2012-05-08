/* Libvisual - The audio visualisation framework.
 *
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id: lv_bin.c,v 1.29 2006/02/08 18:55:12 synap Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "config.h"
#include "lv_bin.h"
#include "lv_common.h"
#include "lv_list.h"
#include "gettext.h"

/* WARNING: Utterly shit ahead, i've screwed up on this and i need to
 * rewrite it. And i can't say i feel like it at the moment so be
 * patient :)  */

static void bin_dtor (VisObject *object);

static VisVideoDepth bin_get_suitable_depth (VisBin *bin, VisVideoDepth depth);

static void bin_dtor (VisObject *object)
{
	VisBin *bin = VISUAL_BIN (object);

	visual_return_if_fail (bin != NULL);

	if (bin->actor)
		visual_object_unref (VISUAL_OBJECT (bin->actor));

	if (bin->input)
		visual_object_unref (VISUAL_OBJECT (bin->input));

	if (bin->morph)
		visual_object_unref (VISUAL_OBJECT (bin->morph));

	if (bin->actmorph)
		visual_object_unref (VISUAL_OBJECT (bin->actmorph));

	if (bin->actmorphvideo)
		visual_video_unref (bin->actmorphvideo);

	if (bin->privvid)
		visual_video_unref (bin->privvid);

	visual_time_free (bin->morphtime);
}

static VisVideoDepth bin_get_suitable_depth (VisBin *bin, int depthflag)
{
	int depth;

	switch (bin->depthpreferred) {
		case VISUAL_BIN_DEPTH_LOWEST:
			depth = visual_video_depth_get_lowest (depthflag);
			break;

		case VISUAL_BIN_DEPTH_HIGHEST:
			depth = visual_video_depth_get_highest (depthflag);
			break;
	}

	/* Is supported within bin natively */
	if (bin->depthflag & depth)
		return depth;

	/* Not supported by the bin, taking the highest depth from the bin */
	return visual_video_depth_get_highest_nogl (bin->depthflag);
}

VisBin *visual_bin_new ()
{
	VisBin *bin;

	bin = visual_mem_new0 (VisBin, 1);
	visual_object_init (VISUAL_OBJECT (bin), bin_dtor);

	bin->morphautomatic = TRUE;

	bin->morphmode = VISUAL_MORPH_MODE_TIME;
	bin->morphtime = visual_time_new_with_values (4, 0);

	bin->depthpreferred = VISUAL_BIN_DEPTH_HIGHEST;

	return bin;
}

void visual_bin_realize (VisBin *bin)
{
	visual_return_if_fail (bin);

	if (bin->actor != NULL)
		visual_actor_realize (bin->actor);

	if (bin->input != NULL)
		visual_input_realize (bin->input);

	if (bin->morph != NULL)
		visual_morph_realize (bin->morph);
}

void visual_bin_set_actor (VisBin *bin, VisActor *actor)
{
	visual_return_if_fail (bin != NULL);

	bin->actor = actor;
	bin->managed = FALSE;
}

VisActor *visual_bin_get_actor (VisBin *bin)
{
	visual_return_val_if_fail (bin != NULL, NULL);

	return bin->actor;
}

void visual_bin_set_input (VisBin *bin, VisInput *input)
{
	visual_return_if_fail (bin != NULL);

	bin->input = input;
	bin->inputmanaged = FALSE;
}

VisInput *visual_bin_get_input (VisBin *bin)
{
	visual_return_val_if_fail (bin != NULL, NULL);

	return bin->input;
}

void visual_bin_set_morph (VisBin *bin, VisMorph *morph)
{
	visual_return_if_fail (bin != NULL);

	bin->morph = morph;
	bin->morphmanaged = FALSE;
}

void visual_bin_set_morph_by_name (VisBin *bin, const char *morphname)
{
	VisMorph *morph;
	int depthflag;

	visual_return_if_fail (bin != NULL);

	if (bin->morph)
		visual_object_unref (VISUAL_OBJECT (bin->morph));

	morph = visual_morph_new (morphname);

	bin->morph = morph;
	bin->morphmanaged = TRUE;

	visual_return_if_fail (morph->plugin != NULL);

	depthflag = visual_morph_get_supported_depth (morph);

	if (visual_video_depth_is_supported (depthflag, visual_video_get_depth (bin->actvideo)) <= 0) {
		visual_object_unref (VISUAL_OBJECT (morph));
		bin->morph = NULL;

		return;
	}
}

VisMorph *visual_bin_get_morph (VisBin *bin)
{
	visual_return_val_if_fail (bin != NULL, NULL);

	return bin->morph;
}

void visual_bin_connect (VisBin *bin, VisActor *actor, VisInput *input)
{
    int depthflag;

    visual_return_if_fail (bin != NULL);
    visual_return_if_fail (actor != NULL);
    visual_return_if_fail (input != NULL);

    visual_bin_set_actor (bin, actor);
    visual_bin_set_input (bin, input);

    depthflag = visual_actor_get_supported_depth (actor);

    if (depthflag == VISUAL_VIDEO_DEPTH_GL) {
        visual_bin_set_depth (bin, VISUAL_VIDEO_DEPTH_GL);
    } else {
        visual_bin_set_depth (bin, bin_get_suitable_depth(bin, depthflag));
    }

    bin->depthforcedmain = bin->depth;
}

void visual_bin_connect_by_names (VisBin *bin, const char *actname, const char *inname)
{
	VisActor *actor;
	VisInput *input;

	visual_return_if_fail (bin != NULL);

	/* Create the actor */
	actor = visual_actor_new (actname);
	visual_return_if_fail (actor != NULL);

	/* Create the input */
	input = visual_input_new (inname);
	visual_return_if_fail (input != NULL);

	/* Connect */
	visual_bin_connect (bin, actor, input);

	bin->managed = TRUE;
	bin->inputmanaged = TRUE;
}

void visual_bin_sync (VisBin *bin, int noevent)
{
	VisVideo *video;
	VisVideo *actvideo;

	visual_return_if_fail (bin != NULL);

	visual_log (VISUAL_LOG_DEBUG, "starting sync");

	/* Sync the actor regarding morph */
	if (bin->morphing && bin->morphstyle == VISUAL_SWITCH_STYLE_MORPH &&
			visual_video_get_depth (bin->actvideo) != VISUAL_VIDEO_DEPTH_GL && !bin->depthfromGL) {

		visual_morph_set_video (bin->morph, bin->actvideo);

		video = bin->privvid;
		if (video == NULL) {
			visual_log (VISUAL_LOG_DEBUG, "Private video data NULL");
			return;
		}

		visual_video_free_buffer (video);
		visual_video_copy_attrs (video, bin->actvideo);

		visual_log (VISUAL_LOG_DEBUG, "pitches actvideo %d, new video %d",
		            visual_video_get_pitch (bin->actvideo),
		            visual_video_get_pitch (video));

		visual_log (VISUAL_LOG_DEBUG, "phase1 bin->privvid %p", (void *) bin->privvid);
		if (visual_video_get_depth (bin->actmorph->video) == VISUAL_VIDEO_DEPTH_GL) {
			visual_video_set_buffer (video, NULL);
			video = bin->actvideo;
		} else
			visual_video_allocate_buffer (video);

		visual_log (VISUAL_LOG_DEBUG, "phase2");
	} else {
		video = bin->actvideo;
		if (!video) {
			visual_log (VISUAL_LOG_DEBUG, "Actor video is NULL");
			return;
		}

		visual_log (VISUAL_LOG_DEBUG, "setting new video from actvideo %d %d",
		            visual_video_get_depth (video),
		            visual_video_get_bpp (video));
	}

	/* Main actor */
/*	visual_actor_realize (bin->actor); */
	visual_actor_set_video (bin->actor, video);

	visual_log (VISUAL_LOG_DEBUG, "one last video pitch check %d depth old %d forcedmain %d noevent %d",
			visual_video_get_pitch (video), bin->depthold,
			bin->depthforcedmain, noevent);

	if (bin->managed) {
		if (bin->depthold == VISUAL_VIDEO_DEPTH_GL)
			visual_actor_video_negotiate (bin->actor, bin->depthforcedmain, FALSE, TRUE);
		else
			visual_actor_video_negotiate (bin->actor, bin->depthforcedmain, noevent, TRUE);
	} else {
		if (bin->depthold == VISUAL_VIDEO_DEPTH_GL)
			visual_actor_video_negotiate (bin->actor, 0, FALSE, TRUE);
		else
			visual_actor_video_negotiate (bin->actor, 0, noevent, FALSE);
	}

	visual_log (VISUAL_LOG_DEBUG, "pitch after main actor negotiate %d",
	            visual_video_get_pitch (video));

	/* Morphing actor */
	if (bin->actmorphmanaged && bin->morphing &&
			bin->morphstyle == VISUAL_SWITCH_STYLE_MORPH) {

		actvideo = bin->actmorphvideo;
		if (actvideo == NULL) {
			visual_log (VISUAL_LOG_DEBUG, "Morph video is NULL");
			return;
		}

		visual_video_free_buffer (actvideo);

		visual_video_copy_attrs (actvideo, video);

		if (visual_video_get_depth (bin->actor->video) != VISUAL_VIDEO_DEPTH_GL)
			visual_video_allocate_buffer (actvideo);

		visual_actor_realize (bin->actmorph);

		visual_log (VISUAL_LOG_DEBUG, "phase3 pitch of real framebuffer %d",
		            visual_video_get_pitch (bin->actvideo));

		if (bin->actmorphmanaged)
			visual_actor_video_negotiate (bin->actmorph, bin->depthforced, FALSE, TRUE);
		else
			visual_actor_video_negotiate (bin->actmorph, 0, FALSE, FALSE);
	}

	visual_log (VISUAL_LOG_DEBUG, "end sync function");
}

void visual_bin_set_video (VisBin *bin, VisVideo *video)
{
	visual_return_if_fail (bin != NULL);

	if (bin->actvideo && bin->actvideo != video) {
		visual_video_unref (bin->actvideo);
	}

	bin->actvideo = video;

	if (bin->actvideo) {
		visual_video_ref (bin->actvideo);
	}
}

void visual_bin_set_supported_depth (VisBin *bin, int depthflag)
{
	visual_return_if_fail (bin != NULL);

	bin->depthflag = depthflag;
}

void visual_bin_set_preferred_depth (VisBin *bin, VisBinDepth depthpreferred)
{
	visual_return_if_fail (bin != NULL);

	bin->depthpreferred = depthpreferred;
}

void visual_bin_set_depth (VisBin *bin, int depth)
{
	visual_return_if_fail (bin != NULL);

	bin->depthold = bin->depth;

	if (!visual_video_depth_is_supported (bin->depthflag, depth))
		return;

	visual_log (VISUAL_LOG_DEBUG, "old: %d new: %d", bin->depth, depth);
	if (bin->depth != depth)
		bin->depthchanged = TRUE;

	if (bin->depth == VISUAL_VIDEO_DEPTH_GL && bin->depthchanged)
		bin->depthfromGL = TRUE;
	else
		bin->depthfromGL = FALSE;

	bin->depth = depth;

	if (bin->actvideo) {
	    visual_video_set_depth (bin->actvideo, depth);
	}
}

int visual_bin_get_depth (VisBin *bin)
{
	visual_return_val_if_fail (bin != NULL, -1);

	return bin->depth;
}

int visual_bin_depth_changed (VisBin *bin)
{
	visual_return_val_if_fail (bin != NULL, -1);

	if (!bin->depthchanged)
		return FALSE;

	bin->depthchanged = FALSE;

	return TRUE;
}

VisPalette *visual_bin_get_palette (VisBin *bin)
{
	visual_return_val_if_fail (bin != NULL, NULL);

	if (bin->morphing)
		return visual_morph_get_palette (bin->morph);
	else
		return visual_actor_get_palette (bin->actor);
}

void visual_bin_switch_actor_by_name (VisBin *bin, const char *actname)
{
	VisActor *actor;
	VisVideo *video = NULL;
	int depthflag;
	int width, height, depth;

	visual_return_if_fail (bin != NULL);
	visual_return_if_fail (actname != NULL);

	visual_log (VISUAL_LOG_DEBUG, "switching to a new actor: %s, old actor: %s", actname, bin->actor->plugin->info->plugname);

	/* Destroy if there already is a managed one */
	if (bin->actmorphmanaged) {
		if (bin->actmorph != NULL) {
			visual_object_unref (VISUAL_OBJECT (bin->actmorph));

			if (bin->actmorphvideo != NULL)
				visual_video_unref (bin->actmorphvideo);
		}
	}

	/* Create a new managed actor */
	actor = visual_actor_new (actname);
	visual_return_if_fail (actor != NULL);

	width  = visual_video_get_width  (bin->actvideo);
	height = visual_video_get_height (bin->actvideo);
	depth  = visual_video_get_depth  (bin->actvideo);

	depthflag = visual_actor_get_supported_depth (actor);
	if (visual_video_depth_is_supported (depthflag, VISUAL_VIDEO_DEPTH_GL)) {
		visual_log (VISUAL_LOG_INFO, _("Switching to GL mode"));

		bin->depthforced = VISUAL_VIDEO_DEPTH_GL;
		bin->depthforcedmain = VISUAL_VIDEO_DEPTH_GL;

		depth = VISUAL_VIDEO_DEPTH_GL;

		visual_bin_set_depth (bin, VISUAL_VIDEO_DEPTH_GL);
		bin->depthchanged = TRUE;
	} else {
		visual_log (VISUAL_LOG_INFO, _("Switching away from Gl mode -- or non Gl switch"));


		/* Switching from GL */
		visual_video_set_depth (video, bin_get_suitable_depth (bin, depthflag));

		visual_log (VISUAL_LOG_DEBUG, "after depth fixating");

		/* After a depth change, the pitch value needs an update from the client
		 * if it's different from width * bpp, after a visual_bin_sync
		 * the issues are fixed again */
		visual_log (VISUAL_LOG_INFO, _("video depth (from fixate): %d"), visual_video_get_depth (video));

		/* FIXME check if there are any unneeded depth transform environments and drop these */
		visual_log (VISUAL_LOG_DEBUG, "checking if we need to drop something: depthforcedmain: %d actvideo->depth %d",
		            bin->depthforcedmain, visual_video_get_depth (bin->actvideo));

		/* Drop a transformation environment when not needed */
		if (bin->depthforcedmain != visual_video_get_depth (bin->actvideo)) {
			visual_actor_video_negotiate (bin->actor, bin->depthforcedmain, TRUE, TRUE);
			visual_log (VISUAL_LOG_DEBUG, "[[[[optionally a bogus transform environment, dropping]]]]");
		}

		if (visual_video_get_depth (bin->actvideo) > visual_video_get_depth (video)
				&& visual_video_get_depth (bin->actvideo) != VISUAL_VIDEO_DEPTH_GL
				&& bin->morphstyle == VISUAL_SWITCH_STYLE_MORPH) {

			visual_log (VISUAL_LOG_INFO, _("old depth is higher, video depth %d, depth %d, bin depth %d"),
			            visual_video_get_depth (video), depth, bin->depth);

			bin->depthforced = depth;
			bin->depthforcedmain = bin->depth;

			visual_bin_set_depth (bin, visual_video_get_depth (bin->actvideo));
			visual_video_set_depth (video, visual_video_get_depth (bin->actvideo));

		} else if (visual_video_get_depth (bin->actvideo) != VISUAL_VIDEO_DEPTH_GL) {

			visual_log (VISUAL_LOG_INFO, _("new depth is higher, or equal: video depth %d, depth %d bin depth %d"),
			            visual_video_get_depth (video), depth, bin->depth);

			visual_log (VISUAL_LOG_DEBUG, "depths i can locate: actvideo: %d   bin: %d	 bin-old: %d",
			            visual_video_get_depth (bin->actvideo), bin->depth, bin->depthold);

			bin->depthforced = visual_video_get_depth (video);
			bin->depthforcedmain = bin->depth;

			visual_log (VISUAL_LOG_DEBUG, "depthforcedmain in switch by name: %d", bin->depthforcedmain);
			visual_log (VISUAL_LOG_DEBUG, "visual_bin_set_depth %d", visual_video_get_depth (video));
			visual_bin_set_depth (bin, visual_video_get_depth (video));

		} else {
			/* Don't force ourself into a GL depth, seen we do a direct
			 * switch in the run */
			bin->depthforced = visual_video_get_depth (video);
			bin->depthforcedmain = visual_video_get_depth (video);

			visual_log (VISUAL_LOG_INFO, _("Switching from Gl TO framebuffer for real, framebuffer depth: %d"),
			            visual_video_get_depth (video));
		}

		visual_log (VISUAL_LOG_INFO, _("Target depth selected: %d"), depth);
	}

	video = visual_video_new_with_buffer (width, height, depth);

	visual_log (VISUAL_LOG_INFO, _("video pitch of that what connects to the new actor %d"),
	            visual_video_get_pitch (video));

	visual_actor_set_video (actor, video);

	bin->actmorphvideo = video;
	bin->actmorphmanaged = TRUE;

	visual_log (VISUAL_LOG_INFO, _("switching... ******************************************"));
	visual_bin_switch_actor (bin, actor);

	visual_log (VISUAL_LOG_INFO, _("end switch actor by name function ******************"));
}

void visual_bin_switch_actor (VisBin *bin, VisActor *actor)
{
	VisVideo *privvid;

	visual_return_if_fail (bin != NULL);
	visual_return_if_fail (actor != NULL);

	/* Set the new actor */
	bin->actmorph = actor;

	visual_log (VISUAL_LOG_DEBUG, "entering...");

	/* Free the private video */
	if (bin->privvid != NULL) {
		visual_video_unref (bin->privvid);

		bin->privvid = NULL;
	}

	visual_log (VISUAL_LOG_INFO, _("depth of the main actor: %d"),
	            visual_video_get_depth (bin->actor->video));

	/* Starting the morph, but first check if we don't have anything todo with openGL */
	if (bin->morphstyle == VISUAL_SWITCH_STYLE_MORPH &&
			visual_video_get_depth (bin->actor->video) != VISUAL_VIDEO_DEPTH_GL &&
			visual_video_get_depth (bin->actmorph->video) != VISUAL_VIDEO_DEPTH_GL &&
			!bin->depthfromGL) {

		if (bin->morph != NULL && bin->morph->plugin != NULL) {
			visual_morph_set_rate (bin->morph, 0);

			visual_morph_set_video (bin->morph, bin->actvideo);

			if (bin->morphautomatic)
				visual_morph_set_mode (bin->morph, bin->morphmode);
			else
				visual_morph_set_mode (bin->morph, VISUAL_MORPH_MODE_SET);

			visual_morph_set_time (bin->morph, bin->morphtime);
			visual_morph_set_steps (bin->morph, bin->morphsteps);
		}

		bin->morphrate = 0;
		bin->morphstepsdone = 0;

		visual_log (VISUAL_LOG_DEBUG, "phase 1");
		/* Allocate a private video for the main actor, so the morph
		 * can draw to the framebuffer */
		privvid = visual_video_new ();

		visual_log (VISUAL_LOG_DEBUG, "actvideo->depth %d actmorph->video->depth %d",
		            visual_video_get_depth (bin->actvideo),
		            visual_video_get_depth (bin->actmorph->video));

		visual_log (VISUAL_LOG_DEBUG, "phase 2");
		visual_video_copy_attrs (privvid, bin->actvideo);
		visual_log (VISUAL_LOG_DEBUG, "phase 3 pitch privvid %d actvideo %d",
		            visual_video_get_pitch (privvid),
		            visual_video_get_pitch (bin->actvideo));

		visual_video_allocate_buffer (privvid);

		visual_log (VISUAL_LOG_DEBUG, "phase 4");
		/* Initial privvid initialize */

		visual_log (VISUAL_LOG_DEBUG, "actmorph->video->depth %d %p",
		            visual_video_get_depth (bin->actmorph->video),
		            (void *) visual_video_get_pixels (bin->actvideo));

		if (visual_video_get_pixels (bin->actvideo) != NULL && visual_video_get_pixels (privvid) != NULL)
			visual_mem_copy (visual_video_get_pixels (privvid), visual_video_get_pixels (bin->actvideo),
					visual_video_get_size (privvid));
		else if (visual_video_get_pixels (privvid) != NULL)
			visual_mem_set (visual_video_get_pixels (privvid), 0, visual_video_get_size (privvid));

		visual_actor_set_video (bin->actor, privvid);
		bin->privvid = privvid;
	} else {
		visual_log (VISUAL_LOG_DEBUG, "Pointer actvideo->pixels %p", visual_video_get_pixels (bin->actvideo));
		if (visual_video_get_depth (bin->actor->video) != VISUAL_VIDEO_DEPTH_GL &&
				visual_video_get_pixels (bin->actvideo) != NULL) {
			visual_mem_set (visual_video_get_pixels (bin->actvideo), 0, visual_video_get_size (bin->actvideo));
		}
	}

	visual_log (VISUAL_LOG_DEBUG, "Leaving, actor->video->depth: %d actmorph->video->depth: %d",
	            visual_video_get_depth (bin->actor->video),
	            visual_video_get_depth (bin->actmorph->video));

	bin->morphing = TRUE;
}

void visual_bin_switch_finalize (VisBin *bin)
{
	int depthflag;

	visual_return_if_fail (bin != NULL);

	visual_log (VISUAL_LOG_DEBUG, "Entering...");
	if (bin->managed)
		visual_object_unref (VISUAL_OBJECT (bin->actor));

	/* Copy over the depth to be sure, and for GL plugins */
/*	bin->actvideo->depth = bin->actmorphvideo->depth;
	visual_video_set_depth (bin->actvideo, bin->actmorphvideo->depth); */

	if (bin->actmorphmanaged) {
		visual_video_unref (bin->actmorphvideo);

		bin->actmorphvideo = NULL;
	}

	if (bin->privvid != NULL) {
		visual_video_unref (bin->privvid);

		bin->privvid = NULL;
	}

	bin->actor = bin->actmorph;
	bin->actmorph = NULL;

	visual_actor_set_video (bin->actor, bin->actvideo);

	bin->morphing = FALSE;

	if (bin->morphmanaged) {
		visual_object_unref (VISUAL_OBJECT (bin->morph));
		bin->morph = NULL;
	}

	visual_log (VISUAL_LOG_DEBUG, " - in finalize - fscking depth from actvideo: %d %d",
	            visual_video_get_depth (bin->actvideo),
	            visual_video_get_bpp (bin->actvideo));

	depthflag = visual_actor_get_supported_depth (bin->actor);
	visual_video_set_depth (bin->actvideo, bin_get_suitable_depth (bin, depthflag));
	visual_bin_set_depth (bin, visual_video_get_depth (bin->actvideo));

	bin->depthforcedmain = visual_video_get_depth (bin->actvideo);
	visual_log (VISUAL_LOG_DEBUG, "bin->depthforcedmain in finalize %d", bin->depthforcedmain);

	/* FIXME replace with a depth fixer */
	if (bin->depthchanged) {
		visual_log (VISUAL_LOG_INFO, _("negotiate without event"));
		visual_actor_video_negotiate (bin->actor, bin->depthforcedmain, TRUE, TRUE);
		visual_log (VISUAL_LOG_INFO, _("end negotiate without event"));
	/*	visual_bin_sync (bin); */
	}

	visual_log (VISUAL_LOG_DEBUG, "Leaving...");
}

void visual_bin_switch_set_style (VisBin *bin, VisBinSwitchStyle style)
{
	visual_return_if_fail (bin != NULL);

	bin->morphstyle = style;
}

void visual_bin_switch_set_steps (VisBin *bin, int steps)
{
	visual_return_if_fail (bin != NULL);

	bin->morphsteps = steps;
}

void visual_bin_switch_set_automatic (VisBin *bin, int automatic)
{
	visual_return_if_fail (bin != NULL);

	bin->morphautomatic = automatic;
}

void visual_bin_switch_set_rate (VisBin *bin, float rate)
{
	visual_return_if_fail (bin != NULL);

	bin->morphrate = rate;
}

void visual_bin_switch_set_mode (VisBin *bin, VisMorphMode mode)
{
	visual_return_if_fail (bin != NULL);

	bin->morphmode = mode;
}

void visual_bin_switch_set_time (VisBin *bin, long sec, long usec)
{
	visual_return_if_fail (bin != NULL);

	visual_time_set (bin->morphtime, sec, usec * VISUAL_NSEC_PER_USEC);
}

void visual_bin_run (VisBin *bin)
{
	visual_return_if_fail (bin != NULL);
	visual_return_if_fail (bin->actor != NULL);
	visual_return_if_fail (bin->input != NULL);

	visual_input_run (bin->input);

	/* If we have a direct switch, do this BEFORE we run the actor,
	 * else we can get into trouble especially with GL, also when
	 * switching away from a GL plugin this is needed */
	if (bin->morphing) {
		/* We realize here, because it doesn't realize
		 * on switch, the reason for this is so that after a
		 * switch call, especially in a managed bin the
		 * depth can be requested and set, this is important
		 * for openGL plugins, the realize method checks
		 * for double realize itself so we don't have
		 * to check this, it's a bit hacky */
		visual_return_if_fail (bin->actmorph != NULL);
		visual_return_if_fail (bin->actmorph->plugin != NULL);

		if (!bin->actmorph->plugin->realized) {
			visual_actor_realize (bin->actmorph);

			if (bin->actmorphmanaged)
				visual_actor_video_negotiate (bin->actmorph, bin->depthforced, FALSE, TRUE);
			else
				visual_actor_video_negotiate (bin->actmorph, 0, FALSE, FALSE);
		}

		/* When we've got multiple switch events without a sync we need
		 * to realize the main actor as well */
		visual_return_if_fail (bin->actor->plugin != NULL);
		if (!bin->actor->plugin->realized) {
			visual_actor_realize (bin->actor);

			if (bin->managed)
				visual_actor_video_negotiate (bin->actor, bin->depthforced, FALSE, TRUE);
			else
				visual_actor_video_negotiate (bin->actor, 0, FALSE, FALSE);
		}

		/* When the style is DIRECT or the context is GL we shouldn't try
		 * to morph and instead finalize at once */
		visual_return_if_fail (bin->actor->video != NULL);
		if (bin->morphstyle == VISUAL_SWITCH_STYLE_DIRECT ||
			visual_video_get_depth (bin->actor->video) == VISUAL_VIDEO_DEPTH_GL) {

			visual_bin_switch_finalize (bin);

			/* We can't start drawing yet, the client needs to catch up with
			 * the depth change */
			return;
		}
	}

	/* We realize here because in a managed bin the depth for openGL is
	 * requested after the connect, thus we can realize there yet */
	visual_actor_realize (bin->actor);

	visual_actor_run (bin->actor, bin->input->audio);

	if (bin->morphing) {
		visual_return_if_fail (bin->actmorph != NULL);
		visual_return_if_fail (bin->actmorph->video != NULL);
		visual_return_if_fail (bin->actor->video != NULL);

		if (bin->morphstyle == VISUAL_SWITCH_STYLE_MORPH &&
			visual_video_get_depth (bin->actmorph->video) != VISUAL_VIDEO_DEPTH_GL &&
			visual_video_get_depth (bin->actor->video) != VISUAL_VIDEO_DEPTH_GL) {

			visual_actor_run (bin->actmorph, bin->input->audio);

			if (bin->morph == NULL || bin->morph->plugin == NULL) {
				visual_bin_switch_finalize (bin);
				return;
			}

			/* Same goes for the morph, we realize it here for depth changes
			 * (especially the openGL case */
			visual_morph_realize (bin->morph);
			visual_morph_run (bin->morph, bin->input->audio, bin->actor->video, bin->actmorph->video);

			if (visual_morph_is_done (bin->morph))
				visual_bin_switch_finalize (bin);
		} else {
/*			visual_bin_switch_finalize (bin); */
		}
	}
}
