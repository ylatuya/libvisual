/* Libvisual - The audio visualisation framework.
 *
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id: lv_actor.c,v 1.39.2.1 2006/03/04 12:32:47 descender Exp $
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
#include "lv_actor.h"
#include "lv_common.h"
#include "lv_plugin_registry.h"
#include <cstring>
#include <vector>
#include <functional>
#include <algorithm>

namespace {


  inline LV::PluginList const&
  get_actor_plugin_list ()
  {
      return LV::PluginRegistry::instance()->get_plugins_by_type (VISUAL_PLUGIN_TYPE_ACTOR);
  }

} // LV namespace


static void actor_dtor (VisObject *object);

static VisActorPlugin *get_actor_plugin (VisActor *actor);
static int negotiate_video_with_unsupported_depth (VisActor *actor, VisVideoDepth rundepth, int noevent, int forced);
static int negotiate_video (VisActor *actor, int noevent);

static int visual_actor_init (VisActor *actor, const char *actorname);

static void actor_dtor (VisObject *object)
{
    VisActor *actor = VISUAL_ACTOR (object);

    if (actor->plugin) {
        {
            // FIXME: Hack to free songinfo
            VisActorPlugin *actplugin = (VisActorPlugin *) actor->plugin->info->plugin;
            visual_songinfo_free (actplugin->songinfo);
        }

        visual_plugin_unload (actor->plugin);
    }

    if (actor->ditherpal)
        visual_palette_free (actor->ditherpal);

    if (actor->transform)
        visual_video_unref (actor->transform);

    if (actor->fitting)
        visual_video_unref (actor->fitting);

    if (actor->video)
        visual_video_unref (actor->video);

    visual_songinfo_free (actor->songcompare);
}

static VisActorPlugin *get_actor_plugin (VisActor *actor)
{
    visual_return_val_if_fail (actor != nullptr, nullptr);
    visual_return_val_if_fail (actor->plugin != nullptr, nullptr);

    return VISUAL_ACTOR_PLUGIN (actor->plugin->info->plugin);
}

VisPluginData *visual_actor_get_plugin (VisActor *actor)
{
    return actor->plugin;
}

const char *visual_actor_get_next_by_name_gl (const char *name)
{
    const char *next = nullptr;
    bool have_gl;

    do {
        next = visual_actor_get_next_by_name (next);
        if (!next)
            return nullptr;

        VisPluginData*  plugin    = visual_plugin_load (VISUAL_PLUGIN_TYPE_ACTOR, next);
        VisActorPlugin* actplugin = VISUAL_ACTOR_PLUGIN (plugin->info->plugin);

        have_gl = (actplugin->vidoptions.depth & VISUAL_VIDEO_DEPTH_GL) > 0;

        visual_plugin_unload (plugin);

    } while (!have_gl);

    return next;
}

const char *visual_actor_get_prev_by_name_gl (const char *name)
{
    const char *prev = name;
    bool have_gl;

    do {
        prev = visual_actor_get_prev_by_name (prev);
        if (!prev)
            return nullptr;

        VisPluginData*  plugin    = visual_plugin_load (VISUAL_PLUGIN_TYPE_ACTOR, prev);
        VisActorPlugin* actplugin = VISUAL_ACTOR_PLUGIN (plugin->info->plugin);

        have_gl = (actplugin->vidoptions.depth & VISUAL_VIDEO_DEPTH_GL) > 0;

        visual_plugin_unload (plugin);

    } while (!have_gl);

    return prev;
}

const char *visual_actor_get_next_by_name_nogl (const char *name)
{
    const char *next = name;
    bool have_gl;

    do {
        next = visual_actor_get_next_by_name (next);
        if (!next)
            return nullptr;

        VisPluginData*  plugin    = visual_plugin_load (VISUAL_PLUGIN_TYPE_ACTOR, next);
        VisActorPlugin* actplugin = VISUAL_ACTOR_PLUGIN (plugin->info->plugin);

        have_gl = (actplugin->vidoptions.depth & VISUAL_VIDEO_DEPTH_GL) > 0;

        visual_plugin_unload (plugin);

    } while (have_gl);

    return next;
}

const char *visual_actor_get_prev_by_name_nogl (const char *name)
{
    const char *prev = name;
    bool have_gl;

    do {
        prev = visual_actor_get_prev_by_name (prev);
        if (!prev)
            return nullptr;

        VisPluginData*  plugin    = visual_plugin_load (VISUAL_PLUGIN_TYPE_ACTOR, prev);
        VisActorPlugin* actplugin = VISUAL_ACTOR_PLUGIN (plugin->info->plugin);

        have_gl = (actplugin->vidoptions.depth & VISUAL_VIDEO_DEPTH_GL) > 0;

        visual_plugin_unload (plugin);

    } while (have_gl);

    return prev;
}

const char *visual_actor_get_next_by_name (const char *name)
{
    return LV::plugin_get_next_by_name (get_actor_plugin_list (), name);
}

const char *visual_actor_get_prev_by_name (char const* name)
{
    return LV::plugin_get_prev_by_name (get_actor_plugin_list (), name);
}

VisActor *visual_actor_new (const char *actorname)
{
    VisActor *actor;
    int result;

    actor = visual_mem_new0 (VisActor, 1);

    result = visual_actor_init (actor, actorname);
    if (result != VISUAL_OK) {
        visual_mem_free (actor);
        return nullptr;
    }

    return actor;
}

int visual_actor_init (VisActor *actor, const char *actorname)
{
    VisPluginEnviron *enve;
    VisActorPluginEnviron *actenviron;

    visual_return_val_if_fail (actor != nullptr, -VISUAL_ERROR_ACTOR_NULL);

    if (actorname && get_actor_plugin_list ().empty ()) {
        visual_log (VISUAL_LOG_ERROR, "the plugin list is empty");

        return -VISUAL_ERROR_PLUGIN_NO_LIST;
    }

    /* Do the VisObject initialization */
    visual_object_init (VISUAL_OBJECT (actor), actor_dtor);

    /* Reset the VisActor data */
    actor->plugin = nullptr;
    actor->video = nullptr;
    actor->transform = nullptr;
    actor->fitting = nullptr;
    actor->ditherpal = nullptr;

    actor->songcompare = visual_songinfo_new (VISUAL_SONGINFO_TYPE_NULL);

    if (!actorname)
        return VISUAL_OK;

    if (!LV::PluginRegistry::instance()->has_plugin (VISUAL_PLUGIN_TYPE_ACTOR, actorname)) {
        return -VISUAL_ERROR_PLUGIN_NOT_FOUND;
    }

    actor->plugin = visual_plugin_load (VISUAL_PLUGIN_TYPE_ACTOR, actorname);

    // FIXME: Hack to initialize songinfo
    {
        VisActorPlugin *actplugin = (VisActorPlugin *) actor->plugin->info->plugin;
        actplugin->songinfo = visual_songinfo_new (VISUAL_SONGINFO_TYPE_NULL);
    }

    /* Adding the VisActorPluginEnviron */
    actenviron = visual_mem_new0 (VisActorPluginEnviron, 1);
    visual_object_init (VISUAL_OBJECT (actenviron), nullptr);

    enve = visual_plugin_environ_new (VISUAL_ACTOR_PLUGIN_ENVIRON, VISUAL_OBJECT (actenviron));
    visual_plugin_environ_add (actor->plugin, enve);

    return VISUAL_OK;
}

