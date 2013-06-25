/*
 * skeltrack-util.h
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

#include <glib.h>
#include "skeltrack-joint.h"

typedef struct _Label Label;
typedef struct _Node Node;

struct _Label {
  gint index;
  Label *parent;
  GList *nodes;
  Node *bridge_node;
  Node *to_node;
  gint lower_screen_y;
  gint higher_z;
  gint lower_z;
  gdouble normalized_num_nodes;
};

struct _Node {
  gint i;
  gint j;
  gint x;
  gint y;
  gint z;
  GList *neighbors;
  GList *linked_nodes;
  Label *label;
};

Node *        get_closest_node_to_joint        (GList *extremas,
                                                SkeltrackJoint *joint,
                                                gint *distance);

Node *        get_closest_node                 (GList *node_list, Node *from);

Node *        get_closest_torso_node           (GList *node_list,
                                                Node  *from,
                                                Node  *head);

Label *       get_main_component               (GList   *node_list,
                                                Node    *from,
                                                gdouble  min_normalized_nr_nodes);

Label *       label_find                       (Label *label);

void          label_union                      (Label *a, Label *b);

gint          get_distance                     (Node *a, Node *b);

void          free_label                       (Label *label);

void          clean_labels                     (GList *labels);

void          free_node                        (Node *node,
                                                gboolean unlink_node_first);

void          clean_nodes                      (GList *nodes);

GList *       remove_nodes_with_label          (GList *nodes,
                                                Node **node_matrix,
                                                gint width,
                                                Label *label);

Label *       get_lowest_index_label           (Label **neighbor_labels);

Label *       new_label                        (gint index);

void          join_components_to_main          (GList *nodes,
                                                Label *lowest_component_label,
                                                guint horizontal_max_distance,
                                                guint depth_max_distance,
                                                guint graph_distance_threshold);

void          set_joint_from_node              (SkeltrackJointList *joints,
                                                Node *node,
                                                SkeltrackJointId id,
                                                gint dimension_reduction);

gint *        create_new_dist_matrix           (gint matrix_size);

gboolean      dijkstra_to                      (GList *nodes,
                                                Node *source,
                                                Node *target,
                                                gint width,
                                                gint height,
                                                gint *distances,
                                                Node **previous);

void          convert_screen_coords_to_mm      (guint width,
                                                guint height,
                                                guint dimension_reduction,
                                                guint i,
                                                guint j,
                                                gint  z,
                                                gint *x,
                                                gint *y);

void          convert_mm_to_screen_coords      (guint  width,
                                                guint  height,
                                                guint  dimension_reduction,
                                                gint   x,
                                                gint   y,
                                                gint   z,
                                                guint *i,
                                                guint *j);
