print("Loading hfudt")

-- create the HFUDT protocol
p_hfudt = Proto("hfudt", "HFUDT Protocol")

-- create fields shared between packets in HFUDT
local f_data = ProtoField.string("hfudt.data", "Data")

-- create the fields for data packets in HFUDT
local f_length = ProtoField.uint16("hfudt.length", "Length", base.DEC)
local f_control_bit = ProtoField.uint8("hfudt.control", "Control Bit", base.DEC)
local f_reliable_bit = ProtoField.uint8("hfudt.reliable", "Reliability Bit", base.DEC)
local f_message_bit = ProtoField.uint8("hfudt.message", "Message Bit", base.DEC)
local f_obfuscation_level = ProtoField.uint8("hfudt.obfuscation_level", "Obfuscation Level", base.DEC)
local f_sequence_number = ProtoField.uint32("hfudt.sequence_number", "Sequence Number", base.DEC)
local f_message_position = ProtoField.uint8("hfudt.message_position", "Message Position", base.DEC)
local f_message_number = ProtoField.uint32("hfudt.message_number", "Message Number", base.DEC)
local f_message_part_number = ProtoField.uint32("hfudt.message_part_number", "Message Part Number", base.DEC)
local f_type = ProtoField.uint8("hfudt.type", "Type", base.DEC)
local f_version = ProtoField.uint8("hfudt.version", "Version", base.DEC)
local f_type_text = ProtoField.string("hfudt.type_text", "TypeText")
local f_sender_id = ProtoField.guid("hfudt.sender_id", "Sender ID", base.DEC)

-- create the fields for control packets in HFUDT
local f_control_type = ProtoField.uint16("hfudt.control_type", "Control Type", base.DEC)
local f_control_type_text = ProtoField.string("hfudt.control_type_text", "Control Type Text", base.ASCII)
local f_ack_sequence_number = ProtoField.uint32("hfudt.ack_sequence_number", "ACKed Sequence Number", base.DEC)
local f_control_sub_sequence = ProtoField.uint32("hfudt.control_sub_sequence", "Control Sub-Sequence Number", base.DEC)
local f_nak_sequence_number = ProtoField.uint32("hfudt.nak_sequence_number", "NAKed Sequence Number", base.DEC)
local f_nak_range_end = ProtoField.uint32("hfudt.nak_range_end", "NAK Range End", base.DEC)

-- avatar data fields
local f_avatar_data_id = ProtoField.guid("hfudt.avatar_id", "Avatar ID", base.DEC)
local f_avatar_data_has_flags = ProtoField.string("hfudt.avatar_has_flags", "Has Flags")
local f_avatar_data_position = ProtoField.string("hfudt.avatar_data_position", "Position")
local f_avatar_data_dimensions = ProtoField.string("hfudt.avatar_data_dimensions", "Dimensions")
local f_avatar_data_offset = ProtoField.string("hfudt.avatar_data_offset", "Offset")
local f_avatar_data_look_at_position = ProtoField.string("hfudt.avatar_data_look_at_position", "Look At Position")
local f_avatar_data_audio_loudness = ProtoField.string("hfudt.avatar_data_audio_loudness", "Audio Loudness")
local f_avatar_data_additional_flags = ProtoField.string("hfudt.avatar_data_additional_flags", "Additional Flags")
local f_avatar_data_parent_id = ProtoField.guid("hfudt.avatar_data_parent_id", "Parent ID")
local f_avatar_data_parent_joint_index = ProtoField.string("hfudt.avatar_data_parent_joint_index", "Parent Joint Index")
local f_avatar_data_local_position = ProtoField.string("hfudt.avatar_data_local_position", "Local Position")
local f_avatar_data_valid_rotations = ProtoField.string("hfudt.avatar_data_valid_rotations", "Valid Rotations")
local f_avatar_data_valid_translations = ProtoField.string("hfudt.avatar_data_valid_translations", "Valid Translations")
local f_avatar_data_default_rotations = ProtoField.string("hfudt.avatar_data_default_rotations", "Valid Default")
local f_avatar_data_default_translations = ProtoField.string("hfudt.avatar_data_default_translations", "Valid Default")

local SEQUENCE_NUMBER_MASK = 0x07FFFFFF

p_hfudt.fields = {
  f_length,
  f_control_bit, f_reliable_bit, f_message_bit, f_sequence_number, f_type, f_type_text, f_version,
  f_sender_id, f_avatar_data_id, f_avatar_data_parent_id,
  f_message_position, f_message_number, f_message_part_number, f_obfuscation_level,
  f_control_type, f_control_type_text, f_control_sub_sequence, f_ack_sequence_number, f_nak_sequence_number, f_nak_range_end,
  f_data
}

