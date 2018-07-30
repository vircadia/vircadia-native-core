print("Loading hf-entity")

-- create the entity protocol
p_hf_entity = Proto("hf-entity", "HF Entity Protocol")

-- entity packet fields
local f_entity_sequence_number = ProtoField.uint16("hf_entity.sequence_number", "Sequence Number")
local f_entity_timestamp = ProtoField.uint64("hf_entity.timestamp", "Timestamp")
local f_octal_code_three_bit_sections = ProtoField.uint8("hf_entity.octal_code_three_bit_sections", "Octal Code Three Bit Sections")
local f_octal_code = ProtoField.bytes("hf_entity.octal_code", "Octal Code")
local f_entity_id = ProtoField.guid("hf_entity.entity_id", "Entity ID")
local f_last_edited = ProtoField.uint64("hf_entity.last_edited", "Last Edited")
local f_coded_property_type = ProtoField.bytes("hf_entity.coded_property_type", "Coded Property Type")
local f_property_type = ProtoField.uint32("hf_entity.property_type", "Property Type")
local f_coded_update_delta = ProtoField.bytes("hf_entity.f_coded_update_delta", "Coded Update Delta")
local f_update_delta = ProtoField.uint32("hf_entity.update_delta", "Update Delta")

p_hf_entity.fields = {
  f_entity_sequence_number, f_entity_timestamp,
  f_octal_code_three_bit_sections, f_octal_code,
  f_last_edited, f_entity_id,
  f_coded_property_type, f_property_type,
  f_coded_update_delta, f_update_delta
}

function p_hf_entity.dissector(buf, pinfo, tree)
  pinfo.cols.protocol = p_hf_entity.name

  entity_subtree = tree:add(p_hf_entity, buf())

  local i = 0

  entity_subtree:add_le(f_entity_sequence_number, buf(i, 2))
  i = i + 2

  entity_subtree:add_le(f_entity_timestamp, buf(i, 8))
  i = i + 8

  -- figure out the number of three bit sections in the octal code
  local octal_code_three_bit_sections = buf(i, 1):le_uint()
  entity_subtree:add_le(f_octal_code_three_bit_sections, buf(i, 1))
  i = i + 1

  -- read the bytes for the octal code
  local octal_code_bytes = math.ceil((octal_code_three_bit_sections * 3) / 8)
  entity_subtree:add_le(f_octal_code, buf(i, octal_code_bytes))
  i = i + octal_code_bytes

  -- read the last edited timestamp
  entity_subtree:add_le(f_last_edited, buf(i, 8))
  i = i + 8

  -- read the entity ID
  entity_subtree:add(f_entity_id, buf(i, 16))
  i = i + 16

  -- figure out the property type and the size of the coded value
  local property_type, coded_property_bytes = number_of_coded_bytes(buf(i))
  entity_subtree:add(f_coded_property_type, buf(i, coded_property_bytes))
  entity_subtree:add(f_property_type, property_type)
  i = i + coded_property_bytes

  -- figure out the update delta and the size of the coded value
  local update_delta, coded_update_delta_bytes = number_of_coded_bytes(buf(i))
  entity_subtree:add(f_coded_update_delta, buf(i, coded_update_delta_bytes))
  entity_subtree:add(f_update_delta, update_delta)
  i = i + coded_update_delta_bytes
end

function number_of_coded_bytes(buf)
  local coded_buffer = buf(0, 4):le_uint() -- max 64 bit value means max 10 header bits

  -- first figure out the total number of bytes for the coded value based
  -- on the bits in the header
  local total_coded_bytes = 1

  for bit = 0, 10, 1 do
    local header_bit = bit32.extract(coded_buffer, bit)

    if header_bit == 1 then
      total_coded_bytes = total_coded_bytes + 1
    else
      break
    end
  end

  -- pull out the bits and write them to our decoded value
  local decoded_value = 0
  local decoded_position = 0
  local total_bits = total_coded_bytes * 8

  for bit = total_coded_bytes, total_bits - 1, 1 do
    local value_bit = bit32.extract(coded_buffer, total_bits - bit - 1)
    decoded_value = bit32.replace(decoded_value, value_bit, decoded_position)
    decoded_position = decoded_position + 1
  end

  return decoded_value, total_coded_bytes
end
