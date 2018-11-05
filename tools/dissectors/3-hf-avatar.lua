print("Loading hf-avatar")

-- create the avatar protocol
p_hf_avatar = Proto("hf-avatar", "HF Avatar Protocol")

-- avatar data fields
local f_avatar_id = ProtoField.guid("hf_avatar.avatar_id", "Avatar ID")
local f_avatar_data_has_flags = ProtoField.string("hf_avatar.avatar_has_flags", "Has Flags")
local f_avatar_data_position = ProtoField.string("hf_avatar.avatar_data_position", "Position")
local f_avatar_data_dimensions = ProtoField.string("hf_avatar.avatar_data_dimensions", "Dimensions")
local f_avatar_data_offset = ProtoField.string("hf_avatar.avatar_data_offset", "Offset")
local f_avatar_data_look_at_position = ProtoField.string("hf_avatar.avatar_data_look_at_position", "Look At Position")
local f_avatar_data_audio_loudness = ProtoField.string("hf_avatar.avatar_data_audio_loudness", "Audio Loudness")
local f_avatar_data_additional_flags = ProtoField.string("hf_avatar.avatar_data_additional_flags", "Additional Flags")
local f_avatar_data_parent_id = ProtoField.guid("hf_avatar.avatar_data_parent_id", "Parent ID")
local f_avatar_data_parent_joint_index = ProtoField.string("hf_avatar.avatar_data_parent_joint_index", "Parent Joint Index")
local f_avatar_data_local_position = ProtoField.string("hf_avatar.avatar_data_local_position", "Local Position")
local f_avatar_data_valid_rotations = ProtoField.string("hf_avatar.avatar_data_valid_rotations", "Valid Rotations")
local f_avatar_data_valid_translations = ProtoField.string("hf_avatar.avatar_data_valid_translations", "Valid Translations")
local f_avatar_data_default_rotations = ProtoField.string("hf_avatar.avatar_data_default_rotations", "Valid Default")
local f_avatar_data_default_translations = ProtoField.string("hf_avatar.avatar_data_default_translations", "Valid Default")
local f_avatar_data_sizes = ProtoField.string("hf_avatar.avatar_sizes", "Sizes")


p_hf_avatar.fields = {
  f_avatar_id, f_avatar_data_parent_id
}

local packet_type_extractor = Field.new('hfudt.type')

function p_hf_avatar.dissector(buf, pinfo, tree)
  pinfo.cols.protocol = p_hf_avatar.name

  avatar_subtree = tree:add(p_hf_avatar, buf())

  local i = 0

  local avatar_data

  local packet_type = packet_type_extractor().value

  if packet_type == 6 then
    -- AvatarData packet

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
      avatar_subtree:add(f_avatar_id, buf(i, 16))
      i = i + 16

      local avatar_data_packet_len = buf:len() - i
      avatar_data = decode_avatar_data_packet(buf(i, avatar_data_packet_len))
      i = i + avatar_data_packet_len

      add_avatar_data_subtrees(avatar_data)
    end
  end
end


function add_avatar_data_subtrees(avatar_data)
  if avatar_data["has_flags"] then
    avatar_subtree:add(f_avatar_data_has_flags, avatar_data["has_flags"])
  end
  if avatar_data["position"] then
    avatar_subtree:add(f_avatar_data_position, avatar_data["position"])
  end
  if avatar_data["dimensions"] then
    avatar_subtree:add(f_avatar_data_dimensions, avatar_data["dimensions"])
  end
  if avatar_data["offset"] then
    avatar_subtree:add(f_avatar_data_offset, avatar_data["offset"])
  end
  if avatar_data["look_at_position"] then
    avatar_subtree:add(f_avatar_data_look_at_position, avatar_data["look_at_position"])
  end
  if avatar_data["audio_loudness"] then
    avatar_subtree:add(f_avatar_data_audio_loudness, avatar_data["audio_loudness"])
  end
  if avatar_data["additional_flags"] then
    avatar_subtree:add(f_avatar_data_additional_flags, avatar_data["additional_flags"])
  end
  if avatar_data["parent_id"] then
    avatar_subtree:add(f_avatar_data_parent_id, avatar_data["parent_id"])
  end
  if avatar_data["parent_joint_index"] then
    avatar_subtree:add(f_avatar_data_parent_joint_index, avatar_data["parent_joint_index"])
  end
  if avatar_data["local_position"] then
    avatar_subtree:add(f_avatar_data_local_position, avatar_data["local_position"])
  end
  if avatar_data["valid_rotations"] then
    avatar_subtree:add(f_avatar_data_valid_rotations, avatar_data["valid_rotations"])
  end
  if avatar_data["valid_translations"] then
    avatar_subtree:add(f_avatar_data_valid_translations, avatar_data["valid_translations"])
  end
  if avatar_data["default_rotations"] then
    avatar_subtree:add(f_avatar_data_default_rotations, avatar_data["default_rotations"])
  end
  if avatar_data["default_translations"] then
    avatar_subtree:add(f_avatar_data_default_translations, avatar_data["default_translations"])
  end
  if avatar_data["sizes"] then
    avatar_subtree:add(f_avatar_data_sizes, avatar_data["sizes"])
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

  result["sizes"] = ""

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

    local joint_poses_start = i

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

    result["sizes"] = result["sizes"] .. " Poses: " .. (i - joint_poses_start)

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

  result["sizes"] = result["sizes"] .. " Total: " .. i

  return result
end
