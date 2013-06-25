/*
 * skeltrack-util.c
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

#include <math.h>

#include "skeltrack-util.h"

/* @TODO: Expose these to the user */
static const gfloat SCALE_FACTOR = .0021;
static const gint MIN_DISTANCE = -10.0;

static SkeltrackJoint *
node_to_joint (Node *node, SkeltrackJointId id, gint dimension_reduction)
{
  SkeltrackJoint *joint;

  if (node == NULL)
    return NULL;

  joint = g_slice_new0 (SkeltrackJoint);
  joint->id = id;
  joint->x = node->x;
  joint->y = node->y;
  joint->z = node->z;
  joint->screen_x = node->i * dimension_reduction;
  joint->screen_y = node->j * dimension_reduction;

  return joint;
}

static guint
get_distance_from_joint (Node *node, SkeltrackJoint *joint)
{
  guint dx, dy, dz;
  dx = ABS (node->x - joint->x);
  dy = ABS (node->y - joint->y);
  dz = ABS (node->z - joint->z);
  return sqrt (dx * dx + dy * dy + dz * dz);
}

static void
unlink_node (Node *node)
{
  Node *neighbor;
  GList *current_neighbor;

  for (current_neighbor = g_list_first (node->neighbors);
       current_neighbor != NULL;
       current_neighbor = g_list_next (current_neighbor))
    {
      neighbor = (Node *) current_neighbor->data;
      neighbor->neighbors = g_list_remove (neighbor->neighbors, node);
    }

  for (current_neighbor = g_list_first (node->linked_nodes);
       current_neighbor != NULL;
       current_neighbor = g_list_next (current_neighbor))
    {
      neighbor = (Node *) current_neighbor->data;
      neighbor->linked_nodes = g_list_remove (neighbor->linked_nodes, node);
    }

  g_list_free (node->neighbors);
  g_list_free (node->linked_nodes);
  node->neighbors = NULL;
  node->linked_nodes = NULL;
}

static Node *
get_closest_node_with_distances (GList *node_list,
                                 Node *from,
                                 guint x_dist,
                                 guint y_dist,
                                 guint z_dist,
                                 gint *closest_node_dist)
{
  Node *closest = NULL;
  gint distance = -1;
  GList *current_node;

  /* @TODO: Replace this and use closest pair of points
     algorithm and ensure O(n log n) instead of brute-force */

  for (current_node = g_list_first (node_list);
       current_node != NULL;
       current_node = g_list_next (current_node))
    {
      guint dx, dy, dz;
      Node *node;
      gint current_distance;
      node = (Node *) current_node->data;

      dx = ABS (from->x - node->x);
      dy = ABS (from->y - node->y);
      dz = ABS (from->z - node->z);

      if (dx > x_dist || dy > y_dist || dz > z_dist)
        continue;

      current_distance = sqrt (dx * dx + dy * dy + dz * dz);
      if (closest == NULL || distance > current_distance)
        {
          closest = node;
          distance = current_distance;
        }
    }

  *closest_node_dist = distance;
  return closest;
}

Node *
get_closest_node_to_joint (GList *extremas,
                           SkeltrackJoint *joint,
                           gint *distance)
{
  GList *current_node;
  gint dist = -1;
  Node *closest_node = NULL;

  for (current_node = g_list_first (extremas);
       current_node != NULL;
       current_node = g_list_next (current_node))
    {
      guint current_dist;
      Node *node = (Node *) current_node->data;
      if (node == NULL)
        continue;

      current_dist = get_distance_from_joint (node, joint);
      if (dist == -1 || current_dist < dist)
        {
          closest_node = node;
          dist = current_dist;
        }
    }
  *distance = dist;
  return closest_node;
}

gint
get_distance (Node *a, Node *b)
{
  guint dx, dy, dz;
  dx = ABS (a->x - b->x);
  dy = ABS (a->y - b->y);
  dz = ABS (a->z - b->z);
  return sqrt (dx * dx + dy * dy + dz * dz);
}

Node *
get_closest_torso_node (GList *node_list, Node *from, Node *head)
{
  Node *closest = NULL;
  gint distance = -1;
  GList *current_node;

  /* @TODO: Replace this and use closest pair of points
     algorithm and ensure O(n log n) instead of brute-force */

  for (current_node = g_list_first (node_list);
       current_node != NULL;
       current_node = g_list_next (current_node))
    {
      Node *node;
      gint current_distance;
      node = (Node *) current_node->data;
      if (node->z >= head->z &&
          node->y >= from->y)
        {
          current_distance = get_distance (node, from);
          if (closest == NULL || current_distance < distance)
            {
              closest = node;
              distance = current_distance;
            }
        }
    }
  return closest;
}

