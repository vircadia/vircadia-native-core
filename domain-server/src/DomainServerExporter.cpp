//
//  DomainServerExporter.cpp
//  domain-server/src
//
//  Created by Dale Glass on 3 Apr 2020.
//  Copyright 2020 Dale Glass
//
//  Prometheus exporter
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// TODO:
//
// Look into the data provided by OctreeServer::handleHTTPRequest in assignment-client/src/octree/OctreeServer.cpp
// Turns out the octree server (entity server) can optionally deliver additional statistics via another HTTP server
// that is disabled by default. This functionality can be enabled by setting statusPort to a port number.
//
// Look into what appears in Audio Mixer -> z_listeners -> jitter -> injectors, so far it's been an empty list.

#include <QLoggingCategory>
#include <QUrl>
#include <QJsonObject>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSet>

#include "DomainServerExporter.h"
#include "DependencyManager.h"
#include "LimitedNodeList.h"
#include "HTTPConnection.h"
#include "DomainServerNodeData.h"

Q_LOGGING_CATEGORY(domain_server_exporter, "hifi.domain_server.prometheus_exporter")

static const QMap<QString, DomainServerExporter::MetricType> TYPE_MAP {
    { "asset_server_assignment_stats_num_queued_check_ins"                                        , DomainServerExporter::MetricType::Gauge },
    { "asset_server_connection_stats_cw_p"                                                        , DomainServerExporter::MetricType::Gauge },
    { "asset_server_connection_stats_down_mb_s"                                                   , DomainServerExporter::MetricType::Gauge },
    { "asset_server_connection_stats_est_max_p_s"                                                 , DomainServerExporter::MetricType::Gauge },
    { "asset_server_connection_stats_last_heard_ago_msecs"                                        , DomainServerExporter::MetricType::Gauge },
    { "asset_server_connection_stats_last_heard_time_msecs"                                       , DomainServerExporter::MetricType::Gauge },
    { "asset_server_connection_stats_period_us"                                                   , DomainServerExporter::MetricType::Gauge },
    { "asset_server_connection_stats_rtt_ms"                                                      , DomainServerExporter::MetricType::Gauge },
    { "asset_server_connection_stats_up_mb_s"                                                     , DomainServerExporter::MetricType::Gauge },
    { "asset_server_downstream_stats_duplicates"                                                  , DomainServerExporter::MetricType::Counter },
    { "asset_server_downstream_stats_recvd_p_s"                                                   , DomainServerExporter::MetricType::Gauge },
    { "asset_server_downstream_stats_recvd_packets"                                               , DomainServerExporter::MetricType::Counter },
    { "asset_server_downstream_stats_sent_ack"                                                    , DomainServerExporter::MetricType::Counter },
    { "asset_server_io_stats_inbound_kbps"                                                        , DomainServerExporter::MetricType::Gauge },
    { "asset_server_io_stats_inbound_pps"                                                         , DomainServerExporter::MetricType::Gauge },
    { "asset_server_io_stats_outbound_kbps"                                                       , DomainServerExporter::MetricType::Gauge },
    { "asset_server_io_stats_outbound_pps"                                                        , DomainServerExporter::MetricType::Gauge },
    { "asset_server_upstream_stats_procd_ack"                                                     , DomainServerExporter::MetricType::Counter },
    { "asset_server_upstream_stats_recvd_ack"                                                     , DomainServerExporter::MetricType::Counter },
    { "asset_server_upstream_stats_retransmitted"                                                 , DomainServerExporter::MetricType::Counter },
    { "asset_server_upstream_stats_sent_p_s"                                                      , DomainServerExporter::MetricType::Gauge },
    { "asset_server_upstream_stats_sent_packets"                                                  , DomainServerExporter::MetricType::Counter },
    { "audio_mixer_assignment_stats_num_queued_check_ins"                                         , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_avg_listeners_per_frame"                                                       , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_avg_listeners_silent_per_frame"                                                , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_avg_streams_per_frame"                                                         , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_avg_timing_stats_us_per_check_time"                                            , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_avg_timing_stats_us_per_check_time_trailing"                                   , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_avg_timing_stats_us_per_events"                                                , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_avg_timing_stats_us_per_events_trailing"                                       , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_avg_timing_stats_us_per_frame"                                                 , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_avg_timing_stats_us_per_frame_trailing"                                        , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_avg_timing_stats_us_per_mix"                                                   , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_avg_timing_stats_us_per_mix_trailing"                                          , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_avg_timing_stats_us_per_packets"                                               , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_avg_timing_stats_us_per_packets_trailing"                                      , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_avg_timing_stats_us_per_sleep"                                                 , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_avg_timing_stats_us_per_sleep_trailing"                                        , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_avg_timing_stats_us_per_tic"                                                   , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_avg_timing_stats_us_per_tic_trailing"                                          , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_io_stats_inbound_kbps"                                                         , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_io_stats_inbound_pps"                                                          , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_io_stats_outbound_kbps"                                                        , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_io_stats_outbound_pps"                                                         , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_listeners_jitter_downstream_available"                                         , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_listeners_jitter_downstream_available_avg_10s"                                 , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_listeners_jitter_downstream_avg_gap_30s_usecs"                                 , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_listeners_jitter_downstream_avg_gap_usecs"                                     , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_listeners_jitter_downstream_desired"                                           , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_listeners_jitter_downstream_lost_percent"                                      , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_listeners_jitter_downstream_lost_percent_30s"                                  , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_listeners_jitter_downstream_max_gap_30s_usecs"                                 , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_listeners_jitter_downstream_max_gap_usecs"                                     , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_listeners_jitter_downstream_min_gap_30s_usecs"                                 , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_listeners_jitter_downstream_min_gap_usecs"                                     , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_listeners_jitter_downstream_not_mixed"                                         , DomainServerExporter::MetricType::Counter },
    { "audio_mixer_listeners_jitter_downstream_overflows"                                         , DomainServerExporter::MetricType::Counter },
    { "audio_mixer_listeners_jitter_downstream_starves"                                           , DomainServerExporter::MetricType::Counter },
    { "audio_mixer_listeners_jitter_downstream_unplayed"                                          , DomainServerExporter::MetricType::Counter },
    { "audio_mixer_listeners_jitter_injectors"                                                    , DomainServerExporter::MetricType::Counter },
    { "audio_mixer_listeners_jitter_upstream_available"                                           , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_listeners_jitter_upstream_available_avg_10s"                                   , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_listeners_jitter_upstream_avg_gap_30s_usecs"                                   , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_listeners_jitter_upstream_avg_gap_usecs"                                       , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_listeners_jitter_upstream_desired_calc"                                        , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_listeners_jitter_upstream_lost_percent"                                        , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_listeners_jitter_upstream_lost_percent_30s"                                    , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_listeners_jitter_upstream_max_gap_30s_usecs"                                   , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_listeners_jitter_upstream_max_gap_usecs"                                       , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_listeners_jitter_upstream_mic_desired"                                         , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_listeners_jitter_upstream_min_gap_30s_usecs"                                   , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_listeners_jitter_upstream_min_gap_usecs"                                       , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_listeners_jitter_upstream_not_mixed"                                           , DomainServerExporter::MetricType::Counter },
    { "audio_mixer_listeners_jitter_upstream_overflows"                                           , DomainServerExporter::MetricType::Counter },
    { "audio_mixer_listeners_jitter_upstream_silents_dropped"                                     , DomainServerExporter::MetricType::Counter },
    { "audio_mixer_listeners_jitter_upstream_starves"                                             , DomainServerExporter::MetricType::Counter },
    { "audio_mixer_listeners_jitter_upstream_unplayed"                                            , DomainServerExporter::MetricType::Counter },
    { "audio_mixer_listeners_outbound_kbps"                                                       , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_mix_stats_active_streams"                                                      , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_mix_stats_active_to_inactive"                                                  , DomainServerExporter::MetricType::Counter },
    { "audio_mixer_mix_stats_active_to_skippped"                                                  , DomainServerExporter::MetricType::Counter },
    { "audio_mixer_mix_stats_avg_mixes_per_block"                                                 , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_mix_stats_hrtf_renders"                                                        , DomainServerExporter::MetricType::Counter },
    { "audio_mixer_mix_stats_hrtf_resets"                                                         , DomainServerExporter::MetricType::Counter },
    { "audio_mixer_mix_stats_hrtf_updates"                                                        , DomainServerExporter::MetricType::Counter },
    { "audio_mixer_mix_stats_inactive_streams"                                                    , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_mix_stats_inactive_to_active"                                                  , DomainServerExporter::MetricType::Counter },
    { "audio_mixer_mix_stats_inactive_to_skippped"                                                , DomainServerExporter::MetricType::Counter },
    { "audio_mixer_mix_stats_percent_hrtf_mixes"                                                  , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_mix_stats_percent_manual_echo_mixes"                                           , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_mix_stats_percent_manual_stereo_mixes"                                         , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_mix_stats_skipped_streams"                                                     , DomainServerExporter::MetricType::Counter },
    { "audio_mixer_mix_stats_skippped_to_active"                                                  , DomainServerExporter::MetricType::Counter },
    { "audio_mixer_mix_stats_skippped_to_inactive"                                                , DomainServerExporter::MetricType::Counter },
    { "audio_mixer_mix_stats_total_mixes"                                                         , DomainServerExporter::MetricType::Counter },
    { "audio_mixer_silent_packets_per_frame"                                                      , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_threads"                                                                       , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_throttling_ratio"                                                              , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_trailing_mix_ratio"                                                            , DomainServerExporter::MetricType::Gauge },
    { "audio_mixer_use_dynamic_jitter_buffers"                                                    , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_assignment_stats_num_queued_check_ins"                                        , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_avatars_av_data_receive_rate"                                                 , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_avatars_avg_other_av_skips_per_second"                                        , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_avatars_avg_other_av_starves_per_second"                                      , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_avatars_delta_full_vs_avatar_data_kbps"                                       , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_avatars_inbound_av_data_kbps"                                                 , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_avatars_inbound_kbps"                                                         , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_avatars_num_avs_sent_last_frame"                                              , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_avatars_outbound_av_data_kbps"                                                , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_avatars_outbound_av_traits_kbps"                                              , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_avatars_outbound_kbps"                                                        , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_avatars_recent_other_av_in_view"                                              , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_avatars_recent_other_av_out_of_view"                                          , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_avatars_total_num_out_of_order_sends"                                         , DomainServerExporter::MetricType::Counter },
    { "avatar_mixer_average_listeners_last_second"                                                , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_broadcast_loop_rate"                                                          , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_io_stats_inbound_kbps"                                                        , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_io_stats_inbound_pps"                                                         , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_io_stats_outbound_kbps"                                                       , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_io_stats_outbound_pps"                                                        , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_parallel_tasks_broadcast_avatar_data_functor"                                 , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_parallel_tasks_broadcast_avatar_data_innner"                                  , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_parallel_tasks_broadcast_avatar_data_lock_wait"                               , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_parallel_tasks_broadcast_avatar_data_node_transform"                          , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_parallel_tasks_broadcast_avatar_data_total"                                   , DomainServerExporter::MetricType::Counter },
    { "avatar_mixer_parallel_tasks_display_name_management_total"                                 , DomainServerExporter::MetricType::Counter },
    { "avatar_mixer_parallel_tasks_process_queued_avatar_data_packets_lock_wait"                  , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_parallel_tasks_process_queued_avatar_data_packets_total"                      , DomainServerExporter::MetricType::Counter },
    { "avatar_mixer_single_core_tasks_incoming_packets_handle_avatar_identity_packet"             , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_single_core_tasks_incoming_packets_handle_avatar_query_packet"                , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_single_core_tasks_incoming_packets_handle_kill_avatar_packet"                 , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_single_core_tasks_incoming_packets_handle_node_ignore_request_packet"         , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_single_core_tasks_incoming_packets_handle_radius_ignore_request_packet"       , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_single_core_tasks_incoming_packets_handle_requests_domain_list_data_packet"   , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_single_core_tasks_process_events"                                             , DomainServerExporter::MetricType::Counter },
    { "avatar_mixer_single_core_tasks_queue_incoming_packet"                                      , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_single_core_tasks_send_stats"                                                 , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_slaves_aggregate_per_frame_received_1_nodes_processed"                        , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_slaves_aggregate_per_frame_sent_1_nodes_broadcasted_to"                       , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_slaves_aggregate_per_frame_sent_2_average_others_included"                    , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_slaves_aggregate_per_frame_sent_3_average_over_budget_avatars"                , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_slaves_aggregate_per_frame_sent_4_average_data_bytes"                         , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_slaves_aggregate_per_frame_sent_5_average_traits_bytes"                       , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_slaves_aggregate_per_frame_sent_6_average_identity_bytes"                     , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_slaves_aggregate_per_frame_sent_7_average_hero_avatars"                       , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_slaves_aggregate_per_frame_timing_1_process_incoming_packets"                 , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_slaves_aggregate_per_frame_timing_2_ignore_calculation"                       , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_slaves_aggregate_per_frame_timing_3_to_byte_array"                            , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_slaves_aggregate_per_frame_timing_4_avatar_data_packing"                      , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_slaves_aggregate_per_frame_timing_5_packet_sending"                           , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_slaves_aggregate_per_frame_timing_6_job_elapsed_time"                         , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_threads"                                                                      , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_throttling_ratio"                                                             , DomainServerExporter::MetricType::Gauge },
    { "avatar_mixer_trailing_mix_ratio"                                                           , DomainServerExporter::MetricType::Gauge },
    { "entity_script_server_assignment_stats_num_queued_check_ins"                                , DomainServerExporter::MetricType::Gauge },
    { "entity_script_server_io_stats_inbound_kbps"                                                , DomainServerExporter::MetricType::Gauge },
    { "entity_script_server_io_stats_inbound_pps"                                                 , DomainServerExporter::MetricType::Gauge },
    { "entity_script_server_io_stats_outbound_kbps"                                               , DomainServerExporter::MetricType::Gauge },
    { "entity_script_server_io_stats_outbound_pps"                                                , DomainServerExporter::MetricType::Gauge },
    { "entity_script_server_nodes_inbound_kbit_s"                                                 , DomainServerExporter::MetricType::Gauge },
    { "entity_script_server_nodes_outbound_kbit_s"                                                , DomainServerExporter::MetricType::Gauge },
    { "entity_script_server_nodes_reliable_packet_s"                                              , DomainServerExporter::MetricType::Gauge },
    { "entity_script_server_nodes_unreliable_packet_s"                                            , DomainServerExporter::MetricType::Gauge },
    { "entity_script_server_octree_stats_element_count"                                           , DomainServerExporter::MetricType::Gauge },
    { "entity_script_server_octree_stats_internal_element_count"                                  , DomainServerExporter::MetricType::Gauge },
    { "entity_script_server_octree_stats_leaf_element_count"                                      , DomainServerExporter::MetricType::Gauge },
    { "entity_script_server_script_engine_stats_number_running_scripts"                           , DomainServerExporter::MetricType::Gauge },
    { "entity_server_assignment_stats_num_queued_check_ins"                                       , DomainServerExporter::MetricType::Gauge },
    { "entity_server_entity_server_inbound_data_packet_queue"                                     , DomainServerExporter::MetricType::Gauge },
    { "entity_server_entity_server_inbound_data_total_elements"                                   , DomainServerExporter::MetricType::Gauge },
    { "entity_server_entity_server_inbound_data_total_packets"                                    , DomainServerExporter::MetricType::Gauge },
    { "entity_server_entity_server_inbound_timing_avg_lock_wait_time_per_element"                 , DomainServerExporter::MetricType::Gauge },
    { "entity_server_entity_server_inbound_timing_avg_lock_wait_time_per_packet"                  , DomainServerExporter::MetricType::Gauge },
    { "entity_server_entity_server_inbound_timing_avg_process_time_per_element"                   , DomainServerExporter::MetricType::Gauge },
    { "entity_server_entity_server_inbound_timing_avg_process_time_per_packet"                    , DomainServerExporter::MetricType::Gauge },
    { "entity_server_entity_server_inbound_timing_avg_transit_time_per_packet"                    , DomainServerExporter::MetricType::Gauge },
    { "entity_server_entity_server_misc_clients"                                                  , DomainServerExporter::MetricType::Gauge },
    { "entity_server_entity_server_misc_persist_file_load_time_seconds"                           , DomainServerExporter::MetricType::Gauge },
    { "entity_server_entity_server_misc_threads_handle_pacekt_send"                               , DomainServerExporter::MetricType::Gauge },
    { "entity_server_entity_server_misc_threads_packet_distributor"                               , DomainServerExporter::MetricType::Gauge },
    { "entity_server_entity_server_misc_threads_processing"                                       , DomainServerExporter::MetricType::Gauge },
    { "entity_server_entity_server_misc_threads_write_datagram"                                   , DomainServerExporter::MetricType::Gauge },
    { "entity_server_entity_server_misc_uptime_seconds"                                           , DomainServerExporter::MetricType::Counter },
    { "entity_server_entity_server_octree_element_count"                                          , DomainServerExporter::MetricType::Gauge },
    { "entity_server_entity_server_octree_internal_element_count"                                 , DomainServerExporter::MetricType::Gauge },
    { "entity_server_entity_server_octree_leaf_element_count"                                     , DomainServerExporter::MetricType::Gauge },
    { "entity_server_entity_server_outbound_data_total_bytes"                                     , DomainServerExporter::MetricType::Counter },
    { "entity_server_entity_server_outbound_data_total_bytes_bit_masks"                           , DomainServerExporter::MetricType::Counter },
    { "entity_server_entity_server_outbound_data_total_bytes_octal_codes"                         , DomainServerExporter::MetricType::Counter },
    { "entity_server_entity_server_outbound_data_total_bytes_wasted"                              , DomainServerExporter::MetricType::Counter },
    { "entity_server_entity_server_outbound_data_total_packets"                                   , DomainServerExporter::MetricType::Counter },
    { "entity_server_entity_server_outbound_timing_avg_compress_and_write_time"                   , DomainServerExporter::MetricType::Gauge },
    { "entity_server_entity_server_outbound_timing_avg_encode_time"                               , DomainServerExporter::MetricType::Gauge },
    { "entity_server_entity_server_outbound_timing_avg_inside_time"                               , DomainServerExporter::MetricType::Gauge },
    { "entity_server_entity_server_outbound_timing_avg_loop_time"                                 , DomainServerExporter::MetricType::Gauge },
    { "entity_server_entity_server_outbound_timing_avg_send_time"                                 , DomainServerExporter::MetricType::Gauge },
    { "entity_server_entity_server_outbound_timing_avg_tree_traverse_time"                        , DomainServerExporter::MetricType::Gauge },
    { "entity_server_entity_server_outbound_timing_node_wait_time"                                , DomainServerExporter::MetricType::Gauge },
    { "entity_server_io_stats_inbound_kbps"                                                       , DomainServerExporter::MetricType::Gauge },
    { "entity_server_io_stats_inbound_pps"                                                        , DomainServerExporter::MetricType::Gauge },
    { "entity_server_io_stats_outbound_kbps"                                                      , DomainServerExporter::MetricType::Gauge },
    { "entity_server_io_stats_outbound_pps"                                                       , DomainServerExporter::MetricType::Gauge },
    { "messages_mixer_assignment_stats_num_queued_check_ins"                                      , DomainServerExporter::MetricType::Gauge },
    { "messages_mixer_io_stats_inbound_kbps"                                                      , DomainServerExporter::MetricType::Gauge },
    { "messages_mixer_io_stats_inbound_pps"                                                       , DomainServerExporter::MetricType::Gauge },
    { "messages_mixer_io_stats_outbound_kbps"                                                     , DomainServerExporter::MetricType::Gauge },
    { "messages_mixer_io_stats_outbound_pps"                                                      , DomainServerExporter::MetricType::Gauge },
    { "messages_mixer_messages_inbound_kbps"                                                      , DomainServerExporter::MetricType::Gauge },
    { "messages_mixer_messages_outbound_kbps"                                                     , DomainServerExporter::MetricType::Gauge }
};


// Things we're not going to convert for various reasons, such as containing text,
// or having a value followed by an unit ("5.2 seconds").
//
// Things like text like usernames have no place in the Prometheus model, so they can be skipped.
//
// For numeric values with an unit, instead of trying to parse it, the stats will just need to
// have a second copy of the metric added, with the value expressed as a number, with the original
// being blacklisted here.

static const QSet<QString> BLACKLIST = {
    "asset_server_connection_stats_last_heard",                 // Timestamp as a string
    "asset_server_username",                                    // Username
    "audio_mixer_listeners_jitter_downstream_avg_gap",          // Number as string with unit name, alternative added
    "audio_mixer_listeners_jitter_downstream_avg_gap_30s",      // Number as string with unit name, alternative added
    "audio_mixer_listeners_jitter_downstream_max_gap",          // Number as string with unit name, alternative added
    "audio_mixer_listeners_jitter_downstream_max_gap_30s",      // Number as string with unit name, alternative added
    "audio_mixer_listeners_jitter_downstream_min_gap",          // Number as string with unit name, alternative added
    "audio_mixer_listeners_jitter_downstream_min_gap_30s",      // Number as string with unit name, alternative added
    "audio_mixer_listeners_jitter_injectors",                   // Array, empty. TODO: check if this ever contains anything.
    "audio_mixer_listeners_jitter_upstream",                    // Only exists in the absence of a connection
    "audio_mixer_listeners_jitter_upstream_avg_gap",            // Number as string with unit name, alternative added
    "audio_mixer_listeners_jitter_upstream_avg_gap_30s",        // Number as string with unit name, alternative added
    "audio_mixer_listeners_jitter_upstream_max_gap",            // Number as string with unit name, alternative added
    "audio_mixer_listeners_jitter_upstream_max_gap_30s",        // Number as string with unit name, alternative added
    "audio_mixer_listeners_jitter_upstream_min_gap",            // Number as string with unit name, alternative added
    "audio_mixer_listeners_jitter_upstream_min_gap_30s",        // Number as string with unit name, alternative added
    "audio_mixer_listeners_username",                           // Username
    "avatar_mixer_avatars_display_name",                        // Username
    "avatar_mixer_avatars_username",                            // Username
    "entity_script_server_nodes_node_type",                     // Username
    "entity_script_server_nodes_username",                      // Username
    "entity_server_entity_server_misc_configuration",           // Text
    "entity_server_entity_server_misc_detailed_stats_url",      // URL
    "entity_server_entity_server_misc_persist_file_load_time",  // Number as string with unit name, alternative added
    "entity_server_entity_server_misc_uptime",                  // Number as string with unit name, alternative added
    "messages_mixer_messages_username"                          // Username
};

DomainServerExporter::DomainServerExporter() {
}

bool DomainServerExporter::handleHTTPRequest(HTTPConnection* connection, const QUrl& url, bool skipSubHandler) {
    const QString URI_METRICS = "/metrics";
    const QString EXPORTER_MIME_TYPE = "text/plain";

    qCDebug(domain_server_exporter) << "Request on URL " << url;

    if (url.path() == URI_METRICS) {
        auto nodeList = DependencyManager::get<LimitedNodeList>();
        QString output = "";
        QTextStream outStream(&output);

        nodeList->eachNode([this, &outStream](const SharedNodePointer& node) { generateMetricsForNode(outStream, node); });

        connection->respond(HTTPConnection::StatusCode200, output.toUtf8(), qPrintable(EXPORTER_MIME_TYPE));
        return true;
    }

    return false;
}

QString DomainServerExporter::escapeName(const QString& name) {
    QRegularExpression invalidCharacters("[^A-Za-z0-9_]");

    QString result = name;

    // If a key is named something like: "6. threads", turn it into just "threads"
    result.replace(QRegularExpression("^\\d+\\. "), "");
    result.replace(QRegularExpression("^\\d+_"), "");

    // If a key is named something like "z_listeners", turn it into just "listeners"
    result.replace(QRegularExpression("^z_"), "");

    // If a key is named something like "lost%", change it to "lost_percent_".
    // redundant underscores will be removed below.
    result.replace(QRegularExpression("%"), "_percent_");

    // change mixedCaseNames to mixed_case_names
    result.replace(QRegularExpression("([a-z])([A-Z])"), "\\1_\\2");

    // Replace all invalid characters with a _
    result.replace(invalidCharacters, "_");

    // Remove any "_" characters at the beginning or end
    result.replace(QRegularExpression("^_+"), "");
    result.replace(QRegularExpression("_+$"), "");

    // Replace any duplicated _ characters with a single one
    result.replace(QRegularExpression("_+"), "_");

    result = result.toLower();

    return result;
}

void DomainServerExporter::generateMetricsForNode(QTextStream& stream, const SharedNodePointer& node) {
    QJsonObject statsObject = static_cast<DomainServerNodeData*>(node->getLinkedData())->getStatsJSONObject();
    QString nodeType = NodeType::getNodeTypeName(static_cast<NodeType_t>(node->getType()));

    stream << "\n\n\n";
    stream << "###############################################################\n";
    stream << "# " << nodeType << "\n";
    stream << "###############################################################\n";

    generateMetricsFromJson(stream, nodeType, escapeName(nodeType), QHash<QString, QString>(), statsObject);
}

void DomainServerExporter::generateMetricsFromJson(QTextStream& stream,
                                                   QString originalPath,
                                                   QString path,
                                                   QHash<QString, QString> labels,
                                                   const QJsonObject& root) {
    for (auto iter = root.constBegin(); iter != root.constEnd(); ++iter) {
        auto escapedKey = escapeName(iter.key());
        auto metricValue = iter.value();
        auto metricName = path + "_" + escapedKey;
        auto origMetricName = originalPath + " -> " + iter.key();

        if (metricValue.isObject()) {
            QUuid possible_uuid = QUuid::fromString(iter.key());

            if (possible_uuid.isNull()) {
                generateMetricsFromJson(stream, originalPath + " -> " + iter.key(), path + "_" + escapedKey, labels,
                                        iter.value().toObject());
            } else {
                labels.insert("uuid", possible_uuid.toString(QUuid::WithoutBraces));
                generateMetricsFromJson(stream, originalPath, path, labels, iter.value().toObject());
            }

            continue;
        }

        if (BLACKLIST.contains(metricName)) {
            continue;
        }

        bool conversionOk = false;
        double converted = 0;

        if (metricValue.isString()) {
            // Prometheus only deals with numeric values. See if this string contains a valid one

            QString tmp = metricValue.toString();
            converted = tmp.toDouble(&conversionOk);

            if (!conversionOk) {
                qCWarning(domain_server_exporter) << "Failed to convert value of " << origMetricName << " (" << metricName
                                                  << ") to double: " << tmp << "'";
                continue;
            }
        }

        stream << QString("\n# HELP %1 %2 -> %3\n").arg(metricName).arg(originalPath).arg(iter.key());

        if (TYPE_MAP.contains(metricName)) {
            stream << "# TYPE " << metricName << " ";
            switch (TYPE_MAP[metricName]) {
                case DomainServerExporter::MetricType::Untyped:
                    stream << "untyped";
                    break;
                case DomainServerExporter::MetricType::Counter:
                    stream << "counter";
                    break;
                case DomainServerExporter::MetricType::Gauge:
                    stream << "gauge";
                    break;
                case DomainServerExporter::MetricType::Histogram:
                    stream << "histogram";
                    break;
                case DomainServerExporter::MetricType::Summary:
                    stream << "summary";
                    break;
            }
            stream << "\n";
        } else {
            qCWarning(domain_server_exporter)
                << "Type for metric " << origMetricName << " (" << metricName << ") not known.";
        }

        stream << path << "_" << escapedKey;
        if (!labels.isEmpty()) {
            stream << "{";

            bool isFirst = true;
            QHashIterator<QString, QString> iter(labels);

            while (iter.hasNext()) {
                iter.next();

                if (!isFirst) {
                    stream << ",";
                }

                QString escapedValue = iter.value();
                escapedValue.replace("\\", "\\\\");
                escapedValue.replace("\"", "\\\"");
                escapedValue.replace("\n", "\\\n");

                stream << iter.key() << "=\"" << escapedValue << "\"";

                isFirst = false;
            }
            stream << "}";
        }

        stream << " ";

        if (metricValue.isBool()) {
            stream << (iter.value().toBool() ? "1" : "0");
        } else if (metricValue.isDouble()) {
            stream << metricValue.toDouble();
        } else if (metricValue.isString()) {
            // Converted above
            stream << converted;
        } else {
            qCWarning(domain_server_exporter)
                << "Can't convert metric " << origMetricName << "(" << metricName << ") with value " << metricValue;
        }

        stream << "\n";
    }
}