void visual_actor_realize (VisActor *actor)
{
    visual_return_if_fail (actor != nullptr);
    visual_return_if_fail (actor->plugin != nullptr);

    visual_plugin_realize (actor->plugin);
}

VisSongInfo *visual_actor_get_songinfo (VisActor *actor)
{
    VisActorPlugin *actplugin;

    visual_return_val_if_fail (actor != nullptr, nullptr);

    actplugin = get_actor_plugin (actor);
    visual_return_val_if_fail (actplugin != nullptr, nullptr);

    return actplugin->songinfo;
}

VisPalette *visual_actor_get_palette (VisActor *actor)
{
    VisActorPlugin *actplugin;

    visual_return_val_if_fail (actor != nullptr, nullptr);

    actplugin = get_actor_plugin (actor);

    if (!actplugin) {
        visual_log (VISUAL_LOG_ERROR,
            "The given actor does not reference any actor plugin");
        return nullptr;
    }

    if (actor->transform &&
        visual_video_get_depth (actor->video) == VISUAL_VIDEO_DEPTH_8BIT) {

        return actor->ditherpal;

    } else {
        return actplugin->palette (visual_actor_get_plugin (actor));
    }

    return nullptr;
}

int visual_actor_video_negotiate (VisActor *actor, VisVideoDepth rundepth, int noevent, int forced)
{
    visual_return_val_if_fail (actor != nullptr, -VISUAL_ERROR_ACTOR_NULL);
    visual_return_val_if_fail (actor->plugin != nullptr, -VISUAL_ERROR_PLUGIN_NULL);
    visual_return_val_if_fail (actor->video != nullptr, -VISUAL_ERROR_ACTOR_VIDEO_NULL);

    if (actor->transform) {
        visual_video_unref (actor->transform);
        actor->transform = nullptr;
    }

    if (actor->fitting) {
        visual_video_unref (actor->fitting);
        actor->fitting = nullptr;
    }

    if (actor->ditherpal) {
        visual_palette_free (actor->ditherpal);
        actor->ditherpal = nullptr;
    }

    visual_log (VISUAL_LOG_INFO, "Negotiating plugin %s", actor->plugin->info->name);

    // Set up any required intermediary pixel buffers

    int depthflag = visual_actor_get_supported_depth (actor);

	if (!visual_video_depth_is_supported (depthflag, visual_video_get_depth (actor->video)) ||
			(forced && visual_video_get_depth (actor->video) != rundepth))
        return negotiate_video_with_unsupported_depth (actor, rundepth, noevent, forced);
    else
        return negotiate_video (actor, noevent);

    return -VISUAL_ERROR_IMPOSSIBLE;
}