Node *
get_closest_node (GList *node_list, Node *from)
{
  Node *closest = NULL;
  gint distance = -1;
  GList *current_node;

  /* @TODO: Replace this and use closest pair of points
     algorithm and ensure O(n log n) instead of brute-force */

  for (current_node = g_list_first (node_list);
       current_node != NULL;
       current_node = g_list_next (current_node))
    {
      Node *node;
      gint current_distance;
      node = (Node *) current_node->data;
      if (closest == NULL)
        {
          closest = node;
          distance = get_distance (node, from);
          continue;
        }
      current_distance = get_distance (node, from);
      if (current_distance < distance)
        {
          closest = node;
          distance = current_distance;
        }
    }
  return closest;
}

Label *
get_main_component (GList *node_list, Node *from, gdouble min_normalized_nr_nodes)
{
  Label *main_component = NULL;
  gint distance = -1;
  GList *current_node;

  for (current_node = g_list_first (node_list);
       current_node != NULL;
       current_node = g_list_next (current_node))
  {
    Node *node;
    Label *label;
    gint current_distance;
    node = (Node *) current_node->data;
    label = node->label;

    if (main_component == NULL &&
        label->normalized_num_nodes > min_normalized_nr_nodes)
      {
        main_component = label;
        distance = get_distance (node, from);
        continue;
      }

    current_distance = get_distance (node, from);
    if (current_distance < distance &&
        label->normalized_num_nodes > min_normalized_nr_nodes)
      {
        main_component = label;
        distance = current_distance;
      }
  }

  return main_component;
}

Label *
label_find (Label *label)
{
  Label *parent;

  g_return_val_if_fail (label != NULL, NULL);

  parent = label->parent;
  if (parent == label)
    return parent;
  else
    return label_find (parent);
}

void
label_union (Label *a, Label *b)
{
  Label *root_a, *root_b;
  root_a = label_find (a);
  root_b = label_find (b);
  if (root_a->index < root_b->index)
    {
      b->parent = root_a;
    }
  else
    {
      a->parent = root_b;
    }
}

void
free_label (Label *label)
{
  g_list_free (label->nodes);
  label->nodes = NULL;
  g_slice_free (Label, label);
}

void
clean_labels (GList *labels)
{
  GList *current = g_list_first (labels);
  while (current != NULL)
    {
      Label *label;
      label = (Label *) current->data;
      free_label (label);
      current = g_list_next (current);
    }
}

void
free_node (Node *node, gboolean unlink_node_first)
{
  if (unlink_node_first)
    {
      unlink_node (node);
    }
  else
    {
      g_list_free (node->neighbors);
      g_list_free (node->linked_nodes);
      node->neighbors = NULL;
      node->linked_nodes = NULL;
    }
  g_slice_free (Node, node);
}

void
clean_nodes (GList *nodes)
{
  GList *current = g_list_first (nodes);
  while (current != NULL)
    {
      Node *node;
      node = (Node *) current->data;
      free_node (node, FALSE);
      current = g_list_next (current);
    }
}

GList *
remove_nodes_with_label (GList *nodes,
                         Node **node_matrix,
                         gint width,
                         Label *label)
{
  Node *node;
  GList *link_to_delete, *current_node;

  current_node = g_list_first (nodes);
  while (current_node != NULL)
    {
      node = (Node *) current_node->data;
      if (node->label == label)
        {
          link_to_delete = current_node;
          current_node = g_list_next (current_node);
          nodes = g_list_delete_link (nodes, link_to_delete);
          node_matrix[width * node->j + node->i] = NULL;
          free_node (node, TRUE);
          continue;
        }
      current_node = g_list_next (current_node);
    }
  return nodes;
}

Label *
get_lowest_index_label (Label **neighbor_labels)
{
  guint index;
  Label *lowest_index_label = NULL;

  lowest_index_label = neighbor_labels[0];
  for (index = 1; index < 4; index++)
    {
      if (neighbor_labels[index] == NULL)
        continue;

      if (lowest_index_label == NULL ||
          lowest_index_label->index < neighbor_labels[index]->index)
        {
          lowest_index_label = neighbor_labels[index];
        }
    }

  return lowest_index_label;
}

Label *
new_label (gint index)
{
  Label *label = g_slice_new (Label);
  label->index = index;
  label->parent = label;
  label->nodes = NULL;
  label->bridge_node = NULL;
  label->to_node = NULL;
  label->lower_screen_y = -1;
  label->higher_z = -1;
  label->lower_z = -1;
  label->normalized_num_nodes = -1;

  return label;
}

void
join_components_to_main (GList *labels,
                         Label *main_component_label,
                         guint horizontal_max_distance,
                         guint depth_max_distance,
                         guint graph_distance_threshold)
{
  GList *current_label;

  for (current_label = g_list_first (labels);
       current_label != NULL;
       current_label = g_list_next (current_label))
    {
      gint closer_distance = -1;
      Label *label;
      GList *current_node, *nodes;

      label = (Label *) current_label->data;
      if (label == main_component_label)
        continue;

      /* Skip nodes behind main component */
      if (label->higher_z > main_component_label->higher_z +
          graph_distance_threshold)
          continue;

      nodes = label->nodes;
      for (current_node = g_list_first (nodes);
           current_node != NULL;
           current_node = g_list_next (current_node))
        {
          Node *node;
          gint current_distance;
          node = (Node *) current_node->data;
          /* Skip nodes that belong to the same component or
             that a not in the edge of their component */
          if (g_list_length (node->neighbors) == 8)
            continue;

          Node *closest_node =
            get_closest_node_with_distances (main_component_label->nodes,
                                             node,
                                             horizontal_max_distance,
                                             horizontal_max_distance,
                                             depth_max_distance,
                                             &current_distance);
          if (closest_node &&
              (current_distance < closer_distance ||
               closer_distance == -1))
            {
              node->label->bridge_node = node;
              node->label->to_node = closest_node;
              closer_distance = current_distance;
            }
        }
    }
}

