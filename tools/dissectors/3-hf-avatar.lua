print("Loading hf-avatar")

-- create the avatar protocol
p_hf_avatar = Proto("hf-avatar", "HF Avatar Protocol")

-- avatar data fields
local f_avatar_id = ProtoField.guid("hf_avatar.avatar_id", "Avatar ID")

-- sizes in bytes
local f_avatar_header_size = ProtoField.int32("hf_avatar.header_size", "Header Size", base.DEC)
local f_avatar_global_position_size = ProtoField.int32("hf_avatar.global_position_size", "Global Position Size", base.DEC)
local f_avatar_bounding_box_size = ProtoField.int32("hf_avatar.bounding_box_size", "Bounding Box Size", base.DEC)
local f_avatar_orientation_size = ProtoField.int32("hf_avatar.orientation_size", "Orientation Size", base.DEC)
local f_avatar_scale_size = ProtoField.int32("hf_avatar.scale_size", "Scale Size", base.DEC)
local f_avatar_look_at_position_size = ProtoField.int32("hf_avatar.look_at_position_size", "Look At Position Size", base.DEC)
local f_avatar_audio_loudness_size = ProtoField.int32("hf_avatar.audio_loudness_size", "Audio Loudness Size", base.DEC)
local f_avatar_sensor_to_world_size = ProtoField.int32("hf_avatar.sensor_to_world_size", "Sensor To World Matrix Size", base.DEC)
local f_avatar_additional_flags_size = ProtoField.int32("hf_avatar.additional_flags_size", "Additional Flags Size", base.DEC)
local f_avatar_parent_info_size = ProtoField.int32("hf_avatar.parent_info_size", "Parent Info Size", base.DEC)
local f_avatar_local_position_size = ProtoField.int32("hf_avatar.local_position_size", "Local Position Size", base.DEC)
local f_avatar_face_tracker_info_size = ProtoField.int32("hf_avatar.face_tracker_info_size", "Face Tracker Info Size", base.DEC)
local f_avatar_joint_data_size = ProtoField.int32("hf_avatar.joint_data_size", "Joint Data Size", base.DEC)
local f_avatar_default_pose_flags_size = ProtoField.int32("hf_avatar.default_pose_flags_size", "Default Pose Flags Size", base.DEC)
local f_avatar_grab_joints_size = ProtoField.int32("hf_avatar.grab_joints_size", "Grab Joints Size", base.DEC)

local f_avatar_joint_data_bit_vector_size = ProtoField.int32("hf_avatar.joint_data_bit_vector_size", "Joint Data Bit Vector Size", base.DEC)
local f_avatar_joint_data_rotations_size = ProtoField.int32("hf_avatar.joint_data_rotations_size", "Joint Data Rotations Size", base.DEC)
local f_avatar_joint_data_translations_size = ProtoField.int32("hf_avatar.joint_data_translations_size", "Joint Data Translations Size", base.DEC)
local f_avatar_joint_data_faux_joints_size = ProtoField.int32("hf_avatar.joint_data_faux_joints_size", "Joint Data Faux Joints Size", base.DEC)
local f_avatar_joint_data_other_size = ProtoField.int32("hf_avatar.joint_data_other_size", "Joint Data Other Size", base.DEC)

local f_avatar_bulk_count = ProtoField.int32("hf_avatar.bulk_count", "Bulk Count", base.DEC)

-- contents
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

-- avatar trait data fields
local f_avatar_trait_data = ProtoField.bytes("hf_avatar.avatar_trait_data", "Avatar Trait Data")

local f_avatar_trait_id = ProtoField.guid("hf_avatar.trait_avatar_id", "Trait Avatar ID")
local f_avatar_trait_type = ProtoField.int8("hf_avatar.trait_type", "Trait Type")
local f_avatar_trait_version = ProtoField.int32("hf_avatar.trait_version", "Trait Version")
local f_avatar_trait_instance_id = ProtoField.guid("hf_avatar.trait_instance_id", "Trait Instance ID")
local f_avatar_trait_binary = ProtoField.bytes("hf_avatar.trait_binary", "Trait Binary Data")