p_hfudt.prefs["udp_port"] = Pref.uint("UDP Port", 40102, "UDP Port for HFUDT Packets (udt::Socket bound port)")

local control_types = {
  [0] = { "ACK", "Acknowledgement" },
  [1] = { "ACK2", "Acknowledgement of acknowledgement" },
  [2] = { "LightACK", "Light Acknowledgement" },
  [3] = { "NAK", "Loss report (NAK)" },
  [4] = { "TimeoutNAK", "Loss report re-transmission (TimeoutNAK)" },
  [5] = { "Handshake", "Handshake" },
  [6] = { "HandshakeACK", "Acknowledgement of Handshake" },
  [7] = { "ProbeTail", "Probe tail" },
  [8] = { "HandshakeRequest", "Request a Handshake" }
}

local message_positions = {
  [0] = "ONLY",
  [1] = "LAST",
  [2] = "FIRST",
  [3] = "MIDDLE"
}

local packet_types = {
  [0] = "Unknown",
  [1] = "StunResponse",
  [2] = "DomainList",
  [3] = "Ping",
  [4] = "PingReply",
  [5] = "KillAvatar",
  [6] = "AvatarData",
  [7] = "InjectAudio",
  [8] = "MixedAudio",
  [9] = "MicrophoneAudioNoEcho",
  [10] = "MicrophoneAudioWithEcho",
  [11] = "BulkAvatarData",
  [12] = "SilentAudioFrame",
  [13] = "DomainListRequest",
  [14] = "RequestAssignment",
  [15] = "CreateAssignment",
  [16] = "DomainConnectionDenied",
  [17] = "MuteEnvironment",
  [18] = "AudioStreamStats",
  [19] = "DomainServerPathQuery",
  [20] = "DomainServerPathResponse",
  [21] = "DomainServerAddedNode",
  [22] = "ICEServerPeerInformation",
  [23] = "ICEServerQuery",
  [24] = "OctreeStats",
  [25] = "Jurisdiction",
  [26] = "JurisdictionRequest",
  [27] = "AssignmentClientStatus",
  [28] = "NoisyMute",
  [29] = "AvatarIdentity",
  [30] = "AvatarBillboard",
  [31] = "DomainConnectRequest",
  [32] = "DomainServerRequireDTLS",
  [33] = "NodeJsonStats",
  [34] = "OctreeDataNack",
  [35] = "StopNode",
  [36] = "AudioEnvironment",
  [37] = "EntityEditNack",
  [38] = "ICEServerHeartbeat",
  [39] = "ICEPing",
  [40] = "ICEPingReply",
  [41] = "EntityData",
  [42] = "EntityQuery",
  [43] = "EntityAdd",
  [44] = "EntityErase",
  [45] = "EntityEdit",
  [46] = "DomainServerConnectionToken",
  [47] = "DomainSettingsRequest",
  [48] = "DomainSettings",
  [49] = "AssetGet",
  [50] = "AssetGetReply",
  [51] = "AssetUpload",
  [52] = "AssetUploadReply",
  [53] = "AssetGetInfo",
  [54] = "AssetGetInfoReply"
}