static int negotiate_video_with_unsupported_depth (VisActor *actor, VisVideoDepth rundepth, int noevent, int forced)
{
    VisActorPlugin *actplugin = get_actor_plugin (actor);

    int depthflag = visual_actor_get_supported_depth (actor);

    VisVideoDepth req_depth = forced ? rundepth : visual_video_depth_get_highest_nogl (depthflag);

    /* If there is only GL (which gets returned by highest nogl if
     * nothing else is there, stop here */
    if (req_depth == VISUAL_VIDEO_DEPTH_GL)
        return -VISUAL_ERROR_ACTOR_GL_NEGOTIATE;

    int req_width  = visual_video_get_width (actor->video);
    int req_height = visual_video_get_height (actor->video);

    actplugin->requisition (visual_actor_get_plugin (actor), &req_width, &req_height);

    actor->transform = visual_video_new_with_buffer (req_width, req_height, req_depth);

    if (visual_video_get_depth (actor->video) == VISUAL_VIDEO_DEPTH_8BIT)
        actor->ditherpal = visual_palette_new (256);

    if (!noevent) {
        visual_event_queue_add (actor->plugin->eventqueue,
                                visual_event_new_resize (req_width, req_height));
    }

    return VISUAL_OK;
}

static int negotiate_video (VisActor *actor, int noevent)
{
    VisActorPlugin *actplugin = get_actor_plugin (actor);

    int req_width  = visual_video_get_width  (actor->video);
    int req_height = visual_video_get_height (actor->video);

    actplugin->requisition (visual_actor_get_plugin (actor), &req_width, &req_height);

    // Size fitting enviroment
    if (req_width != visual_video_get_width (actor->video) || req_height != visual_video_get_height (actor->video)) {
        if (visual_video_get_depth (actor->video) != VISUAL_VIDEO_DEPTH_GL) {
            actor->fitting = visual_video_new_with_buffer (req_width, req_height, visual_video_get_depth (actor->video));
        }

        visual_video_set_dimension (actor->video, req_width, req_height);
    }

    // FIXME: This should be moved into the if block above. It's out
    // here because plugins depend on this to receive information
    // about initial dimensions
    if (!noevent) {
        visual_event_queue_add (actor->plugin->eventqueue,
                                visual_event_new_resize (req_width, req_height));
    }

    return VISUAL_OK;
}