p_hf_avatar.fields = {
  f_avatar_id,
  f_avatar_data_parent_id,

  f_avatar_header_size,
  f_avatar_global_position_size,
  f_avatar_bounding_box_size,
  f_avatar_orientation_size,
  f_avatar_scale_size,
  f_avatar_look_at_position_size,
  f_avatar_audio_loudness_size,
  f_avatar_sensor_to_world_size,
  f_avatar_additional_flags_size,
  f_avatar_parent_info_size,
  f_avatar_local_position_size,
  f_avatar_face_tracker_info_size,
  f_avatar_joint_data_size,
  f_avatar_default_pose_flags_size,
  f_avatar_grab_joints_size,
  f_avatar_bulk_count,

  f_avatar_joint_data_bit_vector_size,
  f_avatar_joint_data_rotations_size,
  f_avatar_joint_data_translations_size,
  f_avatar_joint_data_faux_joints_size,
  f_avatar_joint_data_other_size,

  f_avatar_trait_data,
  f_avatar_trait_type, f_avatar_trait_id,
  f_avatar_trait_version, f_avatar_trait_binary,
  f_avatar_trait_instance_id
}

local packet_type_extractor = Field.new('hfudt.type')

INSTANCED_TYPES = {
  [1] = true
}

TOTAL_TRAIT_TYPES = 2

function p_hf_avatar.dissector(buf, pinfo, tree)
  pinfo.cols.protocol = p_hf_avatar.name

  avatar_subtree = tree:add(p_hf_avatar, buf())

  local i = 0

  local avatar_data

  -- sizes
  local avatar_data_sizes = {
    header_size = 0,
    global_position_size = 0,
    bounding_box_size = 0,
    orientation_size = 0,
    scale_size = 0,
    look_at_position_size = 0,
    audio_loudness_size = 0,
    sensor_to_world_size = 0,
    additional_flags_size = 0,
    parent_info_size = 0,
    local_position_size = 0,
    face_tracker_info_size = 0,
    joint_data_size = 0,
    default_pose_flags_size = 0,
    grab_joints_size = 0,
    bulk_count = 0,
    joint_data_bit_vector_size = 0,
    joint_data_rotations_size = 0,
    joint_data_translations_size = 0,
    joint_data_faux_joints_size = 0,
    joint_data_other_size = 0
  }


  local packet_type = packet_type_extractor().value
  avatar_data_sizes.header_size = avatar_data_sizes.header_size + 1

  if packet_type == 6 then
    -- AvatarData packet

    -- uint16 sequence_number
    local sequence_number = buf(i, 2):le_uint()
    i = i + 2
    avatar_data_sizes.header_size = avatar_data_sizes.header_size + 2

    local avatar_data_packet_len = buf:len() - i
    avatar_data = decode_avatar_data_packet(buf(i, avatar_data_packet_len), avatar_data_sizes)
    i = i + avatar_data_packet_len

    add_avatar_data_subtrees(avatar_data)
    add_avatar_data_sizes(avatar_data_sizes)

  elseif packet_type == 11 then
    -- BulkAvatarData packet
    while i < buf:len() do
      -- avatar_id is first 16 bytes
      avatar_subtree:add(f_avatar_id, buf(i, 16))
      i = i + 16
      avatar_data_sizes.header_size = avatar_data_sizes.header_size + 16

      local avatar_data_packet_len = buf:len() - i
      avatar_data = decode_avatar_data_packet(buf(i, avatar_data_packet_len), avatar_data_sizes)
      i = i + avatar_data.bytes_consumed

      add_avatar_data_subtrees(avatar_data)
      avatar_data_sizes.bulk_count = avatar_data_sizes.bulk_count + 1
    end

    add_avatar_data_sizes(avatar_data_sizes)

  elseif packet_type == 100 then
    -- BulkAvatarTraits packet

    -- loop over the packet until we're done reading it
    while i < buf:len() do
      i = i + read_avatar_trait_data(buf(i))
    end
  end
end

