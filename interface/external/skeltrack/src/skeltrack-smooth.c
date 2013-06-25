/*
 * skeltrack-smooth.c
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

#include "skeltrack-smooth.h"

static gfloat
holt_double_exp_formula_st (gfloat alpha,
                            gfloat previous_trend,
                            gfloat current_value,
                            gfloat previous_smoothed_value)
{
  return alpha * current_value +
    (1.0 - alpha) * (previous_smoothed_value + previous_trend);
}

static gfloat
holt_double_exp_formula_bt (gfloat beta,
                            gfloat previous_trend,
                            gfloat current_smoothed_value,
                            gfloat previous_smoothed_value)
{
  return beta * (current_smoothed_value - previous_smoothed_value) +
    (1.0 - beta) * previous_trend;
}

static void
holt_double_exp_joint (gfloat alpha,
                       gfloat beta,
                       SkeltrackJoint *smoothed_joint,
                       SkeltrackJoint *current_joint,
                       SkeltrackJoint *trend_joint)
{
  gfloat new_x, new_y, new_z, new_screen_x, new_screen_y;
  new_x = holt_double_exp_formula_st (alpha,
                                      trend_joint->x,
                                      current_joint->x,
                                      smoothed_joint->x);
  new_y = holt_double_exp_formula_st (alpha,
                                      trend_joint->y,
                                      current_joint->y,
                                      smoothed_joint->y);
  new_z = holt_double_exp_formula_st (alpha,
                                      trend_joint->z,
                                      current_joint->z,
                                      smoothed_joint->z);
  new_screen_x = holt_double_exp_formula_st (alpha,
                                             trend_joint->screen_x,
                                             current_joint->screen_x,
                                             smoothed_joint->screen_x);
  new_screen_y = holt_double_exp_formula_st (alpha,
                                             trend_joint->screen_y,
                                             current_joint->screen_y,
                                             smoothed_joint->screen_y);
  trend_joint->x = holt_double_exp_formula_bt (beta,
                                               trend_joint->x,
                                               new_x,
                                               smoothed_joint->x);
  trend_joint->y = holt_double_exp_formula_bt (beta,
                                               trend_joint->y,
                                               new_y,
                                               smoothed_joint->y);
  trend_joint->z = holt_double_exp_formula_bt (beta,
                                               trend_joint->z,
                                               new_z,
                                               smoothed_joint->z);
  trend_joint->screen_x = holt_double_exp_formula_bt (beta,
                                                      trend_joint->screen_x,
                                                      new_screen_x,
                                                      smoothed_joint->screen_x);
  trend_joint->screen_y = holt_double_exp_formula_bt (beta,
                                                      trend_joint->screen_y,
                                                      new_screen_y,
                                                      smoothed_joint->screen_y);
  smoothed_joint->x = new_x;
  smoothed_joint->y = new_y;
  smoothed_joint->z = new_z;
  smoothed_joint->screen_x = new_screen_x;
  smoothed_joint->screen_y = new_screen_y;
}

void
reset_joints_persistency_counter (SmoothData *smooth_data)
{
  guint i;
  for (i = 0; i < SKELTRACK_JOINT_MAX_JOINTS; i++)
    {
      smooth_data->joints_persistency_counter[i] =
        smooth_data->joints_persistency;
    }
}

static void
decrease_joints_persistency (SmoothData *smooth_data)
{
  guint i;
  for (i = 0; i < SKELTRACK_JOINT_MAX_JOINTS; i++)
    {
      SkeltrackJoint *smoothed_joint = NULL;
      SkeltrackJoint *trend_joint = NULL;

      if (smooth_data->smoothed_joints)
        smoothed_joint = smooth_data->smoothed_joints[i];
      if (smooth_data->trend_joints)
        trend_joint = smooth_data->trend_joints[i];

      if (smoothed_joint != NULL || trend_joint != NULL)
        {
          if (smooth_data->joints_persistency_counter[i] > 0)
            smooth_data->joints_persistency_counter[i]--;
          else
            {
              skeltrack_joint_free (smoothed_joint);
              skeltrack_joint_free (trend_joint);
              if (smoothed_joint)
                smooth_data->smoothed_joints[i] = NULL;
              if (trend_joint)
                smooth_data->trend_joints[i] = NULL;
              smooth_data->joints_persistency_counter[i] = smooth_data->joints_persistency;
            }
        }
    }
}

void
smooth_joints (SmoothData *data,
               SkeltrackJointList new_joints)
{
  guint i;

  if (new_joints == NULL)
    {
      decrease_joints_persistency (data);
      return;
    }

  if (data->smoothed_joints == NULL)
    {
      data->smoothed_joints = skeltrack_joint_list_new ();
      for (i = 0; i < SKELTRACK_JOINT_MAX_JOINTS; i++)
        {
          data->smoothed_joints[i] = skeltrack_joint_copy (new_joints[i]);
        }
      return;
    }
  if (data->trend_joints == NULL)
    {
      data->trend_joints = skeltrack_joint_list_new ();
    }

  for (i = 0; i < SKELTRACK_JOINT_MAX_JOINTS; i++)
    {
      SkeltrackJoint *joint, *smoothed_joint, *trend_joint;

      smoothed_joint = data->smoothed_joints[i];
      trend_joint = data->trend_joints[i];
      joint = new_joints[i];
      if (joint == NULL)
        {
          if (smoothed_joint != NULL)
            {
              if (data->joints_persistency_counter[i] > 0)
                data->joints_persistency_counter[i]--;
              else
                {
                  skeltrack_joint_free (smoothed_joint);
                  skeltrack_joint_free (trend_joint);
                  data->smoothed_joints[i] = NULL;
                  data->trend_joints[i] = NULL;
                  data->joints_persistency_counter[i] = data->joints_persistency;
                }
            }
          continue;
        }
      data->joints_persistency_counter[i] = data->joints_persistency;

      if (smoothed_joint == NULL)
        {
          data->smoothed_joints[i] = skeltrack_joint_copy (joint);
          continue;
        }

      if (trend_joint == NULL)
        {
          /* First case (when there are only initial values) */
          trend_joint = g_slice_new0 (SkeltrackJoint);
          trend_joint->x = joint->x - smoothed_joint->x;
          trend_joint->y = joint->y - smoothed_joint->y;
          trend_joint->z = joint->z - smoothed_joint->z;
          trend_joint->screen_x = joint->screen_x - smoothed_joint->screen_x;
          trend_joint->screen_y = joint->screen_y - smoothed_joint->screen_y;
          data->trend_joints[i] = trend_joint;
        }
      else
        {
          /* @TODO: Check if we should give the control of each factor
             independently (data-smoothing-factor and trend-smoothing-factor).
          */
          holt_double_exp_joint (data->smoothing_factor,
                                 data->smoothing_factor,
                                 smoothed_joint,
                                 joint,
                                 trend_joint);
        }
    }
}
