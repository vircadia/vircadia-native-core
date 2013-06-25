/*
 * skeltrack.h
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

#ifndef __SKELTRACK_SKELETON_H__
#define __SKELTRACK_SKELETON_H__

#include <skeltrack-joint.h>
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define SKELTRACK_TYPE_SKELETON (skeltrack_skeleton_get_type ())
#define SKELTRACK_SKELETON(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SKELTRACK_TYPE_SKELETON, SkeltrackSkeleton))
#define SKELTRACK_SKELETON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SKELTRACK_TYPE_SKELETON, SkeltrackSkeletonClass))
#define SKELTRACK_IS_SKELETON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SKELTRACK_TYPE_SKELETON))
#define SKELTRACK_IS_SKELETON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SKELTRACK_TYPE_SKELETON))
#define SKELTRACK_SKELETON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SKELTRACK_TYPE_SKELETON, SkeltrackSkeletonClass))

typedef struct _SkeltrackSkeleton SkeltrackSkeleton;
typedef struct _SkeltrackSkeletonClass SkeltrackSkeletonClass;
typedef struct _SkeltrackSkeletonPrivate SkeltrackSkeletonPrivate;

struct _SkeltrackSkeleton
{
  GObject parent;

  /*< private >*/
  SkeltrackSkeletonPrivate *priv;
};

/**
 * SkeltrackSkeletonClass:
 **/
struct _SkeltrackSkeletonClass
{
  GObjectClass parent_class;
};

GType                 skeltrack_skeleton_get_type               (void) G_GNUC_CONST;

SkeltrackSkeleton *   skeltrack_skeleton_new                    (void);

void                  skeltrack_skeleton_track_joints           (SkeltrackSkeleton   *self,
                                                                 guint16             *buffer,
                                                                 guint                width,
                                                                 guint                height,
                                                                 GCancellable        *cancellable,
                                                                 GAsyncReadyCallback  callback,
                                                                 gpointer             user_data);

SkeltrackJointList    skeltrack_skeleton_track_joints_finish    (SkeltrackSkeleton *self,
                                                                 GAsyncResult      *result,
                                                                 GError           **error);

SkeltrackJointList    skeltrack_skeleton_track_joints_sync      (SkeltrackSkeleton   *self,
                                                                 guint16             *buffer,
                                                                 guint                width,
                                                                 guint                height,
                                                                 GCancellable        *cancellable,
                                                                 GError             **error);

void                  skeltrack_skeleton_get_focus_point        (SkeltrackSkeleton   *self,
                                                                 gint                *x,
                                                                 gint                *y,
                                                                 gint                *z);

void                  skeltrack_skeleton_set_focus_point        (SkeltrackSkeleton   *self,
                                                                 gint                 x,
                                                                 gint                 y,
                                                                 gint                 z);

G_END_DECLS

#endif /* __SKELTRACK_SKELETON_H__ */