function read_avatar_trait_data(buf)
  local i = 0

  -- avatar_id is first 16 bytes
  local avatar_id_bytes = buf(i, 16)
  i = i + 16

  local traits_map = {}

  -- loop over all of the traits for this avatar
  while i < buf:len() do
    -- pull out this trait type
    local trait_type = buf(i, 1):le_int()
    i = i + 1

    debug("The trait type is " .. trait_type)

    -- bail on our while if the trait type is null (-1)
    if trait_type == -1 then break end

    local trait_map = {}

    -- pull out the trait version
    trait_map.version = buf(i, 4):le_int()
    i = i + 4

    if INSTANCED_TYPES[trait_type] ~= nil then
      -- pull out the trait instance ID
      trait_map.instance_ID = buf(i, 16)
      i = i + 16
    end

    -- pull out the trait binary size
    trait_map.binary_size = buf(i, 2):le_int()
    i = i + 2

    -- unpack the binary data as long as this wasn't a delete
    if trait_map.binary_size ~= -1 then
      -- pull out the binary data for the trait
      trait_map.binary_data = buf(i, trait_map.binary_size)
      i = i + trait_map.binary_size
    end

    traits_map[trait_type] = trait_map
  end

  -- add a subtree including all of the data for this avatar
  debug("Adding trait data of " .. i .. " bytes to the avatar tree")
  local this_avatar_tree = avatar_subtree:add(f_avatar_trait_data, buf(0, i))

  this_avatar_tree:add(f_avatar_trait_id, avatar_id_bytes)

  -- enumerate the pulled traits and add them to the tree
  local trait_type = 0
  while trait_type < TOTAL_TRAIT_TYPES do
    trait = traits_map[trait_type]

    if trait ~= nil then
      this_avatar_tree:add(f_avatar_trait_type, trait_type)
      this_avatar_tree:add(f_avatar_trait_version, trait.version)
      this_avatar_tree:add(f_avatar_trait_binary, trait.binary_data)

      if trait.instance_ID ~= nil then
        this_avatar_tree:add(f_avatar_trait_instance_id, trait.instance_ID)
      end
    end

    trait_type = trait_type + 1
  end

  return i
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
end

function add_avatar_data_sizes(avatar_data_sizes)
  if avatar_data_sizes.header_size then
    avatar_subtree:add(f_avatar_header_size, avatar_data_sizes.header_size)
  end
  if avatar_data_sizes.global_position_size then
    avatar_subtree:add(f_avatar_global_position_size, avatar_data_sizes.global_position_size)
  end
  if avatar_data_sizes.bounding_box_size then
    avatar_subtree:add(f_avatar_bounding_box_size, avatar_data_sizes.bounding_box_size)
  end
  if avatar_data_sizes.orientation_size then
    avatar_subtree:add(f_avatar_orientation_size, avatar_data_sizes.orientation_size)
  end
  if avatar_data_sizes.scale_size then
    avatar_subtree:add(f_avatar_scale_size, avatar_data_sizes.scale_size)
  end
  if avatar_data_sizes.look_at_position_size then
    avatar_subtree:add(f_avatar_look_at_position_size, avatar_data_sizes.look_at_position_size)
  end
  if avatar_data_sizes.audio_loudness_size then
    avatar_subtree:add(f_avatar_audio_loudness_size, avatar_data_sizes.audio_loudness_size)
  end
  if avatar_data_sizes.sensor_to_world_size then
    avatar_subtree:add(f_avatar_sensor_to_world_size, avatar_data_sizes.sensor_to_world_size)
  end
  if avatar_data_sizes.additional_flags_size then
    avatar_subtree:add(f_avatar_additional_flags_size, avatar_data_sizes.additional_flags_size)
  end
  if avatar_data_sizes.parent_info_size then
    avatar_subtree:add(f_avatar_parent_info_size, avatar_data_sizes.parent_info_size)
  end
  if avatar_data_sizes.local_position_size then
    avatar_subtree:add(f_avatar_local_position_size, avatar_data_sizes.local_position_size)
  end
  if avatar_data_sizes.face_tracker_info_size then
    avatar_subtree:add(f_avatar_face_tracker_info_size, avatar_data_sizes.face_tracker_info_size)
  end
  if avatar_data_sizes.joint_data_size then
    avatar_subtree:add(f_avatar_joint_data_size, avatar_data_sizes.joint_data_size)
  end
  if avatar_data_sizes.default_pose_flags_size then
    avatar_subtree:add(f_avatar_default_pose_flags_size, avatar_data_sizes.default_pose_flags_size)
  end
  if avatar_data_sizes.grab_joints_size then
    avatar_subtree:add(f_avatar_grab_joints_size, avatar_data_sizes.grab_joints_size)
  end
  if avatar_data_sizes.bulk_count then
    avatar_subtree:add(f_avatar_bulk_count, avatar_data_sizes.bulk_count)
  end

  if avatar_data_sizes.joint_data_bit_vector_size then
    avatar_subtree:add(f_avatar_joint_data_bit_vector_size, avatar_data_sizes.joint_data_bit_vector_size)
  end
  if avatar_data_sizes.joint_data_rotations_size then
    avatar_subtree:add(f_avatar_joint_data_rotations_size, avatar_data_sizes.joint_data_rotations_size)
  end
  if avatar_data_sizes.joint_data_translations_size then
    avatar_subtree:add(f_avatar_joint_data_translations_size, avatar_data_sizes.joint_data_translations_size)
  end
  if avatar_data_sizes.joint_data_faux_joints_size then
    avatar_subtree:add(f_avatar_joint_data_faux_joints_size, avatar_data_sizes.joint_data_faux_joints_size)
  end
  if avatar_data_sizes.joint_data_other_size then
    avatar_subtree:add(f_avatar_joint_data_other_size, avatar_data_sizes.joint_data_other_size)
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

