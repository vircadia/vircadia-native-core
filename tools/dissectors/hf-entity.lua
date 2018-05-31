-- create the entity protocol
p_hf_entity = Proto("hf-entity", "HF Entity Protocol")

-- entity packet fields
local f_entity_sequence_number = ProtoField.uint16("hf_entity.sequence_number", "Sequence Number")
local f_entity_timestamp = ProtoField.uint64("hf_entity.timestamp", "Timestamp")
local f_octal_code_bytes = ProtoField.uint8("hf_entity.octal_code_bytes", "Octal Code Bytes")
local f_entity_id = ProtoField.guid("hf_entity.entity_id", "Entity ID")

p_hf_entity.fields = {
  f_entity_sequence_number, f_entity_timestamp, f_octal_code_bytes, f_entity_id
}

function p_hf_entity.dissector(buf, pinfo, tree)
  pinfo.cols.protocol = p_hf_entity.name

  entity_subtree = tree:add(p_hf_entity, buf())

  i = 0

  entity_subtree:add_le(f_entity_sequence_number, buf(i, 2))
  i = i + 2

  entity_subtree:add_le(f_entity_timestamp, buf(i, 4))
  i = i + 4

  -- figure out the number of bytes the octal code takes
  local octal_code_bytes = buf(i, 1):le_uint()
  entity_subtree:add_le(f_octal_code_bytes, buf(i, 1))

  -- skip over the octal code
  i = i + 1 + octal_code_bytes

  -- read the entity ID
  entity_subtree:add(f_entity_id, buf(i, 16))
end
