/*
 * skeltrak-joint.h
 *
 * Skeltrack - A Free Software skeleton tracking library
 * Copyright (C) 2012 Igalia S.L.
 *
 * Authors:
 *  Joaquim Rocha <jrocha@igalia.com>
 *  Eduardo Lima Mitev <elima@igalia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 3, or (at your option) any later version as published by
 * the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License at http://www.gnu.org/licenses/lgpl-3.0.txt
 * for more details.
 */

#ifndef __SKELTRACK_JOINT_H__
#define __SKELTRACK_JOINT_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define SKELTRACK_TYPE_JOINT (skeltrack_joint_get_type ())
#define SKELTRACK_JOINT_MAX_JOINTS 7

typedef struct _SkeltrackJoint SkeltrackJoint;
typedef SkeltrackJoint **SkeltrackJointList;

/**
 * SkeltrackJointId:
 * @SKELTRACK_JOINT_ID_HEAD: The head
 * @SKELTRACK_JOINT_ID_LEFT_SHOULDER: The left shoulder
 * @SKELTRACK_JOINT_ID_RIGHT_SHOULDER: The right shoulder
 * @SKELTRACK_JOINT_ID_LEFT_ELBOW: The left elbow
 * @SKELTRACK_JOINT_ID_RIGHT_ELBOW: The right elbow
 * @SKELTRACK_JOINT_ID_LEFT_HAND: The left hand
 * @SKELTRACK_JOINT_ID_RIGHT_HAND: The right hand
 *
 * Available joint ids.
 **/
typedef enum {
  SKELTRACK_JOINT_ID_HEAD,
  SKELTRACK_JOINT_ID_LEFT_SHOULDER,
  SKELTRACK_JOINT_ID_RIGHT_SHOULDER,
  SKELTRACK_JOINT_ID_LEFT_ELBOW,
  SKELTRACK_JOINT_ID_RIGHT_ELBOW,
  SKELTRACK_JOINT_ID_LEFT_HAND,
  SKELTRACK_JOINT_ID_RIGHT_HAND
} SkeltrackJointId;

/**
 * SkeltrackJoint:
 * @id: The id of the joint
 * @x: The x coordinate of the joint in the space (in mm)
 * @y: The y coordinate of the joint in the space (in mm)
 * @z: The z coordinate of the joint in the space (in mm)
 * @screen_x: The x coordinate of the joint in the screen (in pixels)
 * @screen_y: The y coordinate of the joint in the screen (in pixels)
 **/
struct _SkeltrackJoint
{
  SkeltrackJointId id;

  gint x;
  gint y;
  gint z;

  gint screen_x;
  gint screen_y;
};

GType                skeltrack_joint_get_type          (void);
gpointer             skeltrack_joint_copy              (SkeltrackJoint    *joint);
void                 skeltrack_joint_free              (SkeltrackJoint    *joint);
void                 skeltrack_joint_list_free         (SkeltrackJointList list);
SkeltrackJointList   skeltrack_joint_list_new          (void);
SkeltrackJoint *     skeltrack_joint_list_get_joint    (SkeltrackJointList list,
                                                        SkeltrackJointId   id);

G_END_DECLS

#endif /* __SKELTRACK_JOINT_H__ */