function validity_bits_to_string(buf, num_bits)
  -- first pass, decode each bit into an array of booleans
  local i = 0
  local bit = 0
  local booleans = {}
  for n = 1, num_bits do
    local value = (bit32.band(buf(i, 1):uint(), bit32.lshift(1, bit)) ~= 0)
    booleans[#booleans + 1] = (value and 1 or 0)
    bit = bit + 1
    if bit == 8 then
      i = i + 1
      bit = 0
    end
  end

  return table.concat(booleans, "")
end

function decode_avatar_data_packet(buf, avatar_data_sizes)

  local i = 0
  local result = {}

  -- uint16 has_flags
  local has_flags = buf(i, 2):le_uint()
  i = i + 2
  avatar_data_sizes.header_size = avatar_data_sizes.header_size + 2

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
  local has_grab_joints = (bit32.band(has_flags, 8192) ~= 0)

  result["has_flags"] = string.format("HasFlags: 0x%x", has_flags)

  if has_global_position then
    local position = decode_vec3(buf(i, 12))
    result["position"] = string.format("Position: %.3f, %.3f, %.3f", position[1], position[2], position[3])
    i = i + 12
    avatar_data_sizes.global_position_size = avatar_data_sizes.global_position_size + 12
  end

  if has_bounding_box then
    local dimensions = decode_vec3(buf(i, 12))
    i = i + 12
    local offset = decode_vec3(buf(i, 12))
    i = i + 12
    avatar_data_sizes.bounding_box_size = avatar_data_sizes.bounding_box_size + (12 + 12)
    result["dimensions"] = string.format("Dimensions: %.3f, %.3f, %.3f", dimensions[1], dimensions[2], dimensions[3])
    result["offset"] = string.format("Offset: %.3f, %.3f, %.3f", offset[1], offset[2], offset[3])
  end

  if has_orientation then
    -- TODO: orientation is hard to decode...
    i = i + 6
    avatar_data_sizes.orientation_size = avatar_data_sizes.orientation_size + 6
  end

  if has_scale then
    -- TODO: scale is hard to decode...
    i = i + 2
    avatar_data_sizes.scale_size = avatar_data_sizes.scale_size + 2
  end

  if has_look_at_position then
    local look_at = decode_vec3(buf(i, 12))
    i = i + 12
    avatar_data_sizes.look_at_position_size = avatar_data_sizes.look_at_position_size + 12
    result["look_at_position"] = string.format("Look At Position: %.3f, %.3f, %.3f", look_at[1], look_at[2], look_at[3])
  end

  if has_audio_loudness then
    local loudness = buf(i, 1):uint()
    i = i + 1
    avatar_data_sizes.audio_loudness_size = avatar_data_sizes.audio_loudness_size + 1
    result["audio_loudness"] = string.format("Audio Loudness: %d", loudness)
  end

  if has_sensor_to_world_matrix then
    -- TODO: sensor to world matrix is hard to decode
    i = i + 20
    avatar_data_sizes.sensor_to_world_size = avatar_data_sizes.sensor_to_world_size + 20
  end

  if has_additional_flags then
    local flags = buf(i, 2):uint()
    i = i + 2
    result["additional_flags"] = string.format("Additional Flags: 0x%x", flags)
    avatar_data_sizes.additional_flags_size = avatar_data_sizes.additional_flags_size + 2
  end

  if has_parent_info then
    local parent_id = buf(i, 16)
    i = i + 16
    local parent_joint_index = buf(i, 2):le_int()
    i = i + 2
    avatar_data_sizes.parent_info_size = avatar_data_sizes.parent_info_size + 18
    result["parent_id"] = parent_id
    result["parent_joint_index"] = string.format("Parent Joint Index: %d", parent_joint_index)
  end

  if has_local_position then
    local local_pos = decode_vec3(buf(i, 12))
    i = i + 12
    avatar_data_sizes.local_position_size = avatar_data_sizes.local_position_size + 12
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
    avatar_data_sizes.face_tracker_info_size = avatar_data_sizes.face_tracker_info_size + 17 + (num_blendshape_coefficients * 4)
    -- TODO: insert blendshapes into result
  end

  if has_joint_data then

    local joint_poses_start = i

    local num_joints = buf(i, 1):uint()
    i = i + 1
    avatar_data_sizes.joint_data_other_size = avatar_data_sizes.joint_data_other_size + 1

    local num_validity_bytes = math.ceil(num_joints / 8)

    local indices = decode_validity_bits(buf(i, num_validity_bytes), num_joints)
    local s = validity_bits_to_string(buf(i, num_validity_bytes), num_joints)
    i = i + num_validity_bytes
    avatar_data_sizes.joint_data_bit_vector_size = avatar_data_sizes.joint_data_bit_vector_size + num_validity_bytes

    result["valid_rotations"] = "Valid Rotations: " .. string.format("(%d/%d) {", #indices, num_joints) .. s .. "}"

    -- TODO: skip rotations for now
    i = i + (#indices * 6)
    avatar_data_sizes.joint_data_rotations_size = avatar_data_sizes.joint_data_rotations_size + (#indices * 6)

    indices = decode_validity_bits(buf(i, num_validity_bytes), num_joints)
    s = validity_bits_to_string(buf(i, num_validity_bytes), num_joints)
    i = i + num_validity_bytes
    avatar_data_sizes.joint_data_bit_vector_size = avatar_data_sizes.joint_data_bit_vector_size + num_validity_bytes
    result["valid_translations"] = "Valid Translations: " .. string.format("(%d/%d) {", #indices, num_joints) .. s .. "}"

    -- TODO: skip maxTranslationDimension
    i = i + 4
    avatar_data_sizes.joint_data_other_size = avatar_data_sizes.joint_data_other_size + 4

    -- TODO: skip translations for now
    i = i + (#indices * 6)
    avatar_data_sizes.joint_data_translations_size = avatar_data_sizes.joint_data_translations_size + (#indices * 6)

    -- TODO: skip faux joint data
    i = i + (2 * 12)
    avatar_data_sizes.joint_data_faux_joints_size = avatar_data_sizes.joint_data_faux_joints_size + (2 * 12)

    avatar_data_sizes.joint_data_size = avatar_data_sizes.joint_data_size + (i - joint_poses_start)
  end

  if has_grab_joints then
    -- TODO: skip grab joints
    i = i + 84
    avatar_data_sizes.grab_joints_size = avatar_data_sizes.grab_joints_size + 84
  end

  if has_joint_default_pose_flags then
    local num_joints = buf(i, 1):uint()
    i = i + 1
    local num_validity_bytes = math.ceil(num_joints / 8)

    local indices = decode_validity_bits(buf(i, num_validity_bytes), num_joints)
    local s = validity_bits_to_string(buf(i, num_validity_bytes), num_joints)
    i = i + num_validity_bytes
    result["default_rotations"] = "Default Rotations: " .. string.format("(%d/%d) {", #indices, num_joints) .. s .. "}"

    indices = decode_validity_bits(buf(i, num_validity_bytes), num_joints)
    s = validity_bits_to_string(buf(i, num_validity_bytes), num_joints)
    i = i + num_validity_bytes
    result["default_translations"] = "Default Translations: " .. string.format("(%d/%d) {", #indices, num_joints) .. s .. "}"

    avatar_data_sizes.default_pose_flags_size = avatar_data_sizes.default_pose_flags_size + 2 * num_validity_bytes
  end

  result["bytes_consumed"] = i

  return result
end
