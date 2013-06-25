/*
 * skeltrack-joint.c
 *
 * Skeltrack - A Free Software skeleton tracking library
 * Copyright (C) 2012 Igalia S.L.
 *
 * Authors:
 *  Joaquim Rocha <jrocha@igalia.com>
 *  Eduardo Lima Mitev <elima@igalia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License at http://www.gnu.org/licenses/lgpl-3.0.txt
 * for more details.
 */

/**
 * SECTION:skeltrack-joint
 * @short_description: Data structure that holds information about
 * a skeleton joint.
 *
 * A #SkeltrackJoint is built automatically by #SkeltrackSkeleton when
 * it finds a skeleton joint and can be used to get information about it.
 * Each #SkeltrackJoint holds an id, given by #SkeltrackJointId that indicates
 * which of the human skeleton joints it represents.
 *
 * Spacial information about a joint is given by the @x, @y and @z coordinates.
 * To represent the joint in a 2D, the variables @screen_x and
 * @screen_y will indicate the joint's position in the screen and are calculated
 * taking into account the #SkeltrackSkeleton:dimension-reduction (it will
 * be multiplied by this value).
 *
 * The tracked list of joints is represented by #SkeltrackJointList and given
 * by skeltrack_skeleton_track_joints_finish().
 * To get a #SkeltrackJoint from a #SkeltrackJointList object, use the
 * skeltrack_joint_list_get_joint() indicating the needed #SkeltrackJointId.
 *
 * A #SkeltrackJointList can be freed by using skeltrack_joint_list_free().
 * A #SkeltrackJoint can be copied by skeltrack_joint_copy() and freed by
 * skeltrack_joint_free().
 **/

#include <string.h>
#include "skeltrack-joint.h"

/**
 * skeltrack_joint_get_type:
 *
 * Returns: The registered #GType for #SkeltrackJoint boxed type
 **/
GType
skeltrack_joint_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
    type = g_boxed_type_register_static ("SkeltrackJoint",
                                         (GBoxedCopyFunc) skeltrack_joint_copy,
                                         (GBoxedFreeFunc) skeltrack_joint_free);
  return type;
}

/**
 * skeltrack_joint_copy:
 * @joint: The #SkeltrackJoint to copy
 *
 * Makes an exact copy of a #SkeltrackJoint object.
 *
 * Returns: (transfer full): A newly created #SkeltrackJoint. Use
 * skeltrack_joint_free() to free it.
 **/
gpointer
skeltrack_joint_copy (SkeltrackJoint *joint)
{
  SkeltrackJoint *new_joint;

  if (joint == NULL)
    return NULL;

  new_joint = g_slice_new0 (SkeltrackJoint);
  memcpy (new_joint, joint, sizeof (SkeltrackJoint));

  return new_joint;
}

/**
 * skeltrack_joint_free:
 * @joint: The #SkeltrackJoint to free
 *
 * Frees a #SkeltrackJoint object.
 **/
void
skeltrack_joint_free (SkeltrackJoint *joint)
{
  g_slice_free (SkeltrackJoint, joint);
}


/**
 * skeltrack_joint_list_free:
 * @list: The #SkeltrackJointList to free
 *
 * Frees a #SkeltrackJointList object and each #SkeltrackJoint
 * in it.
 **/
void
skeltrack_joint_list_free (SkeltrackJointList list)
{
  gint i;

  if (list == NULL)
    return;

  for (i = 0; i < SKELTRACK_JOINT_MAX_JOINTS; i++)
    {
      g_slice_free (SkeltrackJoint, list[i]);
    }
  g_slice_free1 (SKELTRACK_JOINT_MAX_JOINTS * sizeof (SkeltrackJoint *), list);
}

/**
 * skeltrack_joint_list_get_joint:
 * @list: The #SkeltrackJointList
 * @id: The #SkeltrackJointId of the joint to get
 *
 * Gets a joint from a list of skeleton joints. The joint
 * returned needs to be freed by using skeltrack_joint_free() or,
 * alternatively, the whole list and its joints can be freed by using
 * skeltrack_joint_list_free().
 *
 * Returns: (transfer full): The #SkeltrackJoint that corresponds to
 * the given @id or %NULL if that joint wasn't found.
 **/
SkeltrackJoint *
skeltrack_joint_list_get_joint (SkeltrackJointList list, SkeltrackJointId id)
{
  return list[id];
}

/**
 * skeltrack_joint_list_new:
 *
 * Created a new list of #SkeltrackJointsList with its joints as #NULL.
 * When it is no longer needed, free it with skeltrack_joint_list_free().
 *
 * Returns: (transfer full): A newly allocated #SkeltrackJointList
 **/
SkeltrackJointList
skeltrack_joint_list_new (void)
{
  return (SkeltrackJointList) g_slice_alloc0 (SKELTRACK_JOINT_MAX_JOINTS *
                                              sizeof (SkeltrackJoint *));
}