void
set_joint_from_node (SkeltrackJointList *joints,
                     Node *node,
                     SkeltrackJointId id,
                     gint dimension_reduction)
{
  (*joints)[id] = node_to_joint (node, id, dimension_reduction);
}

gint *
create_new_dist_matrix (gint matrix_size)
{
  guint i;
  gint *distances;

  distances = g_slice_alloc0 (matrix_size * sizeof (gint));
  for (i = 0; i < matrix_size; i++)
    {
      distances[i] = -1;
    }

  return distances;
}

gboolean
dijkstra_to (GList *nodes, Node *source, Node *target,
             gint width, gint height,
             gint *distances, Node **previous)
{
  gint nr;
  GList *unvisited_nodes, *current;

  for (current = g_list_first (nodes);
       previous != NULL && current != NULL;
       current = g_list_next (current))
    {
      Node *node;
      node = (Node *) current->data;
      previous[node->j * width + node->i] = NULL;
    }
  distances[source->j * width + source->i] = 0;

  unvisited_nodes = g_list_copy (nodes);
  nr = 0;
  while (unvisited_nodes != NULL)
    {
      Node *node;
      GList *current_neighbor, *shorter_dist_node, *cur_node;

      shorter_dist_node = g_list_first (unvisited_nodes);
      cur_node = g_list_next (shorter_dist_node);
      while (cur_node != NULL)
        {
          Node *value, *shorter_dist;
          value = (Node *) cur_node->data;
          shorter_dist = (Node *) shorter_dist_node->data;
          if (distances[shorter_dist->j * width + shorter_dist->i] == -1 ||
              (distances[value->j * width + value->i] != -1 &&
               distances[value->j * width +
                         value->i] < distances[shorter_dist->j * width +
                                               shorter_dist->i]))
            {
              shorter_dist_node = cur_node;
            }
          cur_node = g_list_next (cur_node);
        }

      node = (Node *) shorter_dist_node->data;
      if (distances[node->j * width + node->i] == -1)
        {
          break;
        }

      current_neighbor = g_list_first (node->neighbors);
      while (current_neighbor)
        {
          gint dist;
          Node *neighbor;

          neighbor = (Node *) current_neighbor->data;
          dist = get_distance (node, neighbor) +
            distances[node->j * width + node->i];

          if (distances[neighbor->j * width + neighbor->i] == -1 ||
              dist < distances[neighbor->j * width + neighbor->i])
            {
              distances[neighbor->j * width + neighbor->i] = dist;
              if (previous != NULL)
                {
                  previous[neighbor->j * width + neighbor->i] = node;
                }
              nr++;
            }
          if (target != NULL && neighbor == target)
            {
              g_list_free (unvisited_nodes);
              return TRUE;
            }

          current_neighbor = g_list_next (current_neighbor);
        }
      unvisited_nodes = g_list_delete_link (unvisited_nodes, shorter_dist_node);
    }
  g_list_free (unvisited_nodes);
  return FALSE;
}

void
convert_screen_coords_to_mm (guint width,
                             guint height,
                             guint dimension_reduction,
                             guint i,
                             guint j,
                             gint  z,
                             gint *x,
                             gint *y)
{
  gfloat width_height_relation =
    width > height ? (gfloat) width / height : (gfloat) height / width;
  /* Formula from http://openkinect.org/wiki/Imaging_Information */
  *x = round((i * dimension_reduction - width * dimension_reduction / 2.0) *
             (z + MIN_DISTANCE) * SCALE_FACTOR * width_height_relation);
  *y = round((j * dimension_reduction - height * dimension_reduction / 2.0) *
             (z + MIN_DISTANCE) * SCALE_FACTOR);
}

void
convert_mm_to_screen_coords (guint  width,
                             guint  height,
                             guint  dimension_reduction,
                             gint   x,
                             gint   y,
                             gint   z,
                             guint *i,
                             guint *j)
{
  gfloat width_height_relation =
    width > height ? (gfloat) width / height : (gfloat) height / width;

  if (z + MIN_DISTANCE == 0)
    {
      *i = 0;
      *j = 0;
      return;
    }

  *i = round (width / 2.0 + x / ((gfloat) (z + MIN_DISTANCE) * SCALE_FACTOR *
                                 dimension_reduction * width_height_relation));
  *j = round (height / 2.0 + y / ((gfloat) (z + MIN_DISTANCE) * SCALE_FACTOR *
                                  dimension_reduction));
}