/**
 * Gives the by the plugin natively supported depths
 *
 * @param actor Pointer to a VisActor of which the supported depth of it's
 *    encapsulated plugin is requested.
 *
 * @return an OR value of the VISUAL_VIDEO_DEPTH_* values which can be checked against using AND on success,
 *  -VISUAL_ERROR_ACTOR_NULL, -VISUAL_ERROR_PLUGIN_NULL or -VISUAL_ERROR_ACTOR_PLUGIN_NULL on failure.
 */
VisVideoDepth visual_actor_get_supported_depth (VisActor *actor)
{
    VisActorPlugin *actplugin;

    visual_return_val_if_fail (actor != nullptr, VISUAL_VIDEO_DEPTH_NONE);
    visual_return_val_if_fail (actor->plugin != nullptr, VISUAL_VIDEO_DEPTH_NONE);

    actplugin = get_actor_plugin (actor);

    if (!actplugin)
        return VISUAL_VIDEO_DEPTH_NONE;

    return actplugin->vidoptions.depth;
}

VisVideoAttrOptions *visual_actor_get_video_attribute_options (VisActor *actor)
{
    VisActorPlugin *actplugin;

    visual_return_val_if_fail (actor != nullptr, nullptr);
    visual_return_val_if_fail (actor->plugin != nullptr, nullptr);

    actplugin = get_actor_plugin (actor);

    if (!actplugin)
        return nullptr;

    return &actplugin->vidoptions;
}

void visual_actor_set_video (VisActor *actor, VisVideo *video)
{
    visual_return_if_fail (actor != nullptr);

    if (actor->video && actor->video != video) {
        visual_video_unref (actor->video);
    }

    actor->video = video;

    if (actor->video) {
        visual_video_ref (actor->video);
    }
}

void visual_actor_run (VisActor *actor, VisAudio *audio)
{
    VisActorPlugin *actplugin;
    VisPluginData *plugin;
    VisVideo *video;
    VisVideo *transform;
    VisVideo *fitting;

    /* We don't check for video, because we don't always need a video */
    /*
     * Really? take a look at visual_video_set_palette bellow
     */
    visual_return_if_fail (actor != nullptr);
    visual_return_if_fail (actor->video != nullptr);
    visual_return_if_fail (audio != nullptr);

    actplugin = get_actor_plugin (actor);
    plugin = visual_actor_get_plugin (actor);

    if (!actplugin) {
        visual_log (VISUAL_LOG_ERROR, "The given actor does not reference any actor plugin");
        return;
    }

    /* Songinfo handling */
    if (!visual_songinfo_compare (actor->songcompare, actplugin->songinfo) ||
        visual_songinfo_get_elapsed (actor->songcompare) != visual_songinfo_get_elapsed (actplugin->songinfo)) {

        visual_songinfo_mark (actplugin->songinfo);

        visual_event_queue_add (visual_plugin_get_eventqueue (plugin),
                                visual_event_new_newsong (actplugin->songinfo));

        visual_songinfo_copy (actor->songcompare, actplugin->songinfo);
    }

    video = actor->video;
    transform = actor->transform;
    fitting = actor->fitting;

    /*
     * This needs to happen before palette, render stuff, always, period.
     * Also internal vars can be initialized when params have been set in init on the param
     * events in the event loop.
     */
    visual_plugin_events_pump (actor->plugin);

    /* Set the palette to the target video */
    visual_video_set_palette (video, visual_actor_get_palette (actor));

    /* Yeah some transformation magic is going on here when needed */
    if (transform && (visual_video_get_depth (transform) != visual_video_get_depth (video))) {
        actplugin->render (plugin, transform, audio);

        if (visual_video_get_depth (transform) == VISUAL_VIDEO_DEPTH_8BIT) {
            visual_video_set_palette (transform, visual_actor_get_palette (actor));
            visual_video_convert_depth (video, transform);
        } else {
            visual_video_set_palette (transform, actor->ditherpal);
            visual_video_convert_depth (video, transform);
        }
    } else {
        if (fitting && (visual_video_get_width (fitting) != visual_video_get_width (video) || visual_video_get_height (fitting) != visual_video_get_height (video))) {
            actplugin->render (plugin, fitting, audio);
            visual_video_blit (video, fitting, 0, 0, FALSE);
        } else {
            actplugin->render (plugin, video, audio);
        }
    }
}