function p_hfudt.dissector (buf, pinfo, root)

   -- make sure this isn't a STUN packet - those don't follow HFUDT format
  if pinfo.dst == Address.ip("stun.highfidelity.io") then return end

  -- validate that the packet length is at least the minimum control packet size
  if buf:len() < 4 then return end

  -- create a subtree for HFUDT
  subtree = root:add(p_hfudt, buf(0))

  -- set the packet length
  subtree:add(f_length, buf:len())

  -- pull out the entire first word
  local first_word = buf(0, 4):le_uint()

  -- pull out the control bit and add it to the subtree
  local control_bit = bit32.rshift(first_word, 31)
  subtree:add(f_control_bit, control_bit)

  local data_length = 0

  if control_bit == 1 then
    -- dissect the control packet
    pinfo.cols.protocol = p_hfudt.name .. " Control"

    -- remove the control bit and shift to the right to get the type value
    local shifted_type = bit32.rshift(bit32.lshift(first_word, 1), 17)
    local type = subtree:add(f_control_type, shifted_type)

    if control_types[shifted_type] ~= nil then
      -- if we know this type then add the name
      type:append_text(" (".. control_types[shifted_type][1] .. ")")

      subtree:add(f_control_type_text, control_types[shifted_type][1])
    end

    if shifted_type == 0 or shifted_type == 1 then

      -- this has a sub-sequence number
      local second_word = buf(4, 4):le_uint()
      subtree:add(f_control_sub_sequence, bit32.band(second_word, SEQUENCE_NUMBER_MASK))

      local data_index = 8

      if shifted_type == 0 then
        -- if this is an ACK let's read out the sequence number
        local sequence_number = buf(8, 4):le_uint()
        subtree:add(f_ack_sequence_number, bit32.band(sequence_number, SEQUENCE_NUMBER_MASK))

        data_index = data_index + 4
      end

      data_length = buf:len() - data_index

      -- set the data from whatever is left in the packet
      subtree:add(f_data, buf(data_index, data_length))

    elseif shifted_type == 2 then
      -- this is a Light ACK let's read out the sequence number
      local sequence_number = buf(4, 4):le_uint()
      subtree:add(f_ack_sequence_number, bit32.band(sequence_number, SEQUENCE_NUMBER_MASK))

      data_length = buf:len() - 4

      -- set the data from whatever is left in the packet
      subtree:add(f_data, buf(4, data_length))
    elseif shifted_type == 3 or shifted_type == 4 then
     if buf:len() <= 12 then
       -- this is a NAK  pull the sequence number or range
       local sequence_number = buf(4, 4):le_uint()
       subtree:add(f_nak_sequence_number, bit32.band(sequence_number, SEQUENCE_NUMBER_MASK))

       data_length = buf:len() - 4

       if buf:len() > 8 then
         local range_end = buf(8, 4):le_uint()
         subtree:add(f_nak_range_end, bit32.band(range_end, SEQUENCE_NUMBER_MASK))

         data_length = data_length - 4
       end
     end
    else
      data_length = buf:len() - 4

      -- no sub-sequence number, just read the data
      subtree:add(f_data, buf(4, data_length))
    end
  else
    -- dissect the data packet
    pinfo.cols.protocol = p_hfudt.name

    -- set the reliability bit
    subtree:add(f_reliable_bit, bit32.rshift(first_word, 30))

    local message_bit = bit32.band(0x01, bit32.rshift(first_word, 29))

    -- set the message bit
    subtree:add(f_message_bit, message_bit)

    -- read the obfuscation level
    local obfuscation_bits = bit32.band(0x03, bit32.rshift(first_word, 27))
    subtree:add(f_obfuscation_level, obfuscation_bits)

    -- read the sequence number
    subtree:add(f_sequence_number, bit32.band(first_word, SEQUENCE_NUMBER_MASK))

    local payload_offset = 4

    -- if the message bit is set, handle the second word
    if message_bit == 1 then
        payload_offset = 12

        local second_word = buf(4, 4):le_uint()

        -- read message position from upper 2 bits
        local message_position = bit32.rshift(second_word, 30)
        local position = subtree:add(f_message_position, message_position)

        if message_positions[message_position] ~= nil then
          -- if we know this position then add the name
          position:append_text(" (".. message_positions[message_position] .. ")")
        end

        -- read message number from lower 30 bits
        subtree:add(f_message_number, bit32.band(second_word, 0x3FFFFFFF))

        -- read the message part number
        subtree:add(f_message_part_number, buf(8, 4):le_uint())
    end

    -- read the type
    local packet_type = buf(payload_offset, 1):le_uint()
    local ptype = subtree:add(f_type, packet_type)
    if packet_types[packet_type] ~= nil then
      subtree:add(f_type_text, packet_types[packet_type])
      -- if we know this packet type then add the name
      ptype:append_text(" (".. packet_types[packet_type] .. ")")
    end

    -- read the version
    subtree:add(f_version, buf(payload_offset + 1, 1):le_uint())

    -- read node GUID
    local sender_id = buf(payload_offset + 2, 16)
    subtree:add(f_sender_id, sender_id)

    local i = payload_offset + 18

    -- skip MD6 checksum + 16 bytes
    i = i + 16

    -- AvatarData or BulkAvatarDataPacket
    if packet_type == 6 or packet_type == 11 then

      local avatar_data
      if packet_type == 6 then
        -- AvatarData packet

        -- avatar_id is same as sender_id
        subtree:add(f_avatar_data_id, sender_id)

        -- uint16 sequence_number
        local sequence_number = buf(i, 2):le_uint()
        i = i + 2

        local avatar_data_packet_len = buf:len() - i
        avatar_data = decode_avatar_data_packet(buf(i, avatar_data_packet_len))
        i = i + avatar_data_packet_len

        add_avatar_data_subtrees(avatar_data)

      else
        -- BulkAvatarData packet
        while i < buf:len() do
          -- avatar_id is first 16 bytes
          subtree:add(f_avatar_data_id, buf(i, 16))
          i = i + 16

          local avatar_data_packet_len = buf:len() - i
          avatar_data = decode_avatar_data_packet(buf(i, avatar_data_packet_len))
          i = i + avatar_data_packet_len

          add_avatar_data_subtrees(avatar_data)
        end
      end
    end
  end

  -- return the size of the header
  return buf:len()

end

function add_avatar_data_subtrees(avatar_data)
  if avatar_data["has_flags"] then
    subtree:add(f_avatar_data_has_flags, avatar_data["has_flags"])
  end
  if avatar_data["position"] then
    subtree:add(f_avatar_data_position, avatar_data["position"])
  end
  if avatar_data["dimensions"] then
    subtree:add(f_avatar_data_dimensions, avatar_data["dimensions"])
  end
  if avatar_data["offset"] then
    subtree:add(f_avatar_data_offset, avatar_data["offset"])
  end
  if avatar_data["look_at_position"] then
    subtree:add(f_avatar_data_look_at_position, avatar_data["look_at_position"])
  end
  if avatar_data["audio_loudness"] then
    subtree:add(f_avatar_data_audio_loudness, avatar_data["audio_loudness"])
  end
  if avatar_data["additional_flags"] then
    subtree:add(f_avatar_data_additional_flags, avatar_data["additional_flags"])
  end
  if avatar_data["parent_id"] then
    subtree:add(f_avatar_data_parent_id, avatar_data["parent_id"])
  end
  if avatar_data["parent_joint_index"] then
    subtree:add(f_avatar_data_parent_joint_index, avatar_data["parent_joint_index"])
  end
  if avatar_data["local_position"] then
    subtree:add(f_avatar_data_local_position, avatar_data["local_position"])
  end
  if avatar_data["valid_rotations"] then
    subtree:add(f_avatar_data_valid_rotations, avatar_data["valid_rotations"])
  end
  if avatar_data["valid_translations"] then
    subtree:add(f_avatar_data_valid_translations, avatar_data["valid_translations"])
  end
  if avatar_data["default_rotations"] then
    subtree:add(f_avatar_data_default_rotations, avatar_data["default_rotations"])
  end
  if avatar_data["default_translations"] then
    subtree:add(f_avatar_data_default_translations, avatar_data["default_translations"])
  end
end

function decode_vec3(buf)
  local i = 0
  local x = buf(i, 4):le_float()
  i = i + 4
  local y = buf(i, 4):le_float()
  i = i + 4
  local z = buf(i, 4):le_float()
  i = i + 4
  return {x, y, z}
end

function decode_validity_bits(buf, num_bits)
  -- first pass, decode each bit into an array of booleans
  local i = 0
  local bit = 0
  local booleans = {}
  for n = 1, num_bits do
    local value = (bit32.band(buf(i, 1):uint(), bit32.lshift(1, bit)) ~= 0)
    booleans[#booleans + 1] = value
    bit = bit + 1
    if bit == 8 then
      i = i + 1
      bit = 0
    end
  end

  -- second pass, create a list of indices whos booleans are true
  local result = {}
  for n = 1, #booleans do
    if booleans[n] then
      result[#result + 1] = n
    end
  end

  return result
end

function decode_avatar_data_packet(buf)

  local i = 0
  local result = {}

  -- uint16 has_flags
  local has_flags = buf(i, 2):le_uint()
  i = i + 2

  local has_global_position = (bit32.band(has_flags, 1) ~= 0)
  local has_bounding_box = (bit32.band(has_flags, 2) ~= 0)
  local has_orientation = (bit32.band(has_flags, 4) ~= 0)
  local has_scale = (bit32.band(has_flags, 8) ~= 0)
  local has_look_at_position = (bit32.band(has_flags, 16) ~= 0)
  local has_audio_loudness = (bit32.band(has_flags, 32) ~= 0)
  local has_sensor_to_world_matrix = (bit32.band(has_flags, 64) ~= 0)
  local has_additional_flags = (bit32.band(has_flags, 128) ~= 0)
  local has_parent_info = (bit32.band(has_flags, 256) ~= 0)
  local has_local_position = (bit32.band(has_flags, 512) ~= 0)
  local has_face_tracker_info = (bit32.band(has_flags, 1024) ~= 0)
  local has_joint_data = (bit32.band(has_flags, 2048) ~= 0)
  local has_joint_default_pose_flags = (bit32.band(has_flags, 4096) ~= 0)

  result["has_flags"] = string.format("HasFlags: 0x%x", has_flags)

  if has_global_position then
    local position = decode_vec3(buf(i, 12))
    result["position"] = string.format("Position: %.3f, %.3f, %.3f", position[1], position[2], position[3])
    i = i + 12
  end

  if has_bounding_box then
    local dimensions = decode_vec3(buf(i, 12))
    i = i + 12
    local offset = decode_vec3(buf(i, 12))
    i = i + 12
    result["dimensions"] = string.format("Dimensions: %.3f, %.3f, %.3f", dimensions[1], dimensions[2], dimensions[3])
    result["offset"] = string.format("Offset: %.3f, %.3f, %.3f", offset[1], offset[2], offset[3])
  end

  if has_orientation then
    -- TODO: orientation is hard to decode...
    i = i + 6
  end

  if has_scale then
    -- TODO: scale is hard to decode...
    i = i + 2
  end

  if has_look_at_position then
    local look_at = decode_vec3(buf(i, 12))
    i = i + 12
    result["look_at_position"] = string.format("Look At Position: %.3f, %.3f, %.3f", look_at[1], look_at[2], look_at[3])
  end

  if has_audio_loudness then
    local loudness = buf(i, 1):uint()
    i = i + 1
    result["audio_loudness"] = string.format("Audio Loudness: %d", loudness)
  end

  if has_sensor_to_world_matrix then
    -- TODO: sensor to world matrix is hard to decode
    i = i + 20
  end

  if has_additional_flags then
    local flags = buf(i, 1):uint()
    i = i + 1
    result["additional_flags"] = string.format("Additional Flags: 0x%x", flags)
  end

  if has_parent_info then
    local parent_id = buf(i, 16)
    i = i + 16
    local parent_joint_index = buf(i, 2):le_int()
    i = i + 2
    result["parent_id"] = parent_id
    result["parent_joint_index"] = string.format("Parent Joint Index: %d", parent_joint_index)
  end

  if has_local_position then
    local local_pos = decode_vec3(buf(i, 12))
    i = i + 12
    result["local_position"] = string.format("Local Position: %.3f, %.3f, %.3f", local_pos[1], local_pos[2], local_pos[3])
  end

  if has_face_tracker_info then
    local left_eye_blink = buf(i, 4):le_float()
    i = i + 4
    local right_eye_blink = buf(i, 4):le_float()
    i = i + 4
    local average_loudness = buf(i, 4):le_float()
    i = i + 4
    local brow_audio_lift = buf(i, 4):le_float()
    i = i + 4
    local num_blendshape_coefficients = buf(i, 1):uint()
    i = i + 1
    local blendshape_coefficients = {}
    for n = 1, num_blendshape_coefficients do
      blendshape_coefficients[n] = buf(i, 4):le_float()
      i = i + 4
    end
    -- TODO: insert blendshapes into result
  end

  if has_joint_data then

    local num_joints = buf(i, 1):uint()
    i = i + 1
    local num_validity_bytes = math.ceil(num_joints / 8)

    local indices = decode_validity_bits(buf(i, num_validity_bytes), num_joints)
    i = i + num_validity_bytes
    result["valid_rotations"] = "Valid Rotations: " .. string.format("(%d/%d) {", #indices, num_joints) .. table.concat(indices, ", ") .. "}"

    -- TODO: skip rotations for now
    i = i + #indices * 6

    indices = decode_validity_bits(buf(i, num_validity_bytes), num_joints)
    i = i + num_validity_bytes
    result["valid_translations"] = "Valid Translations: " .. string.format("(%d/%d) {", #indices, num_joints) .. table.concat(indices, ", ") .. "}"

    -- TODO: skip translations for now
    i = i + #indices * 6

    -- TODO: skip hand controller data
    i = i + 24

  end

  if has_joint_default_pose_flags then
    local num_joints = buf(i, 1):uint()
    i = i + 1
    local num_validity_bytes = math.ceil(num_joints / 8)

    local indices = decode_validity_bits(buf(i, num_validity_bytes), num_joints)
    i = i + num_validity_bytes
    result["default_rotations"] = "Default Rotations: " .. string.format("(%d/%d) {", #indices, num_joints) .. table.concat(indices, ", ") .. "}"

    indices = decode_validity_bits(buf(i, num_validity_bytes), num_joints)
    i = i + num_validity_bytes
    result["default_translations"] = "Default Translations: " .. string.format("(%d/%d) {", #indices, num_joints) .. table.concat(indices, ", ") .. "}"
  end

  return result
end

function p_hfudt.init()
  local udp_dissector_table = DissectorTable.get("udp.port")

  for port=1000, 65000 do
    udp_dissector_table:add(port, p_hfudt)
  end
end
